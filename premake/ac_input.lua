include("ac_common_settings")

local RD = "../"

project("ac-input")
  kind("StaticLib")

  uuid("b5d11330-999c-11ee-9ec1-0800200c9a66")

  files({
    RD .. "external/c_array/c_array.h",
    RD .. "external/c_hashmap/c_hashmap.h",
    RD .. "external/c_hashmap/c_hashmap.c",
    RD .. "include/*",
    RD .. "internal/ac_private.h",
    RD .. "internal/input/input.h",
    RD .. "internal/input/input.c",
  })

  filter({ "system:windows" })
    files({
      RD .. "internal/input/input_windows.h",
      RD .. "internal/input/input_windows.c",
    })
  filter({})

  filter({ "system:xbox*" })
    files({
      RD .. "internal/input/input_xbox.h",
      RD .. "internal/input/input_xbox.c",
    })
  filter({})

  filter({ "system:linux" })
    files({
      RD .. "internal/input/input_linux.h",
      RD .. "internal/input/input_linux.c",
    })
  filter({})

  filter({ "system:macosx or ios" })
    files({
      RD .. "internal/input/input_apple.h",
      RD .. "internal/input/input_apple.m",
    })
  filter({})

  filter({ "system:nintendo-switch" })
    files({
      RD .. "internal/input/input_nn.h",
      RD .. "internal/input/input_nn.cpp",
    })
  filter({})

  filter({ "system:ps*" })
    files({
      RD .. "internal/input/input_ps.h",
      RD .. "internal/input/input_ps.c",
    })
  filter({})
