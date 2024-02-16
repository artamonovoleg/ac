if (not _ACTION) then
  return
end

require("premake-export-compile-commands/export-compile-commands")
require("premake-vscode/vscode")
require("premake-cli/cli")
if (os.isdir("premake-consoles")) then
  if (not DESKTOP_ONLY) then
    HAS_CONSOLES_IMPL = true
  end
  require("premake-consoles/consoles")
end

AC_BUILD_DIR_OPTION = "build-dir"

newoption {
  trigger     = "build-dir",
  description = "build directory",
  default     = path.getabsolute("../../build/") .. "/"
}

local RD = path.getabsolute("../") .. "/"

if (_ACTION == "vscode" or _ACTION == "export-compile-commands") then
  location(_MAIN_SCRIPT_DIR .. "/../../.vscode/")
else
  location(_MAIN_SCRIPT_DIR .. "/../makefiles/" .. _ACTION .. "-" .. os.target())
end

premake.tools.gcc.shared.warnings.Everything = { "-Wall", "-Wextra" }

-- premake.override(premake.vstudio, "projectPlatform", function(base, cfg)
--   if (not (cfg.system == "windows")) then
--     return base(cfg)
--   end

--   return cfg.buildcfg
-- end)

-- premake.override(premake.vstudio, "solutionPlatform", function(base, cfg)
--   if (not (cfg.system == "windows")) then
--     return base(cfg)
--   end

--   return cfg.platform
-- end)

configurations({
  "release",
  "debug",
  "dist"
})

local real_project = project

