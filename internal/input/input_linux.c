#include "ac_private.h"

#if (AC_PLATFORM_LINUX && AC_INCLUDE_INPUT)

#include "input_linux.h"

static inline void
ac_linux_input_connect_gamepad(ac_linux_gamepad* p)
{
  p->button_map[0] = ac_gamepad_button_a;
  p->button_map[1] = ac_gamepad_button_b;
  p->button_map[2] = ac_gamepad_button_x;
  p->button_map[3] = ac_gamepad_button_y;
  p->button_map[4] = ac_gamepad_button_left_shoulder;
  p->button_map[5] = ac_gamepad_button_right_shoulder;
  p->button_map[6] = ac_gamepad_button_unknown;
  p->button_map[7] = ac_gamepad_button_menu;
  p->button_map[8] = ac_gamepad_button_options;
  p->button_map[9] = ac_gamepad_button_left_thumbstick;
  p->button_map[10] = ac_gamepad_button_right_thumbstick;

  p->axis_map[0] = ac_gamepad_axis_left_thumbstick_x;
  p->axis_map[1] = ac_gamepad_axis_left_thumbstick_y;
  p->axis_map[2] = ac_gamepad_axis_left_trigger;
  p->axis_map[3] = ac_gamepad_axis_right_thumbstick_x;
  p->axis_map[4] = ac_gamepad_axis_right_thumbstick_y;
  p->axis_map[5] = ac_gamepad_axis_right_trigger;
  p->axis_map[6] = -ac_gamepad_button_dpad_right;
  p->axis_map[7] = -ac_gamepad_button_dpad_up;
}

static inline void
ac_linux_input_process_gamepad_event(
  ac_linux_gamepad* p,
  ac_gamepad_state* state,
  struct js_event*  event)
{
  static const float AC_MAX_AXIS_VALUE = 32767.0f;

  event->type &= ~(uint32_t)(JS_EVENT_INIT);

  ac_gamepad_axis   axis = ac_gamepad_axis_unknown;
  ac_gamepad_button button = ac_gamepad_button_unknown;
  float             value = 0;

  switch (event->type)
  {
  case JS_EVENT_AXIS:
  {
    value = (float)event->value / AC_MAX_AXIS_VALUE;

    if (p->axis_map[event->number] < 0)
    {
      button = (ac_gamepad_button)(-p->axis_map[event->number]);
      if (button == ac_gamepad_button_dpad_up)
      {
        if (value > 0)
        {
          button = ac_gamepad_button_dpad_down;
          value = true;
        }
        else if (value < 0)
        {
          button = ac_gamepad_button_dpad_up;
          value = true;
        }
      }
      else if (button == ac_gamepad_button_dpad_right)
      {
        if (value > 0)
        {
          button = ac_gamepad_button_dpad_right;
          value = true;
        }
        else if (value < 0)
        {
          button = ac_gamepad_button_dpad_left;
          value = true;
        }
      }
    }
    else
    {
      axis = (ac_gamepad_axis)p->axis_map[event->number];
      switch (axis)
      {
      case ac_gamepad_axis_left_thumbstick_y:
      case ac_gamepad_axis_right_thumbstick_y:
        value = -value;
        break;
      default:
        break;
      }
    }
    break;
  }
  case JS_EVENT_BUTTON:
  {
    button = (ac_gamepad_button)p->button_map[event->number];
    value = event->value;
    break;
  }
  }

  if (axis != ac_gamepad_axis_unknown)
  {
    state->axis[axis] = value;
  }
  else if (button != ac_gamepad_button_unknown)
  {
    state->buttons[button] = (bool)value;
  }
}

static inline void
ac_linux_input_get_gamepad_state(
  ac_input         input_handle,
  ac_gamepad_index gamepad_index)
{
  AC_FROM_HANDLE(input, ac_linux_input);

  ac_linux_gamepad* src = &input->gamepads[gamepad_index];
  ac_gamepad_state* dst = &input->common.gamepads[gamepad_index].state;

  if (src->fd < 0)
  {
    static const char* gamepad_ids[] = {
      "/dev/input/js0",
      "/dev/input/js1",
      "/dev/input/js2",
      "/dev/input/js3",
    };
    AC_STATIC_ASSERT(AC_COUNTOF(gamepad_ids) == AC_MAX_GAMEPADS);

    src->fd = open(gamepad_ids[gamepad_index], O_RDONLY | O_NONBLOCK);
    ac_linux_input_connect_gamepad(src);
  }

  dst->connected = src->fd >= 0;

  if (!dst->connected)
  {
    return;
  }

  struct js_event event;
  ssize_t         bytes = -1;

  while ((bytes = read(src->fd, &event, sizeof(struct js_event))) ==
         sizeof(struct js_event))
  {
    ac_linux_input_process_gamepad_event(src, dst, &event);
  }

  if (
    bytes == -1 &&
    (errno == EBADF || errno == ECONNRESET || errno == ENOTCONN ||
     errno == EIO || errno == ENXIO || errno == ENODEV))
  {
    AC_ZEROP(src);
    src->fd = -1;
    dst->connected = false;
  }
}

