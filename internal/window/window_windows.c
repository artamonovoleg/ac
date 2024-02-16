#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS && AC_INCLUDE_WINDOW)

#include "window_windows.h"

static LRESULT CALLBACK
ac_windows_window_wnd_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  ac_window window_handle =
    (ac_window)(uintptr_t)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

  if (!window_handle)
  {
    return DefWindowProcW(hwnd, msg, wparam, lparam);
  }

  AC_FROM_HANDLE(window, ac_windows_window);

  switch (msg)
  {
  case WM_MOVE:
  {
    ac_monitor monitor = window->common.state.monitor;

    RECT client_rect;
    GetClientRect(window->hwnd, &client_rect);

    POINT top_left = {
      client_rect.left,
      client_rect.top,
    };
    POINT bottom_right = {
      client_rect.right,
      client_rect.bottom,
    };

    RECT monitor_rect = {
      .left = monitor->x,
      .right = monitor->x + (int32_t)monitor->width,
      .top = monitor->y,
      .bottom = monitor->y + (int32_t)monitor->height,
    };

    if (
      !PtInRect(&monitor_rect, top_left) ||
      !PtInRect(&monitor_rect, bottom_right))
    {
      ac_window_event event;
      event.type = ac_window_event_type_monitor_change;
      ac_window_send_event(window_handle, &event);
    }

    return 0;
  }
  case WM_DISPLAYCHANGE:
  {
    ac_window_event event;
    event.type = ac_window_event_type_monitor_change;
    ac_window_send_event(window_handle, &event);
    return 0;
  }
  case WM_CHAR:
  {
    if (IS_HIGH_SURROGATE(wparam))
    {
      window->high_surrogate = (WCHAR)wparam;
    }
    else
    {
      uint32_t codepoint = 0;

      if (wparam >= 0xdc00 && wparam <= 0xdfff)
      {
        if (window->high_surrogate)
        {
          codepoint += (uint32_t)((window->high_surrogate - 0xd800) << 10);
          codepoint += (WCHAR)wparam - 0xdc00;
          codepoint += 0x10000;
        }
      }
      else
      {
        codepoint = (WCHAR)wparam;
      }
      window->high_surrogate = 0;

      ac_window_event event;
      event.type = ac_window_event_type_character;
      event.character = codepoint;
      ac_window_send_event(window_handle, &event);
    }

    return 0;
  }
  case WM_SETFOCUS:
  {
    ac_window_event event;
    event.type = ac_window_event_type_focus_own;
    ac_window_send_event(window_handle, &event);
    break;
  }
  case WM_KILLFOCUS:
  case WM_ENTERIDLE:
  {
    ac_window_event event;
    event.type = ac_window_event_type_focus_lost;
    ac_window_send_event(window_handle, &event);
    break;
  }
  case WM_SIZE:
  {
    ac_window_event event;
    event.type = ac_window_event_type_resize;
    event.resize.width = LOWORD(lparam);
    event.resize.height = HIWORD(lparam);
    ac_window_send_event(window_handle, &event);

    return 0;
  }
  case WM_DESTROY:
  {
    PostQuitMessage(0);
    break;
  }
  case WM_CLOSE:
  {
    ac_window_event event;
    event.type = ac_window_event_type_close;
    ac_window_send_event(window_handle, &event);
    return 0;
  }
  case WM_SETCURSOR:
  {
    if (LOWORD(lparam) == HTCLIENT)
    {
      if (window->cursor_hidden)
      {
        SetCursor(NULL);
      }
      else
      {
        SetCursor(window->default_cursor);
      }

      return TRUE;
    }

    break;
  }
  case WM_UNICHAR:
  {
    if (wparam == UNICODE_NOCHAR)
    {
      return TRUE;
    }

    return 0;
  }
  case WM_ERASEBKGND:
  {
    return TRUE;
  }
  case WM_NCACTIVATE:
  case WM_NCPAINT:
  {
    if (window->common.state.borderless)
    {
      return TRUE;
    }

    break;
  }
  default:
  {
    break;
  }
  }

  return DefWindowProcW(hwnd, msg, wparam, lparam);
}

