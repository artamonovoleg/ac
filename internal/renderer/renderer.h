#pragma once

#include "ac_private.h"

#if (AC_INCLUDE_RENDERER)

#include <ac_shader_compiler/ac_shader_compiler.h>

#if defined(__cplusplus)
extern "C"
{
#endif

typedef struct ac_queue_internal {
  ac_device     device;
  ac_queue_type type;
  ac_mutex      mtx;
} ac_queue_internal;

typedef struct ac_cmd_pool_internal {
  ac_queue queue;
} ac_cmd_pool_internal;

typedef struct ac_cmd_internal {
  ac_device            device;
  ac_cmd_pool          pool;
  ac_pipeline          pipeline;
  ac_descriptor_buffer db;
  uint32_t             sets[ac_space_count];
} ac_cmd_internal;

typedef struct ac_fence_internal {
  ac_device     device;
  ac_fence_bits bits;
} ac_fence_internal;

typedef struct ac_sampler_internal {
  ac_device device;
} ac_sampler_internal;

typedef struct ac_image_internal {
  ac_device           device;
  uint32_t            width;
  uint32_t            height;
  ac_format           format;
  uint8_t             samples;
  uint16_t            levels;
  uint16_t            layers;
  ac_image_usage_bits usage;
  ac_image_type       type;
} ac_image_internal;

typedef struct ac_buffer_internal {
  ac_device            device;
  uint64_t             size;
  ac_buffer_usage_bits usage;
  ac_memory_usage      memory_usage;
  void*                mapped_memory;
} ac_buffer_internal;

typedef struct ac_swapchain_internal {
  ac_device      device;
  uint32_t       image_count;
  ac_image*      images;
  ac_queue       queue;
  bool           vsync;
  uint32_t       image_index;
  ac_color_space color_space;
} ac_swapchain_internal;

typedef struct ac_shader_internal {
  ac_device       device;
  ac_shader_stage stage;
  void*           reflection;
} ac_shader_internal;

typedef struct ac_dsl_internal {
  ac_device          device;
  uint32_t           binding_count;
  ac_shader_binding* bindings;
} ac_dsl_internal;

typedef struct ac_descriptor_buffer_internal {
  ac_device device;
} ac_descriptor_buffer_internal;

typedef struct ac_pipeline_internal {
  ac_device        device;
  ac_pipeline_type type;
} ac_pipeline_internal;

typedef struct ac_sbt_internal {
  ac_device device;
} ac_sbt_internal;

typedef struct ac_as_internal {
  ac_device  device;
  ac_as_type type;
  uint64_t   scratch_size;
} ac_as_internal;

typedef struct ac_dsl_info_internal {
  ac_dsl_info        info;
  uint32_t           binding_count;
  ac_shader_binding* bindings;
} ac_dsl_info_internal;

typedef struct ac_device_internal {
  ac_device_debug_bits debug_bits;
  ac_device_properties props;
  bool                 support_raytracing;
  bool                 support_mesh_shaders;
  uint32_t             queue_map[ac_queue_type_count];
  uint32_t             queue_count;
  ac_queue             queues[ac_queue_type_count];

  void (*destroy_device)(ac_device);

  ac_result (*queue_wait_idle)(ac_queue);

  ac_result (*queue_submit)(ac_queue, const ac_queue_submit_info*);

  ac_result (*queue_present)(ac_queue, const ac_queue_present_info*);

  ac_result (*create_fence)(ac_device, const ac_fence_info*, ac_fence*);

  void (*destroy_fence)(ac_device, ac_fence);

  ac_result (*get_fence_value)(ac_device, ac_fence, uint64_t*);

  ac_result (*signal_fence)(ac_device, ac_fence, uint64_t);

  ac_result (*wait_fence)(ac_device, ac_fence, uint64_t);

  ac_result (
    *create_swapchain)(ac_device, const ac_swapchain_info*, ac_swapchain*);

  void (*destroy_swapchain)(ac_device, ac_swapchain);

  ac_result (
    *create_cmd_pool)(ac_device, const ac_cmd_pool_info*, ac_cmd_pool*);

  void (*destroy_cmd_pool)(ac_device, ac_cmd_pool);

  ac_result (*reset_cmd_pool)(ac_device device, ac_cmd_pool pool);

  ac_result (*create_cmd)(ac_device, ac_cmd_pool, ac_cmd*);

  void (*destroy_cmd)(ac_device, ac_cmd_pool, ac_cmd);

  ac_result (*begin_cmd)(ac_cmd);

  ac_result (*end_cmd)(ac_cmd);

  ac_result (*acquire_next_image)(ac_device, ac_swapchain, ac_fence);

  ac_result (*create_shader)(ac_device, const ac_shader_info*, ac_shader*);

  void (*destroy_shader)(ac_device, ac_shader);

  ac_result (*create_dsl)(ac_device, const ac_dsl_info_internal*, ac_dsl*);

  void (*destroy_dsl)(ac_device, ac_dsl);

  ac_result (*create_descriptor_buffer)(
    ac_device,
    const ac_descriptor_buffer_info*,
    ac_descriptor_buffer*);

  void (*destroy_descriptor_buffer)(ac_device, ac_descriptor_buffer);

  ac_result (*create_graphics_pipeline)(
    ac_device,
    const ac_pipeline_info*,
    ac_pipeline*);

  ac_result (
    *create_mesh_pipeline)(ac_device, const ac_pipeline_info*, ac_pipeline*);

  ac_result (
    *create_compute_pipeline)(ac_device, const ac_pipeline_info*, ac_pipeline*);

  ac_result (*create_raytracing_pipeline)(
    ac_device,
    const ac_pipeline_info*,
    ac_pipeline*);

  void (*destroy_pipeline)(ac_device, ac_pipeline);

  ac_result (*create_buffer)(ac_device, const ac_buffer_info*, ac_buffer*);

  void (*destroy_buffer)(ac_device, ac_buffer);

  ac_result (*map_memory)(ac_device, ac_buffer);

  void (*unmap_memory)(ac_device, ac_buffer);

  ac_result (*create_sampler)(ac_device, const ac_sampler_info*, ac_sampler*);

  void (*destroy_sampler)(ac_device, ac_sampler);

  ac_result (*create_image)(ac_device, const ac_image_info*, ac_image*);

  void (*destroy_image)(ac_device, ac_image);

  void (*update_descriptor)(
    ac_device,
    ac_descriptor_buffer,
    ac_space,
    uint32_t,
    const ac_descriptor_write*);

  ac_result (*create_sbt)(ac_device, const ac_sbt_info*, ac_sbt*);

  void (*destroy_sbt)(ac_device, ac_sbt);

  ac_result (*create_blas)(ac_device, const ac_as_info*, ac_as*);
  ac_result (*create_tlas)(ac_device, const ac_as_info*, ac_as*);

  void (*destroy_as)(ac_device, ac_as);

  void (*write_as_instances)(ac_device, uint32_t, const ac_as_instance*, void*);

  void (*cmd_begin_rendering)(ac_cmd, const ac_rendering_info*);

  void (*cmd_end_rendering)(ac_cmd);

  void (*cmd_barrier)(
    ac_cmd,
    uint32_t,
    const ac_buffer_barrier*,
    uint32_t,
    const ac_image_barrier*);

  void (*cmd_set_scissor)(ac_cmd, uint32_t, uint32_t, uint32_t, uint32_t);

  void (*cmd_set_viewport)(ac_cmd, float, float, float, float, float, float);

  void (*cmd_bind_pipeline)(ac_cmd, ac_pipeline);

  void (*cmd_draw_mesh_tasks)(ac_cmd, uint32_t, uint32_t, uint32_t);

  void (*cmd_draw)(ac_cmd, uint32_t, uint32_t, uint32_t, uint32_t);

  void (
    *cmd_draw_indexed)(ac_cmd, uint32_t, uint32_t, uint32_t, int32_t, uint32_t);

  void (*cmd_bind_vertex_buffer)(ac_cmd, uint32_t, ac_buffer, uint64_t);

  void (*cmd_bind_index_buffer)(ac_cmd, ac_buffer, uint64_t, ac_index_type);

  void (*cmd_copy_buffer)(
    ac_cmd,
    ac_buffer,
    uint64_t,
    ac_buffer,
    uint64_t,
    uint64_t);

  void (*cmd_copy_buffer_to_image)(
    ac_cmd,
    ac_buffer,
    ac_image,
    const ac_buffer_image_copy*);

  void (*cmd_copy_image)(ac_cmd, ac_image, ac_image, const ac_image_copy*);

  void (*cmd_copy_image_to_buffer)(
    ac_cmd,
    ac_image,
    ac_buffer,
    const ac_buffer_image_copy*);

  void (*cmd_bind_set)(ac_cmd, ac_descriptor_buffer, ac_space, uint32_t);

  void (*cmd_dispatch)(ac_cmd, uint32_t, uint32_t, uint32_t);

  void (*cmd_push_constants)(ac_cmd, uint32_t, const void*);

  void (*cmd_trace_rays)(ac_cmd, ac_sbt, uint32_t, uint32_t, uint32_t);

  void (*cmd_build_as)(ac_cmd, ac_as_build_info*);

  void (*cmd_begin_debug_label)(ac_cmd, const char*, const float[4]);

  void (*cmd_end_debug_label)(ac_cmd);

  void (*cmd_insert_debug_label)(ac_cmd, const char*, const float[4]);

  void (*begin_capture)(ac_device);

  void (*end_capture)(ac_device);
} ac_device_internal;

#if (AC_INCLUDE_VULKAN)
ac_result
ac_vk_create_device(const ac_device_info* info, ac_device* device_handle);
#endif

#if (AC_PLATFORM_APPLE)
ac_result
ac_mtl_create_device(const ac_device_info* info, ac_device* device_handle);
#endif

#if (AC_PLATFORM_WINDOWS || AC_PLATFORM_XBOX)
ac_result
ac_d3d12_create_device(const ac_device_info* info, ac_device* device_handle);
#endif

#if (AC_PLATFORM_PS4)
ac_result
ac_ps4_create_device(const ac_device_info* info, ac_device* device_handle);
#endif

#if (AC_PLATFORM_PS5)
ac_result
ac_ps5_create_device(const ac_device_info* info, ac_device* device_handle);
#endif

static inline uint32_t
ac_get_shader_stage_bit(ac_shader_stage stage)
{
  return (1u << stage);
}

static inline uint32_t
ac_format_size_bytes_internal(ac_format format)
{
  switch (format)
  {
  case ac_format_undefined:
    return 0;
  case ac_format_r8_unorm:
    return 1;
  case ac_format_r8_snorm:
    return 1;
  case ac_format_r8_uint:
    return 1;
  case ac_format_r8_sint:
    return 1;
  case ac_format_r8_srgb:
    return 1;
  case ac_format_r5g6b5_unorm:
    return 2;
  case ac_format_r8g8_unorm:
    return 2;
  case ac_format_r8g8_snorm:
    return 2;
  case ac_format_r8g8_uint:
    return 2;
  case ac_format_r8g8_sint:
    return 2;
  case ac_format_r8g8_srgb:
    return 2;
  case ac_format_r16_unorm:
    return 2;
  case ac_format_r16_snorm:
    return 2;
  case ac_format_r16_uint:
    return 2;
  case ac_format_r16_sint:
    return 2;
  case ac_format_r16_sfloat:
    return 2;
  case ac_format_r16g16b16a16_unorm:
    return 8;
  case ac_format_r16g16b16a16_snorm:
    return 8;
  case ac_format_r16g16b16a16_uint:
    return 8;
  case ac_format_r16g16b16a16_sint:
    return 8;
  case ac_format_r16g16b16a16_sfloat:
    return 8;
  case ac_format_r32g32_uint:
    return 8;
  case ac_format_r32g32_sint:
    return 8;
  case ac_format_r32g32_sfloat:
    return 8;
  case ac_format_r32g32b32_uint:
    return 12;
  case ac_format_r32g32b32_sint:
    return 12;
  case ac_format_r32g32b32_sfloat:
    return 12;
  case ac_format_r32g32b32a32_uint:
    return 16;
  case ac_format_r32g32b32a32_sint:
    return 16;
  case ac_format_r32g32b32a32_sfloat:
    return 16;
  case ac_format_d16_unorm:
    return 2;
  case ac_format_s8_uint:
    return 1;
  default:
    return 4;
  }
}

static inline bool
ac_format_has_depth_aspect_internal(ac_format format)
{
  switch (format)
  {
  case ac_format_d16_unorm:
  case ac_format_d32_sfloat:
    return true;
  default:
    return false;
  }
}

static inline bool
ac_format_depth_or_stencil(ac_format format)
{
  switch (format)
  {
  case ac_format_d16_unorm:
  case ac_format_d32_sfloat:
  case ac_format_s8_uint:
    return true;
  default:
    break;
  }

  return false;
}

static inline ac_format
ac_format_to_unorm_internal(ac_format format)
{
#define srgb_case(fmt)                                                         \
  case ac_format_##fmt##_srgb:                                                 \
    return ac_format_##fmt##_unorm

  switch (format)
  {
    srgb_case(r8);
    srgb_case(r8g8);
    srgb_case(r8g8b8a8);
    srgb_case(b8g8r8a8);
  default:
    break;
  }
#undef srgb_case

  return format;
}

static inline bool
ac_format_is_srgb_internal(ac_format format)
{
  return ac_format_to_unorm_internal(format) != format;
}

static inline uint32_t
ac_format_channel_count_internal(ac_format format)
{
  uint32_t cnt = 4;

  switch (format)
  {
  case ac_format_r8_unorm:
    return 1;
  case ac_format_r8_snorm:
    return 1;
  case ac_format_r8_uint:
    return 1;
  case ac_format_r8_sint:
    return 1;
  case ac_format_r8_srgb:
    return 1;
  case ac_format_r5g6b5_unorm:
    return 3;
  case ac_format_r8g8_unorm:
    return 2;
  case ac_format_r8g8_snorm:
    return 2;
  case ac_format_r8g8_uint:
    return 2;
  case ac_format_r8g8_sint:
    return 2;
  case ac_format_r8g8_srgb:
    return 2;
  case ac_format_r16_unorm:
    return 1;
  case ac_format_r16_snorm:
    return 1;
  case ac_format_r16_uint:
    return 1;
  case ac_format_r16_sint:
    return 1;
  case ac_format_r16_sfloat:
    return 1;
  case ac_format_r16g16_unorm:
    return 2;
  case ac_format_r16g16_snorm:
    return 2;
  case ac_format_r16g16_uint:
    return 2;
  case ac_format_r16g16_sint:
    return 2;
  case ac_format_r16g16_sfloat:
    return 2;
  case ac_format_r32_uint:
    return 1;
  case ac_format_r32_sint:
    return 1;
  case ac_format_r32_sfloat:
    return 1;
  case ac_format_r32g32_uint:
    return 2;
  case ac_format_r32g32_sint:
    return 2;
  case ac_format_r32g32_sfloat:
    return 2;
  case ac_format_r32g32b32_uint:
    return 3;
  case ac_format_r32g32b32_sint:
    return 3;
  case ac_format_r32g32b32_sfloat:
    return 3;
  case ac_format_d16_unorm:
    return 1;
  case ac_format_d32_sfloat:
    return 1;
  case ac_format_s8_uint:
    return 1;
  default:
    break;
  }

  return cnt;
}

static inline bool
ac_format_is_floating_point(ac_format format)
{
  switch (format)
  {
  case ac_format_r8_unorm:
  case ac_format_r8_snorm:
  case ac_format_r8_srgb:
  case ac_format_r5g6b5_unorm:
  case ac_format_r8g8_unorm:
  case ac_format_r8g8_snorm:
  case ac_format_r16_unorm:
  case ac_format_r16_snorm:
  case ac_format_r16_sfloat:
  case ac_format_r8g8b8a8_unorm:
  case ac_format_r8g8b8a8_srgb:
  case ac_format_r16g16_sfloat:
  case ac_format_r32_sfloat:
  case ac_format_r10g10b10a2_unorm:
  case ac_format_b10g10r10a2_unorm:
  case ac_format_e5b9g9r9_ufloat:
  case ac_format_r16g16b16a16_unorm:
  case ac_format_r16g16b16a16_snorm:
  case ac_format_r16g16b16a16_sfloat:
  case ac_format_r32g32_sfloat:
  case ac_format_r32g32b32_sfloat:
  case ac_format_r32g32b32a32_sfloat:
  case ac_format_d16_unorm:
  case ac_format_d32_sfloat:
  case ac_format_b8g8r8a8_unorm:
  case ac_format_b8g8r8a8_srgb:
  case ac_format_r16g16_unorm:
  case ac_format_r16g16_snorm:
  case ac_format_r8g8b8a8_snorm:
    return true;
  case ac_format_r8_uint:
  case ac_format_r8_sint:
  case ac_format_r8g8_uint:
  case ac_format_r8g8_sint:
  case ac_format_r8g8_srgb:
  case ac_format_r16_uint:
  case ac_format_r16_sint:
  case ac_format_r8g8b8a8_uint:
  case ac_format_r8g8b8a8_sint:
  case ac_format_r16g16_uint:
  case ac_format_r16g16_sint:
  case ac_format_r32_uint:
  case ac_format_r32_sint:
  case ac_format_r10g10b10a2_uint:
  case ac_format_b10g10r10a2_uint:
  case ac_format_r16g16b16a16_uint:
  case ac_format_r16g16b16a16_sint:
  case ac_format_r32g32_uint:
  case ac_format_r32g32_sint:
  case ac_format_r32g32b32_uint:
  case ac_format_r32g32b32_sint:
  case ac_format_r32g32b32a32_uint:
  case ac_format_r32g32b32a32_sint:
  case ac_format_s8_uint:
    return false;

  case ac_format_undefined:
    break;

  default:
    break;
  }

  AC_ASSERT(false);
  return false;
}

#if defined(__cplusplus)
}
#endif

#endif