static inline void
ac_linux_window_process_input_event(ac_input input_handle, XEvent* event)
{
  AC_FROM_HANDLE(input, ac_linux_input);

  int         evtype = event->xcookie.evtype;
  XIRawEvent* re = event->xcookie.data;

  switch (evtype)
  {
  case XI_RawMotion:
  {
    if (re->valuators.mask_len)
    {
      // FIXME: this values can be different for different implemenations
      if (
        XIMaskIsSet(re->valuators.mask, 0) ||
        XIMaskIsSet(re->valuators.mask, 1))
      {
        ac_input_event e;
        e.type = ac_input_event_type_mouse_move;
        e.mouse_move.dx = (float)re->raw_values[0];
        e.mouse_move.dy = (float)-re->raw_values[1];
        ac_input_send_event(input_handle, &e);
      }
      else if (XIMaskIsSet(re->valuators.mask, 3))
      {
        ac_input_event e;
        e.type = ac_input_event_type_scroll;
        e.scroll.dx = 0;
        e.scroll.dy = (float)-re->raw_values[0];
        ac_input_send_event(input_handle, &e);
      }
    }

    break;
  }
  case XI_RawButtonPress:
  case XI_RawButtonRelease:
  {
    if (re->detail >= Button1 && re->detail <= Button3)
    {
      ac_input_event e;
      e.type = evtype == XI_RawButtonPress
                 ? ac_input_event_type_mouse_button_down
                 : ac_input_event_type_mouse_button_up;
      e.mouse_button = ac_button_from_linux(re->detail);
      ac_input_send_event(input_handle, &e);
    }
    break;
  }
  case XI_RawKeyPress:
  case XI_RawKeyRelease:
  {
    KeySym keysym =
      input->XkbKeycodeToKeysym(input->dpy, (KeyCode)re->detail, 0, 0);

    if (evtype == XI_RawKeyPress)
    {
      ac_input_event e;
      e.type = ac_input_event_type_key_down;
      e.key = ac_key_from_linux(keysym);
      ac_input_send_event(input_handle, &e);

      if (NoSymbol == keysym)
      {
        break;
      }
    }
    else
    {
      ac_input_event e;
      e.type = ac_input_event_type_key_up;
      e.key = ac_key_from_linux(keysym);
      ac_input_send_event(input_handle, &e);
    }
    break;
  }
  default:
  {
    break;
  }
  }
}

static ac_result
ac_linux_input_init(ac_input* input_handle)
{
  AC_INIT_INTERNAL(input, ac_linux_input);

  if (ac_load_library("libX11.so", &input->x11, false) != ac_result_success)
  {
    AC_ERROR("[ input ] [ linux ]: failed to load x11");
    return ac_result_unknown_error;
  }

#define LOAD_FUN(X) AC_RIF(ac_load_function(input->x11, #X, (void**)&input->X))
  LOAD_FUN(XOpenDisplay);
  LOAD_FUN(XRootWindow);
  LOAD_FUN(XPending);
  LOAD_FUN(XNextEvent);
  LOAD_FUN(XGetEventData);
  LOAD_FUN(XFreeEventData);
  LOAD_FUN(XkbKeycodeToKeysym);
#undef LOAD_FUN

  if (ac_load_library("libXi.so", &input->xi, false) != ac_result_success)
  {
    AC_ERROR("[ input ] [ linux ] : failed to load xi");
    return ac_result_unknown_error;
  }

#define LOAD_FUN(X) AC_RIF(ac_load_function(input->xi, #X, (void**)&input->X))
  LOAD_FUN(XISelectEvents);
#undef LOAD_FUN

  input->dpy = input->XOpenDisplay(NULL);

  if (!input->dpy)
  {
    AC_ERROR("[ input ] [ linux ] : failed to open display");
    return ac_result_unknown_error;
  }
  input->screen = DefaultScreen(input->dpy);
  input->root = input->XRootWindow(input->dpy, input->screen);

  XIEventMask   em;
  unsigned char mask[XIMaskLen(XI_LASTEVENT)];
  AC_ZERO(mask);
  em.deviceid = XIAllMasterDevices;
  em.mask_len = sizeof(mask);
  em.mask = mask;
  XISetMask(mask, XI_RawMotion);
  XISetMask(mask, XI_RawKeyPress);
  XISetMask(mask, XI_RawKeyRelease);
  XISetMask(mask, XI_RawButtonPress);
  XISetMask(mask, XI_RawButtonRelease);

  input->XISelectEvents(input->dpy, input->root, &em, 1);

  for (uint32_t i = 0; i < AC_MAX_GAMEPADS; ++i)
  {
    ac_linux_gamepad* gc = &input->gamepads[i];

    gc->fd = -1;
    gc->type = ac_gamepad_type_unknown;
  }

  return ac_result_success;
}

static void
ac_linux_input_shutdown(ac_input input_handle)
{
  AC_FROM_HANDLE(input, ac_linux_input);

  for (uint32_t i = 0; i < AC_MAX_GAMEPADS; ++i)
  {
    ac_linux_gamepad* gc = &input->gamepads[i];
    if (gc->fd >= 0)
    {
      close(gc->fd);
    }
    gc->fd = -1;
  }

  if (input->dpy)
  {
    input->XCloseDisplay(input->dpy);
  }

  ac_unload_library(input->xi);
  ac_unload_library(input->x11);
}

static void
ac_linux_input_poll_events(ac_input input_handle)
{
  AC_FROM_HANDLE(input, ac_linux_input);

  AC_UNUSED(input);

  while (input->XPending(input->dpy))
  {
    XEvent event;
    input->XNextEvent(input->dpy, &event);

    if (event.type != GenericEvent)
    {
      continue;
    }

    if (!input->XGetEventData(input->dpy, &event.xcookie))
    {
      continue;
    }

    ac_linux_window_process_input_event(input_handle, &event);

    input->XFreeEventData(input->dpy, &event.xcookie);
  }

  for (uint32_t i = 0; i < AC_MAX_GAMEPADS; ++i)
  {
    ac_linux_input_get_gamepad_state(input_handle, i);
  }
}

void
ac_linux_input_get_impl(ac_input_implementation* impl)
{
  impl->init = ac_linux_input_init;
  impl->shutdown = ac_linux_input_shutdown;
  impl->poll_events = ac_linux_input_poll_events;
}

#endif
