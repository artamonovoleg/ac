-- require("premake-export-compile-commands/export-compile-commands")
if (os.isdir("premake-consoles")) then
  require("premake-consoles/consoles")
end

local script_dir = _MAIN_SCRIPT_DIR
script_dir = path.getabsolute(script_dir)
script_dir = path.normalize(script_dir)

local curr_dir = path.getabsolute(".")
curr_dir = path.normalize(curr_dir)

if (script_dir == curr_dir) then
  workspace("ac")

  configurations({
    "release",
    "debug",
    "dist"
  })
end

include("ac_common_settings")
include("ac_compile_shader")
include("ac_core")
include("ac_main")
include("ac_renderer")
include("ac_render_graph")
include("ac_window")
include("ac_input")
