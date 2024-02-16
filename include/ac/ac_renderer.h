#pragma once

#include "ac_base.h"

#if (AC_INCLUDE_RENDERER)

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

#define AC_MAX_ATTACHMENT_COUNT 8
#define AC_MAX_VERTEX_BINDING_COUNT 15
#define AC_MAX_VERTEX_ATTRIBUTE_COUNT 15
#define AC_MAX_FRAME_IN_FLIGHT 2

#define AC_WHOLE_SIZE (0ull)
#define AC_WHOLE_LEVELS (0ull)
#define AC_WHOLE_LAYERS (0ull)
#define AC_SHADER_UNUSED (~0u)

AC_DEFINE_HANDLE(ac_device);
AC_DEFINE_HANDLE(ac_queue);
AC_DEFINE_HANDLE(ac_cmd_pool);
AC_DEFINE_HANDLE(ac_cmd);
AC_DEFINE_HANDLE(ac_fence);
AC_DEFINE_HANDLE(ac_swapchain);
AC_DEFINE_HANDLE(ac_sampler);
AC_DEFINE_HANDLE(ac_image);
AC_DEFINE_HANDLE(ac_buffer);
AC_DEFINE_HANDLE(ac_shader);
AC_DEFINE_HANDLE(ac_dsl);
AC_DEFINE_HANDLE(ac_descriptor_buffer);
AC_DEFINE_HANDLE(ac_pipeline);
AC_DEFINE_HANDLE(ac_as);
AC_DEFINE_HANDLE(ac_sbt);

typedef enum ac_device_debug_bit {
  ac_device_debug_minimal_bit = AC_BIT(0),
  ac_device_debug_validation_bit = AC_BIT(1),
  ac_device_debug_validation_gpu_based_bit = AC_BIT(2),
  ac_device_debug_validation_synchronization_bit = AC_BIT(3),
  ac_device_debug_enable_debugger_attaching_bit = AC_BIT(4)
} ac_device_debug_bit;
typedef uint32_t ac_device_debug_bits;

typedef enum ac_queue_type {
  ac_queue_type_graphics = 0,
  ac_queue_type_compute = 1,
  ac_queue_type_transfer = 2,
  ac_queue_type_count
} ac_queue_type;

typedef enum ac_color_space {
  ac_color_space_srgb = 0,
  ac_color_space_hdr = 1,
} ac_color_space;

typedef enum ac_swapchain_bit {
  ac_swapchain_wants_unorm_format_bit = AC_BIT(0),
  ac_swapchain_wants_hdr_bit = AC_BIT(1),
} ac_swapchain_bit;
typedef uint32_t ac_swapchain_bits;

typedef enum ac_format {
  ac_format_undefined = 0,
  ac_format_r8_unorm = 1,
  ac_format_r8_snorm = 2,
  ac_format_r8_uint = 3,
  ac_format_r8_sint = 4,
  ac_format_r8_srgb = 5,
  ac_format_r5g6b5_unorm = 6,
  ac_format_r8g8_unorm = 7,
  ac_format_r8g8_snorm = 8,
  ac_format_r8g8_uint = 9,
  ac_format_r8g8_sint = 10,
  ac_format_r8g8_srgb = 11,
  ac_format_r16_unorm = 12,
  ac_format_r16_snorm = 13,
  ac_format_r16_uint = 14,
  ac_format_r16_sint = 15,
  ac_format_r16_sfloat = 16,
  ac_format_r8g8b8a8_unorm = 17,
  ac_format_r8g8b8a8_snorm = 18,
  ac_format_r8g8b8a8_uint = 19,
  ac_format_r8g8b8a8_sint = 20,
  ac_format_r8g8b8a8_srgb = 21,
  ac_format_b8g8r8a8_unorm = 22,
  ac_format_b8g8r8a8_srgb = 23,
  ac_format_r16g16_unorm = 24,
  ac_format_r16g16_snorm = 25,
  ac_format_r16g16_uint = 26,
  ac_format_r16g16_sint = 27,
  ac_format_r16g16_sfloat = 28,
  ac_format_r32_uint = 29,
  ac_format_r32_sint = 30,
  ac_format_r32_sfloat = 31,
  ac_format_r10g10b10a2_unorm = 32,
  ac_format_r10g10b10a2_uint = 33,
  ac_format_b10g10r10a2_unorm = 34,
  ac_format_b10g10r10a2_uint = 35,
  ac_format_e5b9g9r9_ufloat = 36,
  ac_format_r16g16b16a16_unorm = 37,
  ac_format_r16g16b16a16_snorm = 38,
  ac_format_r16g16b16a16_uint = 39,
  ac_format_r16g16b16a16_sint = 40,
  ac_format_r16g16b16a16_sfloat = 41,
  ac_format_r32g32_uint = 42,
  ac_format_r32g32_sint = 43,
  ac_format_r32g32_sfloat = 44,
  ac_format_r32g32b32_uint = 45,
  ac_format_r32g32b32_sint = 46,
  ac_format_r32g32b32_sfloat = 47,
  ac_format_r32g32b32a32_uint = 48,
  ac_format_r32g32b32a32_sint = 49,
  ac_format_r32g32b32a32_sfloat = 50,
  ac_format_d16_unorm = 51,
  ac_format_d32_sfloat = 52,
  ac_format_s8_uint = 53,
} ac_format;

