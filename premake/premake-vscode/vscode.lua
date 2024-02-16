local p = premake

p.modules.vscode = {}
local m = p.modules.vscode

local workspace = p.workspace
local project = p.project

local INDENT_SIZE = 2

local SEPARATOR = "_"

local function make_str(indent_level, str)
  return string.rep(" ", INDENT_SIZE * indent_level) .. str .. "\n"
end

local function get_prj_name(cfg, name)
  return name .. SEPARATOR .. cfg.system .. SEPARATOR .. cfg.name
end

local function get_build_task_label(cfg, name)
  return "build" .. SEPARATOR .. get_prj_name(cfg, name)
end

local function gen_project_build_task(l, wks, cfg, prj)
  local s = ""

  l = l + 1
  s = s .. make_str(l, "{")
  l = l + 1
  s = s .. make_str(l, "\"type\": \"shell\",")
  s = s .. make_str(l, "\"label\": \"" .. get_build_task_label(cfg, prj.name) .. "\",")

  s = s .. make_str(l, "\"command\": \"" .. _PREMAKE_COMMAND .. "\",")

  s = s .. make_str(l, "\"args\": [")
  l = l + 1

  s = s .. make_str(l, "\"--cli-config=" .. cfg.name .. "\",")
  s = s .. make_str(l, "\"--cli-project=" .. prj.name .. "\",")
  s = s .. make_str(l, "\"--file=" .. _MAIN_SCRIPT .. "\",")
  s = s .. make_str(l, "\"--os=" .. os.target() .. "\",")
  if (_OPTIONS["cc"] ~= nil) then
    s = s .. make_str(l, "\"--cc=" .. _OPTIONS["cc"] .. "\",")
  end
  s = s .. make_str(l, "\"cli-build\",")

  l = l - 1
  s = s .. make_str(l, "],")

  s = s .. make_str(l, "\"presentation\": {")
  l = l + 1
  s = s .. make_str(l, "\"reveal\": \"silent\"")
  s = s .. make_str(l, "},")
  l = l - 1

  s = s .. make_str(l, "\"problemMatcher\": []")

  l = l - 1
  s = s .. make_str(l, "},")

  return s
end

