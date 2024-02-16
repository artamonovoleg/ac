#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS && AC_INCLUDE_INPUT)

#include "input_windows.h"

static LRESULT CALLBACK
ac_windows_input_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  ac_input input_handle =
    (ac_input)(uintptr_t)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

  if (!input_handle)
  {
    return DefWindowProcW(hwnd, msg, wparam, lparam);
  }

  switch (msg)
  {
  case WM_INPUT:
  {
    uint8_t data[sizeof(RAWINPUTHEADER) + sizeof(RAWHID) + 64];
    UINT    dw_size = sizeof(data);

    if (
      GetRawInputData(
        (HRAWINPUT)lparam,
        RID_INPUT,
        data,
        &dw_size,
        sizeof(RAWINPUTHEADER)) == (UINT)-1)
    {
      break;
    }

    PRAWINPUT raw = (PRAWINPUT)data;

    switch (raw->header.dwType)
    {
    case RIM_TYPEKEYBOARD:
    {
      ac_input_event event;
      if (raw->data.keyboard.Flags & RI_KEY_BREAK)
      {
        event.type = ac_input_event_type_key_up;
      }
      else
      {
        event.type = ac_input_event_type_key_down;
      }

      event.key = ac_key_from_windows(raw->data.keyboard.VKey);
      ac_input_send_event(input_handle, &event);
      break;
    }
    case RIM_TYPEMOUSE:
    {
      static const int btn_down[5] = {
        RI_MOUSE_BUTTON_1_DOWN,
        RI_MOUSE_BUTTON_2_DOWN,
        RI_MOUSE_BUTTON_3_DOWN,
        RI_MOUSE_BUTTON_4_DOWN,
        RI_MOUSE_BUTTON_5_DOWN,
      };
      static const int btn_up[5] = {
        RI_MOUSE_BUTTON_1_UP,
        RI_MOUSE_BUTTON_2_UP,
        RI_MOUSE_BUTTON_3_UP,
        RI_MOUSE_BUTTON_4_UP,
        RI_MOUSE_BUTTON_5_UP,
      };

      if ((raw->data.mouse.lLastX != 0) || (raw->data.mouse.lLastY != 0))
      {
        ac_input_event event;
        event.type = ac_input_event_type_mouse_move;
        event.mouse_move.dx = (float)raw->data.mouse.lLastX;
        event.mouse_move.dy = (float)-raw->data.mouse.lLastY;
        ac_input_send_event(input_handle, &event);
      }

      if (raw->data.mouse.usButtonFlags)
      {
        for (uint32_t i = 0; i < 5; ++i)
        {
          if (raw->data.mouse.usButtonFlags & btn_up[i])
          {
            ac_input_event event;
            event.type = ac_input_event_type_mouse_button_up;
            event.mouse_button = (ac_mouse_button)(i + 1);
            ac_input_send_event(input_handle, &event);
          }

          if (raw->data.mouse.usButtonFlags & btn_down[i])
          {
            ac_input_event event;
            event.type = ac_input_event_type_mouse_button_down;
            event.mouse_button = (ac_mouse_button)(i + 1);
            ac_input_send_event(input_handle, &event);
          }
        }
      }

      if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
      {
        ac_input_event event;
        event.type = ac_input_event_type_scroll;
        event.scroll.dx = 0;
        event.scroll.dy = (SHORT)raw->data.mouse.usButtonData;
        ac_input_send_event(input_handle, &event);
      }

      break;
    }
    default:
    {
      break;
    }
    }

    break;
  }
  }

  return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static ac_result