typedef enum ac_attachment_load_op {
  ac_attachment_load_op_load = 0,
  ac_attachment_load_op_clear = 1,
  ac_attachment_load_op_dont_care = 2
} ac_attachment_load_op;

typedef enum ac_attachment_store_op {
  ac_attachment_store_op_none = 0,
  ac_attachment_store_op_store = 1,
  ac_attachment_store_op_dont_care = 2
} ac_attachment_store_op;

typedef enum ac_memory_usage {
  ac_memory_usage_gpu_only = 0,
  ac_memory_usage_cpu_to_gpu = 1,
  ac_memory_usage_gpu_to_cpu = 2,
  ac_memory_usage_count
} ac_memory_usage;

typedef enum ac_access_bit {
  ac_access_none = 0,
  ac_access_indirect_command_read_bit = AC_BIT(0),
  ac_access_index_read_bit = AC_BIT(1),
  ac_access_vertex_attribute_read_bit = AC_BIT(2),
  ac_access_uniform_read_bit = AC_BIT(3),
  ac_access_shader_read_bit = AC_BIT(4),
  ac_access_shader_write_bit = AC_BIT(5),
  ac_access_color_read_bit = AC_BIT(6),
  ac_access_color_attachment_bit = AC_BIT(7),
  ac_access_depth_stencil_read_bit = AC_BIT(8),
  ac_access_depth_stencil_attachment_bit = AC_BIT(9),
  ac_access_transfer_read_bit = AC_BIT(10),
  ac_access_transfer_write_bit = AC_BIT(11),
  ac_access_acceleration_structure_read_bit = AC_BIT(12),
  ac_access_acceleration_structure_write_bit = AC_BIT(13),
  ac_access_resolve_bit = AC_BIT(14),
} ac_access_bit;
typedef uint32_t ac_access_bits;

typedef enum ac_shader_stage {
  ac_shader_stage_vertex = 0,
  ac_shader_stage_pixel = 1,
  ac_shader_stage_compute = 2,
  ac_shader_stage_raygen = 3,
  ac_shader_stage_any_hit = 4,
  ac_shader_stage_closest_hit = 5,
  ac_shader_stage_miss = 6,
  ac_shader_stage_intersection = 7,
  ac_shader_stage_amplification = 8,
  ac_shader_stage_mesh = 9,
  ac_shader_stage_count,
} ac_shader_stage;

typedef enum ac_pipeline_type {
  ac_pipeline_type_compute = 0,
  ac_pipeline_type_graphics = 1,
  ac_pipeline_type_raytracing = 2,
  ac_pipeline_type_mesh = 3,
} ac_pipeline_type;

typedef enum ac_pipeline_stage_bit {
  ac_pipeline_stage_none = 0,
  ac_pipeline_stage_top_of_pipe_bit = AC_BIT(0),
  ac_pipeline_stage_draw_indirect_bit = AC_BIT(1),
  ac_pipeline_stage_vertex_input_bit = AC_BIT(2),
  ac_pipeline_stage_vertex_shader_bit = AC_BIT(3),
  ac_pipeline_stage_pixel_shader_bit = AC_BIT(4),
  ac_pipeline_stage_early_pixel_tests_bit = AC_BIT(5),
  ac_pipeline_stage_late_pixel_tests_bit = AC_BIT(6),
  ac_pipeline_stage_color_attachment_output_bit = AC_BIT(7),
  ac_pipeline_stage_compute_shader_bit = AC_BIT(8),
  ac_pipeline_stage_transfer_bit = AC_BIT(9),
  ac_pipeline_stage_bottom_of_pipe_bit = AC_BIT(10),
  ac_pipeline_stage_all_graphics_bit = AC_BIT(11),
  ac_pipeline_stage_all_commands_bit = AC_BIT(12),
  ac_pipeline_stage_acceleration_structure_build_bit = AC_BIT(13),
  ac_pipeline_stage_raytracing_shader_bit = AC_BIT(14),
  ac_pipeline_stage_resolve_bit = AC_BIT(15),
} ac_pipeline_stage_bit;
typedef uint32_t ac_pipeline_stage_bits;

typedef enum ac_image_layout {
  ac_image_layout_undefined = 0,
  ac_image_layout_general = 1,
  ac_image_layout_color_write = 2,
  ac_image_layout_depth_stencil_write = 3,
  ac_image_layout_depth_stencil_read = 4,
  ac_image_layout_shader_read = 5,
  ac_image_layout_transfer_src = 6,
  ac_image_layout_transfer_dst = 7,
  ac_image_layout_present_src = 8,
  ac_image_layout_resolve = 9,
} ac_image_layout;

