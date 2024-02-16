include("ac_common_settings")

local RD = "../"

project("ac-window")
  kind("StaticLib")

  uuid("50d695f0-7312-11ee-a565-0800200c9a66")

  files({
    RD .. "external/c_array/c_array.h",
    RD .. "external/c_hashmap/c_hashmap.h",
    RD .. "external/c_hashmap/c_hashmap.c",
    RD .. "include/*",
    RD .. "internal/ac_private.h",
    RD .. "internal/window/window.h",
    RD .. "internal/window/window.c",
  })

  filter({ "system:windows" })
    files({
      RD .. "internal/window/window_windows.h",
      RD .. "internal/window/window_windows.c",
    })
  filter({})

  filter({ "system:xbox*"})
    files({
      RD .. "internal/window/window_xbox.h",
      RD .. "internal/window/window_xbox.c",
    })
  filter({})

  filter({ "system:linux" })
    files({
      RD .. "internal/window/window_linux.h",
      RD .. "internal/window/window_linux.c",
    })
  filter({})

  filter({ "system:macosx" })
    files({
      RD .. "internal/window/window_apple_appkit.h",
      RD .. "internal/window/window_apple_appkit.m",
    })
  filter({})

  filter({ "system:ios" })
    files({
      RD .. "internal/window/window_apple_uikit.h",
      RD .. "internal/window/window_apple_uikit.m",
    })
  filter({})

  filter({ "system:nintendo-switch" })
    files({
      RD .. "internal/window/window_nn.h",
      RD .. "internal/window/window_nn.cpp",
    })
  filter({})

  filter({ "system:ps*" })
    files({
      RD .. "internal/window/window_ps.h",
      RD .. "internal/window/window_ps.c",
    })
  filter({})
