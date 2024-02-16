include("ac_common_settings")

local RD = "../"

project("ac-main")
  kind("StaticLib")

  uuid("ce510460-7309-11ee-a565-0800200c9a66")

  files({
    RD .. "include/*",
    RD .. "internal/ac_private.h",
  })

  filter({ "system:windows" })
      files({
        RD .. "internal/main/main_windows.c"
      })
  filter({})

  filter({ "system:xbox*" })
    files({
      RD .. "internal/main/main_xbox.c"
    })
  filter({})

  filter({ "system:linux"})
    files({
      RD .. "internal/main/main_linux.c"
    })
  filter({})

  filter({ "system:macosx" })
    files({
      RD .. "internal/main/main_apple_appkit.c"
    })
  filter({})

  filter({ "system:ios" })
    files({
      RD .. "internal/main/main_apple_uikit.m",
    })
  filter({})

  filter({ "system:nintendo-switch" })
    files({
      RD .. "internal/main/main_nn.cpp"
    })
  filter({})

  filter({ "system:ps*" })
    files({
      RD .. "internal/main/main_ps.c"
    })
  filter({})
