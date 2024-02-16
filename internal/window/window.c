#include "ac_private.h"

#if (AC_INCLUDE_WINDOW)

#include "window.h"

static ac_window g_window;

void
ac_window_send_event(ac_window window, const ac_window_event* event)
{
  ac_window_state* state = &window->state;

  switch (event->type)
  {
  case ac_window_event_type_resize:
  {
    if (
      event->resize.width == state->width &&
      event->resize.height == state->height)
    {
      return;
    }

    state->width = event->resize.width;
    state->height = event->resize.height;
    break;
  }
  case ac_window_event_type_focus_lost:
  {
    break;
  }
  case ac_window_event_type_focus_own:
  {
    break;
  }
  default:
  {
    break;
  }
  }

  if (window->state.callback)
  {
    window->state.callback(event, state->callback_data);
  }
}

AC_API ac_result
ac_init_window(const char* app_name)
{
  if (g_window)
  {
    return ac_result_success;
  }

  ac_result (*init)(const char* name, ac_window* window) = NULL;

#if (AC_PLATFORM_LINUX)
  init = ac_linux_window_init;
#elif (AC_PLATFORM_APPLE_MACOS)
  init = ac_macos_window_init;
#elif (AC_PLATFORM_APPLE_IOS)
  init = ac_ios_window_init;
#elif (AC_PLATFORM_WINDOWS)
  init = ac_windows_window_init;
#elif (AC_PLATFORM_XBOX)
  init = ac_xbox_window_init;
#elif (AC_PLATFORM_NINTENDO_SWITCH)
  init = ac_nn_window_init;
#elif (AC_PLATFORM_PS)
  init = ac_ps_window_init;
#endif

  if (!init)
  {
    return ac_result_unknown_error;
  }

  ac_result res = init(app_name, &g_window);

  if (res != ac_result_success)
  {
    g_window->shutdown(g_window);
    ac_free(g_window);
    g_window = NULL;
  }

  g_window->state.monitor = g_window->monitors[0];

  return res;
}

AC_API void
ac_shutdown_window(void)
{
  if (!g_window)
  {
    return;
  }

  g_window->shutdown(g_window);
  for (uint32_t i = 0; i < g_window->monitor_count; ++i)
  {
    ac_free(g_window->monitors[i]);
  }
  ac_free(g_window);
  g_window = NULL;
}

AC_API void
ac_window_enumerate_monitors(uint32_t* count, ac_monitor* monitors)
{
  AC_ASSERT(g_window);

  *count = g_window->monitor_count;

  if (monitors == NULL)
  {
    return;
  }

  if ((*count) > 0)
  {
    memcpy(monitors, g_window->monitors, sizeof(ac_monitor) * (*count));
  }
}

AC_API ac_result
ac_window_set_state(const ac_window_state* state)
{
  AC_ASSERT(g_window);
  AC_ASSERT(state);

  ac_window_state  src = *state;
  ac_window_state* dst = &g_window->state;

  ac_monitor monitor = dst->monitor;

  if (src.monitor)
  {
    monitor = src.monitor;
  }

  if (state->width == 0 || state->height == 0)
  {
    src.width = monitor->width;
    src.height = monitor->height;
  }
  else
  {
    src.width = AC_CLAMP(state->width, 800, monitor->width);
    src.height = AC_CLAMP(state->height, 600, monitor->height);
  }

  ac_result result = g_window->set_state(g_window, &src);

  if (result != ac_result_success)
  {
    return result;
  }

  dst->callback = src.callback;
  dst->callback_data = src.callback_data;
  dst->resizable = src.resizable;
  dst->borderless = src.borderless;
  dst->monitor = monitor;

  return result;
}

AC_API ac_window_state
ac_window_get_state(void)
{
  return g_window->state;
}

AC_API float
ac_window_get_dpi_scale(void)
{
  AC_ASSERT(g_window);
  return g_window->get_dpi_scale(g_window);
}

AC_API void
ac_window_hide_cursor(bool hide)
{
  g_window->hide_cursor(g_window, hide);
}

AC_API void
ac_window_grab_cursor(bool grab)
{
  g_window->grab_cursor(g_window, grab);
}

AC_API ac_result
ac_window_get_cursor_position(float* x, float* y)
{
  return g_window->get_cursor_position(g_window, x, y);
}

AC_API void
ac_window_poll_events(void)
{
  AC_ASSERT(g_window);
  g_window->poll_events(g_window);
}

AC_API ac_result
ac_window_get_vk_instance_extensions(uint32_t* count, const char** names)
{
  return g_window->get_vk_instance_extensions(g_window, count, names);
}

AC_API ac_result
ac_window_create_vk_surface(void* instance, void** surface)
{
  return g_window->create_vk_surface(g_window, instance, surface);
}

AC_API void*
ac_window_get_native_handle(void)
{
  return g_window->native_window;
}

AC_API void*
ac_window_get_xlib_display(void)
{
  return g_window->xlib_dpy;
}

#endif