typedef enum ac_buffer_usage_bit {
  ac_buffer_usage_none = 0,
  ac_buffer_usage_transfer_src_bit = AC_BIT(0),
  ac_buffer_usage_transfer_dst_bit = AC_BIT(1),
  ac_buffer_usage_vertex_bit = AC_BIT(2),
  ac_buffer_usage_index_bit = AC_BIT(3),
  ac_buffer_usage_cbv_bit = AC_BIT(4),
  ac_buffer_usage_srv_bit = AC_BIT(5),
  ac_buffer_usage_uav_bit = AC_BIT(6),
  ac_buffer_usage_raytracing_bit = AC_BIT(7),
} ac_buffer_usage_bit;
typedef uint32_t ac_buffer_usage_bits;

typedef enum ac_image_usage_bit {
  ac_image_usage_none = 0,
  ac_image_usage_transfer_src_bit = AC_BIT(0),
  ac_image_usage_transfer_dst_bit = AC_BIT(1),
  ac_image_usage_srv_bit = AC_BIT(2),
  ac_image_usage_uav_bit = AC_BIT(3),
  ac_image_usage_attachment_bit = AC_BIT(4),
} ac_image_usage_bit;
typedef uint32_t ac_image_usage_bits;

typedef enum ac_image_type {
  ac_image_type_1d = 0,
  ac_image_type_2d = 1,
  ac_image_type_cube = 2,
  ac_image_type_1d_array = 3,
  ac_image_type_2d_array = 4,
} ac_image_type;

typedef enum ac_descriptor_type {
  ac_descriptor_type_sampler = 0,
  ac_descriptor_type_srv_image = 1,
  ac_descriptor_type_uav_image = 2,
  ac_descriptor_type_cbv_buffer = 3,
  ac_descriptor_type_srv_buffer = 4,
  ac_descriptor_type_uav_buffer = 5,
  ac_descriptor_type_as = 6,
} ac_descriptor_type;

typedef enum ac_input_rate {
  ac_input_rate_vertex = 0,
  ac_input_rate_instance = 1,
} ac_input_rate;

typedef enum ac_attribute_semantic {
  ac_attribute_semantic_position = 0,
  ac_attribute_semantic_normal = 1,
  ac_attribute_semantic_color = 2,
  ac_attribute_semantic_texcoord0 = 3,
  ac_attribute_semantic_texcoord1 = 4,
  ac_attribute_semantic_texcoord2 = 5,
  ac_attribute_semantic_texcoord3 = 6,
  ac_attribute_semantic_texcoord4 = 7,
  ac_attribute_semantic_texcoord5 = 8,
  ac_attribute_semantic_texcoord6 = 9,
  ac_attribute_semantic_texcoord7 = 10,
  ac_attribute_semantic_texcoord8 = 11,
  ac_attribute_semantic_texcoord9 = 12,
  ac_attribute_semantic_count,
} ac_attribute_semantic;

typedef enum ac_index_type {
  ac_index_type_u16 = 0,
  ac_index_type_u32 = 1,
} ac_index_type;

typedef enum ac_front_face {
  ac_front_face_clockwise = 0,
  ac_front_face_counter_clockwise = 1,
} ac_front_face;

typedef enum ac_polygon_mode {
  ac_polygon_mode_fill = 0,
  ac_polygon_mode_line = 1,
} ac_polygon_mode;

typedef enum ac_sampler_mipmap_mode {
  ac_sampler_mipmap_mode_nearest = 0,
  ac_sampler_mipmap_mode_linear = 1,
} ac_sampler_mipmap_mode;

typedef enum ac_sampler_address_mode {
  ac_sampler_address_mode_repeat = 0,
  ac_sampler_address_mode_mirrored_repeat = 1,
  ac_sampler_address_mode_clamp_to_edge = 2,
  ac_sampler_address_mode_clamp_to_border = 3,
} ac_sampler_address_mode;

typedef enum ac_compare_op {
  ac_compare_op_never = 0,
  ac_compare_op_less = 1,
  ac_compare_op_equal = 2,
  ac_compare_op_less_or_equal = 3,
  ac_compare_op_greater = 4,
  ac_compare_op_not_equal = 5,
  ac_compare_op_greater_or_equal = 6,
  ac_compare_op_always = 7,
} ac_compare_op;

typedef enum ac_cull_mode {
  ac_cull_mode_none = 0,
  ac_cull_mode_front = 1,
  ac_cull_mode_back = 2,
} ac_cull_mode;

typedef enum ac_filter {
  ac_filter_nearest = 0,
  ac_filter_linear = 1,
} ac_filter;

typedef enum ac_primitive_topology {
  ac_primitive_topology_point_list = 0,
  ac_primitive_topology_line_list = 1,
  ac_primitive_topology_line_strip = 2,
  ac_primitive_topology_triangle_list = 3,
  ac_primitive_topology_triangle_strip = 4,
} ac_primitive_topology;