static BOOL CALLBACK
ac_windows_window_monitor_callback(
  HMONITOR handle,
  HDC      hdc,
  RECT*    rect,
  LPARAM   dwdata)
{
  AC_UNUSED(hdc);
  AC_UNUSED(rect);

  ac_windows_add_monitor_data* data = (ac_windows_add_monitor_data*)dwdata;

  ac_window window = data->window;

  if (window->monitor_count >= AC_MAX_MONITORS)
  {
    return TRUE;
  }

  MONITORINFOEXW info = {
    .cbSize = sizeof(info),
  };

  if (GetMonitorInfoW(handle, (LPMONITORINFO)&info) == 0)
  {
    return TRUE;
  }

  const bool is_primary =
    ((info.dwFlags & MONITORINFOF_PRIMARY) == MONITORINFOF_PRIMARY);

  if (is_primary != data->want_primary)
  {
    return TRUE;
  }

  ac_monitor* monitor_handle = &window->monitors[window->monitor_count];
  AC_INIT_INTERNAL(monitor, ac_windows_monitor);

  monitor->handle = handle;
  monitor->common.width =
    (uint32_t)(info.rcMonitor.right - info.rcMonitor.left);
  monitor->common.height =
    (uint32_t)(info.rcMonitor.bottom - info.rcMonitor.top);
  monitor->common.x = info.rcMonitor.left;
  monitor->common.y = info.rcMonitor.top;

  window->monitor_count++;

  return TRUE;
}

static void
ac_windows_window_shutdown(ac_window window_handle)
{
  AC_FROM_HANDLE(window, ac_windows_window);

  if (window->hwnd)
  {
    DestroyWindow(window->hwnd);
    window->hwnd = NULL;
  }

  if (window->hinstance)
  {
    UnregisterClassW(AC_CLASS_WINDOW, window->hinstance);
    window->hinstance = NULL;
  }

  ac_unload_library(window->vk);
}

static ac_result
ac_windows_window_set_state(
  ac_window              window_handle,
  const ac_window_state* state)
{
  AC_FROM_HANDLE(window, ac_windows_window);

  DWORD style = WS_CLIPSIBLINGS | WS_CLIPCHILDREN;
  DWORD ex_style = 0;

  int32_t x = 0;
  int32_t y = 0;
  int32_t width = 0;
  int32_t height = 0;

  ac_monitor monitor = window->common.monitors[0];

  if (state->monitor)
  {
    style |= WS_POPUP;
    ex_style |= WS_EX_TOPMOST;

    monitor = state->monitor;

    width = (int32_t)monitor->width;
    height = (int32_t)monitor->height;
  }
  else
  {
    style |= WS_SYSMENU | WS_MINIMIZEBOX;

    if (!state->borderless)
    {
      style |= WS_CAPTION;

      if (state->resizable)
      {
        style |= (WS_THICKFRAME | WS_MAXIMIZEBOX);
      }
    }
    else
    {
      style |= WS_POPUP;
    }

    width = (int32_t)state->width;
    height = (int32_t)state->height;
  }

  x = (monitor->x + (((int32_t)monitor->width - width) / 2));
  y = (monitor->y + (((int32_t)monitor->height - height) / 2));

  RECT window_rect = {
    .right = width,
    .bottom = height,
  };

  if (!AdjustWindowRectEx(&window_rect, style, FALSE, 0))
  {
    return ac_result_unknown_error;
  }

  if (!SetWindowPos(
        window->hwnd,
        NULL,
        x,
        y,
        window_rect.right,
        window_rect.bottom,
        0))
  {
    return ac_result_unknown_error;
  }

  SetLastError(0);
  if (!SetWindowLongPtrW(window->hwnd, GWL_STYLE, style) && GetLastError())
  {
    return ac_result_unknown_error;
  }

  SetLastError(0);
  if (!SetWindowLongPtrW(window->hwnd, GWL_EXSTYLE, ex_style) && GetLastError())
  {
    return ac_result_unknown_error;
  }

  (void)ShowWindow(window->hwnd, SW_SHOW);

  if (!BringWindowToTop(window->hwnd))
  {
    return ac_result_unknown_error;
  }

  if (!SetForegroundWindow(window->hwnd))
  {
    return ac_result_unknown_error;
  }

  if (SetFocus(window->hwnd) == NULL)
  {
    return ac_result_unknown_error;
  }

  ac_window_state* dst = &window->common.state;

  RECT client_rect;
  if (!GetClientRect(window->hwnd, &client_rect))
  {
    return ac_result_unknown_error;
  }
  dst->width = (uint32_t)(client_rect.right - client_rect.left);
  dst->height = (uint32_t)(client_rect.bottom - client_rect.top);

  return ac_result_success;
}

