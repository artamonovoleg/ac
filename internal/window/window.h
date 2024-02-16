#pragma once

#include "ac_private.h"

#if (AC_INCLUDE_WINDOW)

#if defined(__cplusplus)
extern "C"
{
#endif

AC_DEFINE_HANDLE(ac_window);

typedef struct ac_monitor_internal {
  int32_t     x;
  int32_t     y;
  uint32_t    width;
  uint32_t    height;
  const char* name;
} ac_monitor_internal;

typedef struct ac_window_internal {
  uint32_t   monitor_count;
  ac_monitor monitors[AC_MAX_MONITORS];

  ac_window_state state;

  void* xlib_dpy;
  void* native_window;

  void (*shutdown)(ac_window);
  ac_result (*set_state)(ac_window, const ac_window_state*);
  float (*get_dpi_scale)(ac_window);
  void (*hide_cursor)(ac_window, bool);
  void (*grab_cursor)(ac_window, bool);
  ac_result (*get_cursor_position)(ac_window, float*, float*);
  void (*poll_events)(ac_window);
  ac_result (*get_vk_instance_extensions)(ac_window, uint32_t*, const char**);
  ac_result (*create_vk_surface)(ac_window, void*, void**);
} ac_window_internal;

void
ac_window_send_event(ac_window window, const ac_window_event* data);

#if (AC_PLATFORM_LINUX)
ac_result
ac_linux_window_init(const char* app_name, ac_window* window);
#endif

#if (AC_PLATFORM_WINDOWS)
ac_result
ac_windows_window_init(const char* app_name, ac_window* window);
#endif

#if (AC_PLATFORM_XBOX)
ac_result
ac_xbox_window_init(const char* app_name, ac_window* window);
#endif

#if (AC_PLATFORM_APPLE_MACOS)
ac_result
ac_macos_window_init(const char* app_name, ac_window* window);
#endif

#if (AC_PLATFORM_APPLE_IOS)
ac_result
ac_ios_window_init(const char* app_name, ac_window* window);
#endif

#if (AC_PLATFORM_NINTENDO_SWITCH)
ac_result
ac_nn_window_init(const char* app_name, ac_window* window);
#endif

#if (AC_PLATFORM_PS)
ac_result
ac_ps_window_init(const char* app_name, ac_window* window);
#endif

#if defined(__cplusplus)
}
#endif

#endif
