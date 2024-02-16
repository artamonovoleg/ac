#include "ac_private.h"

#if (AC_PLATFORM_LINUX && AC_INCLUDE_INPUT)

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <linux/joystick.h>
#include <X11/Xutil.h>
#include <X11/extensions/XInput2.h>

#include "input.h"

typedef Display* (*PFN_XOpenDisplay)(const char*);
typedef int (*PFN_XCloseDisplay)(Display*);
typedef Window (*PFN_XRootWindow)(Display*, int);
typedef int (*PFN_XPending)(Display*);
typedef int (*PFN_XNextEvent)(Display*, XEvent*);
typedef Bool (*PFN_XGetEventData)(Display*, XGenericEventCookie*);
typedef void (*PFN_XFreeEventData)(Display*, XGenericEventCookie*);

typedef KeySym (*PFN_XkbKeycodeToKeysym)(Display*, KeyCode, int, int);

typedef int (*PFN_XISelectEvents)(Display*, Window, XIEventMask*, int);

typedef struct ac_linux_gamepad {
  int32_t         fd;
  int32_t         button_map[KEY_CNT - BTN_MISC];
  int32_t         axis_map[ABS_CNT];
  ac_gamepad_type type;
} ac_linux_gamepad;

typedef struct ac_linux_input {
  ac_input_internal common;
  Display*          dpy;
  int               screen;
  Window            root;
  ac_linux_gamepad  gamepads[AC_MAX_GAMEPADS];

  void*                  x11;
  PFN_XOpenDisplay       XOpenDisplay;
  PFN_XCloseDisplay      XCloseDisplay;
  PFN_XRootWindow        XRootWindow;
  PFN_XPending           XPending;
  PFN_XNextEvent         XNextEvent;
  PFN_XGetEventData      XGetEventData;
  PFN_XFreeEventData     XFreeEventData;
  PFN_XkbKeycodeToKeysym XkbKeycodeToKeysym;
  void*                  xi;
  PFN_XISelectEvents     XISelectEvents;
} ac_linux_input;

