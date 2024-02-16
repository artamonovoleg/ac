include("ac_common_settings")

local RD = "../"

project("ac-renderer")
  kind("StaticLib")

  uuid("e84fb8b0-730a-11ee-a565-0800200c9a66")

  files({
    RD .. "external/c_array/c_array.h",
    RD .. "external/c_hashmap/c_hashmap.h",
    RD .. "external/c_hashmap/c_hashmap.c",
    RD .. "external/ac_shader_compiler/ac_shader_compiler.h",
    RD .. "include/*",
    RD .. "internal/ac_private.h",
    RD .. "internal/renderer/renderer.h",
    RD .. "internal/renderer/renderer.c",
    RD .. "external/vk_mem_alloc/vk_mem_alloc.h",
    RD .. "external/vk_mem_alloc/vk_mem_alloc.cpp",
  })

  filter({ "system:windows" })
    files({
      RD .. "external/pix/pix3.h",
      RD .. "external/pix/pix3_win.h",
      RD .. "external/pix/PIXEvents.h",
      RD .. "external/pix/PIXEventsCommon.h",
      RD .. "external/d3d12/d3d12.h",
      RD .. "external/d3d12/d3d12sdklayers.h",
      RD .. "external/d3d12/d3d12shader.h",
      RD .. "external/d3d12/d3d12video.h",
      RD .. "external/d3d12/d3dcommon.h",
      RD .. "external/d3d12/dxgiformat.h",
      RD .. "external/d3d12_mem_alloc/d3d12_mem_alloc.hpp",
      RD .. "external/d3d12_mem_alloc/d3d12_mem_alloc_windows.cpp",
      RD .. "internal/renderer/renderer_d3d12.h",
      RD .. "internal/renderer/renderer_d3d12.cpp",
      RD .. "internal/renderer/renderer_d3d12_windows.h",
      RD .. "internal/renderer/renderer_d3d12_windows.cpp",
      RD .. "internal/renderer/renderer_d3d12_helpers.h",
    })
  filter({})

  filter({ "system:xbox*"})
    files({
      RD .. "external/pix/pix3.h",
      RD .. "external/pix/pix3_win.h",
      RD .. "external/pix/PIXEvents.h",
      RD .. "external/pix/PIXEventsCommon.h",
      RD .. "external/d3d12/d3d12.h",
      RD .. "external/d3d12/d3d12sdklayers.h",
      RD .. "external/d3d12/d3d12shader.h",
      RD .. "external/d3d12/d3d12video.h",
      RD .. "external/d3d12/d3dcommon.h",
      RD .. "external/d3d12/dxgiformat.h",
      RD .. "external/d3d12_mem_alloc/d3d12_mem_alloc.hpp",
      RD .. "external/d3d12_mem_alloc/d3d12_mem_alloc_xbox.cpp",
      RD .. "internal/renderer/renderer_d3d12.h",
      RD .. "internal/renderer/renderer_d3d12.cpp",
      RD .. "internal/renderer/renderer_d3d12_xbox.h",
      RD .. "internal/renderer/renderer_d3d12_xbox.cpp",
      RD .. "internal/renderer/renderer_d3d12_xbox_helpers.h",
    })
  filter({})

  filter({ "system:macosx or ios" })
    files({
      RD .. "internal/renderer/renderer_metal.h",
      RD .. "internal/renderer/renderer_metal.m",
    })
  filter({})

  filter({ "system:ps4" })
    files({
      RD .. "internal/renderer/renderer_ps4.cpp",
      RD .. "internal/renderer/renderer_ps4.h",
      RD .. "internal/renderer/renderer_ps4_helpers.h"
    })
  filter({})

  filter({ "system:ps5" })
    files({
      RD .. "internal/renderer/renderer_ps5.cpp",
      RD .. "internal/renderer/renderer_ps5.h",
      RD .. "internal/renderer/renderer_ps5_helpers.h"
    })
  filter({})

  filter({ "system:not ps* or xbox*" })
    files({
      RD .. "external/vulkan/*",
      RD .. "external/vk_video/*",
      RD .. "internal/renderer/renderer_vulkan.h",
      RD .. "internal/renderer/renderer_vulkan.c",
      RD .. "internal/renderer/renderer_vulkan_helpers.h",
    })
  filter({})
