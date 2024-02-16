local HAS_EXPORT_COMPILE_COMMANDS = os.isdir("../premake-export-compile-commands")

if (HAS_EXPORT_COMPILE_COMMANDS) then
  require("../premake-export-compile-commands/export-compile-commands")
end

local p = premake

p.modules.cli = {}
local m = p.modules.cli

local workspace = p.workspace

local function cli_build()
  local wks = p.global.eachWorkspace()()

  local cfg = _OPTIONS["cli-config"]
  local prj = _OPTIONS["cli-project"]
  local cc = _OPTIONS["cc"]
  local sys = os.target()

  local prmk_sln_cmd = _PREMAKE_COMMAND .. " --file=" .. _MAIN_SCRIPT .. " --os=" .. sys
  if (cc) then
    prmk_sln_cmd = prmk_sln_cmd .. " --cc=" .. cc
  end

  local prmk_cdb_cmd = prmk_sln_cmd .. " export-compile-commands"
  local build_cmd = ""

  local makefiles_dir = _MAIN_SCRIPT_DIR .. "/../makefiles/"

  if (os.host() == "windows") then
    local MSBUILD = ""

    local handle = io.popen("\"\"%ProgramFiles(x86)%\\Microsoft Visual Studio\\Installer\\vswhere.exe\" -latest -prerelease -products * -requires Microsoft.Component.MSBuild -find MSBuild\\**\\Bin\\MSBuild.exe\"")
    local output = handle:read('*a')
    MSBUILD = output:gsub('[\n\r]', '')
    MSBUILD = path.normalize(MSBUILD)

    local sln = makefiles_dir .. "vs2022-" .. os.target() .. "/" .. wks.filename .. ".sln"
    sln = path.normalize(sln)

    build_cmd = "\"" .. MSBUILD .. "\" " .. sln .. " /t:" .. prj .. " /p:Configuration=" .. cfg

    prmk_sln_cmd = prmk_sln_cmd .. " vs2022"
  end

  if (os.host() == "macosx") then
    local dest
    if (os.target() == "macosx") then
      dest = "generic/platform=macOS"
    elseif (os.target() == "ios") then
      dest = "generic/platform=iOS"
    -- elseif (cfg.system == "ios-simulator") then
    --   dest = "generic/platform=iOS Simulator"
    -- elseif (cfg.system == "tvos") then
    --   dest = "generic/platform=tvOS"
    -- elseif (cfg.system == "tvos-simulator") then
    --   dest = "generic/platform=tvOS Simulator"
    -- elseif (cfg.system == "xros") then
    --   dest = "generic/platform=xrOS"
    -- elseif (cfg.system == "xrsimulator") then
    --   dest = "generic/platform=xrOS Simulator"
    else
      error("destination is null " .. cfg.system)
    end

    prmk_sln_cmd = prmk_sln_cmd .. " xcode4"

    local xcwks = makefiles_dir .. "xcode4-" .. os.target() .. "/" .. wks.filename .. ".xcworkspace"
    xcwks = path.normalize(xcwks)

    build_cmd = "xcodebuild "
      .. " -quiet -parallelizeTargets -jobs 9 -workspace "
      .. xcwks
      .. " -configuration "
      .. cfg
      .. " -scheme "
      .. prj
      .. " build "
      .. " -destination "
      .. dest
  end

  if (os.host() == "linux") then
    prmk_sln_cmd = prmk_sln_cmd .. " gmake2"

    local dir = MAKEFILES_DIR .. "gmake2-" .. os.target() .. "/"
    dir = path.normalize(dir)

    build_cmd = "make " .. prj .. " -j30 " .. " config=" .. cfg .. " -C " .. dir
  end

  if (HAS_EXPORT_COMPILE_COMMANDS) then
    os.execute(prmk_cdb_cmd)
  end
  os.execute(prmk_sln_cmd)
  os.execute(build_cmd)
end

newaction({
  trigger = 'cli-build',
  description = 'cli-build',
  execute = cli_build
})

newoption({
  trigger     = "cli-config",
  description = "build config",
  default     = "release",
  category    = "cli",
})

newoption({
  trigger     = "cli-project",
  description = "build project",
  default     = "cli-phony-all",
  category    = "cli",
})

local real_project = project

function project(name)

  real_project("cli-phony-all")
    kind("Utility")

    dependson({ name })

  real_project(name)

end

return m