static inline ac_key
ac_key_from_linux(KeySym keysym)
{
  switch (keysym)
  {
  case XK_A:
  case XK_a:
    return ac_key_a;
  case XK_B:
  case XK_b:
    return ac_key_b;
  case XK_C:
  case XK_c:
    return ac_key_c;
  case XK_D:
  case XK_d:
    return ac_key_d;
  case XK_E:
  case XK_e:
    return ac_key_e;
  case XK_F:
  case XK_f:
    return ac_key_f;
  case XK_G:
  case XK_g:
    return ac_key_g;
  case XK_H:
  case XK_h:
    return ac_key_h;
  case XK_I:
  case XK_i:
    return ac_key_i;
  case XK_J:
  case XK_j:
    return ac_key_j;
  case XK_K:
  case XK_k:
    return ac_key_k;
  case XK_L:
  case XK_l:
    return ac_key_l;
  case XK_M:
  case XK_m:
    return ac_key_m;
  case XK_N:
  case XK_n:
    return ac_key_n;
  case XK_O:
  case XK_o:
    return ac_key_o;
  case XK_P:
  case XK_p:
    return ac_key_p;
  case XK_Q:
  case XK_q:
    return ac_key_q;
  case XK_R:
  case XK_r:
    return ac_key_r;
  case XK_S:
  case XK_s:
    return ac_key_s;
  case XK_T:
  case XK_t:
    return ac_key_t;
  case XK_U:
  case XK_u:
    return ac_key_u;
  case XK_V:
  case XK_v:
    return ac_key_v;
  case XK_W:
  case XK_w:
    return ac_key_w;
  case XK_X:
  case XK_x:
    return ac_key_x;
  case XK_Y:
  case XK_y:
    return ac_key_y;
  case XK_Z:
  case XK_z:
    return ac_key_z;
  case XK_0:
    return ac_key_zero;
  case XK_1:
    return ac_key_one;
  case XK_2:
    return ac_key_two;
  case XK_3:
    return ac_key_three;
  case XK_4:
    return ac_key_four;
  case XK_5:
    return ac_key_five;
  case XK_6:
    return ac_key_six;
  case XK_7:
    return ac_key_seven;
  case XK_8:
    return ac_key_eight;
  case XK_9:
    return ac_key_nine;
  case XK_Return:
    return ac_key_return;
  case XK_Escape:
    return ac_key_escape;
  case XK_BackSpace:
    return ac_key_backspace;
  case XK_Tab:
    return ac_key_tab;
  case XK_space:
    return ac_key_spacebar;
  case XK_hyphen:
  case XK_minus:
    return ac_key_hyphen;
  case XK_equal:
    return ac_key_equal_sign;
  case XK_bracketleft:
    return ac_key_open_bracket;
  case XK_bracketright:
    return ac_key_close_bracket;
  case XK_backslash:
    return ac_key_backslash;
  case XK_semicolon:
    return ac_key_semicolon;
  case XK_grave:
    return ac_key_tilde;
  case XK_comma:
    return ac_key_comma;
  case XK_period:
    return ac_key_period;
  case XK_slash:
    return ac_key_slash;
  case XK_Caps_Lock:
    return ac_key_caps_lock;
  case XK_F1:
    return ac_key_f1;
  case XK_F2:
    return ac_key_f2;
  case XK_F3:
    return ac_key_f3;
  case XK_F4:
    return ac_key_f4;
  case XK_F5:
    return ac_key_f5;
  case XK_F6:
    return ac_key_f6;
  case XK_F7:
    return ac_key_f7;
  case XK_F8:
    return ac_key_f8;
  case XK_F9:
    return ac_key_f9;
  case XK_F10:
    return ac_key_f10;
  case XK_F11:
    return ac_key_f11;
  case XK_F12:
    return ac_key_f12;
  case XK_Print:
    return ac_key_print_screen;
  case XK_Scroll_Lock:
    return ac_key_scroll_lock;
  case XK_Pause:
    return ac_key_pause;
  case XK_Insert:
    return ac_key_insert;
  case XK_Home:
    return ac_key_home;
  case XK_Page_Up:
    return ac_key_page_up;
  case XK_Delete:
    return ac_key_delete;
  case XK_End:
    return ac_key_end;
  case XK_Page_Down:
    return ac_key_page_down;
  case XK_Right:
    return ac_key_right_arrow;
  case XK_Left:
    return ac_key_left_arrow;
  case XK_Down:
    return ac_key_down_arrow;
  case XK_Up:
    return ac_key_up_arrow;
  case XK_Num_Lock:
    return ac_key_keypad_num_lock;
  case XK_KP_Divide:
    return ac_key_keypad_slash;
  case XK_KP_Multiply:
    return ac_key_keypad_asterisk;
  case XK_KP_Subtract:
    return ac_key_keypad_hyphen;
  case XK_KP_Add:
    return ac_key_keypad_plus;
  case XK_KP_Enter:
    return ac_key_keypad_enter;
  case XK_KP_0:
    return ac_key_keypad_0;
  case XK_KP_1:
    return ac_key_keypad_1;
  case XK_KP_2:
    return ac_key_keypad_2;
  case XK_KP_3:
    return ac_key_keypad_3;
  case XK_KP_4:
    return ac_key_keypad_4;
  case XK_KP_5:
    return ac_key_keypad_5;
  case XK_KP_6:
    return ac_key_keypad_6;
  case XK_KP_7:
    return ac_key_keypad_7;
  case XK_KP_8:
    return ac_key_keypad_8;
  case XK_KP_9:
    return ac_key_keypad_9;
  case XK_Control_L:
    return ac_key_left_control;
  case XK_Shift_L:
    return ac_key_left_shift;
  case XK_Alt_L:
    return ac_key_left_alt;
  case XK_Super_L:
    return ac_key_left_super;
  case XK_Control_R:
    return ac_key_right_control;
  case XK_Shift_R:
    return ac_key_right_shift;
  case XK_Alt_R:
    return ac_key_right_alt;
  case XK_Super_R:
    return ac_key_right_super;
  default:
    break;
  }
  return ac_key_unknown;
}

static inline ac_mouse_button
ac_button_from_linux(int button)
{
  return (ac_mouse_button)button;
}

#endif
