#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS && AC_INCLUDE_INPUT)

#include <Windows.h>
#include <Xinput.h>
#include "input.h"

#define AC_CLASS_INPUT L"ac.class.input"

typedef enum ac_windows_hid_usage_page {
  ac_windows_hid_usage_page_generic = 0x01,
} ac_windows_hid_usage_page;

typedef enum ac_windows_hid_usage {
  ac_windows_hid_usage_generic_mouse = 0x02,
  ac_windows_hid_usage_generic_gamepad = 0x05,
  ac_windows_hid_usage_generic_keyboard = 0x06,
} ac_windows_hid_usage;

typedef struct ac_windows_input {
  ac_input_internal common;
  HINSTANCE         hinstance;
  ATOM              input_class;
  HWND              hwnd;
} ac_windows_input;

static inline float
ac_windows_gamepad_normalize_axis(int16_t v, bool trigger)
{
  float value = (float)v;

  if (trigger)
  {
    return value / (float)UINT8_MAX;
  }

  return (value < 0) ? -(value / (float)INT16_MIN) : (value / (float)INT16_MAX);
}

static inline ac_key
ac_key_from_windows(int32_t scancode)
{
  switch (scancode)
  {
  case 'A':
    return ac_key_a;
  case 'B':
    return ac_key_b;
  case 'C':
    return ac_key_c;
  case 'D':
    return ac_key_d;
  case 'E':
    return ac_key_e;
  case 'F':
    return ac_key_f;
  case 'G':
    return ac_key_g;
  case 'H':
    return ac_key_h;
  case 'I':
    return ac_key_i;
  case 'J':
    return ac_key_j;
  case 'K':
    return ac_key_k;
  case 'L':
    return ac_key_l;
  case 'M':
    return ac_key_m;
  case 'N':
    return ac_key_n;
  case 'O':
    return ac_key_o;
  case 'P':
    return ac_key_p;
  case 'Q':
    return ac_key_q;
  case 'R':
    return ac_key_r;
  case 'S':
    return ac_key_s;
  case 'T':
    return ac_key_t;
  case 'U':
    return ac_key_u;
  case 'V':
    return ac_key_v;
  case 'W':
    return ac_key_w;
  case 'X':
    return ac_key_x;
  case 'Y':
    return ac_key_y;
  case 'Z':
    return ac_key_z;
  case '0':
    return ac_key_zero;
  case '1':
    return ac_key_one;
  case '2':
    return ac_key_two;
  case '3':
    return ac_key_three;
  case '4':
    return ac_key_four;
  case '5':
    return ac_key_five;
  case '6':
    return ac_key_six;
  case '7':
    return ac_key_seven;
  case '8':
    return ac_key_eight;
  case '9':
    return ac_key_nine;
  case VK_RETURN:
    return ac_key_return;
  case VK_ESCAPE:
    return ac_key_escape;
  case VK_BACK:
    return ac_key_backspace;
  case VK_TAB:
    return ac_key_tab;
  case VK_SPACE:
    return ac_key_spacebar;
  case VK_OEM_MINUS:
    return ac_key_hyphen;
  case VK_OEM_PLUS:
    return ac_key_equal_sign;
  case 219:
    return ac_key_open_bracket;
  case 221:
    return ac_key_close_bracket;
  case 226:
    return ac_key_backslash;
  case 186:
    return ac_key_semicolon;
  case 222:
    return ac_key_quote;
  case 192:
    return ac_key_tilde;
  case VK_OEM_COMMA:
    return ac_key_comma;
  case VK_OEM_PERIOD:
    return ac_key_period;
  case 191:
    return ac_key_slash;
  case VK_CAPITAL:
    return ac_key_caps_lock;
  case VK_F1:
    return ac_key_f1;
  case VK_F2:
    return ac_key_f2;
  case VK_F3:
    return ac_key_f3;
  case VK_F4:
    return ac_key_f4;
  case VK_F5:
    return ac_key_f5;
  case VK_F6:
    return ac_key_f6;
  case VK_F7:
    return ac_key_f7;
  case VK_F8:
    return ac_key_f8;
  case VK_F9:
    return ac_key_f9;
  case VK_F10:
    return ac_key_f10;
  case VK_F11:
    return ac_key_f11;
  case VK_F12:
    return ac_key_f12;
  case VK_SNAPSHOT:
    return ac_key_print_screen;
  case VK_SCROLL:
    return ac_key_scroll_lock;
  case VK_PAUSE:
    return ac_key_pause;
  case VK_INSERT:
    return ac_key_insert;
  case VK_HOME:
    return ac_key_home;
  case VK_PRIOR:
    return ac_key_page_up;
  case VK_DELETE:
    return ac_key_delete;
  case VK_END:
    return ac_key_end;
  case VK_NEXT:
    return ac_key_page_down;
  case VK_RIGHT:
    return ac_key_right_arrow;
  case VK_LEFT:
    return ac_key_left_arrow;
  case VK_DOWN:
    return ac_key_down_arrow;
  case VK_UP:
    return ac_key_up_arrow;
  case VK_NUMLOCK:
    return ac_key_keypad_num_lock;
  case VK_DIVIDE:
    return ac_key_keypad_slash;
  case VK_MULTIPLY:
    return ac_key_keypad_asterisk;
  case VK_SUBTRACT:
    return ac_key_keypad_hyphen;
  case VK_ADD:
    return ac_key_keypad_plus;
  case VK_NUMPAD0:
    return ac_key_keypad_0;
  case VK_NUMPAD1:
    return ac_key_keypad_1;
  case VK_NUMPAD2:
    return ac_key_keypad_2;
  case VK_NUMPAD3:
    return ac_key_keypad_3;
  case VK_NUMPAD4:
    return ac_key_keypad_4;
  case VK_NUMPAD5:
    return ac_key_keypad_5;
  case VK_NUMPAD6:
    return ac_key_keypad_6;
  case VK_NUMPAD7:
    return ac_key_keypad_7;
  case VK_NUMPAD8:
    return ac_key_keypad_8;
  case VK_NUMPAD9:
    return ac_key_keypad_9;
  case VK_LCONTROL:
    return ac_key_left_control;
  case VK_LSHIFT:
    return ac_key_left_shift;
  case VK_MENU:
  case VK_LMENU:
    return ac_key_left_alt;
  case VK_LWIN:
    return ac_key_left_super;
  case VK_RCONTROL:
    return ac_key_right_control;
  case VK_RSHIFT:
    return ac_key_right_shift;
  case VK_RMENU:
    return ac_key_right_alt;
  case VK_RWIN:
    return ac_key_right_super;
  default:
    return ac_key_unknown;
  }
}

#endif
