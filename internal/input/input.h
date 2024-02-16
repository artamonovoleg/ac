#pragma once

#include "ac_private.h"

#if (AC_INCLUDE_INPUT)

#if defined(__cplusplus)
extern "C"
{
#endif

AC_DEFINE_HANDLE(ac_input);

typedef struct ac_input_internal {
  ac_input_callback callback;
  void*             callback_data;
  bool              keys[ac_key_count];
  struct {
    float dx, dy;
  } mouse_move, mouse_scroll;
  ac_mouse_state mouse_state;
  struct {
    ac_gamepad_type  type;
    ac_gamepad_state state;
  } gamepads[AC_MAX_GAMEPADS];
} ac_input_internal;

typedef struct ac_input_implementation {
  ac_result (*init)(ac_input*);
  void (*shutdown)(ac_input);
  void (*poll_events)(ac_input);
} ac_input_implementation;

void
ac_input_send_event(ac_input input, const ac_input_event* event);

#if (AC_PLATFORM_LINUX)
void
ac_linux_input_get_impl(ac_input_implementation* impl);
#endif

#if (AC_PLATFORM_APPLE)
void
ac_apple_input_get_impl(ac_input_implementation* impl);
#endif

#if (AC_PLATFORM_WINDOWS)
void
ac_windows_input_get_impl(ac_input_implementation* impl);
#endif

#if (AC_PLATFORM_XBOX)
void
ac_xbox_input_get_impl(ac_input_implementation* impl);
#endif

#if (AC_PLATFORM_NINTENDO_SWITCH)
void
ac_nn_input_get_impl(ac_input_implementation* impl);
#endif

#if (AC_PLATFORM_PS)
void
ac_ps_input_get_impl(ac_input_implementation* impl);
#endif

#if defined(__cplusplus)
}
#endif

#endif
