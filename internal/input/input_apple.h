#include "ac_private.h"

#if (AC_PLATFORM_APPLE && AC_INCLUDE_INPUT)

#import <GameController/GameController.h>
#include "input.h"

// FIXME:
#define AC_GC_KEY_CODE_COUNT 250

typedef struct ac_apple_input {
  ac_input_internal   common;
  NSMutableArray<id>* observers;
  GCController*       controllers[AC_MAX_GAMEPADS];
  ac_gamepad_type     types[AC_MAX_GAMEPADS];
} ac_apple_input;

static inline ac_key
ac_key_from_apple(GCKeyCode key)
{
  if (key >= AC_GC_KEY_CODE_COUNT)
  {
    return ac_key_unknown;
  }

  static ac_key table[AC_GC_KEY_CODE_COUNT] = {0};

  table[GCKeyCodeKeyA] = ac_key_a;
  table[GCKeyCodeKeyB] = ac_key_b;
  table[GCKeyCodeKeyC] = ac_key_c;
  table[GCKeyCodeKeyD] = ac_key_d;
  table[GCKeyCodeKeyE] = ac_key_e;
  table[GCKeyCodeKeyF] = ac_key_f;
  table[GCKeyCodeKeyG] = ac_key_g;
  table[GCKeyCodeKeyH] = ac_key_h;
  table[GCKeyCodeKeyI] = ac_key_i;
  table[GCKeyCodeKeyJ] = ac_key_j;
  table[GCKeyCodeKeyK] = ac_key_k;
  table[GCKeyCodeKeyL] = ac_key_l;
  table[GCKeyCodeKeyM] = ac_key_m;
  table[GCKeyCodeKeyN] = ac_key_n;
  table[GCKeyCodeKeyO] = ac_key_o;
  table[GCKeyCodeKeyP] = ac_key_p;
  table[GCKeyCodeKeyQ] = ac_key_q;
  table[GCKeyCodeKeyR] = ac_key_r;
  table[GCKeyCodeKeyS] = ac_key_s;
  table[GCKeyCodeKeyT] = ac_key_t;
  table[GCKeyCodeKeyU] = ac_key_u;
  table[GCKeyCodeKeyV] = ac_key_v;
  table[GCKeyCodeKeyW] = ac_key_w;
  table[GCKeyCodeKeyX] = ac_key_x;
  table[GCKeyCodeKeyY] = ac_key_y;
  table[GCKeyCodeKeyZ] = ac_key_z;
  table[GCKeyCodeOne] = ac_key_one;
  table[GCKeyCodeTwo] = ac_key_two;
  table[GCKeyCodeThree] = ac_key_three;
  table[GCKeyCodeFour] = ac_key_four;
  table[GCKeyCodeFive] = ac_key_five;
  table[GCKeyCodeSix] = ac_key_six;
  table[GCKeyCodeSeven] = ac_key_seven;
  table[GCKeyCodeEight] = ac_key_eight;
  table[GCKeyCodeNine] = ac_key_nine;
  table[GCKeyCodeZero] = ac_key_zero;
  table[GCKeyCodeReturnOrEnter] = ac_key_return;
  table[GCKeyCodeEscape] = ac_key_escape;
  table[GCKeyCodeDeleteOrBackspace] = ac_key_backspace;
  table[GCKeyCodeTab] = ac_key_tab;
  table[GCKeyCodeSpacebar] = ac_key_spacebar;
  table[GCKeyCodeHyphen] = ac_key_hyphen;
  table[GCKeyCodeEqualSign] = ac_key_equal_sign;
  table[GCKeyCodeOpenBracket] = ac_key_open_bracket;
  table[GCKeyCodeCloseBracket] = ac_key_close_bracket;
  table[GCKeyCodeBackslash] = ac_key_backslash;
  table[GCKeyCodeSemicolon] = ac_key_semicolon;
  table[GCKeyCodeQuote] = ac_key_quote;
  table[GCKeyCodeGraveAccentAndTilde] = ac_key_tilde;
  table[GCKeyCodeComma] = ac_key_comma;
  table[GCKeyCodePeriod] = ac_key_period;
  table[GCKeyCodeSlash] = ac_key_slash;
  table[GCKeyCodeCapsLock] = ac_key_caps_lock;
  table[GCKeyCodeF1] = ac_key_f1;
  table[GCKeyCodeF2] = ac_key_f2;
  table[GCKeyCodeF3] = ac_key_f3;
  table[GCKeyCodeF4] = ac_key_f4;
  table[GCKeyCodeF5] = ac_key_f5;
  table[GCKeyCodeF6] = ac_key_f6;
  table[GCKeyCodeF7] = ac_key_f7;
  table[GCKeyCodeF8] = ac_key_f8;
  table[GCKeyCodeF9] = ac_key_f9;
  table[GCKeyCodeF10] = ac_key_f10;
  table[GCKeyCodeF11] = ac_key_f11;
  table[GCKeyCodeF12] = ac_key_f12;
  table[GCKeyCodePrintScreen] = ac_key_print_screen;
  table[GCKeyCodeScrollLock] = ac_key_scroll_lock;
  table[GCKeyCodePause] = ac_key_pause;
  table[GCKeyCodeInsert] = ac_key_insert;
  table[GCKeyCodeHome] = ac_key_home;
  table[GCKeyCodePageUp] = ac_key_page_up;
  table[GCKeyCodeDeleteForward] = ac_key_delete;
  table[GCKeyCodeEnd] = ac_key_end;
  table[GCKeyCodePageDown] = ac_key_page_down;
  table[GCKeyCodeRightArrow] = ac_key_right_arrow;
  table[GCKeyCodeLeftArrow] = ac_key_left_arrow;
  table[GCKeyCodeDownArrow] = ac_key_down_arrow;
  table[GCKeyCodeUpArrow] = ac_key_up_arrow;
  table[GCKeyCodeKeypadNumLock] = ac_key_keypad_num_lock;
  table[GCKeyCodeSlash] = ac_key_keypad_slash;
  table[GCKeyCodeKeypadAsterisk] = ac_key_keypad_asterisk;
  table[GCKeyCodeKeypadHyphen] = ac_key_keypad_hyphen;
  table[GCKeyCodeKeypadPlus] = ac_key_keypad_plus;
  table[GCKeyCodeKeypadEnter] = ac_key_keypad_enter;
  table[GCKeyCodeKeypad0] = ac_key_keypad_0;
  table[GCKeyCodeKeypad1] = ac_key_keypad_1;
  table[GCKeyCodeKeypad2] = ac_key_keypad_2;
  table[GCKeyCodeKeypad3] = ac_key_keypad_3;
  table[GCKeyCodeKeypad4] = ac_key_keypad_4;
  table[GCKeyCodeKeypad5] = ac_key_keypad_5;
  table[GCKeyCodeKeypad6] = ac_key_keypad_6;
  table[GCKeyCodeKeypad7] = ac_key_keypad_7;
  table[GCKeyCodeKeypad8] = ac_key_keypad_8;
  table[GCKeyCodeKeypad9] = ac_key_keypad_9;
  table[GCKeyCodeLeftControl] = ac_key_left_control;
  table[GCKeyCodeLeftShift] = ac_key_left_shift;
  table[GCKeyCodeLeftAlt] = ac_key_left_alt;
  table[GCKeyCodeLeftGUI] = ac_key_left_super;
  table[GCKeyCodeRightControl] = ac_key_right_control;
  table[GCKeyCodeRightShift] = ac_key_right_shift;
  table[GCKeyCodeRightAlt] = ac_key_right_alt;
  table[GCKeyCodeRightGUI] = ac_key_right_super;
  return table[key];
}

#endif
