#pragma once

#include "ac_base.h"

#if (AC_INCLUDE_INPUT)

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnested-anon-types"
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

#define AC_MAX_GAMEPADS 4

typedef uint8_t ac_gamepad_index;

typedef enum ac_key {
  ac_key_unknown = 0,
  ac_key_a = 1,
  ac_key_b = 2,
  ac_key_c = 3,
  ac_key_d = 4,
  ac_key_e = 5,
  ac_key_f = 6,
  ac_key_g = 7,
  ac_key_h = 8,
  ac_key_i = 9,
  ac_key_j = 10,
  ac_key_k = 11,
  ac_key_l = 12,
  ac_key_m = 13,
  ac_key_n = 14,
  ac_key_o = 15,
  ac_key_p = 16,
  ac_key_q = 17,
  ac_key_r = 18,
  ac_key_s = 19,
  ac_key_t = 20,
  ac_key_u = 21,
  ac_key_v = 22,
  ac_key_w = 23,
  ac_key_x = 24,
  ac_key_y = 25,
  ac_key_z = 26,
  ac_key_zero = 27,
  ac_key_one = 28,
  ac_key_two = 29,
  ac_key_three = 30,
  ac_key_four = 31,
  ac_key_five = 32,
  ac_key_six = 33,
  ac_key_seven = 34,
  ac_key_eight = 35,
  ac_key_nine = 36,
  ac_key_return = 37,
  ac_key_escape = 38,
  ac_key_backspace = 39,
  ac_key_tab = 40,
  ac_key_spacebar = 41,
  ac_key_hyphen = 42,
  ac_key_equal_sign = 43,
  ac_key_open_bracket = 44,
  ac_key_close_bracket = 45,
  ac_key_backslash = 46,
  ac_key_semicolon = 47,
  ac_key_quote = 48,
  ac_key_tilde = 49,
  ac_key_comma = 50,
  ac_key_period = 51,
  ac_key_slash = 52,
  ac_key_caps_lock = 53,
  ac_key_f1 = 54,
  ac_key_f2 = 55,
  ac_key_f3 = 56,
  ac_key_f4 = 57,
  ac_key_f5 = 58,
  ac_key_f6 = 59,
  ac_key_f7 = 60,
  ac_key_f8 = 61,
  ac_key_f9 = 62,
  ac_key_f10 = 63,
  ac_key_f11 = 64,
  ac_key_f12 = 65,
  ac_key_print_screen = 66,
  ac_key_scroll_lock = 67,
  ac_key_pause = 68,
  ac_key_insert = 69,
  ac_key_home = 70,
  ac_key_page_up = 71,
  ac_key_delete = 72,
  ac_key_end = 73,
  ac_key_page_down = 74,
  ac_key_right_arrow = 75,
  ac_key_left_arrow = 76,
  ac_key_down_arrow = 77,
  ac_key_up_arrow = 78,
  ac_key_keypad_num_lock = 79,
  ac_key_keypad_slash = 80,
  ac_key_keypad_asterisk = 81,
  ac_key_keypad_hyphen = 82,
  ac_key_keypad_plus = 83,
  ac_key_keypad_enter = 84,
  ac_key_keypad_0 = 85,
  ac_key_keypad_1 = 86,
  ac_key_keypad_2 = 87,
  ac_key_keypad_3 = 88,
  ac_key_keypad_4 = 89,
  ac_key_keypad_5 = 90,
  ac_key_keypad_6 = 91,
  ac_key_keypad_7 = 92,
  ac_key_keypad_8 = 93,
  ac_key_keypad_9 = 94,
  ac_key_left_control = 95,
  ac_key_left_shift = 96,
  ac_key_left_alt = 97,
  ac_key_left_super = 98,
  ac_key_right_control = 99,
  ac_key_right_shift = 100,
  ac_key_right_alt = 101,
  ac_key_right_super = 102,
  ac_key_count
} ac_key;

typedef enum ac_mouse_button {
  ac_mouse_button_unknown = 0,
  ac_mouse_button_left = 1,
  ac_mouse_button_middle = 2,
  ac_mouse_button_right = 3,
  ac_mouse_button_forward = 4,
  ac_mouse_button_back = 5,
  ac_mouse_button_count
} ac_mouse_button;

typedef enum ac_gamepad_type {
  ac_gamepad_type_unknown = 0,
  ac_gamepad_type_xbox = 1,
  ac_gamepad_type_ps = 2,
  ac_gamepad_type_switch = 3,
} ac_gamepad_type;

typedef enum ac_gamepad_button {
  ac_gamepad_button_unknown = 0,
  ac_gamepad_button_a = 1,
  ac_gamepad_button_b = 2,
  ac_gamepad_button_x = 3,
  ac_gamepad_button_y = 4,
  ac_gamepad_button_left_shoulder = 5,
  ac_gamepad_button_right_shoulder = 6,
  ac_gamepad_button_left_thumbstick = 7,
  ac_gamepad_button_right_thumbstick = 8,
  ac_gamepad_button_dpad_left = 9,
  ac_gamepad_button_dpad_right = 10,
  ac_gamepad_button_dpad_up = 11,
  ac_gamepad_button_dpad_down = 12,
  ac_gamepad_button_menu = 13,
  ac_gamepad_button_options = 14,
  ac_gamepad_button_count
} ac_gamepad_button;

typedef enum ac_gamepad_axis {
  ac_gamepad_axis_unknown = 0,
  ac_gamepad_axis_left_trigger = 1,
  ac_gamepad_axis_left_thumbstick_x = 2,
  ac_gamepad_axis_left_thumbstick_y = 3,
  ac_gamepad_axis_right_trigger = 4,
  ac_gamepad_axis_right_thumbstick_x = 5,
  ac_gamepad_axis_right_thumbstick_y = 6,
  ac_gamepad_axis_count
} ac_gamepad_axis;

typedef enum ac_input_event_type {
  ac_input_event_type_key_down = 0,
  ac_input_event_type_key_up = 1,
  ac_input_event_type_mouse_button_down = 2,
  ac_input_event_type_mouse_button_up = 3,
  ac_input_event_type_scroll = 4,
  ac_input_event_type_mouse_move = 5,
} ac_input_event_type;

typedef struct ac_input_event {
  ac_input_event_type type;
  union {
    struct {
      float dx, dy;
    } scroll;
    struct {
      float dx, dy;
    } mouse_move;
    ac_key          key;
    ac_mouse_button mouse_button;
  };
} ac_input_event;

typedef void (*ac_input_callback)(const ac_input_event* event, void* data);

typedef struct ac_mouse_state {
  struct {
    float dx;
    float dy;
  } move, scroll;
  bool buttons[ac_mouse_button_count];
} ac_mouse_state;

typedef struct ac_gamepad_state {
  bool  connected;
  bool  buttons[ac_gamepad_button_count];
  float axis[ac_gamepad_axis_count];
} ac_gamepad_state;

typedef struct ac_input_info {
  ac_input_callback callback;
  void*             callback_data;
} ac_input_info;

AC_API ac_result
ac_init_input(const ac_input_info* info);

AC_API void
ac_shutdown_input(void);

AC_API bool
ac_input_get_key_state(ac_key key);

AC_API void
ac_input_get_mouse_state(ac_mouse_state* state);

AC_API void
ac_input_get_gamepad_state(ac_gamepad_index index, ac_gamepad_state* state);

AC_API ac_gamepad_type
ac_input_get_gamepad_type(ac_gamepad_index index);

AC_API void
ac_input_poll_events(void);

#if defined(__cplusplus)
}
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
