#pragma once

#include "ac_base.h"

#if (AC_INCLUDE_WINDOW)

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnested-anon-types"
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

#define AC_MAX_MONITORS 3

AC_DEFINE_HANDLE(ac_monitor);

typedef enum ac_window_event_type {
  ac_window_event_type_resize = 0,
  ac_window_event_type_monitor_change = 1,
  ac_window_event_type_close = 2,
  ac_window_event_type_focus_lost = 3,
  ac_window_event_type_focus_own = 4,
  ac_window_event_type_character = 5,
} ac_window_event_type;

typedef struct ac_window_event {
  ac_window_event_type type;
  union {
    struct {
      uint32_t width;
      uint32_t height;
    } resize;
    uint32_t character;
  };
} ac_window_event;

typedef void (*ac_window_callback)(const ac_window_event* event, void* data);

typedef struct ac_window_state {
  uint32_t           width;
  uint32_t           height;
  ac_monitor         monitor;
  ac_window_callback callback;
  void*              callback_data;
  bool               resizable : 1;
  bool               borderless : 1;
} ac_window_state;

AC_API ac_result
ac_init_window(const char* app_name);

AC_API void
ac_shutdown_window(void);

AC_API void
ac_window_enumerate_monitors(uint32_t* count, ac_monitor* monitors);

AC_API ac_result
ac_window_set_state(const ac_window_state* state);

AC_API ac_window_state
ac_window_get_state(void);

AC_API void
ac_window_hide_cursor(bool hide);

AC_API void
ac_window_grab_cursor(bool grab);

AC_API ac_result
ac_window_get_cursor_position(float* x, float* y);

AC_API float
ac_window_get_dpi_scale(void);

AC_API void
ac_window_poll_events(void);

AC_API ac_result
ac_window_get_vk_instance_extensions(uint32_t* count, const char** names);

AC_API ac_result
ac_window_create_vk_surface(void* instance, void** surface);

AC_API void*
ac_window_get_native_handle(void);

AC_API void*
ac_window_get_xlib_display(void);

#if defined(__cplusplus)
}
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