static float
ac_windows_window_get_dpi_scale(ac_window window_handle)
{
  AC_UNUSED(window_handle);

  UINT dpi = GetDpiForSystem();

  return (float)dpi / 96.0f;
}

static void
ac_windows_window_hide_cursor(ac_window window_handle, bool hide)
{
  AC_FROM_HANDLE(window, ac_windows_window);

  window->cursor_hidden = hide;

  if (hide)
  {
    (void)SetCursor(NULL);
  }
  else
  {
    (void)SetCursor(window->default_cursor);
  }
}

static void
ac_windows_window_grab_cursor(ac_window window_handle, bool grab)
{
  if (grab)
  {
    AC_FROM_HANDLE(window, ac_windows_window);
    SetCapture(window->hwnd);

    RECT client_rect;
    GetClientRect(window->hwnd, &client_rect);
    POINT point = {
      (client_rect.left + client_rect.right) / 2,
      (client_rect.top + client_rect.bottom) / 2,
    };
    ClientToScreen(window->hwnd, &point);

    SetCursorPos(point.x, point.y);

    RECT clip_rect = {
      point.x - 1,
      point.y - 1,
      point.x + 1,
      point.y + 1,
    };
    ClipCursor(&clip_rect);
  }
  else
  {
    ClipCursor(NULL);
    ReleaseCapture();
  }
}

static ac_result
ac_windows_window_get_cursor_position(
  ac_window window_handle,
  float*    x,
  float*    y)
{
  AC_FROM_HANDLE(window, ac_windows_window);

  POINT point;
  if (!GetCursorPos(&point))
  {
    return ac_result_unknown_error;
  }

  if (!ScreenToClient(window->hwnd, &point))
  {
    return ac_result_unknown_error;
  }

  *x = (float)point.x;
  *y = (float)point.y;

  return ac_result_success;
}

static void
ac_windows_window_poll_events(ac_window window_handle)
{
  AC_UNUSED(window_handle);

  MSG msg;
  while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
  {
    TranslateMessage(&msg);
    DispatchMessageW(&msg);
  }
}

static ac_result
ac_windows_window_get_vk_instance_extensions(
  ac_window    window_handle,
  uint32_t*    count,
  const char** names)
{
  AC_UNUSED(window_handle);

  static const char* extensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
  };

  *count = AC_COUNTOF(extensions);

  if (names == NULL)
  {
    return ac_result_success;
  }

  for (uint32_t i = 0; i < *count; ++i)
  {
    names[i] = extensions[i];
  }

  return ac_result_success;
}

static ac_result
ac_windows_window_create_vk_surface(
  ac_window window_handle,
  void*     instance,
  void**    surface)
{
  AC_FROM_HANDLE(window, ac_windows_window);

  if (!window->vkGetInstanceProcAddr)
  {
    return ac_result_unknown_error;
  }

  PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR =
    (PFN_vkCreateWin32SurfaceKHR)(void*)window->vkGetInstanceProcAddr(
      (VkInstance)instance,
      "vkCreateWin32SurfaceKHR");

  if (!vkCreateWin32SurfaceKHR)
  {
    return ac_result_unknown_error;
  }

  VkWin32SurfaceCreateInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
    .hinstance = window->hinstance,
    .hwnd = window->hwnd,
  };

  VkResult res = vkCreateWin32SurfaceKHR(
    (VkInstance)instance,
    &info,
    NULL,
    (VkSurfaceKHR*)surface);

  if (res != VK_SUCCESS)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

