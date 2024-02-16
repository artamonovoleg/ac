#pragma once

#include "ac_private.h"

#if (AC_INCLUDE_WINDOW && AC_PLATFORM_APPLE_NOT_MACOS)

#import <UIKit/UIKit.h>

#define VK_NO_PROTOTYPES 1
#define VK_USE_PLATFORM_IOS_MVK 1
#include <vulkan/vulkan.h>

#include "window.h"

typedef struct ac_ios_monitor {
  ac_monitor_internal common;
} ac_ios_monitor;

typedef struct ac_ios_window {
  ac_window_internal common;

  UIView*   view;
  UIWindow* window;

  void*                     vk;
  PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
} ac_ios_window;

#endif