typedef enum ac_blend_factor {
  ac_blend_factor_zero = 0,
  ac_blend_factor_one = 1,
  ac_blend_factor_src_color = 2,
  ac_blend_factor_one_minus_src_color = 3,
  ac_blend_factor_dst_color = 4,
  ac_blend_factor_one_minus_dst_color = 5,
  ac_blend_factor_src_alpha = 6,
  ac_blend_factor_one_minus_src_alpha = 7,
  ac_blend_factor_dst_alpha = 8,
  ac_blend_factor_one_minus_dst_alpha = 9,
  ac_blend_factor_src_alpha_saturate = 10,
} ac_blend_factor;

typedef enum ac_blend_op {
  ac_blend_op_add = 0,
  ac_blend_op_subtract = 1,
  ac_blend_op_reverse_subtract = 2,
  ac_blend_op_min = 3,
  ac_blend_op_max = 4,
} ac_blend_op;

typedef enum ac_space {
  ac_space0 = 0,
  ac_space1 = 1,
  ac_space2 = 2,
  ac_space_count,
} ac_space;

typedef enum ac_channel_bit {
  ac_channel_r_bit = AC_BIT(0),
  ac_channel_g_bit = AC_BIT(1),
  ac_channel_b_bit = AC_BIT(2),
  ac_channel_a_bit = AC_BIT(3),
} ac_channel_bit;

typedef uint32_t ac_channel_bits;

typedef enum ac_raytracing_group_type {
  ac_raytracing_group_type_general = 0,
  ac_raytracing_group_type_triangles = 1,
  ac_raytracing_group_type_procedural = 2,
} ac_raytracing_group_type;

typedef enum ac_as_instance_bit {
  ac_as_instance_triangle_facing_cull_disable_bit = AC_BIT(0),
  ac_as_instance_triangle_flip_facing_bit = AC_BIT(1),
  ac_as_instance_force_opaque_bit = AC_BIT(2),
  ac_as_instance_force_no_opaque_bit = AC_BIT(3),
} ac_as_instance_bit;

typedef uint32_t ac_as_instance_bits;

typedef enum ac_geometry_bit {
  ac_geometry_opaque_bit = AC_BIT(0),
  ac_geometry_no_duplicate_any_hit_invocation_bit = AC_BIT(1),
} ac_geometry_bit;

typedef uint32_t ac_geometry_bits;

typedef enum ac_geometry_type {
  ac_geometry_type_triangles = 0,
  ac_geometry_type_aabbs = 1,
} ac_geometry_type;

typedef enum ac_as_type {
  ac_as_type_bottom_level = 0,
  ac_as_type_top_level = 1,
} ac_as_type;

typedef struct ac_wsi {
  void* user_data;
  void* native_window;

  ac_result (*get_vk_instance_extensions)(
    void*        data,
    uint32_t*    count,
    const char** names);

  ac_result (*create_vk_surface)(void* data, void* instance, void** surface);
} ac_wsi;

typedef struct ac_device_properties {
  const char* api;
  uint64_t    cbv_buffer_alignment;
  uint64_t    image_row_alignment;
  uint64_t    image_alignment;
  uint8_t     max_sample_count;
  uint32_t    as_instance_size;
} ac_device_properties;

typedef struct ac_device_info {
  const ac_wsi*        wsi;
  ac_device_debug_bits debug_bits;
  bool                 force_vulkan;
} ac_device_info;

typedef struct ac_cmd_pool_info {
  ac_queue queue;
} ac_cmd_pool_info;

typedef struct ac_sampler_info {
  ac_filter               mag_filter;
  ac_filter               min_filter;
  ac_sampler_mipmap_mode  mipmap_mode;
  ac_sampler_address_mode address_mode_u;
  ac_sampler_address_mode address_mode_v;
  ac_sampler_address_mode address_mode_w;
  float                   mip_lod_bias;
  bool                    anisotropy_enable;
  uint32_t                max_anisotropy;
  bool                    compare_enable;
  ac_compare_op           compare_op;
  float                   min_lod;
  float                   max_lod;
} ac_sampler_info;

typedef struct ac_clear_value {
  union {
    float color[4];
    struct {
      float   depth;
      uint8_t stencil;
    };
  };
} ac_clear_value;

typedef struct ac_image_info {
  uint32_t            width;
  uint32_t            height;
  ac_format           format;
  uint8_t             samples;
  uint16_t            layers;
  uint16_t            levels;
  ac_image_usage_bits usage;
  ac_clear_value      clear_value;
  ac_image_type       type;
  const char*         name;
} ac_image_info;

typedef struct ac_buffer_info {
  uint64_t             size;
  ac_buffer_usage_bits usage;
  ac_memory_usage      memory_usage;
  const char*          name;
} ac_buffer_info;

typedef struct ac_swapchain_info {
  ac_queue          queue;
  ac_swapchain_bits bits;
  bool              vsync;
  uint32_t          min_image_count;
  const ac_wsi*     wsi;
  uint32_t          width;
  uint32_t          height;
} ac_swapchain_info;

typedef enum ac_fence_bit {
  ac_fence_present_bit = AC_BIT(0),
} ac_fence_bit;
typedef uint32_t ac_fence_bits;

