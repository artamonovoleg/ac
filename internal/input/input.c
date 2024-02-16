#include "ac_private.h"

#if (AC_INCLUDE_INPUT)

#include "input.h"

static ac_input_implementation impl;
static ac_input                g_input = NULL;

void
ac_input_send_event(ac_input input, const ac_input_event* event)
{
  switch (event->type)
  {
  case ac_input_event_type_mouse_move:
  {
    input->mouse_move.dx += event->mouse_move.dx;
    input->mouse_move.dy += event->mouse_move.dy;
    break;
  }
  case ac_input_event_type_scroll:
  {
    input->mouse_scroll.dx += event->scroll.dx;
    input->mouse_scroll.dy += event->scroll.dy;
    break;
  }
  case ac_input_event_type_mouse_button_up:
  {
    if (event->mouse_button == ac_mouse_button_unknown)
    {
      return;
    }

    if (!input->mouse_state.buttons[event->mouse_button])
    {
      return;
    }

    input->mouse_state.buttons[event->mouse_button] = false;
    break;
  }
  case ac_input_event_type_mouse_button_down:
  {
    if (event->mouse_button == ac_mouse_button_unknown)
    {
      return;
    }

    if (input->mouse_state.buttons[event->mouse_button])
    {
      break;
    }

    input->mouse_state.buttons[event->mouse_button] = true;
    break;
  }
  case ac_input_event_type_key_up:
  {
    if (event->key == ac_key_unknown)
    {
      return;
    }

    if (!input->keys[event->key])
    {
      return;
    }

    input->keys[event->key] = false;
    break;
  }
  case ac_input_event_type_key_down:
  {
    if (event->key == ac_key_unknown)
    {
      return;
    }

    if (input->keys[event->key])
    {
      return;
    }

    input->keys[event->key] = true;
    break;
  }
  default:
  {
    break;
  }
  }

  if (input->callback)
  {
    input->callback(event, input->callback_data);
  }
}

AC_API ac_result
ac_init_input(const ac_input_info* info)
{
  if (g_input)
  {
    return ac_result_success;
  }

  void (*get_impl)(ac_input_implementation* impl) = NULL;

#if (AC_PLATFORM_LINUX)
  get_impl = ac_linux_input_get_impl;
#elif (AC_PLATFORM_APPLE)
  get_impl = ac_apple_input_get_impl;
#elif (AC_PLATFORM_WINDOWS)
  get_impl = ac_windows_input_get_impl;
#elif (AC_PLATFORM_XBOX)
  get_impl = ac_xbox_input_get_impl;
#elif (AC_PLATFORM_NINTENDO_SWITCH)
  get_impl = ac_nn_input_get_impl;
#elif (AC_PLATFORM_PS5)
  get_impl = ac_ps_input_get_impl;
#endif

  if (!get_impl)
  {
    return ac_result_unknown_error;
  }

  get_impl(&impl);

  ac_result res = impl.init(&g_input);

  if (res != ac_result_success)
  {
    impl.shutdown(g_input);
    ac_free(g_input);
    g_input = NULL;
    return res;
  }

  g_input->callback = info->callback;
  g_input->callback_data = info->callback_data;

  return res;
}

AC_API void
ac_shutdown_input(void)
{
  if (!g_input)
  {
    return;
  }

  impl.shutdown(g_input);
  ac_free(g_input);
  g_input = NULL;
}

AC_API bool
ac_input_get_key_state(ac_key key)
{
  AC_ASSERT(key >= 0 && key < ac_key_count);
  return g_input->keys[key];
}

AC_API void
ac_input_get_mouse_state(ac_mouse_state* state)
{
  *state = g_input->mouse_state;
}

AC_API void
ac_input_get_gamepad_state(ac_gamepad_index index, ac_gamepad_state* state)
{
  AC_ASSERT(index < AC_MAX_GAMEPADS);

  *state = g_input->gamepads[index].state;
}

AC_API ac_gamepad_type
ac_input_get_gamepad_type(ac_gamepad_index index)
{
  AC_ASSERT(index < AC_MAX_GAMEPADS);

  return g_input->gamepads[index].type;
}

AC_API void
ac_input_poll_events(void)
{
  g_input->mouse_state.move.dx = g_input->mouse_move.dx;
  g_input->mouse_state.move.dy = g_input->mouse_move.dy;
  g_input->mouse_state.scroll.dx = g_input->mouse_scroll.dx;
  g_input->mouse_state.scroll.dy = g_input->mouse_scroll.dy;

  AC_ZERO(g_input->mouse_move);
  AC_ZERO(g_input->mouse_scroll);

  impl.poll_events(g_input);
}

#endif
