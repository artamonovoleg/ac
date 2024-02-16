#include "ac_private.h"

#if (AC_PLATFORM_LINUX && AC_INCLUDE_WINDOW)

#include "window_linux.h"

#define MWM_HINTS_DECORATIONS 2
#define MWM_DECOR_ALL 1

static inline void
ac_linux_window_send_event_to_wm(
  ac_window window_handle,
  Atom      type,
  long      a,
  long      b,
  long      c,
  long      d,
  long      e)
{
  AC_FROM_HANDLE(window, ac_linux_window);

  XEvent event = {ClientMessage};
  event.xclient.window = window->handle;
  event.xclient.format = 32; // Data is 32-bit longs
  event.xclient.message_type = type;
  event.xclient.data.l[0] = a;
  event.xclient.data.l[1] = b;
  event.xclient.data.l[2] = c;
  event.xclient.data.l[3] = d;
  event.xclient.data.l[4] = e;

  window->XSendEvent(
    window->dpy,
    window->root,
    False,
    SubstructureNotifyMask | SubstructureRedirectMask,
    &event);
}

static inline void
ac_linux_window_load_atoms(ac_window window_handle)
{
  AC_FROM_HANDLE(window, ac_linux_window);

  window->_NET_WM_STATE_REMOVE = 0;
  window->_NET_WM_STATE_ADD = 1;
  window->_NET_WM_NAME =
    window->XInternAtom(window->dpy, "_NET_WM_NAME", False);
  window->_NET_WM_STATE =
    window->XInternAtom(window->dpy, "_NET_WM_STATE", False);
  window->_NET_WM_STATE_FULLSCREEN =
    window->XInternAtom(window->dpy, "_NET_WM_STATE_FULLSCREEN", False);
  window->_NET_WM_STATE_MAXIMIZED_VERT =
    window->XInternAtom(window->dpy, "_NET_WM_STATE_MAXIMIZED_VERT", False);
  window->_NET_WM_STATE_MAXIMIZED_HORZ =
    window->XInternAtom(window->dpy, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
  window->_NET_WM_PING =
    window->XInternAtom(window->dpy, "_NET_WM_PING", False);
  window->_NET_WM_PID = window->XInternAtom(window->dpy, "_NET_WM_PID", False);
  window->WM_PROTOCOLS =
    window->XInternAtom(window->dpy, "WM_PROTOCOLS", False);
  window->WM_DELETE_WINDOW =
    window->XInternAtom(window->dpy, "WM_DELETE_WINDOW", False);
  window->UTF8_STRING = window->XInternAtom(window->dpy, "UTF8_STRING", False);
  window->_NET_WM_ICON_NAME =
    window->XInternAtom(window->dpy, "_NET_WM_ICON_NAME", False);
  window->_MOTIF_WM_HINTS =
    window->XInternAtom(window->dpy, "_MOTIF_WM_HINTS", False);
  window->_NET_ACTIVE_WINDOW =
    window->XInternAtom(window->dpy, "_NET_ACTIVE_WINDOW", False);
}

static inline void
ac_linux_window_create_cursors(ac_window window_handle)
{
  AC_FROM_HANDLE(window, ac_linux_window);

  Pixmap      pixmap;
  XColor      black;
  static char data[] = {0, 0, 0, 0, 0, 0, 0, 0};
  black.red = black.green = black.blue = 0;

  pixmap = window->XCreateBitmapFromData(window->dpy, window->root, data, 8, 8);
  window->hidden_cursor =
    window
      ->XCreatePixmapCursor(window->dpy, pixmap, pixmap, &black, &black, 0, 0);
  window->XFreePixmap(window->dpy, pixmap);
}

static Bool
ac_linux_window_is_map_notify(Display* dpy, XEvent* ev, XPointer win)
{
  AC_UNUSED(dpy);
  return ev->type == MapNotify && ev->xmap.window == *((Window*)((void*)win));
}

static inline ac_result
ac_linux_window_poll_monitors(ac_window window_handle)
{
  AC_FROM_HANDLE(window, ac_linux_window);

  XRRScreenResources* sr =
    window->XRRGetScreenResourcesCurrent(window->dpy, window->root);

  for (int i = 0; i < sr->noutput; i++)
  {
    XRROutputInfo* oi =
      window->XRRGetOutputInfo(window->dpy, sr, sr->outputs[i]);
    if (oi->connection != RR_Connected || oi->crtc == None)
    {
      window->XRRFreeOutputInfo(oi);
      continue;
    }

    XRRCrtcInfo* ci = window->XRRGetCrtcInfo(window->dpy, sr, oi->crtc);

    ac_monitor* monitor_handle =
      &window->common.monitors[window->common.monitor_count++];
    AC_INIT_INTERNAL(monitor, ac_linux_monitor);

    monitor->common.x = ci->x;
    monitor->common.y = ci->y;
    monitor->common.width = ci->width;
    monitor->common.height = ci->height;

    monitor->output = sr->outputs[i];
    monitor->crtc = oi->crtc;
    monitor->index = (uint32_t)i;

    window->XRRFreeOutputInfo(oi);
    window->XRRFreeCrtcInfo(ci);
  }

  window->XRRFreeScreenResources(sr);

  return ac_result_success;
}

static void
ac_linux_window_shutdown(ac_window window_handle)
{
  AC_FROM_HANDLE(window, ac_linux_window);

  if (window->handle)
  {
    window->XUnmapWindow(window->dpy, window->handle);
    window->XDestroyWindow(window->dpy, window->handle);
    window->handle = (Window)0;
  }

  if (window->colormap)
  {
    window->XFreeColormap(window->dpy, window->colormap);
    window->colormap = (Colormap)0;
  }

  if (window->hidden_cursor)
  {
    window->XFreeCursor(window->dpy, window->hidden_cursor);
  }

  if (window->dpy)
  {
    window->XCloseDisplay(window->dpy);
  }

  ac_unload_library(window->vk);
  ac_unload_library(window->xrandr);
  ac_unload_library(window->x11);
}

static ac_result
ac_linux_window_set_state(ac_window window_handle, const ac_window_state* state)
{
  AC_FROM_HANDLE(window, ac_linux_window);

  uint32_t hint_width = state->width;
  uint32_t hint_height = state->height;

  if (window->_NET_WM_STATE && !state->monitor)
  {
    Atom states[3];
    int  count = 0;

    bool maximized = state->width == 0 || state->height == 0;

    if (maximized)
    {
      if (
        window->_NET_WM_STATE_MAXIMIZED_VERT &&
        window->_NET_WM_STATE_MAXIMIZED_HORZ)
      {
        states[count++] = window->_NET_WM_STATE_MAXIMIZED_VERT;
        states[count++] = window->_NET_WM_STATE_MAXIMIZED_HORZ;
      }

      XWindowAttributes attribs;
      window->XGetWindowAttributes(window->dpy, window->handle, &attribs);
      hint_width = (uint32_t)attribs.screen->width;
      hint_height = (uint32_t)attribs.screen->height;
    }

    if (count)
    {
      window->XChangeProperty(
        window->dpy,
        window->handle,
        window->_NET_WM_STATE,
        XA_ATOM,
        32,
        PropModeReplace,
        (unsigned char*)states,
        count);
    }
  }

  // TODO: force these settings, half of wm won't apply them on second set
  {
    XSizeHints* hints = window->XAllocSizeHints();
    if (!hints)
    {
      AC_ERROR("[ xlib ]: failed to allocate size hints");
      return ac_result_unknown_error;
    }

    if (!state->resizable)
    {
      hints->flags |= (PMinSize | PMaxSize);
      hints->min_width = hints->max_width = hint_width;
      hints->min_height = hints->max_height = hint_height;
    }

    hints->flags |= (PSize | PWinGravity);
    hints->width = hint_width;
    hints->height = hint_height;
    hints->win_gravity = StaticGravity;

    window->XSetWMNormalHints(window->dpy, window->handle, hints);
    window->XFree(hints);
  }

  {
    struct {
      unsigned long flags;
      unsigned long functions;
      unsigned long decorations;
      long          input_mode;
      unsigned long status;
    } hints = {0};

    hints.flags = MWM_HINTS_DECORATIONS;
    hints.decorations = state->borderless ? 0 : MWM_DECOR_ALL;

    window->XChangeProperty(
      window->dpy,
      window->handle,
      window->_MOTIF_WM_HINTS,
      window->_MOTIF_WM_HINTS,
      32,
      PropModeReplace,
      (unsigned char*)&hints,
      sizeof(hints) / sizeof(long));
  }

  {
    XWindowAttributes attr;
    window->XGetWindowAttributes(window->dpy, window->handle, &attr);

    XEvent event;

    if (attr.map_state == IsUnmapped)
    {
      window->XMapRaised(window->dpy, window->handle);
      window->XIfEvent(
        window->dpy,
        &event,
        &ac_linux_window_is_map_notify,
        (XPointer)&window->handle);
      window->XFlush(window->dpy);
    }
  }

  {
    XWindowAttributes wa;
    window->XGetWindowAttributes(window->dpy, window->handle, &wa);

    if (window->_NET_ACTIVE_WINDOW)
    {
      ac_linux_window_send_event_to_wm(
        window_handle,
        window->_NET_ACTIVE_WINDOW,
        1,
        0,
        0,
        0,
        0);
    }
    else if (wa.map_state == IsViewable)
    {
      window->XRaiseWindow(window->dpy, window->handle);
      window->XSetInputFocus(
        window->dpy,
        window->handle,
        RevertToParent,
        CurrentTime);
    }
  }

  if (state->monitor)
  {
    AC_FROM_HANDLE2(monitor, state->monitor, ac_linux_monitor);

    ac_linux_window_send_event_to_wm(
      window_handle,
      window->_NET_WM_STATE_FULLSCREEN,
      monitor->index,
      monitor->index,
      monitor->index,
      monitor->index,
      0);

    ac_linux_window_send_event_to_wm(
      window_handle,
      window->_NET_WM_STATE,
      (long)window->_NET_WM_STATE_ADD,
      (long)window->_NET_WM_STATE_FULLSCREEN,
      0,
      1,
      0);
  }

  window->XFlush(window->dpy);

  {
    ac_window_state* dst = &window->common.state;

    XWindowAttributes attribs;
    window->XGetWindowAttributes(window->dpy, window->handle, &attribs);

    dst->width = (uint32_t)attribs.width;
    dst->height = (uint32_t)attribs.height;
  }

  return ac_result_success;
}

static float
ac_linux_window_get_dpi_scale(ac_window window_handle)
{
  AC_FROM_HANDLE(window, ac_linux_window);

  return window->dpi;
}

static void
ac_linux_window_hide_cursor(ac_window window_handle, bool hide)
{
  AC_FROM_HANDLE(window, ac_linux_window);

  if (hide)
  {
    window->XDefineCursor(window->dpy, window->handle, window->hidden_cursor);
    window->XFlush(window->dpy);
  }
  else
  {
    window->XUndefineCursor(window->dpy, window->handle);
    window->XFlush(window->dpy);
  }
}

static void
ac_linux_window_grab_cursor(ac_window window_handle, bool grab)
{
  AC_FROM_HANDLE(window, ac_linux_window);

  if (grab)
  {
    XWindowAttributes attribs;
    window->XGetWindowAttributes(window->dpy, window->handle, &attribs);

    ac_monitor monitor = window->common.state.monitor;

    int x = monitor->x + attribs.x + (attribs.width / 2);
    int y = monitor->y + attribs.y + (attribs.height / 2);

    window->XWarpPointer(window->dpy, None, window->handle, 0, 0, 0, 0, x, y);

    window->XGrabPointer(
      window->dpy,
      window->handle,
      False,
      ButtonPressMask | ButtonReleaseMask | PointerMotionMask | FocusChangeMask,
      GrabModeAsync,
      GrabModeAsync,
      window->handle,
      None,
      CurrentTime);
  }
  else
  {
    AC_FROM_HANDLE(window, ac_linux_window);
    window->XUngrabPointer(window->dpy, CurrentTime);
  }

  window->XFlush(window->dpy);
}

static ac_result
ac_linux_window_get_cursor_position(ac_window window_handle, float* x, float* y)
{
  AC_FROM_HANDLE(window, ac_linux_window);

  Window       root, child;
  int          root_x, root_y, child_x, child_y;
  unsigned int mask;

  window->XQueryPointer(
    window->dpy,
    window->handle,
    &root,
    &child,
    &root_x,
    &root_y,
    &child_x,
    &child_y,
    &mask);

  *x = (float)child_x;
  *y = (float)child_y;

  return ac_result_success;
}

static void
ac_linux_window_poll_events(ac_window window_handle)
{
  AC_FROM_HANDLE(window, ac_linux_window);

  while (window->XPending(window->dpy))
  {
    XEvent event;
    window->XNextEvent(window->dpy, &event);

    switch (event.type)
    {
    case FocusOut:
    {
      ac_window_event e;
      e.type = ac_window_event_type_focus_lost;
      ac_window_send_event(window_handle, &e);
      break;
    }
    case FocusIn:
    {
      ac_window_event e;
      e.type = ac_window_event_type_focus_own;
      ac_window_send_event(window_handle, &e);
      break;
    }
    case LeaveNotify:
    {
      ac_window_event e;
      e.type = ac_window_event_type_focus_lost;
      ac_window_send_event(window_handle, &e);
      break;
    }
    case EnterNotify:
    {
      ac_window_event e;
      e.type = ac_window_event_type_focus_own;
      ac_window_send_event(window_handle, &e);
      break;
    }
    case ClientMessage:
    {
      if (event.xclient.message_type == None)
      {
        return;
      }

      if (event.xclient.message_type == window->WM_PROTOCOLS)
      {
        const Atom protocol = (const Atom)event.xclient.data.l[0];
        if (protocol == None)
        {
          return;
        }

        if (protocol == window->WM_DELETE_WINDOW)
        {
          ac_window_event e;
          e.type = ac_window_event_type_close;
          ac_window_send_event(window_handle, &e);
        }
      }
      break;
    }
    case ConfigureNotify:
    {
      ac_window_event e;
      e.type = ac_window_event_type_resize;
      e.resize.width = (uint32_t)event.xconfigure.width;
      e.resize.height = (uint32_t)event.xconfigure.height;
      ac_window_send_event(window_handle, &e);
      break;
    }
    case DestroyNotify:
    {
      break;
    }
    default:
    {
      break;
    }
    }
  }
}

static ac_result
ac_linux_window_get_vk_instance_extensions(
  ac_window    window_handle,
  uint32_t*    count,
  const char** names)
{
  AC_UNUSED(window_handle);

  static const char* extensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
  };

  *count = AC_COUNTOF(extensions);

  if (names == NULL)
  {
    return ac_result_success;
  }

  for (uint32_t i = 0; i < *count; ++i)
  {
    names[i] = extensions[i];
  }

  return ac_result_success;
}

