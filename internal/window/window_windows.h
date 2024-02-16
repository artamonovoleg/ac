#pragma once

#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS && AC_INCLUDE_WINDOW)

#include <Windows.h>
#define VK_NO_PROTOTYPES 1
#define VK_USE_PLATFORM_WIN32_KHR 1
#include <vulkan/vulkan.h>
#include "window.h"

#define AC_CLASS_WINDOW L"ac.class.window"

typedef struct ac_windows_add_monitor_data {
  ac_window window;
  int       display_index;
  bool      want_primary;
} ac_windows_add_monitor_data;

typedef struct ac_windows_monitor {
  ac_monitor_internal common;
  HMONITOR            handle;
} ac_windows_monitor;

typedef struct ac_windows_window {
  ac_window_internal common;
  HINSTANCE          hinstance;
  ATOM               window_class;
  HWND               hwnd;
  WCHAR              high_surrogate;
  HCURSOR            default_cursor;

  bool cursor_hidden;

  void*                     vk;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
} ac_windows_window;

#endif