typedef struct ac_fence_info {
  ac_fence_bits bits;
} ac_fence_info;

typedef struct ac_fence_submit_info {
  ac_fence               fence;
  ac_pipeline_stage_bits stages;
  uint64_t               value;
} ac_fence_submit_info;

typedef struct ac_queue_submit_info {
  uint32_t              cmd_count;
  ac_cmd*               cmds;
  uint32_t              wait_fence_count;
  ac_fence_submit_info* wait_fences;
  uint32_t              signal_fence_count;
  ac_fence_submit_info* signal_fences;
} ac_queue_submit_info;

typedef struct ac_queue_present_info {
  uint32_t     wait_fence_count;
  ac_fence*    wait_fences;
  ac_swapchain swapchain;
} ac_queue_present_info;

typedef struct ac_attachment_info {
  ac_image               image;
  ac_attachment_load_op  load_op;
  ac_attachment_store_op store_op;
  ac_clear_value         clear_value;
  ac_image               resolve_image;
} ac_attachment_info;

typedef struct ac_rendering_info {
  uint32_t           color_attachment_count;
  ac_attachment_info color_attachments[AC_MAX_ATTACHMENT_COUNT];
  ac_attachment_info depth_attachment;
} ac_rendering_info;

typedef struct ac_buffer_barrier {
  ac_buffer              buffer;
  ac_pipeline_stage_bits src_stage;
  ac_pipeline_stage_bits dst_stage;
  ac_access_bits         src_access;
  ac_access_bits         dst_access;
  ac_queue               src_queue;
  ac_queue               dst_queue;
  uint64_t               offset;
  uint64_t               size;
} ac_buffer_barrier;

typedef struct ac_image_subresource_range {
  uint16_t base_level;
  uint16_t levels;
  uint16_t base_layer;
  uint16_t layers;
} ac_image_subresource_range;

typedef struct ac_image_barrier {
  ac_image                   image;
  ac_pipeline_stage_bits     src_stage;
  ac_pipeline_stage_bits     dst_stage;
  ac_access_bits             src_access;
  ac_access_bits             dst_access;
  ac_image_layout            old_layout;
  ac_image_layout            new_layout;
  ac_queue                   src_queue;
  ac_queue                   dst_queue;
  ac_image_subresource_range range;
} ac_image_barrier;

typedef struct ac_shader_info {
  ac_shader_stage stage;
  const void*     code;
  const char*     name;
} ac_shader_info;

typedef struct ac_dsl_info {
  uint32_t    shader_count;
  ac_shader*  shaders;
  const char* name;
} ac_dsl_info;

typedef struct ac_descriptor_buffer_info {
  ac_dsl      dsl;
  uint32_t    max_sets[ac_space_count];
  const char* name;
} ac_descriptor_buffer_info;

typedef struct ac_vertex_binding_info {
  uint32_t      binding;
  uint32_t      stride;
  ac_input_rate input_rate;
} ac_vertex_binding_info;

typedef struct ac_vertex_attribute_info {
  uint32_t              binding;
  ac_format             format;
  uint32_t              offset;
  ac_attribute_semantic semantic;
} ac_vertex_attribute_info;

typedef struct ac_vertex_layout {
  uint32_t                 binding_count;
  ac_vertex_binding_info   bindings[AC_MAX_VERTEX_BINDING_COUNT];
  uint32_t                 attribute_count;
  ac_vertex_attribute_info attributes[AC_MAX_VERTEX_ATTRIBUTE_COUNT];
} ac_vertex_layout;

typedef struct ac_rasterizer_state_info {
  ac_cull_mode    cull_mode;
  ac_front_face   front_face;
  ac_polygon_mode polygon_mode;
  bool            depth_bias_enable;
  int32_t         depth_bias_constant_factor;
  float           depth_bias_slope_factor;
} ac_rasterizer_state_info;

typedef struct ac_depth_state_info {
  bool          depth_test;
  bool          depth_write;
  ac_compare_op compare_op;
} ac_depth_state_info;

typedef struct ac_blend_attachment_state {
  ac_blend_factor src_factor;
  ac_blend_factor dst_factor;
  ac_blend_factor src_alpha_factor;
  ac_blend_factor dst_alpha_factor;
  ac_blend_op     op;
  ac_blend_op     alpha_op;
} ac_blend_attachment_state;

typedef struct ac_blend_state_info {
  ac_blend_attachment_state attachment_states[AC_MAX_ATTACHMENT_COUNT];
} ac_blend_state_info;

typedef struct ac_compute_pipeline_info {
  ac_dsl    dsl;
  ac_shader shader;
} ac_compute_pipeline_info;

