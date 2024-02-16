#pragma once

#include "ac_private.h"

#if (AC_PLATFORM_LINUX && AC_INCLUDE_WINDOW)

#include <unistd.h>
#define VK_NO_PROTOTYPES 1
#define VK_USE_PLATFORM_XLIB_KHR 1
#include <vulkan/vulkan.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/extensions/Xrandr.h>

#include "window.h"

typedef Display* (*PFN_XOpenDisplay)(const char*);
typedef int (*PFN_XCloseDisplay)(Display*);
typedef int (*PFN_XPending)(Display*);
typedef int (*PFN_XNextEvent)(Display*, XEvent*);
typedef XClassHint* (*PFN_XAllocClassHint)(void);
typedef XSizeHints* (*PFN_XAllocSizeHints)(void);
typedef XWMHints* (*PFN_XAllocWMHints)(void);
typedef int (*PFN_XSetClassHint)(Display*, Window, XClassHint*);
typedef int (*PFN_XSetWMHints)(Display*, Window, XWMHints*);
typedef void (*PFN_XSetWMNormalHints)(Display*, Window, XSizeHints*);
typedef Status (*PFN_XSetWMProtocols)(Display*, Window, Atom*, int);
typedef int (*PFN_XFlush)(Display*);
typedef int (*PFN_XFree)(void*);
typedef Status (*PFN_XSendEvent)(Display*, Window, Bool, long, XEvent*);
typedef Pixmap (*PFN_XCreateBitmapFromData)(
  Display*,
  Drawable,
  _Xconst char*,
  unsigned int,
  unsigned int);
typedef Cursor (*PFN_XCreatePixmapCursor)(
  Display*,
  Pixmap,
  Pixmap,
  XColor*,
  XColor*,
  unsigned int,
  unsigned int);
typedef int (*PFN_XFreePixmap)(Display*, Pixmap);
typedef int (*PFN_XDefineCursor)(Display*, Window, Cursor);
typedef int (*PFN_XUndefineCursor)(Display*, Window);
typedef int (*PFN_XFreeCursor)(Display*, Cursor);
typedef Window (*PFN_XRootWindow)(Display*, int);
typedef Atom (*PFN_XInternAtom)(Display*, const char*, Bool);
typedef Status (
  *PFN_XGetWindowAttributes)(Display*, Window, XWindowAttributes*);
typedef Window (*PFN_XCreateWindow)(
  Display*,
  Window,
  int,
  int,
  unsigned int,
  unsigned int,
  unsigned int,
  int,
  unsigned int,
  Visual*,
  unsigned long,
  XSetWindowAttributes*);
typedef int (*PFN_XMapRaised)(Display*, Window);
typedef int (*PFN_XRaiseWindow)(Display*, Window);
typedef int (*PFN_XSetInputFocus)(Display*, Window, int, Time);
typedef int (*PFN_XUnmapWindow)(Display*, Window);
typedef int (*PFN_XDestroyWindow)(Display*, Window);
typedef Colormap (*PFN_XCreateColormap)(Display*, Window, Visual*, int);
typedef int (*PFN_XFreeColormap)(Display*, Colormap);
typedef Bool (*PFN_XQueryPointer)(
  Display*,
  Window,
  Window*,
  Window*,
  int*,
  int*,
  int*,
  int*,
  unsigned int*);
typedef int (*PFN_XWarpPointer)(
  Display*,
  Window,
  Window,
  int,
  int,
  unsigned int,
  unsigned int,
  int,
  int);
typedef int (*PFN_XGrabPointer)(
  Display*,
  Window,
  Bool,
  unsigned int,
  int,
  int,
  Window,
  Cursor,
  Time);
typedef int (*PFN_XUngrabPointer)(Display*, Time);
typedef int (*PFN_XIfEvent)(
  Display*,
  XEvent*,
  Bool (*)(Display*, XEvent*, XPointer),
  XPointer);
typedef int (*PFN_XChangeProperty)(
  Display*,
  Window,
  Atom,
  Atom,
  int,
  int,
  const unsigned char*,
  int);
typedef void (*PFN_Xutf8SetWMProperties)(
  Display*,
  Window,
  const char*,
  const char*,
  char**,
  int,
  XSizeHints*,
  XWMHints*,
  XClassHint*);
typedef char* (*PFN_XResourceManagerString)(Display*);
typedef void (*PFN_XrmInitialize)(void);
typedef XrmDatabase (*PFN_XrmGetStringDatabase)(_Xconst char*);
typedef Bool (*PFN_XrmGetResource)(
  XrmDatabase,
  _Xconst char*,
  _Xconst char*,
  char**,
  XrmValue*);
typedef void (*PFN_XrmDestroyDatabase)(XrmDatabase);


typedef XRRScreenResources* (
  *PFN_XRRGetScreenResourcesCurrent)(Display* dpy, Window window);
