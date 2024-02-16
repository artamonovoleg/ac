include("ac_common_settings")

local RD = "../"

project("ac-core")
  kind("StaticLib")

  uuid("301C9280-16B3-11EE-9B4B-0800200C9A66")

  dependson("ac-shaders")

  files({
    RD .. "external/c_array/c_array.h",
    RD .. "external/c_hashmap/c_hashmap.c",
    RD .. "external/c_hashmap/c_hashmap.h",
    RD .. "include/*",
    RD .. "internal/ac_private.h",
    RD .. "internal/core/core.c",
    RD .. "internal/core/memory_manager.c",
    RD .. "internal/core/memory_manager.h",
    RD .. "internal/core/log.c",
    RD .. "internal/core/fs.c",
    RD .. "internal/core/fs.h",
    RD .. "internal/core/timer.h"
  })

  filter({ "system:windows" })
    files({
      RD .. "internal/core/core_winapi.c",
      RD .. "internal/core/thread_winapi.c",
      RD .. "internal/core/timer_winapi.c",
      RD .. "internal/core/memory_manager_windows.c",
      RD .. "internal/core/fs_windows.c",
      RD .. "internal/core/fs_windows.h",
    })
  filter({})

  filter({ "system:xbox*" })
    files({
        RD .. "internal/core/core_winapi.c",
        RD .. "internal/core/thread_winapi.c",
        RD .. "internal/core/timer_winapi.c",
        RD .. "internal/core/memory_manager_xbox.c",
        RD .. "internal/core/fs_xbox.c",
        RD .. "internal/core/fs_xbox.h"
    })
  filter({})

  filter({ "system:linux" })
    files({
      RD .. "internal/core/core_unix.c",
      RD .. "internal/core/memory_manager_unix.c",
      RD .. "internal/core/thread_unix.c",
      RD .. "internal/core/timer_unix.c",
      RD .. "internal/core/fs_linux.h",
      RD .. "internal/core/fs_linux.c",
    })
  filter({})

  filter({ "system:macosx or ios" })
    files({
      RD .. "internal/core/core_unix.c",
      RD .. "internal/core/memory_manager_unix.c",
      RD .. "internal/core/thread_unix.c",
      RD .. "internal/core/timer_unix.c",
      RD .. "internal/core/fs_apple.m",
      RD .. "internal/core/fs_apple.h",
    })
  filter({})

  filter({ "system:nintendo-switch" })
    files({
      RD .. "internal/core/core_nn.cpp",
      RD .. "internal/core/memory_manager_nn.cpp",
      RD .. "internal/core/fs_nn.cpp",
      RD .. "internal/core/fs_nn.h",
      RD .. "internal/core/thread_nn.cpp",
      RD .. "internal/core/timer_nn.cpp",
    })
  filter({})

  filter({ "system:ps*"})
    files({
      RD .. "internal/core/core_ps.c",
      RD .. "internal/core/memory_manager_ps.c",
      RD .. "internal/core/fs_ps.c",
      RD .. "internal/core/fs_ps.h",
      RD .. "internal/core/thread_ps.c",
      RD .. "internal/core/timer_ps.c",
    })
  filter({})