typedef struct ac_graphics_pipeline_info {
  ac_rasterizer_state_info rasterizer_info;
  ac_depth_state_info      depth_state_info;
  uint32_t                 samples;
  uint32_t                 color_attachment_count;
  ac_channel_bits       color_attachment_discard_bits[AC_MAX_ATTACHMENT_COUNT];
  ac_format             color_attachment_formats[AC_MAX_ATTACHMENT_COUNT];
  ac_format             depth_stencil_format;
  ac_blend_state_info   blend_state_info;
  ac_dsl                dsl;
  ac_shader             pixel_shader;
  ac_shader             vertex_shader;
  ac_vertex_layout      vertex_layout;
  ac_primitive_topology topology;
} ac_graphics_pipeline_info;

typedef struct ac_mesh_pipeline_info {
  ac_rasterizer_state_info rasterizer_info;
  ac_depth_state_info      depth_state_info;
  uint32_t                 samples;
  uint32_t                 color_attachment_count;
  ac_channel_bits     color_attachment_discard_bits[AC_MAX_ATTACHMENT_COUNT];
  ac_format           color_attachment_formats[AC_MAX_ATTACHMENT_COUNT];
  ac_format           depth_stencil_format;
  ac_blend_state_info blend_state_info;
  ac_dsl              dsl;
  ac_shader           pixel_shader;
  ac_shader           task_shader;
  ac_shader           mesh_shader;
} ac_mesh_pipeline_info;

typedef struct ac_raytracing_group_info {
  ac_raytracing_group_type type;
  uint32_t                 general;
  uint32_t                 closest_hit;
  uint32_t                 any_hit;
  uint32_t                 intersection;
} ac_raytracing_group_info;

typedef struct ac_raytracing_pipeline_info {
  ac_dsl                    dsl;
  uint32_t                  shader_count;
  ac_shader*                shaders;
  uint32_t                  group_count;
  ac_raytracing_group_info* groups;
  uint32_t                  max_ray_recursion_depth;
} ac_raytracing_pipeline_info;

typedef struct ac_pipeline_info {
  ac_pipeline_type type;
  const char*      name;
  union {
    ac_graphics_pipeline_info   graphics;
    ac_mesh_pipeline_info       mesh;
    ac_compute_pipeline_info    compute;
    ac_raytracing_pipeline_info raytracing;
  };
} ac_pipeline_info;

typedef struct ac_descriptor {
  union {
    struct {
      ac_buffer buffer;
      uint64_t  offset;
      uint64_t  range;
    };
    struct {
      ac_image image;
      uint32_t level;
    };
    ac_sampler sampler;
    ac_as      as;
  };
} ac_descriptor;

typedef struct ac_descriptor_write {
  uint32_t           count;
  uint32_t           reg;
  ac_descriptor_type type;
  ac_descriptor*     descriptors;
} ac_descriptor_write;

typedef struct ac_buffer_image_copy {
  uint64_t buffer_offset;
  uint32_t x;
  uint32_t y;
  uint32_t width;
  uint32_t height;
  uint32_t level;
  uint32_t layer;
} ac_buffer_image_copy;

typedef struct ac_image_copy {
  struct {
    uint32_t x;
    uint32_t y;
    uint32_t level;
    uint32_t layer;
  } src, dst;
  uint32_t width;
  uint32_t height;
} ac_image_copy;

typedef struct ac_transform_matrix {
  float matrix[3][4];
} ac_transform_matrix;

typedef struct ac_as_instance {
  ac_transform_matrix transform;
  uint32_t            instance_index;
  uint32_t            mask;
  uint32_t            instance_sbt_offset;
  ac_as_instance_bits bits;
  ac_as               as;
} ac_as_instance;

typedef struct ac_as_geometry_triangles {
  ac_format     vertex_format;
  uint64_t      vertex_stride;
  uint32_t      vertex_count;
  uint32_t      index_count;
  ac_index_type index_type;
  struct {
    ac_buffer buffer;
    uint64_t  offset;
  } vertex, index, transform;
} ac_as_geometry_triangles;

typedef struct ac_as_geometry_aabs {
  ac_buffer buffer;
  uint64_t  offset;
  uint64_t  stride;
} ac_as_geometry_aabs;

typedef struct ac_as_geometry {
  ac_geometry_bits bits;
  ac_geometry_type type;
  union {
    ac_as_geometry_triangles triangles;
    ac_as_geometry_aabs      aabs;
  };
} ac_as_geometry;

typedef struct ac_sbt_info {
  ac_pipeline pipeline;
  uint32_t    group_count;
  const char* name;
} ac_sbt_info;

typedef struct ac_as_info {
  ac_as_type  type;
  const char* name;
  uint32_t    count;
  union {
    struct {
      ac_as_geometry* geometries;
    };
    struct {
      uint64_t        instances_buffer_offset;
      ac_buffer       instances_buffer;
      ac_as_instance* instances;
    };
  };
} ac_as_info;

typedef struct ac_as_build_info {
  ac_as     as;
  ac_buffer scratch_buffer;
  uint64_t  scratch_buffer_offset;
} ac_as_build_info;

AC_API ac_result
ac_create_device(const ac_device_info* info, ac_device* device);

AC_API void
ac_destroy_device(ac_device device);

AC_API ac_result
ac_queue_wait_idle(ac_queue queue);

