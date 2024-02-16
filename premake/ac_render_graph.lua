include("ac_common_settings")
require("ac_compile_shader")

local RD = "../"

project("ac-render-graph-shaders")
  kind("Utility")

  uuid("301C5FA0-175C-11EE-9B4B-0800200C9A66")

  ac_compile_shader("../internal/render_graph/shaders/blit.acsl", "cs")

project("ac-render-graph")
  kind("StaticLib")

  uuid("5a913960-730d-11ee-a565-0800200c9a66")

  dependson("ac-render-graph-shaders")

  files({
    RD .. "external/c_array/c_array.h",
    RD .. "external/c_hashmap/c_hashmap.h",
    RD .. "external/c_hashmap/c_hashmap.c",
    RD .. "include/*",
    RD .. "internal/ac_private.h",
    RD .. "internal/renderer/renderer.h",
    RD .. "internal/render_graph/render_graph.h",
    RD .. "internal/render_graph/builder.c",
    RD .. "internal/render_graph/common_pass.c",
    RD .. "internal/render_graph/debug.c",
    RD .. "internal/render_graph/execute.c",
    RD .. "internal/render_graph/storage.c",
  })
