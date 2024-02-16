#pragma once

#include "ac_private.h"

#if (AC_INCLUDE_RENDERER) && (AC_PLATFORM_WINDOWS || AC_PLATFORM_XBOX)

#include "renderer_d3d12.h"

#define AC_D3D12_RIF(x)                                                        \
  do                                                                           \
  {                                                                            \
    HRESULT err = (x);                                                         \
    if (FAILED(err))                                                           \
    {                                                                          \
      AC_ERROR("[ renderer ] [ d3d12 ] hresult: %s 0x%x", #x, err);            \
      AC_ASSERT(false);                                                        \
      return ac_result_unknown_error;                                          \
    }                                                                          \
  }                                                                            \
  while (false)

#define AC_D3D12_EIF(x)                                                        \
  do                                                                           \
  {                                                                            \
    HRESULT err = (x);                                                         \
    if (FAILED(err))                                                           \
    {                                                                          \
      AC_ERROR("[ renderer ] [ d3d12 ] hresult: %s 0x%x", #x, err);            \
      AC_ASSERT(false);                                                        \
      goto end;                                                                \
    }                                                                          \
  }                                                                            \
  while (false)


#define AC_D3D12_INVALID_HANDLE (static_cast<uint64_t>(-1))
#define AC_D3D12_SAFE_RELEASE(x)                                               \
  do                                                                           \
  {                                                                            \
    if ((x))                                                                   \
    {                                                                          \
      x->Release();                                                            \
    }                                                                          \
  }                                                                            \
  while (false)

#define AC_D3D12_CONVERT_NAME(name, input)                                     \
  wchar_t name[100];                                                           \
  int32_t _len = MultiByteToWideChar(CP_UTF8, 0, input, -1, NULL, 0);          \
  MultiByteToWideChar(CP_UTF8, 0, input, -1, name, _len);

#define AC_D3D12_CPU_HEAP_SIZE 2048

static inline HRESULT
ac_d3d12_create_cpu_heap(
  ac_d3d12_device*           device,
  D3D12_DESCRIPTOR_HEAP_TYPE type,
  ac_d3d12_cpu_heap*         p)
{
  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.NumDescriptors = AC_D3D12_CPU_HEAP_SIZE;
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
  heap_desc.Type = type;

  HRESULT res = S_OK;

  res =
    device->device->CreateDescriptorHeap(&heap_desc, AC_IID_PPV_ARGS(&p->heap));

  if (res != S_OK)
  {
    return res;
  }

  p->stack_size = AC_D3D12_CPU_HEAP_SIZE;
  p->stack =
    static_cast<uint64_t*>(ac_calloc(sizeof(uint64_t) * p->stack_size));
  for (uint32_t i = 0; i < AC_D3D12_CPU_HEAP_SIZE; ++i)
  {
    p->stack[i] = i;
  }

  return res;
}

static inline void
ac_d3d12_destroy_cpu_heap(ac_d3d12_cpu_heap* p)
{
  if (!p->heap)
  {
    return;
  }

  AC_ASSERT(p->stack_size == AC_D3D12_CPU_HEAP_SIZE);
  ac_free(p->stack);
  p->heap->Release();
}

static inline uint64_t
ac_d3d12_cpu_heap_alloc_handle(ac_d3d12_cpu_heap* p)
{
  AC_ASSERT(p->stack_size > 0);
  uint64_t handle = p->stack[p->stack_size - 1];
  p->stack_size--;
  return handle;
}

static inline void
ac_d3d12_cpu_heap_free_handle(ac_d3d12_cpu_heap* p, uint64_t handle)
{
  AC_ASSERT(p->stack_size < AC_D3D12_CPU_HEAP_SIZE);
  p->stack[p->stack_size++] = handle;
}

static inline D3D12_CPU_DESCRIPTOR_HANDLE
ac_d3d12_get_cpu_handle(
  ID3D12DescriptorHeap* heap,
  uint32_t              descriptor_size,
  uint64_t              handle)
{
  return {
    heap->GetCPUDescriptorHandleForHeapStart().ptr +
    (descriptor_size * handle)};
}

static inline D3D12_GPU_DESCRIPTOR_HANDLE
ac_d3d12_get_gpu_handle(
  ID3D12DescriptorHeap* heap,
  uint32_t              descriptor_size,
  uint64_t              handle)
{
  return {
    heap->GetGPUDescriptorHandleForHeapStart().ptr +
    (descriptor_size * handle)};
}

static inline void
ac_d3d12_set_heaps(ac_d3d12_cmd* cmd, ac_d3d12_descriptor_buffer* db)
{
  if (cmd->db == db)
  {
    return;
  }

  cmd->db = db;

  switch (cmd->common.pipeline->type)
  {
  case ac_pipeline_type_graphics:
    cmd->cmd->SetGraphicsRootSignature(db->signature);
    break;
  case ac_pipeline_type_compute:
    cmd->cmd->SetComputeRootSignature(db->signature);
    break;
  default:
    AC_ASSERT(false);
    break;
  }

  uint32_t              heap_count = 0;
  ID3D12DescriptorHeap* heaps[2] = {};

  if (db->resource_heap)
  {
    heaps[heap_count++] = db->resource_heap;
  }

  if (db->sampler_heap)
  {
    heaps[heap_count++] = db->sampler_heap;
  }

  if (heap_count)
  {
    cmd->cmd->SetDescriptorHeaps(heap_count, heaps);
  }
}

static inline UINT
ac_d3d12_get_subresource_index(uint32_t mip, uint32_t layer, uint32_t mips)
{
  return (mip + (layer * mips));
}

static inline ac_result
ac_d3d12_create_fence2(
  ac_d3d12_device*     device,
  const ac_fence_info* info,
  ac_d3d12_fence*      fence)
{
  AC_UNUSED(info);
  AC_D3D12_RIF(device->device->CreateFence(
    0,
    D3D12_FENCE_FLAG_NONE,
    AC_IID_PPV_ARGS(&fence->fence)));
  fence->value = 0;
  fence->event = CreateEventW(NULL, FALSE, FALSE, NULL);

  if (fence->event == NULL)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

static inline void
ac_d3d12_destroy_fence2(ac_d3d12_fence* fence)
{
  if (fence->event)
  {
    CloseHandle(fence->event);
  }
  AC_D3D12_SAFE_RELEASE(fence->fence);
}

static inline D3D12_COMMAND_LIST_TYPE
ac_queue_type_to_d3d12(ac_queue_type type)
{
  switch (type)
  {
  case ac_queue_type_graphics:
    return D3D12_COMMAND_LIST_TYPE_DIRECT;
  case ac_queue_type_compute:
    return D3D12_COMMAND_LIST_TYPE_COMPUTE;
  case ac_queue_type_transfer:
    return D3D12_COMMAND_LIST_TYPE_COPY;
  default:
    AC_ASSERT(false);
    return static_cast<D3D12_COMMAND_LIST_TYPE>(-1);
  }
}

static inline D3D12_FILL_MODE
ac_polygon_mode_to_d3d12(ac_polygon_mode mode)
{
  switch (mode)
  {
  case ac_polygon_mode_fill:
    return D3D12_FILL_MODE_SOLID;
  case ac_polygon_mode_line:
    return D3D12_FILL_MODE_WIREFRAME;
  default:
    AC_ASSERT(false);
    return static_cast<D3D12_FILL_MODE>(-1);
  }
}

static inline D3D12_CULL_MODE
ac_cull_mode_to_d3d12(ac_cull_mode mode)
{
  switch (mode)
  {
  case ac_cull_mode_none:
    return D3D12_CULL_MODE_NONE;
  case ac_cull_mode_back:
    return D3D12_CULL_MODE_BACK;
  case ac_cull_mode_front:
    return D3D12_CULL_MODE_FRONT;
  default:
    AC_ASSERT(false);
    return static_cast<D3D12_CULL_MODE>(-1);
  }
}

static inline D3D12_PRIMITIVE_TOPOLOGY_TYPE
ac_primitive_topology_type_to_d3d12(ac_primitive_topology topology)
{
  switch (topology)
  {
  case ac_primitive_topology_point_list:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
  case ac_primitive_topology_line_list:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
  case ac_primitive_topology_line_strip:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
  case ac_primitive_topology_triangle_list:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  case ac_primitive_topology_triangle_strip:
    return D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  default:
    AC_ASSERT(false);
    return static_cast<D3D12_PRIMITIVE_TOPOLOGY_TYPE>(-1);
  }
}

static inline D3D12_PRIMITIVE_TOPOLOGY
ac_primitive_topology_to_d3d12(ac_primitive_topology topology)
{
  switch (topology)
  {
  case ac_primitive_topology_point_list:
    return D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
  case ac_primitive_topology_line_list:
    return D3D_PRIMITIVE_TOPOLOGY_LINELIST;
  case ac_primitive_topology_line_strip:
    return D3D_PRIMITIVE_TOPOLOGY_LINESTRIP;
  case ac_primitive_topology_triangle_list:
    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
  case ac_primitive_topology_triangle_strip:
    return D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
  default:
    AC_ASSERT(false);
    return static_cast<D3D12_PRIMITIVE_TOPOLOGY>(-1);
  }
}

static inline D3D12_BLEND
ac_blend_factor_to_d3d12(ac_blend_factor factor)
{
  switch (factor)
  {
  case ac_blend_factor_zero:
    return D3D12_BLEND_ZERO;
  case ac_blend_factor_one:
    return D3D12_BLEND_ONE;
  case ac_blend_factor_src_color:
    return D3D12_BLEND_SRC_COLOR;
  case ac_blend_factor_one_minus_src_color:
    return D3D12_BLEND_INV_SRC_COLOR;
  case ac_blend_factor_dst_color:
    return D3D12_BLEND_DEST_COLOR;
  case ac_blend_factor_one_minus_dst_color:
    return D3D12_BLEND_INV_DEST_COLOR;
  case ac_blend_factor_src_alpha:
    return D3D12_BLEND_SRC_ALPHA;
  case ac_blend_factor_one_minus_src_alpha:
    return D3D12_BLEND_INV_SRC_ALPHA;
  case ac_blend_factor_dst_alpha:
    return D3D12_BLEND_DEST_ALPHA;
  case ac_blend_factor_one_minus_dst_alpha:
    return D3D12_BLEND_INV_DEST_ALPHA;
  case ac_blend_factor_src_alpha_saturate:
    return D3D12_BLEND_SRC_ALPHA_SAT;
  default:
    AC_ASSERT(false);
    return static_cast<D3D12_BLEND>(-1);
  }
}

static inline D3D12_BLEND_OP
ac_blend_op_to_d3d12(ac_blend_op op)
{
  switch (op)
  {
  case ac_blend_op_add:
    return D3D12_BLEND_OP_ADD;
  case ac_blend_op_subtract:
    return D3D12_BLEND_OP_SUBTRACT;
  case ac_blend_op_reverse_subtract:
    return D3D12_BLEND_OP_REV_SUBTRACT;
  case ac_blend_op_min:
    return D3D12_BLEND_OP_MIN;
  case ac_blend_op_max:
    return D3D12_BLEND_OP_MAX;
  default:
    AC_ASSERT(false);
    return static_cast<D3D12_BLEND_OP>(-1);
  }
}

static inline D3D12_HEAP_TYPE
ac_memory_usage_to_d3d12(ac_memory_usage usage)
{
  switch (usage)
  {
  case ac_memory_usage_gpu_only:
    return D3D12_HEAP_TYPE_DEFAULT;
  case ac_memory_usage_cpu_to_gpu:
    return D3D12_HEAP_TYPE_UPLOAD;
  case ac_memory_usage_gpu_to_cpu:
    return D3D12_HEAP_TYPE_READBACK;
  default:
    AC_ASSERT(false);
    return static_cast<D3D12_HEAP_TYPE>(-1);
  }
}

static inline D3D12_FILTER
ac_filter_to_d3d12(
  ac_filter              min_filter,
  ac_filter              mag_filter,
  ac_sampler_mipmap_mode mip_map_mode,
  bool                   anisotropy,
  bool                   comparison_filter_enabled)
{
  if (anisotropy)
  {
    return (
      comparison_filter_enabled ? D3D12_FILTER_COMPARISON_ANISOTROPIC
                                : D3D12_FILTER_ANISOTROPIC);
  }

  int32_t filter = (min_filter << 4) | (mag_filter << 2) | (mip_map_mode);

  int32_t base_filter = comparison_filter_enabled
                          ? D3D12_FILTER_COMPARISON_MIN_MAG_MIP_POINT
                          : D3D12_FILTER_MIN_MAG_MIP_POINT;

  return static_cast<D3D12_FILTER>(base_filter + filter);
}

static inline D3D12_TEXTURE_ADDRESS_MODE
ac_address_mode_to_d3d12(ac_sampler_address_mode mode)
{
  switch (mode)
  {
  case ac_sampler_address_mode_repeat:
    return D3D12_TEXTURE_ADDRESS_MODE_WRAP;
  case ac_sampler_address_mode_mirrored_repeat:
    return D3D12_TEXTURE_ADDRESS_MODE_MIRROR;
  case ac_sampler_address_mode_clamp_to_edge:
    return D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
  case ac_sampler_address_mode_clamp_to_border:
    return D3D12_TEXTURE_ADDRESS_MODE_BORDER;
  default:
    AC_ASSERT(false);
    return static_cast<D3D12_TEXTURE_ADDRESS_MODE>(-1);
  }
}

static inline D3D12_COMPARISON_FUNC
ac_compare_op_to_d3d12(ac_compare_op op)
{
  switch (op)
  {
  case ac_compare_op_never:
    return D3D12_COMPARISON_FUNC_NEVER;
  case ac_compare_op_less:
    return D3D12_COMPARISON_FUNC_LESS;
  case ac_compare_op_equal:
    return D3D12_COMPARISON_FUNC_EQUAL;
  case ac_compare_op_less_or_equal:
    return D3D12_COMPARISON_FUNC_LESS_EQUAL;
  case ac_compare_op_greater:
    return D3D12_COMPARISON_FUNC_GREATER;
  case ac_compare_op_not_equal:
    return D3D12_COMPARISON_FUNC_NOT_EQUAL;
  case ac_compare_op_greater_or_equal:
    return D3D12_COMPARISON_FUNC_GREATER_EQUAL;
  case ac_compare_op_always:
    return D3D12_COMPARISON_FUNC_ALWAYS;
  default:
    AC_ASSERT(false);
    return static_cast<D3D12_COMPARISON_FUNC>(-1);
  }
}

static inline D3D12_DESCRIPTOR_RANGE_TYPE
ac_descriptor_type_to_d3d12(ac_descriptor_type type)
{
  switch (type)
  {
  case ac_descriptor_type_cbv_buffer:
    return D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
  case ac_descriptor_type_sampler:
    return D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
  case ac_descriptor_type_srv_image:
  case ac_descriptor_type_srv_buffer:
  case ac_descriptor_type_as:
    return D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
  case ac_descriptor_type_uav_image:
  case ac_descriptor_type_uav_buffer:
    return D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
  default:
    break;
  }

  AC_ASSERT(false);
  return static_cast<D3D12_DESCRIPTOR_RANGE_TYPE>(-1);
}

static inline void
ac_attribute_semantic_to_d3d12(
  ac_attribute_semantic semantic,
  const char**          name,
  UINT*                 index)
{
  *index = 0;
  switch (semantic)
  {
  case ac_attribute_semantic_position:
  {
    *name = "POSITION";
    break;
  }
  case ac_attribute_semantic_normal:
  {
    *name = "NORMAL";
    break;
  }
  case ac_attribute_semantic_color:
  {
    *name = "COLOR";
    break;
  }
  case ac_attribute_semantic_texcoord0:
  case ac_attribute_semantic_texcoord1:
  case ac_attribute_semantic_texcoord2:
  case ac_attribute_semantic_texcoord3:
  case ac_attribute_semantic_texcoord4:
  case ac_attribute_semantic_texcoord5:
  case ac_attribute_semantic_texcoord6:
  case ac_attribute_semantic_texcoord7:
  case ac_attribute_semantic_texcoord8:
  case ac_attribute_semantic_texcoord9:
  {
    *name = "TEXCOORD";
    *index = static_cast<UINT>(semantic - ac_attribute_semantic_texcoord0);
    break;
  }
  default:
  {
    AC_ASSERT(false);
    break;
  }
  }
}

#if (AC_D3D12_USE_ENHANCED_BARRIERS)
static inline D3D12_BARRIER_ACCESS
ac_access_bits_to_d3d12(ac_access_bits f)
{
  D3D12_BARRIER_ACCESS flags = static_cast<D3D12_BARRIER_ACCESS>(0);
  if (f & ac_access_indirect_command_read_bit)
  {
    flags |= D3D12_BARRIER_ACCESS_INDIRECT_ARGUMENT;
  }
  if (f & ac_access_index_read_bit)
  {
    flags |= D3D12_BARRIER_ACCESS_INDEX_BUFFER;
  }
  if (f & ac_access_vertex_attribute_read_bit)
  {
    flags |= D3D12_BARRIER_ACCESS_VERTEX_BUFFER;
  }
  if (f & ac_access_uniform_read_bit)
  {
    flags |= D3D12_BARRIER_ACCESS_CONSTANT_BUFFER;
  }
  if (f & ac_access_shader_read_bit && !(f & ac_access_shader_write_bit))
  {
    flags |= D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
  }
  if (f & ac_access_shader_write_bit)
  {
    flags |= D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
  }
  if (f & (ac_access_color_read_bit | ac_access_color_attachment_bit))
  {
    flags |= D3D12_BARRIER_ACCESS_RENDER_TARGET;
  }

  if (f & ac_access_depth_stencil_attachment_bit)
  {
    flags |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
  }

  if (f & ac_access_depth_stencil_read_bit)
  {
    flags |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
  }

  if (f & ac_access_transfer_read_bit)
  {
    flags |= D3D12_BARRIER_ACCESS_COPY_SOURCE;
  }
  if (f & ac_access_transfer_write_bit)
  {
    flags |= D3D12_BARRIER_ACCESS_COPY_DEST;
  }
  if (f & ac_access_acceleration_structure_read_bit)
  {
    flags |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_READ;
  }
  if (f & ac_access_acceleration_structure_write_bit)
  {
    flags |= D3D12_BARRIER_ACCESS_RAYTRACING_ACCELERATION_STRUCTURE_WRITE;
  }
  if (f & ac_access_resolve_bit)
  {
    flags |= D3D12_BARRIER_ACCESS_RESOLVE_DEST;
  }

  if (!flags)
  {
    flags = D3D12_BARRIER_ACCESS_NO_ACCESS;
  }

  return flags;
}

static inline D3D12_BARRIER_SYNC
ac_pipeline_stage_bits_to_d3d12(ac_pipeline_stage_bits f, bool first_scope)
{
  D3D12_BARRIER_SYNC flags = D3D12_BARRIER_SYNC_NONE;

  if (f & ac_pipeline_stage_top_of_pipe_bit)
  {
    flags |= first_scope ? D3D12_BARRIER_SYNC_NONE : D3D12_BARRIER_SYNC_ALL;
  }
  if (f & ac_pipeline_stage_draw_indirect_bit)
  {
    flags |= D3D12_BARRIER_SYNC_EXECUTE_INDIRECT;
  }
  if (f & ac_pipeline_stage_vertex_input_bit)
  {
    flags |= D3D12_BARRIER_SYNC_INDEX_INPUT;
  }
  if (f & ac_pipeline_stage_vertex_shader_bit)
  {
    flags |= D3D12_BARRIER_SYNC_VERTEX_SHADING;
  }
  if (f & ac_pipeline_stage_pixel_shader_bit)
  {
    flags |= D3D12_BARRIER_SYNC_PIXEL_SHADING;
  }
  if (
    f & (ac_pipeline_stage_early_pixel_tests_bit |
         ac_pipeline_stage_late_pixel_tests_bit))
  {
    flags |= D3D12_BARRIER_SYNC_DEPTH_STENCIL;
  }
  if (f & ac_pipeline_stage_color_attachment_output_bit)
  {
    flags |= D3D12_BARRIER_SYNC_RENDER_TARGET;
  }
  if (f & ac_pipeline_stage_compute_shader_bit)
  {
    flags |= D3D12_BARRIER_SYNC_COMPUTE_SHADING;
  }
  if (f & ac_pipeline_stage_transfer_bit)
  {
    flags |= D3D12_BARRIER_SYNC_COPY;
  }
  // FIXME: no equivalent again
  if (f & ac_pipeline_stage_bottom_of_pipe_bit)
  {
    flags |= first_scope ? D3D12_BARRIER_SYNC_ALL : D3D12_BARRIER_SYNC_NONE;
  }
  if (f & ac_pipeline_stage_all_graphics_bit)
  {
    flags |= D3D12_BARRIER_SYNC_ALL_SHADING;
  }
  if (f & ac_pipeline_stage_all_commands_bit)
  {
    flags |= D3D12_BARRIER_SYNC_ALL;
  }
  if (f & ac_pipeline_stage_acceleration_structure_build_bit)
  {
    flags |= D3D12_BARRIER_SYNC_BUILD_RAYTRACING_ACCELERATION_STRUCTURE;
  }
  if (f & ac_pipeline_stage_raytracing_shader_bit)
  {
    flags |= D3D12_BARRIER_SYNC_RAYTRACING;
  }
  if (f & ac_pipeline_stage_resolve_bit)
  {
    flags |= D3D12_BARRIER_SYNC_RESOLVE;
  }

  return flags;
}

static inline D3D12_BARRIER_LAYOUT
ac_image_layout_to_d3d12(ac_image_layout layout)
{
  switch (layout)
  {
  case ac_image_layout_undefined:
    return D3D12_BARRIER_LAYOUT_UNDEFINED;
  case ac_image_layout_general:
    return D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
  case ac_image_layout_color_write:
    return D3D12_BARRIER_LAYOUT_RENDER_TARGET;
  case ac_image_layout_depth_stencil_write:
    return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
  case ac_image_layout_depth_stencil_read:
    return D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ;
  case ac_image_layout_shader_read:
    return D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;
  case ac_image_layout_transfer_src:
    return D3D12_BARRIER_LAYOUT_COPY_SOURCE;
  case ac_image_layout_transfer_dst:
    return D3D12_BARRIER_LAYOUT_COPY_DEST;
  case ac_image_layout_present_src:
    return D3D12_BARRIER_LAYOUT_PRESENT;
  case ac_image_layout_resolve:
    return D3D12_BARRIER_LAYOUT_RESOLVE_DEST;
  default:
    break;
  }

  AC_ASSERT(false);
  return static_cast<D3D12_BARRIER_LAYOUT>(-1);
}

#endif

static inline D3D12_RESOURCE_STATES
ac_image_layout_to_d3d12_resource_states(
  ac_image_layout layout,
  ac_queue_type   type)
{
  switch (layout)
  {
  case ac_image_layout_undefined:
    return D3D12_RESOURCE_STATE_COMMON;
  case ac_image_layout_general:
    return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  case ac_image_layout_color_write:
    return D3D12_RESOURCE_STATE_RENDER_TARGET;
  case ac_image_layout_depth_stencil_write:
    return D3D12_RESOURCE_STATE_DEPTH_WRITE;
  case ac_image_layout_depth_stencil_read:
    return D3D12_RESOURCE_STATE_DEPTH_READ;
  case ac_image_layout_shader_read:
    if (type == ac_queue_type_graphics)
    {
      return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE |
             D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
    }
    else
    {
      return D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
    }
  case ac_image_layout_transfer_src:
    return D3D12_RESOURCE_STATE_COPY_SOURCE;
  case ac_image_layout_transfer_dst:
    return D3D12_RESOURCE_STATE_COPY_DEST;
  case ac_image_layout_present_src:
    return D3D12_RESOURCE_STATE_COMMON;
  case ac_image_layout_resolve:
    return D3D12_RESOURCE_STATE_RESOLVE_DEST;
  default:
    break;
  }

  AC_ASSERT(false);
  return static_cast<D3D12_RESOURCE_STATES>(-1);
}

#if (AC_D3D12_USE_RAYTRACING)
static inline D3D12_RAYTRACING_GEOMETRY_TYPE
ac_geometry_type_to_d3d12(ac_geometry_type type)
{
  switch (type)
  {
  case ac_geometry_type_triangles:
    return D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
  case ac_geometry_type_aabbs:
    return D3D12_RAYTRACING_GEOMETRY_TYPE_PROCEDURAL_PRIMITIVE_AABBS;
  default:
    break;
  }

  AC_ASSERT(false);
  return static_cast<D3D12_RAYTRACING_GEOMETRY_TYPE>(-1);
}

static inline D3D12_RAYTRACING_GEOMETRY_FLAGS
ac_geometry_bits_to_d3d12(ac_geometry_bits bits)
{
  D3D12_RAYTRACING_GEOMETRY_FLAGS flags = D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;

  if (bits & ac_geometry_opaque_bit)
  {
    flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
  }

  if (bits & ac_geometry_no_duplicate_any_hit_invocation_bit)
  {
    flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION;
  }

  return flags;
}

static inline D3D12_RAYTRACING_INSTANCE_FLAGS
ac_as_instance_bits_to_d3d12(ac_as_instance_bits bits)
{
  D3D12_RAYTRACING_INSTANCE_FLAGS flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;

  if (bits & ac_as_instance_triangle_facing_cull_disable_bit)
  {
    flags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
  }

  if (bits & ac_as_instance_triangle_flip_facing_bit)
  {
    flags |= D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_FRONT_COUNTERCLOCKWISE;
  }

  if (bits & ac_as_instance_force_opaque_bit)
  {
    flags |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_OPAQUE;
  }

  if (bits & ac_as_instance_force_no_opaque_bit)
  {
    flags |= D3D12_RAYTRACING_INSTANCE_FLAG_FORCE_NON_OPAQUE;
  }

  return flags;
}

static inline D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE
ac_as_type_to_d3d12(ac_as_type type)
{
  switch (type)
  {
  case ac_as_type_bottom_level:
    return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
  case ac_as_type_top_level:
    return D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
  default:
    break;
  }
  AC_ASSERT(false);
  return static_cast<D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE>(-1);
}

#endif

static inline DXGI_FORMAT
ac_format_to_d3d12(ac_format format)
{
  switch (format)
  {
  case ac_format_undefined:
    return DXGI_FORMAT_UNKNOWN;
  case ac_format_r5g6b5_unorm:
    return DXGI_FORMAT_B5G6R5_UNORM;
  case ac_format_r8_unorm:
    return DXGI_FORMAT_R8_UNORM;
  case ac_format_r8_snorm:
    return DXGI_FORMAT_R8_SNORM;
  case ac_format_r8_uint:
    return DXGI_FORMAT_R8_UINT;
  case ac_format_r8_sint:
    return DXGI_FORMAT_R8_SINT;
  case ac_format_r8g8_unorm:
    return DXGI_FORMAT_R8G8_UNORM;
  case ac_format_r8g8_snorm:
    return DXGI_FORMAT_R8G8_SNORM;
  case ac_format_r8g8_uint:
    return DXGI_FORMAT_R8G8_UINT;
  case ac_format_r8g8_sint:
    return DXGI_FORMAT_R8G8_SINT;
  case ac_format_r8g8b8a8_unorm:
    return DXGI_FORMAT_R8G8B8A8_UNORM;
  case ac_format_r8g8b8a8_snorm:
    return DXGI_FORMAT_R8G8B8A8_SNORM;
  case ac_format_r8g8b8a8_uint:
    return DXGI_FORMAT_R8G8B8A8_UINT;
  case ac_format_r8g8b8a8_sint:
    return DXGI_FORMAT_R8G8B8A8_SINT;
  case ac_format_r8g8b8a8_srgb:
    return DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
  case ac_format_b8g8r8a8_unorm:
    return DXGI_FORMAT_B8G8R8A8_UNORM;
  case ac_format_b8g8r8a8_srgb:
    return DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
  case ac_format_r10g10b10a2_unorm:
    return DXGI_FORMAT_R10G10B10A2_UNORM;
  case ac_format_r10g10b10a2_uint:
    return DXGI_FORMAT_R10G10B10A2_UINT;
  case ac_format_r16_unorm:
    return DXGI_FORMAT_R16_UNORM;
  case ac_format_r16_snorm:
    return DXGI_FORMAT_R16_SNORM;
  case ac_format_r16_uint:
    return DXGI_FORMAT_R16_UINT;
  case ac_format_r16_sint:
    return DXGI_FORMAT_R16_SINT;
  case ac_format_r16_sfloat:
    return DXGI_FORMAT_R16_FLOAT;
  case ac_format_r16g16_unorm:
    return DXGI_FORMAT_R16G16_UNORM;
  case ac_format_r16g16_snorm:
    return DXGI_FORMAT_R16G16_SNORM;
  case ac_format_r16g16_uint:
    return DXGI_FORMAT_R16G16_UINT;
  case ac_format_r16g16_sint:
    return DXGI_FORMAT_R16G16_SINT;
  case ac_format_r16g16_sfloat:
    return DXGI_FORMAT_R16G16_FLOAT;
  case ac_format_r16g16b16a16_unorm:
    return DXGI_FORMAT_R16G16B16A16_UNORM;
  case ac_format_r16g16b16a16_snorm:
    return DXGI_FORMAT_R16G16B16A16_SNORM;
  case ac_format_r16g16b16a16_uint:
    return DXGI_FORMAT_R16G16B16A16_UINT;
  case ac_format_r16g16b16a16_sint:
    return DXGI_FORMAT_R16G16B16A16_SINT;
  case ac_format_r16g16b16a16_sfloat:
    return DXGI_FORMAT_R16G16B16A16_FLOAT;
  case ac_format_r32_uint:
    return DXGI_FORMAT_R32_UINT;
  case ac_format_r32_sint:
    return DXGI_FORMAT_R32_SINT;
  case ac_format_r32_sfloat:
    return DXGI_FORMAT_R32_FLOAT;
  case ac_format_r32g32_uint:
    return DXGI_FORMAT_R32G32_UINT;
  case ac_format_r32g32_sint:
    return DXGI_FORMAT_R32G32_SINT;
  case ac_format_r32g32_sfloat:
    return DXGI_FORMAT_R32G32_FLOAT;
  case ac_format_r32g32b32_uint:
    return DXGI_FORMAT_R32G32B32_UINT;
  case ac_format_r32g32b32_sint:
    return DXGI_FORMAT_R32G32B32_SINT;
  case ac_format_r32g32b32_sfloat:
    return DXGI_FORMAT_R32G32B32_FLOAT;
  case ac_format_r32g32b32a32_uint:
    return DXGI_FORMAT_R32G32B32A32_UINT;
  case ac_format_r32g32b32a32_sint:
    return DXGI_FORMAT_R32G32B32A32_SINT;
  case ac_format_r32g32b32a32_sfloat:
    return DXGI_FORMAT_R32G32B32A32_FLOAT;
  case ac_format_e5b9g9r9_ufloat:
    return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
  case ac_format_d16_unorm:
    return DXGI_FORMAT_R16_TYPELESS;
  case ac_format_d32_sfloat:
    return DXGI_FORMAT_R32_TYPELESS;
  default:
    break;
  }
  return DXGI_FORMAT_UNKNOWN;
}

static inline DXGI_FORMAT
ac_format_to_d3d12_attachment(ac_format format)
{
  switch (format)
  {
  case ac_format_d16_unorm:
    return DXGI_FORMAT_D16_UNORM;
  case ac_format_d32_sfloat:
    return DXGI_FORMAT_D32_FLOAT;
  default:
    break;
  }

  return ac_format_to_d3d12(format);
}

static inline DXGI_FORMAT
ac_format_to_d3d12_srv(ac_format format)
{
  switch (format)
  {
  case ac_format_d16_unorm:
    return DXGI_FORMAT_R16_UNORM;
  case ac_format_d32_sfloat:
    return DXGI_FORMAT_R32_FLOAT;
  default:
    break;
  }

  return ac_format_to_d3d12(format);
}

#endif