AC_API ac_result
ac_queue_submit(ac_queue queue, const ac_queue_submit_info* info);

AC_API ac_result
ac_queue_present(ac_queue queue, const ac_queue_present_info* info);

AC_API ac_result
ac_create_fence(ac_device device, const ac_fence_info* info, ac_fence* fence);

AC_API void
ac_destroy_fence(ac_fence fence);

AC_API ac_result
ac_get_fence_value(ac_fence fence, uint64_t* value);

AC_API ac_result
ac_signal_fence(ac_fence fence, uint64_t value);

AC_API ac_result
ac_wait_fence(ac_fence fence, uint64_t value);

AC_API ac_result
ac_create_swapchain(
  ac_device                device,
  const ac_swapchain_info* info,
  ac_swapchain*            swapchain);

AC_API void
ac_destroy_swapchain(ac_swapchain swapchain);

AC_API ac_color_space
ac_swapchain_get_color_space(ac_swapchain swapchain);

AC_API ac_result
ac_create_cmd_pool(
  ac_device               device,
  const ac_cmd_pool_info* info,
  ac_cmd_pool*            pool);

AC_API void
ac_destroy_cmd_pool(ac_cmd_pool pool);

AC_API ac_result
ac_create_cmd(ac_cmd_pool cmd_pool, ac_cmd* cmd);

AC_API void
ac_destroy_cmd(ac_cmd cmd);

AC_API ac_result
ac_reset_cmd_pool(ac_cmd_pool pool);

AC_API ac_result
ac_begin_cmd(ac_cmd cmd);

AC_API ac_result
ac_end_cmd(ac_cmd cmd);

AC_API ac_result
ac_acquire_next_image(ac_swapchain swapchain, ac_fence fence);

AC_API ac_result
ac_create_shader(
  ac_device             device,
  const ac_shader_info* info,
  ac_shader*            shader);

AC_API void
ac_destroy_shader(ac_shader shader);

AC_API ac_result
ac_create_dsl(ac_device device, const ac_dsl_info* info, ac_dsl* dsl);

AC_API void
ac_destroy_dsl(ac_dsl dsl);

AC_API ac_result
ac_create_descriptor_buffer(
  ac_device                        device,
  const ac_descriptor_buffer_info* info,
  ac_descriptor_buffer*            buffer);

AC_API void
ac_destroy_descriptor_buffer(ac_descriptor_buffer buffer);

AC_API ac_result
ac_create_pipeline(
  ac_device               device,
  const ac_pipeline_info* info,
  ac_pipeline*            pipeline);

AC_API void
ac_destroy_pipeline(ac_pipeline pipeline);

AC_API ac_result
ac_create_buffer(
  ac_device             device,
  const ac_buffer_info* info,
  ac_buffer*            buffer);

AC_API void
ac_destroy_buffer(ac_buffer buffer);

AC_API ac_result
ac_buffer_map_memory(ac_buffer buffer);

AC_API void
ac_buffer_unmap_memory(ac_buffer buffer);

AC_API ac_result
ac_create_sampler(
  ac_device              device,
  const ac_sampler_info* info,
  ac_sampler*            sampler);

AC_API void
ac_destroy_sampler(ac_sampler sampler);

AC_API ac_result
ac_create_image(ac_device device, const ac_image_info* info, ac_image* image);

AC_API void
ac_destroy_image(ac_image image);

AC_API void
ac_update_set(
  ac_descriptor_buffer       buffer,
  ac_space                   space,
  uint32_t                   set,
  uint32_t                   count,
  const ac_descriptor_write* writes);

AC_API ac_result
ac_create_sbt(ac_device device, const ac_sbt_info* info, ac_sbt* sbt);

AC_API void
ac_destroy_sbt(ac_sbt sbt);

AC_API ac_result
ac_create_as(ac_device device, const ac_as_info* info, ac_as* as);

AC_API void
ac_destroy_as(ac_as as);

AC_API void
ac_write_as_instances(
  ac_device             device,
  uint32_t              count,
  const ac_as_instance* instances,
  void*                 mem);

AC_API void
ac_cmd_begin_rendering(ac_cmd cmd, const ac_rendering_info* info);

AC_API void
ac_cmd_end_rendering(ac_cmd cmd);

AC_API void
ac_cmd_barrier(
  ac_cmd                   cmd,
  uint32_t                 buffer_barrier_count,
  const ac_buffer_barrier* buffer_barriers,
  uint32_t                 image_barrier_count,
  const ac_image_barrier*  image_barriers);

AC_API void
ac_cmd_set_scissor(
  ac_cmd   cmd,
  uint32_t x,
  uint32_t y,
  uint32_t width,
  uint32_t height);

AC_API void
ac_cmd_set_viewport(
  ac_cmd cmd,
  float  x,
  float  y,
  float  width,
  float  height,
  float  min_depth,
  float  max_depth);

AC_API void
ac_cmd_bind_pipeline(ac_cmd cmd, ac_pipeline pipeline);