local function gen_project_launch(l, wks, cfg, prj)
  s = ""

  local cmd = cfg.targetdir .. "/" .. prj.name

  if (cfg.targetsuffix) then
    cmd = cmd .. cfg.targetsuffix
  else
    if (os.istarget("windows")) then
      cmd = cmd .. ".exe"
    end
  end

  cmd = path.normalize(cmd)

  local n = get_prj_name(cfg, prj.name)

  if (cfg.system == "windows") then
    l = l + 1
    s = s .. make_str(l, "{")
    l = l + 1
    s = s .. make_str(l, "\"name\": \"" .. n .. "\",")
    s = s .. make_str(l, "\"type\": \"cppvsdbg\",")
    s = s .. make_str(l, "\"request\": \"launch\",")
    s = s .. make_str(l, "\"program\": \"" .. cmd .. "\",")
    s = s .. make_str(l, "\"args\": [],")
    s = s .. make_str(l, "\"stopAtEntry\": false,")
    s = s .. make_str(l, "\"cwd\": \"${workspaceRoot}\",")
    s = s .. make_str(l, "\"environment\": [],")
    s = s .. make_str(l, "\"console\": \"internalConsole\",")
    s = s .. make_str(l, "\"preLaunchTask\": \"" .. get_build_task_label(cfg, prj.name) .. "\"")
    l = l - 1
    s = s .. make_str(l, "},")
  end

  if (cfg.system == "macosx") then
    l = l + 1
    s = s .. make_str(l, "{")
    l = l + 1
    s = s .. make_str(l, "\"name\": \"" .. n .. "\",")
    s = s .. make_str(l, "\"type\": \"lldb\",")
    s = s .. make_str(l, "\"request\": \"launch\",")
    if (prj.kind == "WindowedApp") then
      s = s .. make_str(l, "\"program\": \"" .. cmd .. ".app/Contents/MacOS/" .. prj.name .. "\",")
    else
      s = s .. make_str(l, "\"program\": \"" .. cmd .. "\",")
    end
    s = s .. make_str(l, "\"args\": [],")
    s = s .. make_str(l, "\"cwd\": \"${workspaceRoot}\",")
    s = s .. make_str(l, "\"preLaunchTask\": \"" .. get_build_task_label(cfg, prj.name) .. "\"")
    l = l - 1
    s = s .. make_str(l, "},")
  end

  if (cfg.system == "linux") then
    l = l + 1
    s = s .. make_str(l, "{")
    l = l + 1
    s = s .. make_str(l, "\"name\": \"" .. n .. "\",")
    s = s .. make_str(l, "\"type\": \"cppdbg\",")
    s = s .. make_str(l, "\"request\": \"launch\",")
    s = s .. make_str(l, "\"program\": \"" .. cmd .. "\",")
    s = s .. make_str(l, "\"args\": [],")
    s = s .. make_str(l, "\"stopAtEntry\": false,")
    s = s .. make_str(l, "\"cwd\": \"${workspaceRoot}\",")
    s = s .. make_str(l, "\"environment\": [],")
    s = s .. make_str(l, "\"externalConsole\": false,")
    s = s .. make_str(l, "\"MIMode\": \"gdb\",")
    s = s .. make_str(l, "\"preLaunchTask\": \"" .. get_build_task_label(cfg, prj.name) .. "\",")

    s = s .. make_str(l, "\"setupCommands\": [")
    l = l + 1

    s = s .. make_str(l, "{")
    l = l + 1
    s = s .. make_str(l, "\"description\": \"Enable pretty-printing for gdb\",")
    s = s .. make_str(l, "\"text\": \"-enable-pretty-printing\",")
    s = s .. make_str(l, "\"ignoreFailures\": true")
    l = l - 1
    s = s .. make_str(l, "},")

    s = s .. make_str(l, "{")
    l = l + 1
    s = s .. make_str(l, "\"description\": \"Set Disassembly Flavor to Intel\",")
    s = s .. make_str(l, "\"text\": \"-gdb-set disassembly-flavor intel\",")
    s = s .. make_str(l, "\"ignoreFailures\": true")
    l = l - 1
    s = s .. make_str(l, "}")

    l = l - 1
    s = s .. make_str(l, "]")

    l = l - 1
    s = s .. make_str(l, "},")
  end

  return s
end

local function execute()

  local cfg_tasks = {}
  local cfg_launch = {}
  local wks = p.global.eachWorkspace()()

  local task_json = ""
  local launch_json = ""

  for prj in workspace.eachproject(wks) do
    for cfg in project.eachconfig(prj) do
      if (prj.kind == "WindowedApp" or prj.kind == "ConsoleApp") then
        task_json = task_json .. gen_project_build_task(1, wks, cfg, prj)

        launch_json = launch_json .. gen_project_launch(1, wks, cfg, prj)
      end
    end
  end

  local tasks_file = string.format('tasks.json', cfg_key)
  p.generate(wks, tasks_file, function(wks)
    p.w('{')
      p.w(make_str(1, "\"version\": \"2.0.0\","))
      p.w(make_str(1, "\"tasks\": ["))
      p.w(task_json)
      p.w(make_str(1, "]"))
    p.w('}')
  end)

  local launch_file = 'launch.json'
  p.generate(wks, launch_file, function(wks)
    p.w('{')
      p.w(make_str(1, "\"version\": \"0.2.0\","))
      p.w(make_str(1, "\"configurations\": ["))
      p.w(launch_json)
      p.w(make_str(1, "]"))
    p.w('}')
  end)
end

newaction {
  trigger = 'vscode',
  description = 'Export vscode task.json and launch.json',
  execute = execute
}

return m