ac_result
ac_windows_window_init(const char* app_name, ac_window* window_handle)
{
  AC_INIT_INTERNAL(window, ac_windows_window);

  window->common.shutdown = ac_windows_window_shutdown;
  window->common.set_state = ac_windows_window_set_state;
  window->common.get_dpi_scale = ac_windows_window_get_dpi_scale;
  window->common.hide_cursor = ac_windows_window_hide_cursor;
  window->common.grab_cursor = ac_windows_window_grab_cursor;
  window->common.get_cursor_position = ac_windows_window_get_cursor_position;
  window->common.poll_events = ac_windows_window_poll_events;
  window->common.get_vk_instance_extensions =
    ac_windows_window_get_vk_instance_extensions;
  window->common.create_vk_surface = ac_windows_window_create_vk_surface;

  window->hinstance = GetModuleHandleW(NULL);
  if (!window->hinstance)
  {
    AC_ERROR("[ window ] [ windows ] : failed to get hinstance");
    return ac_result_unknown_error;
  }

  {
    ac_windows_add_monitor_data data = {
      .window = *window_handle,
      .display_index = 0,
      .want_primary = true,
    };

    EnumDisplayMonitors(
      NULL,
      NULL,
      ac_windows_window_monitor_callback,
      (LPARAM)&data);

    data.want_primary = false;

    EnumDisplayMonitors(
      NULL,
      NULL,
      ac_windows_window_monitor_callback,
      (LPARAM)&data);
  }

  if (window->common.monitor_count == 0)
  {
    AC_ERROR("[ window ] [ windows ] : no monitors founded");
    return ac_result_unknown_error;
  }

  window->default_cursor = LoadCursorW(NULL, IDC_ARROW);

  if (!window->default_cursor)
  {
    AC_ERROR("[ window ] [ windows ] : failed to load cursor");
    return ac_result_unknown_error;
  }

  WNDCLASSEX wc = {
    .cbSize = sizeof(wc),
    .style = CS_HREDRAW | CS_VREDRAW,
    .lpfnWndProc = ac_windows_window_wnd_proc,
    .hInstance = window->hinstance,
    .hCursor = window->default_cursor,
    .lpszClassName = AC_CLASS_WINDOW,
  };

  wc.hIcon = LoadImageW(
    NULL,
    IDI_APPLICATION,
    IMAGE_ICON,
    0,
    0,
    LR_DEFAULTSIZE | LR_SHARED);

  if (!wc.hIcon)
  {
    AC_ERROR("[ window ] [ windows ] : failed to load icon");
    return ac_result_unknown_error;
  }

  window->window_class = RegisterClassExW(&wc);
  if (!window->window_class)
  {
    AC_ERROR("[ window ] [ windows ] : failed to register window class");
    return ac_result_unknown_error;
  }

  if (ac_load_library("vulkan-1.dll", &window->vk, false) == ac_result_success)
  {
    AC_RIF(ac_load_function(
      window->vk,
      "vkGetInstanceProcAddr",
      (void**)&window->vkGetInstanceProcAddr));
  }

  wchar_t* title = NULL;
  int32_t  size = MultiByteToWideChar(CP_ACP, 0, app_name, -1, NULL, 0);
  if (!size)
  {
    AC_ERROR("[ window ] [ windows ] : MultiByteToWideChar failed");
    return ac_result_unknown_error;
  }

  title = ac_alloc((size_t)size * sizeof(wchar_t));

  if (!MultiByteToWideChar(CP_ACP, 0, app_name, -1, title, size))
  {
    AC_ERROR("[ window ] [ windows ] : MultiByteToWideChar failed");
    return ac_result_unknown_error;
  }

  window->hwnd = CreateWindowExW(
    0,
    AC_CLASS_WINDOW,
    title,
    0,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    CW_USEDEFAULT,
    NULL,
    NULL,
    window->hinstance,
    NULL);

  ac_free(title);

  if (!window->hwnd)
  {
    AC_ERROR("[ window ] [ windows ] : failed to create window");
    return ac_result_unknown_error;
  }

  SetWindowLongPtrW(window->hwnd, GWLP_USERDATA, (LONG_PTR)window);

  window->common.native_window = window->hwnd;

  return ac_result_success;
}

#endif