AC_API void
ac_cmd_draw_mesh_tasks(
  ac_cmd   cmd,
  uint32_t group_count_x,
  uint32_t group_count_y,
  uint32_t group_count_z);

AC_API void
ac_cmd_draw(
  ac_cmd   cmd,
  uint32_t vertex_count,
  uint32_t instance_count,
  uint32_t first_vertex,
  uint32_t first_instance);

AC_API void
ac_cmd_draw_indexed(
  ac_cmd   cmd,
  uint32_t index_count,
  uint32_t instance_count,
  uint32_t first_index,
  int32_t  first_vertex,
  uint32_t first_instance);

AC_API void
ac_cmd_bind_vertex_buffer(
  ac_cmd    cmd,
  uint32_t  binding,
  ac_buffer buffer,
  uint64_t  offset);

AC_API void
ac_cmd_bind_index_buffer(
  ac_cmd        cmd,
  ac_buffer     buffer,
  uint64_t      offset,
  ac_index_type index_type);

AC_API void
ac_cmd_copy_buffer(
  ac_cmd    cmd,
  ac_buffer src,
  uint64_t  src_offset,
  ac_buffer dst,
  uint64_t  dst_offset,
  uint64_t  size);

AC_API void
ac_cmd_copy_buffer_to_image(
  ac_cmd                      cmd,
  ac_buffer                   src,
  ac_image                    dst,
  const ac_buffer_image_copy* copy);

AC_API void
ac_cmd_copy_image(
  ac_cmd               cmd,
  ac_image             src,
  ac_image             dst,
  const ac_image_copy* copy);

AC_API void
ac_cmd_copy_image_to_buffer(
  ac_cmd                      cmd,
  ac_image                    src,
  ac_buffer                   dst,
  const ac_buffer_image_copy* copy);

AC_API void
ac_cmd_bind_set(
  ac_cmd               cmd,
  ac_descriptor_buffer buffer,
  ac_space             space,
  uint32_t             set);

AC_API void
ac_cmd_dispatch(
  ac_cmd   cmd,
  uint32_t group_count_x,
  uint32_t group_count_y,
  uint32_t group_count_z);

AC_API void
ac_cmd_push_constants(ac_cmd cmd, uint32_t size, const void* data);

AC_API void
ac_cmd_trace_rays(
  ac_cmd   cmd,
  ac_sbt   sbt,
  uint32_t width,
  uint32_t height,
  uint32_t depth);

AC_API void
ac_cmd_build_as(ac_cmd cmd, ac_as_build_info* info);

AC_API void
ac_cmd_begin_debug_label(ac_cmd cmd, const char* name, const float color[4]);

AC_API void
ac_cmd_end_debug_label(ac_cmd cmd);

AC_API void
ac_cmd_insert_debug_label(ac_cmd cmd, const char* name, const float color[4]);

AC_API void
ac_device_begin_capture(ac_device device);

AC_API void
ac_device_end_capture(ac_device device);

/*************************************************/
/*                 utils                         */
/*************************************************/

AC_API uint32_t
ac_format_size_bytes(ac_format format);

AC_API bool
ac_format_has_depth_aspect(ac_format format);

AC_API bool
ac_format_has_stencil_aspect(ac_format format);

AC_API bool
ac_format_is_srgb(ac_format format);

AC_API bool
ac_format_is_norm(ac_format format);

AC_API ac_format
ac_format_to_unorm(ac_format format);

AC_API uint32_t
ac_format_channel_count(ac_format format);

/*************************************************/
/*                 getters                       */
/*************************************************/

AC_API bool
ac_device_support_raytracing(ac_device device);

AC_API bool
ac_device_support_mesh_shaders(ac_device device);

AC_API ac_device_properties
ac_device_get_properties(ac_device device);

AC_API ac_queue
ac_device_get_queue(ac_device device, ac_queue_type type);

AC_API uint32_t
ac_image_get_width(ac_image image);
AC_API uint32_t
ac_image_get_height(ac_image image);
AC_API ac_format
ac_image_get_format(ac_image image);
AC_API uint8_t
ac_image_get_samples(ac_image image);
AC_API uint16_t
ac_image_get_levels(ac_image image);
AC_API uint16_t
ac_image_get_layers(ac_image image);
AC_API ac_image_usage_bits
ac_image_get_usage(ac_image image);
AC_API ac_image_type
ac_image_get_type(ac_image image);
AC_API ac_image_info
ac_image_get_info(ac_image image);

AC_API ac_image
ac_swapchain_get_image(ac_swapchain swapchain);

AC_API uint64_t
ac_buffer_get_size(ac_buffer buffer);

AC_API void*
ac_buffer_get_mapped_memory(ac_buffer buffer);

AC_API ac_buffer_info
ac_buffer_get_info(ac_buffer buffer);

AC_API ac_result
ac_shader_get_workgroup(ac_shader shader, uint8_t wg[3]);

AC_API uint64_t
ac_as_get_scratch_size(ac_as as);

#if defined(__cplusplus)
}
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