ac_windows_input_init(ac_input* input_handle)
{
  AC_INIT_INTERNAL(input, ac_windows_input);

  input->hinstance = GetModuleHandleW(NULL);
  if (!input->hinstance)
  {
    AC_ERROR("[ input ] [ windows ] : failed to get hinstance");
    return ac_result_unknown_error;
  }

  WNDCLASSEX wc = {
    .cbSize = sizeof(wc),
    .lpfnWndProc = ac_windows_input_wnd_proc,
    .hInstance = input->hinstance,
    .lpszClassName = AC_CLASS_INPUT,
  };

  input->input_class = RegisterClassExW(&wc);
  if (!input->input_class)
  {
    AC_ERROR("[ input ] [ windows ]: failed to register window class");
    return ac_result_unknown_error;
  }

  input->hwnd = CreateWindowExW(
    0,
    AC_CLASS_INPUT,
    L"ac.window.input",
    0,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    HWND_MESSAGE,
    NULL,
    input->hinstance,
    NULL);

  if (!input->hwnd)
  {
    AC_ERROR("[ input ] [ windows ] : failed to create window");
    return ac_result_unknown_error;
  }

  SetWindowLongPtrW(input->hwnd, GWLP_USERDATA, (LONG_PTR)input);

  RAWINPUTDEVICE rid[] = {
    {
      .usUsagePage = ac_windows_hid_usage_page_generic,
      .usUsage = ac_windows_hid_usage_generic_mouse,
      .dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK,
      .hwndTarget = input->hwnd,
    },
    {
      .usUsagePage = ac_windows_hid_usage_page_generic,
      .usUsage = ac_windows_hid_usage_generic_keyboard,
      .dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK,
      .hwndTarget = input->hwnd,
    },
  };

  if (!RegisterRawInputDevices(rid, AC_COUNTOF(rid), sizeof(rid[0])))
  {
    AC_ERROR("[ input ] [ windows ] : failed to register raw input devices");
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

static void
ac_windows_input_shutdown(ac_input input_handle)
{
  AC_FROM_HANDLE(input, ac_windows_input);

  if (input->hwnd)
  {
    RAWINPUTDEVICE rid[] = {
      {
        .usUsagePage = ac_windows_hid_usage_page_generic,
        .usUsage = ac_windows_hid_usage_generic_mouse,
        .dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK | RIDEV_REMOVE,
        .hwndTarget = input->hwnd,
      },
      {
        .usUsagePage = ac_windows_hid_usage_page_generic,
        .usUsage = ac_windows_hid_usage_generic_keyboard,
        .dwFlags = RIDEV_DEVNOTIFY | RIDEV_INPUTSINK | RIDEV_REMOVE,
        .hwndTarget = input->hwnd,
      },
    };

    RegisterRawInputDevices(rid, AC_COUNTOF(rid), sizeof(rid[0]));

    DestroyWindow(input->hwnd);
    input->hwnd = NULL;
  }

  if (input->hinstance)
  {
    UnregisterClassW(AC_CLASS_INPUT, input->hinstance);
    input->hinstance = NULL;
  }
}

static void
ac_windows_input_poll_events(ac_input input_handle)
{
  AC_FROM_HANDLE(input, ac_windows_input);

  MSG msg;
  while (PeekMessageW(&msg, input->hwnd, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }

  for (uint32_t i = 0; i < AC_MAX_GAMEPADS; ++i)
  {
    XINPUT_STATE src;
    AC_ZERO(src);

    ac_gamepad_state* dst = &input->common.gamepads[i].state;

    dst->connected = XInputGetState(i, &src) == ERROR_SUCCESS;

    if (!dst->connected)
    {
      continue;
    }

    struct {
      uint64_t          xibutton;
      ac_gamepad_button button;
    } buttons[] = {
      {XINPUT_GAMEPAD_DPAD_UP, ac_gamepad_button_dpad_up},
      {XINPUT_GAMEPAD_DPAD_DOWN, ac_gamepad_button_dpad_down},
      {XINPUT_GAMEPAD_DPAD_LEFT, ac_gamepad_button_dpad_left},
      {XINPUT_GAMEPAD_DPAD_RIGHT, ac_gamepad_button_dpad_right},
      {XINPUT_GAMEPAD_START, ac_gamepad_button_menu},
      {XINPUT_GAMEPAD_BACK, ac_gamepad_button_options},
      {XINPUT_GAMEPAD_LEFT_THUMB, ac_gamepad_button_left_thumbstick},
      {XINPUT_GAMEPAD_RIGHT_THUMB, ac_gamepad_button_right_thumbstick},
      {XINPUT_GAMEPAD_LEFT_SHOULDER, ac_gamepad_button_left_shoulder},
      {XINPUT_GAMEPAD_RIGHT_SHOULDER, ac_gamepad_button_right_shoulder},
      {XINPUT_GAMEPAD_A, ac_gamepad_button_a},
      {XINPUT_GAMEPAD_B, ac_gamepad_button_b},
      {XINPUT_GAMEPAD_X, ac_gamepad_button_x},
      {XINPUT_GAMEPAD_Y, ac_gamepad_button_y},
    };

    for (uint32_t b = 0; b < AC_COUNTOF(buttons); ++b)
    {
      dst->buttons[buttons[b].button] =
        src.Gamepad.wButtons & buttons[b].xibutton;
    }

    dst->axis[ac_gamepad_axis_left_thumbstick_x] =
      ac_windows_gamepad_normalize_axis(src.Gamepad.sThumbLX, false);
    dst->axis[ac_gamepad_axis_left_thumbstick_y] =
      ac_windows_gamepad_normalize_axis(src.Gamepad.sThumbLY, false);
    dst->axis[ac_gamepad_axis_left_trigger] =
      ac_windows_gamepad_normalize_axis(src.Gamepad.bLeftTrigger, true);

    dst->axis[ac_gamepad_axis_right_thumbstick_x] =
      ac_windows_gamepad_normalize_axis(src.Gamepad.sThumbRX, false);
    dst->axis[ac_gamepad_axis_right_thumbstick_y] =
      ac_windows_gamepad_normalize_axis(src.Gamepad.sThumbRY, false);
    dst->axis[ac_gamepad_axis_right_trigger] =
      ac_windows_gamepad_normalize_axis(src.Gamepad.bRightTrigger, true);
  }
}

void
ac_windows_input_get_impl(ac_input_implementation* impl)
{
  impl->init = ac_windows_input_init;
  impl->shutdown = ac_windows_input_shutdown;
  impl->poll_events = ac_windows_input_poll_events;
}

#endif