static ac_result
ac_linux_window_create_vk_surface(
  ac_window window_handle,
  void*     instance,
  void**    surface)
{
  AC_FROM_HANDLE(window, ac_linux_window);

  if (!window->vkGetInstanceProcAddr)
  {
    return ac_result_unknown_error;
  }

  PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR =
    (PFN_vkCreateXlibSurfaceKHR)(void*)window->vkGetInstanceProcAddr(
      (VkInstance)instance,
      "vkCreateXlibSurfaceKHR");

  if (!vkCreateXlibSurfaceKHR)
  {
    return ac_result_unknown_error;
  }

  VkXlibSurfaceCreateInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
    .window = window->handle,
    .dpy = window->dpy,
  };

  VkResult res = vkCreateXlibSurfaceKHR(
    (VkInstance)instance,
    &info,
    NULL,
    (VkSurfaceKHR*)surface);

  if (res != VK_SUCCESS)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

ac_result
ac_linux_window_init(const char* app_name, ac_window* window_handle)
{
  AC_INIT_INTERNAL(window, ac_linux_window);

  window->common.shutdown = ac_linux_window_shutdown;
  window->common.set_state = ac_linux_window_set_state;
  window->common.get_dpi_scale = ac_linux_window_get_dpi_scale;
  window->common.hide_cursor = ac_linux_window_hide_cursor;
  window->common.grab_cursor = ac_linux_window_grab_cursor;
  window->common.get_cursor_position = ac_linux_window_get_cursor_position;
  window->common.poll_events = ac_linux_window_poll_events;
  window->common.get_vk_instance_extensions =
    ac_linux_window_get_vk_instance_extensions;
  window->common.create_vk_surface = ac_linux_window_create_vk_surface;

  if (ac_load_library("libX11.so", &window->x11, false) != ac_result_success)
  {
    AC_ERROR("[ window ] [ linux ]: failed to load x11");
    return ac_result_unknown_error;
  }

#define LOAD_FUN(X)                                                            \
  AC_RIF(ac_load_function(window->x11, #X, (void**)&window->X))
  LOAD_FUN(XOpenDisplay);
  LOAD_FUN(XFlush);
  LOAD_FUN(XCloseDisplay);
  LOAD_FUN(XPending);
  LOAD_FUN(XNextEvent);
  LOAD_FUN(XAllocClassHint);
  LOAD_FUN(XAllocSizeHints);
  LOAD_FUN(XAllocWMHints);
  LOAD_FUN(XSetClassHint);
  LOAD_FUN(XSetWMHints);
  LOAD_FUN(XSetWMNormalHints);
  LOAD_FUN(XSetWMProtocols);
  LOAD_FUN(XFree);
  LOAD_FUN(XSendEvent);
  LOAD_FUN(XCreateBitmapFromData);
  LOAD_FUN(XCreatePixmapCursor);
  LOAD_FUN(XFreePixmap);
  LOAD_FUN(XDefineCursor);
  LOAD_FUN(XUndefineCursor);
  LOAD_FUN(XFreeCursor);
  LOAD_FUN(XRootWindow);
  LOAD_FUN(XInternAtom);
  LOAD_FUN(XGetWindowAttributes);
  LOAD_FUN(XCreateWindow);
  LOAD_FUN(XMapRaised);
  LOAD_FUN(XRaiseWindow);
  LOAD_FUN(XSetInputFocus);
  LOAD_FUN(XUnmapWindow);
  LOAD_FUN(XDestroyWindow);
  LOAD_FUN(XCreateColormap);
  LOAD_FUN(XFreeColormap);
  LOAD_FUN(XQueryPointer);
  LOAD_FUN(XWarpPointer);
  LOAD_FUN(XGrabPointer);
  LOAD_FUN(XUngrabPointer);
  LOAD_FUN(XIfEvent);
  LOAD_FUN(XChangeProperty);
  LOAD_FUN(Xutf8SetWMProperties);
  LOAD_FUN(XResourceManagerString);
  LOAD_FUN(XrmInitialize);
  LOAD_FUN(XrmGetStringDatabase);
  LOAD_FUN(XrmGetResource);
  LOAD_FUN(XrmDestroyDatabase);
#undef LOAD_FUN

  if (
    ac_load_library("libXrandr.so", &window->xrandr, false) !=
    ac_result_success)
  {
    AC_ERROR("[ window ] [ linux ]: failed to load xrandr");
    return ac_result_unknown_error;
  }

#define LOAD_FUN(X)                                                            \
  AC_RIF(ac_load_function(window->xrandr, #X, (void**)&window->X))
  LOAD_FUN(XRRGetScreenResourcesCurrent);
  LOAD_FUN(XRRFreeScreenResources);
  LOAD_FUN(XRRGetOutputInfo);
  LOAD_FUN(XRRFreeOutputInfo);
  LOAD_FUN(XRRGetCrtcInfo);
  LOAD_FUN(XRRFreeCrtcInfo);
#undef LOAD_FUN

  window->dpy = window->XOpenDisplay(NULL);

  if (!window->dpy)
  {
    AC_ERROR("[ window ] [ linux ] : failed to open display");
    return ac_result_unknown_error;
  }

  window->screen = DefaultScreen(window->dpy);
  window->root = window->XRootWindow(window->dpy, window->screen);

  ac_linux_window_load_atoms(*window_handle);
  ac_linux_window_create_cursors(*window_handle);
  ac_linux_window_poll_monitors(*window_handle);

  window->dpi = 96.0f;

  window->XrmInitialize();
  char* resource_string = window->XResourceManagerString(window->dpy);

  if (resource_string)
  {
    XrmDatabase db;
    XrmValue    value;
    char*       type = NULL;
    db = window->XrmGetStringDatabase(resource_string);

    if (window->XrmGetResource(db, "Xft.dpi", "String", &type, &value) == True)
    {
      if (value.addr)
      {
        window->dpi = (float)atof(value.addr);
      }
    }
    window->XrmDestroyDatabase(db);

    window->dpi /= 96.0f;
  }

  const char* vk_lib_names[] = {
    "libvulkan.so.1",
    "libvulkan.so",
  };

  for (uint32_t i = 0; i < AC_COUNTOF(vk_lib_names); ++i)
  {
    void* vk_lib = NULL;

    if (ac_load_library(vk_lib_names[i], &vk_lib, false) == ac_result_success)
    {
      ac_load_function(
        vk_lib,
        "vkGetInstanceProcAddr",
        (void**)&window->vkGetInstanceProcAddr);
      break;
    }
  }

  Visual* visual = DefaultVisual(window->dpy, window->screen);
  int     depth = DefaultDepth(window->dpy, window->screen);

  window->colormap =
    window->XCreateColormap(window->dpy, window->root, visual, AllocNone);

  XSetWindowAttributes wa = {
    .colormap = window->colormap,
    .event_mask = StructureNotifyMask | KeyPressMask | KeyReleaseMask |
                  PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
                  ExposureMask | FocusChangeMask | VisibilityChangeMask |
                  EnterWindowMask | LeaveWindowMask | PropertyChangeMask,
  };

  window->handle = window->XCreateWindow(
    window->dpy,
    window->root,
    0,
    0,
    1,
    1,
    0,
    depth,
    InputOutput,
    visual,
    CWBorderPixel | CWColormap | CWEventMask,
    &wa);

  if (!window->handle)
  {
    AC_ERROR("[ xlib ]: failed to create window");
    return ac_result_unknown_error;
  }

  {
    window->Xutf8SetWMProperties(
      window->dpy,
      window->handle,
      app_name,
      app_name,
      NULL,
      0,
      NULL,
      NULL,
      NULL);

    window->XChangeProperty(
      window->dpy,
      window->handle,
      window->_NET_WM_NAME,
      window->UTF8_STRING,
      8,
      PropModeReplace,
      (const unsigned char*)app_name,
      (int32_t)strlen(app_name));

    window->XChangeProperty(
      window->dpy,
      window->handle,
      window->_NET_WM_ICON_NAME,
      window->UTF8_STRING,
      8,
      PropModeReplace,
      (const unsigned char*)app_name,
      (int32_t)strlen(app_name));
  }

  {
    Atom protocols[] = {
      window->WM_DELETE_WINDOW,
      window->_NET_WM_PING,
    };

    window->XSetWMProtocols(
      window->dpy,
      window->handle,
      protocols,
      sizeof(protocols) / sizeof(Atom));
  }

  {
    const long pid = getpid();

    window->XChangeProperty(
      window->dpy,
      window->handle,
      window->_NET_WM_PID,
      XA_CARDINAL,
      32,
      PropModeReplace,
      (const unsigned char*)&pid,
      1);
  }

  {
    XWMHints* hints = window->XAllocWMHints();
    if (!hints)
    {
      AC_ERROR("[ xlib ]: failed to allocate wm hints");
      return ac_result_unknown_error;
    }

    hints->flags = StateHint;
    hints->initial_state = NormalState;

    window->XSetWMHints(window->dpy, window->handle, hints);
    window->XFree(hints);
  }

  {
    XClassHint* hint = window->XAllocClassHint();
    hint->res_name = ac_const_cast(app_name);
    hint->res_class = ac_const_cast(app_name);

    window->XSetClassHint(window->dpy, window->handle, hint);
    window->XFree(hint);
  }

  return ac_result_success;
}

#endif
