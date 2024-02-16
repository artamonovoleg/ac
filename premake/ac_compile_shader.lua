local RD = path.getabsolute("../") .. "/"

local function setup_cmd(additional_filter, shader, cmd)
  filter({ "files:" .. shader, additional_filter })
    buildaction("none")
    buildinputs({ shader })
    buildcommands({
      "{MKDIR} %{file.directory}/compiled/",
      "{MKDIR} %{file.directory}/compiled/debug",
      cmd
    })
    buildmessage(cmd)

    buildoutputs({ "%{file.directory}/compiled/%{file.basename}.h" })
  filter({})
end

function ac_compile_shader(shader, stages, additional_filter)
  if (path.isabsolute(shader)) then
    error("sorry for this mess, you can't pass absolute path to ac_compile_shader " .. shader)
  end

  local ACSC = RD .. "data/ac-shader-compiler/" .. os.host() .. "/ac-shader-compiler"
  if (os.host() == "windows") then
    ACSC = ACSC .. ".exe"
    ACSC = ACSC:lower()
  end

  files({
    shader
  })

  local prmk_to_ac = {
    windows = "windows",
    linux = "linux",
    macosx = "apple-macos",
    ios = "apple-ios",
    xbox
  }

  prmk_to_ac["windows"] = "windows"
  prmk_to_ac["linux"] = "linux"
  prmk_to_ac["macosx"] = "apple-macos"
  prmk_to_ac["ios"] = "apple-ios"
  prmk_to_ac["xbox-xs"] = "xbox-xs"
  prmk_to_ac["xbox-one"] = "xbox-one"
  prmk_to_ac["nintendo-switch"] = "nintendo-switch"
  prmk_to_ac["ps4"] = "ps4"
  prmk_to_ac["ps5"] = "ps5"

  local cmd = ""
  cmd = cmd .. ACSC
  cmd = cmd .. " -i %{file.relpath}"
  cmd = cmd .. " -o %{file.directory}/compiled/%{file.basename}.h"
  cmd = cmd .. " --debug-dir %{file.directory}/compiled/debug"
  cmd = cmd .. " -s " .. stages
  cmd = cmd .. " --platform " .. prmk_to_ac[os.target()]

  if
  (
    os.istarget("windows") or
    os.istarget("linux") or
    os.istarget("nintendo-switch") or
    os.istarget("macosx") or
    os.istarget("ios")
  )
  then
    cmd = cmd .. " --enable-spirv"
  end

  filter({ "files:" .. shader })
    buildaction("none")
    buildinputs({ shader })
    buildcommands({
      "{MKDIR} %{file.directory}/compiled/",
      "{MKDIR} %{file.directory}/compiled/debug",
      cmd
    })
    buildmessage(cmd)

    buildoutputs({ "%{file.directory}/compiled/%{file.basename}.h" })
  filter({})
end