function project(name)
  real_project(name)

  cppdialect("C++17")
  cdialect("gnu11")

  exceptionhandling("Off")
  rtti("Off")
  symbols("Full")

  externalanglebrackets("On")
  externalwarnings("Off")
  warnings("Everything")

  filter({ "system:windows" })
    architecture("x64")
    dpiawareness("High")
  filter({})

  filter({ "system:windows or xbox-one or xbox-xs", "toolset:msc*" })
    disablewarnings({ "4577" })
  filter({})

  filter({ "system:macosx or ios" })
    buildoptions({ "-fobjc-arc" })
  filter({})

  filter({ "system:macosx or ios", "kind:WindowedApp" })
    files({ "../premake-xcode/base.plist" })
  filter({})

  filter({ "system:not macosx", "system:not ios", "files:**m" })
    buildaction("None")
  filter({})

  flags({
    "FatalWarnings",
    "NoImplicitLink"
  })

  filter({ "kind:*App or Utility" })
    targetdir(_OPTIONS[AC_BUILD_DIR_OPTION] .. "/bin/%{cfg.buildcfg}/%{cfg.system}/%{prj.name}")
  filter({ "kind:*Lib" })
    targetdir(_OPTIONS[AC_BUILD_DIR_OPTION] .. "/lib/%{wks.name}/%{cfg.buildcfg}/%{cfg.system}/%{prj.name}")
  filter({})
  objdir(_OPTIONS[AC_BUILD_DIR_OPTION] .. "/obj/%{wks.name}/%{cfg.buildcfg}/%{cfg.system}/%{prj.name}")

  flags({ "MultiProcessorCompile" })
  fastuptodate("Off")

  filter({ "configurations:debug" })
    optimize("Debug")
    runtime("Debug")
    defines({ "DEBUG=1" })
  filter({})

  filter({ "configurations:release" })
    optimize("On")
    runtime("Release")
    defines({ "NDEBUG=1" })
  filter({})

  filter({ "configurations:dist" })
    optimize("Full")
    runtime("Release")
    defines({ "NDEBUG=1", "AC_INCLUDE_DEBUG=0" })
    flags({ "LinkTimeOptimization" })
  filter({})

  externalincludedirs({
    RD .. "external",
    RD .. "include"
  })

  includedirs({
    RD .. "include",
    RD .. "internal",
  })

  runpathdirs({
    "%{cfg.targetdir}"
  })

  filter({ "kind:ConsoleApp or WindowedApp" })
    dependson({
      "ac-main",
      "ac-core",
    })

    links({
      "ac-main",
      "ac-core",
    })
  filter({})

  filter({ "kind:WindowedApp" })
    dependson({
      "ac-render-graph",
      "ac-renderer",
      "ac-window",
      "ac-input",
    })

    links({
      "ac-render-graph",
      "ac-renderer",
      "ac-window",
      "ac-input",
    })
  filter({})

  filter({ "system:windows" })
    postbuildcommands({
      "{COPYFILE} " .. RD .. "data/d3d12.1.610-sdk/D3D12Core.dll %{cfg.targetdir}",
      "{COPYFILE} " .. RD .. "data/d3d12.1.610-sdk/d3d12SDKLayers.dll %{cfg.targetdir}"
    })
  filter({})

  filter({ "system:macosx or ios", "kind:WindowedApp" })
    links({
      "Metal.framework",
      "GameController.framework",
      "CoreGraphics.framework",
      "Foundation.framework",
      "QuartzCore.framework",
    })

    embedAndSign({
      "Metal.framework",
      "GameController.framework",
      "CoreGraphics.framework",
      "Foundation.framework",
      "QuartzCore.framework",
    })
  filter({})

  filter({ "system:macosx", "kind:WindowedApp" })
    links({
      "Cocoa.framework"
    })

    embedAndSign({
      "Cocoa.framework"
    })
  filter({})

  filter({ "system:ios", "kind:WindowedApp" })
    links({
      "UIKit.framework"
    })

    embedAndSign({
      "UIKit.framework"
    })
  filter({})

  filter({ "system:macosx" })
    xcodebuildsettings({
      ["AVAILABLE_PLATFORMS"] = "macosx",
      ["SUPPORTED_PLATFORMS"] = "macosx",
    })
  filter({})

  filter({ "system:ios" })
    xcodebuildsettings({
      ["AVAILABLE_PLATFORMS"] = "iphoneos iphonesimulator", -- xros xrsimulator appletvos appletvsimulator
      ["SUPPORTED_PLATFORMS"] = "iphoneos iphonesimulator", -- xros xrsimulator appletvos appletvsimulator
    })
  filter({})

  xcodebuildsettings({
    ["MACOSX_DEPLOYMENT_TARGET"] = "13.0",
    ["IPHONEOS_DEPLOYMENT_TARGET"] = "16.0",
    ["TVOS_DEPLOYMENT_TARGET"] = "16.0",
    ["XROS_DEPLOYMENT_TARGET"] = "1.0",
    ["TARGETED_DEVICE_FAMILY"] = "1",
    ["SUPPORTS_MACCATALYST"] = "NO",
    ["SUPPORTS_MAC_DESIGNED_FOR_IPHONE_IPAD"] = "NO",
    ["SUPPORTS_XR_DESIGNED_FOR_IPHONE_IPAD"] = "NO",
    ["ARCHS"] = "$(ARCHS_STANDARD)",
    ["ONLY_ACTIVE_ARCH"] = "NO",
    ["SKIP_INSTALL"] = "YES",
    ["MARKETING_VERSION"] = "1.0",
    ["CURRENT_PROJECT_VERSION"] = "1.0",
    ["PRODUCT_BUNDLE_IDENTIFIER"] = "com.ac." .. name,
    -- ac-shader-compiler requires this to be disabled
    ["ENABLE_USER_SCRIPT_SANDBOXING"] = "NO",
    -- signing
    ["CODE_SIGNING_ALLOWED"] = "NO",
    ["CODE_SIGNING_REQUIRED"] = "NO",
    -- plist
    ["GENERATE_INFOPLIST_FILE"] = "YES",
    ["INFOPLIST_FILE"] = RD .. "premake/Info.plist.in",
    ["INFOPLIST_KEY_CFBundleDisplayName"] = name,
    ["INFOPLIST_KEY_LSApplicationCategoryType"] = "public.app-category.games",
    ["INFOPLIST_KEY_UIStatusBarHidden"] = "YES",
    ["INFOPLIST_KEY_UIStatusBarStyle"] = "UIStatusBarStyleDarkContent",
		["INFOPLIST_KEY_UISupportedInterfaceOrientations"] = "UIInterfaceOrientationLandscapeLeft",
    -- recommended settings
    ["CLANG_ENABLE_OBJC_ARC"] = "YES",
    ["CLANG_ENABLE_OBJC_WEAK"] = "YES",
    ["CLANG_WARN_BLOCK_CAPTURE_AUTORELEASING"] = "YES",
    ["CLANG_WARN_BOOL_CONVERSION"] = "YES",
    ["CLANG_WARN_COMMA"] = "YES",
    ["CLANG_WARN_CONSTANT_CONVERSION"] = "YES",
    ["CLANG_WARN_DEPRECATED_OBJC_IMPLEMENTATIONS"] = "YES",
    ["CLANG_WARN_EMPTY_BODY"] = "YES",
    ["CLANG_WARN_ENUM_CONVERSION"] = "YES",
    ["CLANG_WARN_INFINITE_RECURSION"] = "YES",
    ["CLANG_WARN_INT_CONVERSION"] = "YES",
    ["CLANG_WARN_NON_LITERAL_NULL_CONVERSION"] = "YES",
    ["CLANG_WARN_OBJC_IMPLICIT_RETAIN_SELF"] = "YES",
    ["CLANG_WARN_OBJC_LITERAL_CONVERSION"] = "YES",
    ["CLANG_WARN_QUOTED_INCLUDE_IN_FRAMEWORK_HEADER"] = "YES",
    ["CLANG_WARN_RANGE_LOOP_ANALYSIS"] = "YES",
    ["CLANG_WARN_STRICT_PROTOTYPES"] = "YES",
    ["CLANG_WARN_SUSPICIOUS_MOVE"] = "YES",
    ["CLANG_WARN_UNREACHABLE_CODE"] = "YES",
    -- not an error, apple's typo
    ["CLANG_WARN__DUPLICATE_METHOD_MATCH"] = "YES",
    ["GCC_WARN_UNINITIALIZED_AUTOS"] = "YES",
    ["GCC_WARN_64_TO_32_BIT_CONVERSION"] = "YES",
    ["GCC_WARN_UNDECLARED_SELECTOR"] = "YES",
    ["GCC_WARN_UNUSED_FUNCTION"] = "YES",
    ["GCC_WARN_UNUSED_VARIABLE"] = "YES",
    ["GCC_NO_COMMON_BLOCKS"] = "YES",
    ["DEAD_CODE_STRIPPING"] = "YES",
    ["ENABLE_STRICT_OBJC_MSGSEND"] = "YES",
  })

end
