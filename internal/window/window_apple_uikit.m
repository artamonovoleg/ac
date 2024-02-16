#include "ac_private.h"

#if (AC_INCLUDE_WINDOW && AC_PLATFORM_APPLE_NOT_MACOS)

#if !__has_feature(objc_arc)
#error AC must be built with Objective-C ARC (automatic reference counting) enabled
#endif

#include "window_apple_uikit.h"

static void
ac_ios_window_shutdown(ac_window window_handle)
{
  AC_FROM_HANDLE(window, ac_ios_window);

  window->view = NULL;
  window->window = NULL;
}

static ac_result
ac_ios_window_set_state(ac_window window_handle, const ac_window_state* src)
{
  AC_UNUSED(window_handle);
  AC_UNUSED(src);

  return ac_result_success;
}

static float
ac_ios_window_get_dpi_scale(ac_window window_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_ios_window);
  return (float)(window->window.contentScaleFactor);

  AC_OBJC_END_ARP();
}

static void
ac_ios_window_hide_cursor(ac_window window_handle, bool hide)
{
  AC_UNUSED(window_handle);
  AC_UNUSED(hide);
}

static void
ac_ios_window_grab_cursor(ac_window window_handle, bool grab)
{
  AC_UNUSED(window_handle);
  AC_UNUSED(grab);
}

static ac_result
ac_ios_window_get_cursor_position(ac_window window_handle, float* x, float* y)
{
  AC_UNUSED(window_handle);
  AC_UNUSED(x);
  AC_UNUSED(y);

  return ac_result_unknown_error;
}

static void
ac_ios_window_poll_events(ac_window window_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(window_handle);

  AC_OBJC_END_ARP();
}

static ac_result
ac_ios_window_get_vk_instance_extensions(
  ac_window    window_handle,
  uint32_t*    count,
  const char** names)
{
  AC_UNUSED(window_handle);

  static const char* extensions[] = {
    VK_KHR_SURFACE_EXTENSION_NAME,
    VK_MVK_IOS_SURFACE_EXTENSION_NAME,
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
ac_ios_window_create_vk_surface(
  ac_window window_handle,
  void*     instance,
  void**    surface)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(window, ac_ios_window);

  if (!window->vkGetInstanceProcAddr)
  {
    return ac_result_unknown_error;
  }

  PFN_vkCreateIOSSurfaceMVK vkCreateIOSSurfaceMVK =
    (PFN_vkCreateIOSSurfaceMVK)(void*)window->vkGetInstanceProcAddr(
      (VkInstance)instance,
      "vkCreateIOSSurfaceMVK");

  if (!vkCreateIOSSurfaceMVK)
  {
    return ac_result_unknown_error;
  }

  VkIOSSurfaceCreateInfoMVK info = {
    .sType = VK_STRUCTURE_TYPE_MACOS_SURFACE_CREATE_INFO_MVK,
    .pView = (__bridge const void*)(window->view),
  };

  VkResult res = vkCreateIOSSurfaceMVK(
    (VkInstance)instance,
    &info,
    NULL,
    (VkSurfaceKHR*)surface);

  if (res != VK_SUCCESS)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;

  AC_OBJC_END_ARP();
}

ac_result
ac_ios_window_init(const char* app_name, ac_window* window_handle)
{
  AC_UNUSED(app_name);
  AC_OBJC_BEGIN_ARP();

  AC_INIT_INTERNAL(window, ac_ios_window);

  window->common.shutdown = ac_ios_window_shutdown;
  window->common.set_state = ac_ios_window_set_state;
  window->common.get_dpi_scale = ac_ios_window_get_dpi_scale;
  window->common.hide_cursor = ac_ios_window_hide_cursor;
  window->common.grab_cursor = ac_ios_window_grab_cursor;
  window->common.get_cursor_position = ac_ios_window_get_cursor_position;
  window->common.poll_events = ac_ios_window_poll_events;
  window->common.get_vk_instance_extensions =
    ac_ios_window_get_vk_instance_extensions;
  window->common.create_vk_surface = ac_ios_window_create_vk_surface;

  UIScreen* screen = [UIScreen mainScreen];

  if (!screen)
  {
    return ac_result_unknown_error;
  }

  CGRect bounds = [screen nativeBounds];

  ac_monitor* monitor_handle = &window->common.monitors[0];
  AC_INIT_INTERNAL(monitor, ac_ios_monitor);
  monitor->common.x = bounds.origin.x;
  monitor->common.y = bounds.origin.y;
  monitor->common.width = bounds.size.width;
  monitor->common.height = bounds.size.height;
  monitor->common.name = "Default Monitor";

  window->common.monitor_count = 1;

  ac_window_state* dst = &window->common.state;
  dst->width = monitor->common.width;
  dst->height = monitor->common.height;

  window->common.native_window = (__bridge void*)(window->window);

  const char* vk_lib_names[] = {
    "libvulkan.dylib",
    "libvulkan.1.dylib",
    "libMoltenVK.dylib",
  };

  window->vkGetInstanceProcAddr = NULL;

  for (uint32_t i = 0; i < AC_COUNTOF(vk_lib_names); ++i)
  {
    ac_result res = ac_load_library(vk_lib_names[i], &window->vk, false);

    if (res != ac_result_success)
    {
      continue;
    }

    AC_RIF(ac_load_function(
      window->vk,
      "vkGetInstanceProcAddr",
      (void**)&window->vkGetInstanceProcAddr));
  }

  return ac_result_success;

  AC_OBJC_END_ARP();
}

#endif