typedef void (*PFN_XRRFreeScreenResources)(XRRScreenResources*);
typedef XRROutputInfo* (
  *PFN_XRRGetOutputInfo)(Display*, XRRScreenResources*, RROutput);
typedef void (*PFN_XRRFreeOutputInfo)(XRROutputInfo*);
typedef XRRCrtcInfo* (
  *PFN_XRRGetCrtcInfo)(Display*, XRRScreenResources*, RRCrtc);
typedef void (*PFN_XRRFreeCrtcInfo)(XRRCrtcInfo*);

typedef struct ac_linux_monitor {
  ac_monitor_internal common;
  RROutput            output;
  RRCrtc              crtc;
  RRMode              old_mode;
  uint32_t            index;
} ac_linux_monitor;

typedef struct ac_linux_window {
  ac_window_internal common;
  Display*           dpy;
  int                screen;
  Colormap           colormap;
  Window             root;
  Cursor             hidden_cursor;

  Atom   _NET_WM_NAME;
  Atom   _NET_WM_STATE;
  Atom   _NET_WM_STATE_FULLSCREEN;
  Atom   _NET_WM_STATE_MAXIMIZED_VERT;
  Atom   _NET_WM_STATE_MAXIMIZED_HORZ;
  Atom   _NET_WM_PING;
  Atom   _NET_WM_PID;
  Atom   WM_PROTOCOLS;
  Atom   WM_DELETE_WINDOW;
  Atom   UTF8_STRING;
  Atom   _NET_WM_ICON_NAME;
  Atom   _MOTIF_WM_HINTS;
  Atom   _NET_ACTIVE_WINDOW;
  Atom   _NET_WM_STATE_ADD;
  Atom   _NET_WM_STATE_REMOVE;
  Window handle;

  float dpi;

  void*                     vk;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;

  void*                      x11;
  PFN_XOpenDisplay           XOpenDisplay;
  PFN_XFlush                 XFlush;
  PFN_XCloseDisplay          XCloseDisplay;
  PFN_XPending               XPending;
  PFN_XNextEvent             XNextEvent;
  PFN_XAllocClassHint        XAllocClassHint;
  PFN_XAllocSizeHints        XAllocSizeHints;
  PFN_XAllocWMHints          XAllocWMHints;
  PFN_XSetClassHint          XSetClassHint;
  PFN_XSetWMHints            XSetWMHints;
  PFN_XSetWMNormalHints      XSetWMNormalHints;
  PFN_XSetWMProtocols        XSetWMProtocols;
  PFN_XFree                  XFree;
  PFN_XSendEvent             XSendEvent;
  PFN_XCreateBitmapFromData  XCreateBitmapFromData;
  PFN_XCreatePixmapCursor    XCreatePixmapCursor;
  PFN_XFreePixmap            XFreePixmap;
  PFN_XDefineCursor          XDefineCursor;
  PFN_XUndefineCursor        XUndefineCursor;
  PFN_XFreeCursor            XFreeCursor;
  PFN_XRootWindow            XRootWindow;
  PFN_XInternAtom            XInternAtom;
  PFN_XGetWindowAttributes   XGetWindowAttributes;
  PFN_XCreateWindow          XCreateWindow;
  PFN_XMapRaised             XMapRaised;
  PFN_XRaiseWindow           XRaiseWindow;
  PFN_XSetInputFocus         XSetInputFocus;
  PFN_XUnmapWindow           XUnmapWindow;
  PFN_XDestroyWindow         XDestroyWindow;
  PFN_XCreateColormap        XCreateColormap;
  PFN_XFreeColormap          XFreeColormap;
  PFN_XQueryPointer          XQueryPointer;
  PFN_XWarpPointer           XWarpPointer;
  PFN_XGrabPointer           XGrabPointer;
  PFN_XUngrabPointer         XUngrabPointer;
  PFN_XIfEvent               XIfEvent;
  PFN_XChangeProperty        XChangeProperty;
  PFN_Xutf8SetWMProperties   Xutf8SetWMProperties;
  PFN_XResourceManagerString XResourceManagerString;
  PFN_XrmInitialize          XrmInitialize;
  PFN_XrmGetStringDatabase   XrmGetStringDatabase;
  PFN_XrmGetResource         XrmGetResource;
  PFN_XrmDestroyDatabase     XrmDestroyDatabase;

  void*                            xrandr;
  PFN_XRRGetScreenResourcesCurrent XRRGetScreenResourcesCurrent;
  PFN_XRRFreeScreenResources       XRRFreeScreenResources;
  PFN_XRRGetOutputInfo             XRRGetOutputInfo;
  PFN_XRRFreeOutputInfo            XRRFreeOutputInfo;
  PFN_XRRGetCrtcInfo               XRRGetCrtcInfo;
  PFN_XRRFreeCrtcInfo              XRRFreeCrtcInfo;
} ac_linux_window;

#endif
