#pragma once

#include "ac_private.h"

#if (AC_INCLUDE_VULKAN)

#include "renderer_vulkan.h"

#define AC_VK_RIF(x)                                                           \
  do                                                                           \
  {                                                                            \
    VkResult vk_error = (x);                                                   \
    if (vk_error != VK_SUCCESS)                                                \
    {                                                                          \
      AC_ERROR("[ renderer ] [ vulkan ]: %s %d", #x, vk_error);                \
      return ac_result_unknown_error;                                          \
    }                                                                          \
  }                                                                            \
  while (0)

static inline VkSampleCountFlagBits
ac_samples_to_vk(uint32_t samples)
{
  return (VkSampleCountFlagBits)samples;
}

static inline VkAttachmentLoadOp
ac_attachment_load_op_to_vk(ac_attachment_load_op load_op)
{
  switch (load_op)
  {
  case ac_attachment_load_op_clear:
    return VK_ATTACHMENT_LOAD_OP_CLEAR;
  case ac_attachment_load_op_dont_care:
    return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  case ac_attachment_load_op_load:
    return VK_ATTACHMENT_LOAD_OP_LOAD;
  default:
    AC_ASSERT(false);
    return (VkAttachmentLoadOp)-1;
  }
}

static inline VkAttachmentStoreOp
ac_attachment_store_op_to_vk(ac_attachment_store_op store_op)
{
  switch (store_op)
  {
  case ac_attachment_store_op_none:
    return VK_ATTACHMENT_STORE_OP_NONE;
  case ac_attachment_store_op_store:
    return VK_ATTACHMENT_STORE_OP_STORE;
  case ac_attachment_store_op_dont_care:
    return VK_ATTACHMENT_STORE_OP_DONT_CARE;
  default:
    AC_ASSERT(false);
    return (VkAttachmentStoreOp)-1;
  }
}

static inline VkQueueFlagBits
ac_queue_type_to_vk(ac_queue_type type)
{
  switch (type)
  {
  case ac_queue_type_graphics:
    return VK_QUEUE_GRAPHICS_BIT;
  case ac_queue_type_compute:
    return VK_QUEUE_COMPUTE_BIT;
  case ac_queue_type_transfer:
    return VK_QUEUE_TRANSFER_BIT;
  default:
    AC_ASSERT(false);
    return (VkQueueFlagBits)-1;
  }
}

static inline VkVertexInputRate
ac_input_rate_to_vk(ac_input_rate input_rate)
{
  switch (input_rate)
  {
  case ac_input_rate_vertex:
    return VK_VERTEX_INPUT_RATE_VERTEX;
  case ac_input_rate_instance:
    return VK_VERTEX_INPUT_RATE_INSTANCE;
  default:
    AC_ASSERT(false);
    return (VkVertexInputRate)-1;
  }
}

static inline VkIndexType
ac_index_type_to_vk(ac_index_type index_type)
{
  switch (index_type)
  {
  case ac_index_type_u16:
    return VK_INDEX_TYPE_UINT16;
  case ac_index_type_u32:
    return VK_INDEX_TYPE_UINT32;
  default:
    AC_ASSERT(false);
    return (VkIndexType)-1;
  }
}

static inline VkCullModeFlags
ac_cull_mode_to_vk(ac_cull_mode cull_mode)
{
  switch (cull_mode)
  {
  case ac_cull_mode_back:
    return VK_CULL_MODE_BACK_BIT;
  case ac_cull_mode_front:
    return VK_CULL_MODE_FRONT_BIT;
  case ac_cull_mode_none:
    return VK_CULL_MODE_NONE;
  default:
    AC_ASSERT(false);
    return (VkCullModeFlags)-1;
  }
}

static inline VkFrontFace
ac_front_face_to_vk(ac_front_face front_face)
{
  switch (front_face)
  {
  case ac_front_face_clockwise:
    return VK_FRONT_FACE_CLOCKWISE;
  case ac_front_face_counter_clockwise:
    return VK_FRONT_FACE_COUNTER_CLOCKWISE;
  default:
    AC_ASSERT(false);
    return (VkFrontFace)-1;
  }
}

static inline VkCompareOp
ac_compare_op_to_vk(ac_compare_op op)
{
  switch (op)
  {
  case ac_compare_op_never:
    return VK_COMPARE_OP_NEVER;
  case ac_compare_op_less:
    return VK_COMPARE_OP_LESS;
  case ac_compare_op_equal:
    return VK_COMPARE_OP_EQUAL;
  case ac_compare_op_less_or_equal:
    return VK_COMPARE_OP_LESS_OR_EQUAL;
  case ac_compare_op_greater:
    return VK_COMPARE_OP_GREATER;
  case ac_compare_op_not_equal:
    return VK_COMPARE_OP_NOT_EQUAL;
  case ac_compare_op_greater_or_equal:
    return VK_COMPARE_OP_GREATER_OR_EQUAL;
  case ac_compare_op_always:
    return VK_COMPARE_OP_ALWAYS;
  default:
    AC_ASSERT(false);
    return (VkCompareOp)-1;
  }
}

static inline VkShaderStageFlagBits
ac_shader_stage_to_vk(ac_shader_stage shader_stage)
{
  switch (shader_stage)
  {
  case ac_shader_stage_vertex:
    return VK_SHADER_STAGE_VERTEX_BIT;
  case ac_shader_stage_pixel:
    return VK_SHADER_STAGE_FRAGMENT_BIT;
  case ac_shader_stage_compute:
    return VK_SHADER_STAGE_COMPUTE_BIT;
  case ac_shader_stage_raygen:
    return VK_SHADER_STAGE_RAYGEN_BIT_KHR;
  case ac_shader_stage_any_hit:
    return VK_SHADER_STAGE_ANY_HIT_BIT_KHR;
  case ac_shader_stage_closest_hit:
    return VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR;
  case ac_shader_stage_miss:
    return VK_SHADER_STAGE_MISS_BIT_KHR;
  case ac_shader_stage_intersection:
    return VK_SHADER_STAGE_INTERSECTION_BIT_KHR;
  case ac_shader_stage_amplification:
    return VK_SHADER_STAGE_TASK_BIT_EXT;
  case ac_shader_stage_mesh:
    return VK_SHADER_STAGE_MESH_BIT_EXT;
  default:
    AC_ASSERT(false);
    return (VkShaderStageFlagBits)-1;
  }
}

static inline VkPipelineBindPoint
ac_pipeline_type_to_vk_bind_point(ac_pipeline_type type)
{
  switch (type)
  {
  case ac_pipeline_type_compute:
    return VK_PIPELINE_BIND_POINT_COMPUTE;
  case ac_pipeline_type_graphics:
  case ac_pipeline_type_mesh:
    return VK_PIPELINE_BIND_POINT_GRAPHICS;
  case ac_pipeline_type_raytracing:
    return VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
  default:
    AC_ASSERT(false);
    return (VkPipelineBindPoint)-1;
  }
}

static inline VkDescriptorType
ac_descriptor_type_to_vk(ac_descriptor_type type)
{
  switch (type)
  {
  case ac_descriptor_type_sampler:
    return VK_DESCRIPTOR_TYPE_SAMPLER;
  case ac_descriptor_type_srv_image:
    return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
  case ac_descriptor_type_uav_image:
    return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
  case ac_descriptor_type_cbv_buffer:
    return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
  case ac_descriptor_type_srv_buffer:
  case ac_descriptor_type_uav_buffer:
    return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  case ac_descriptor_type_as:
    return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
  default:
    AC_ASSERT(false);
    return (VkDescriptorType)-1;
  }
}

static inline VkFilter
ac_filter_to_vk(ac_filter filter)
{
  switch (filter)
  {
  case ac_filter_linear:
    return VK_FILTER_LINEAR;
  case ac_filter_nearest:
    return VK_FILTER_NEAREST;
  default:
    AC_ASSERT(false);
    return (VkFilter)-1;
  }
}

static inline VkSamplerMipmapMode
ac_sampler_mipmap_mode_to_vk(ac_sampler_mipmap_mode mode)
{
  switch (mode)
  {
  case ac_sampler_mipmap_mode_nearest:
    return VK_SAMPLER_MIPMAP_MODE_NEAREST;
  case ac_sampler_mipmap_mode_linear:
    return VK_SAMPLER_MIPMAP_MODE_LINEAR;
  default:
    AC_ASSERT(false);
    return (VkSamplerMipmapMode)-1;
  }
}

static inline VkSamplerAddressMode
ac_sampler_address_mode_to_vk(ac_sampler_address_mode mode)
{
  switch (mode)
  {
  case ac_sampler_address_mode_repeat:
    return VK_SAMPLER_ADDRESS_MODE_REPEAT;
  case ac_sampler_address_mode_mirrored_repeat:
    return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
  case ac_sampler_address_mode_clamp_to_edge:
    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
  case ac_sampler_address_mode_clamp_to_border:
    return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
  default:
    AC_ASSERT(false);
    return (VkSamplerAddressMode)-1;
  }
}

static inline VkPolygonMode
ac_polygon_mode_to_vk(ac_polygon_mode mode)
{
  switch (mode)
  {
  case ac_polygon_mode_fill:
    return VK_POLYGON_MODE_FILL;
  case ac_polygon_mode_line:
    return VK_POLYGON_MODE_LINE;
  default:
    AC_ASSERT(false);
    return (VkPolygonMode)-1;
  }
}

static inline VkPrimitiveTopology
ac_primitive_topology_to_vk(ac_primitive_topology topology)
{
  switch (topology)
  {
  case ac_primitive_topology_line_list:
    return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
  case ac_primitive_topology_line_strip:
    return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
  case ac_primitive_topology_point_list:
    return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  case ac_primitive_topology_triangle_strip:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
  case ac_primitive_topology_triangle_list:
    return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  default:
    AC_ASSERT(false);
    return (VkPrimitiveTopology)-1;
  }
}

static inline VkBlendFactor
ac_blend_factor_to_vk(ac_blend_factor factor)
{
  switch (factor)
  {
  case ac_blend_factor_zero:
    return VK_BLEND_FACTOR_ZERO;
  case ac_blend_factor_one:
    return VK_BLEND_FACTOR_ONE;
  case ac_blend_factor_src_color:
    return VK_BLEND_FACTOR_SRC_COLOR;
  case ac_blend_factor_one_minus_src_color:
    return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
  case ac_blend_factor_dst_color:
    return VK_BLEND_FACTOR_DST_COLOR;
  case ac_blend_factor_one_minus_dst_color:
    return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
  case ac_blend_factor_src_alpha:
    return VK_BLEND_FACTOR_SRC_ALPHA;
  case ac_blend_factor_one_minus_src_alpha:
    return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
  case ac_blend_factor_dst_alpha:
    return VK_BLEND_FACTOR_DST_ALPHA;
  case ac_blend_factor_one_minus_dst_alpha:
    return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
  case ac_blend_factor_src_alpha_saturate:
    return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
  default:
    AC_ASSERT(false);
    return (VkBlendFactor)-1;
  }
}

static inline VkBlendOp
ac_blend_op_to_vk(ac_blend_op op)
{
  switch (op)
  {
  case ac_blend_op_add:
    return VK_BLEND_OP_ADD;
  case ac_blend_op_subtract:
    return VK_BLEND_OP_SUBTRACT;
  case ac_blend_op_reverse_subtract:
    return VK_BLEND_OP_REVERSE_SUBTRACT;
  case ac_blend_op_min:
    return VK_BLEND_OP_MIN;
  case ac_blend_op_max:
    return VK_BLEND_OP_MAX;
  default:
    AC_ASSERT(false);
    return (VkBlendOp)-1;
  }
}

static inline VmaMemoryUsage
ac_determine_vma_memory_usage(ac_memory_usage usage)
{
  switch (usage)
  {
  case ac_memory_usage_gpu_only:
    return VMA_MEMORY_USAGE_GPU_ONLY;
  case ac_memory_usage_cpu_to_gpu:
    return VMA_MEMORY_USAGE_CPU_TO_GPU;
  case ac_memory_usage_gpu_to_cpu:
    return VMA_MEMORY_USAGE_GPU_TO_CPU;
  default:
    break;
  }
  AC_ASSERT(false);
  return (VmaMemoryUsage)-1;
}

static inline VkBufferUsageFlags
ac_buffer_usage_bits_to_vk(ac_buffer_usage_bits bits)
{
  VkBufferUsageFlags buffer_usage = 0;

  if (bits & ac_buffer_usage_transfer_src_bit)
  {
    buffer_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
  }

  if (bits & ac_buffer_usage_transfer_dst_bit)
  {
    buffer_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
  }

  if (bits & ac_buffer_usage_vertex_bit)
  {
    buffer_usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
  }

  if (bits & ac_buffer_usage_index_bit)
  {
    buffer_usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
  }

  if (bits & ac_buffer_usage_cbv_bit)
  {
    buffer_usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
  }

  if (bits & (ac_buffer_usage_srv_bit | ac_buffer_usage_uav_bit))
  {
    buffer_usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
  }

  if (bits & ac_buffer_usage_raytracing_bit)
  {
    buffer_usage |=
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT |
      VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
  }

  return buffer_usage;
}

static inline VkImageUsageFlags
ac_image_usage_bits_to_vk(ac_image_usage_bits bits, ac_format format)
{
  VkImageUsageFlags image_usage = (VkImageUsageFlags)0;

  if (bits & ac_image_usage_transfer_src_bit)
  {
    image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
  }

  if (bits & ac_image_usage_transfer_dst_bit)
  {
    image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  }

  if (bits & ac_image_usage_srv_bit)
  {
    image_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
  }

  if (bits & ac_image_usage_uav_bit)
  {
    image_usage |= VK_IMAGE_USAGE_STORAGE_BIT;
  }

  if (bits & ac_image_usage_attachment_bit)
  {
    if (ac_format_depth_or_stencil(format))
    {
      image_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }
    else
    {
      image_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }
  }

  return image_usage;
}

static inline VkAccessFlags
ac_access_bits_to_vk(ac_access_bits f)
{
  VkAccessFlags flags = VK_ACCESS_NONE;
  if (f & ac_access_indirect_command_read_bit)
  {
    flags |= VK_ACCESS_INDIRECT_COMMAND_READ_BIT;
  }
  if (f & ac_access_index_read_bit)
  {
    flags |= VK_ACCESS_INDEX_READ_BIT;
  }
  if (f & ac_access_vertex_attribute_read_bit)
  {
    flags |= VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
  }
  if (f & ac_access_uniform_read_bit)
  {
    flags |= VK_ACCESS_UNIFORM_READ_BIT;
  }
  if (f & ac_access_shader_read_bit)
  {
    flags |= VK_ACCESS_SHADER_READ_BIT;
  }
  if (f & ac_access_shader_write_bit)
  {
    flags |= VK_ACCESS_SHADER_WRITE_BIT;
  }
  if (f & ac_access_color_read_bit)
  {
    flags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
  }
  if (f & (ac_access_color_attachment_bit | ac_access_resolve_bit))
  {
    flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  }
  if (f & ac_access_depth_stencil_read_bit)
  {
    flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
  }
  if (f & ac_access_depth_stencil_attachment_bit)
  {
    flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
  }
  if (f & ac_access_transfer_read_bit)
  {
    flags |= VK_ACCESS_TRANSFER_READ_BIT;
  }
  if (f & ac_access_transfer_write_bit)
  {
    flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
  }
  if (f & ac_access_acceleration_structure_read_bit)
  {
    flags |= VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;
  }
  if (f & ac_access_acceleration_structure_write_bit)
  {
    flags |= VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
  }

  return flags;
}

static inline VkPipelineStageFlags
ac_pipeline_stage_bits_to_vk(ac_pipeline_stage_bits f)
{
  VkPipelineStageFlags flags = VK_PIPELINE_STAGE_NONE;
  if (f & ac_pipeline_stage_top_of_pipe_bit)
  {
    flags |= VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
  }
  if (f & ac_pipeline_stage_draw_indirect_bit)
  {
    flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;
  }
  if (f & ac_pipeline_stage_vertex_input_bit)
  {
    flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
  }
  if (f & ac_pipeline_stage_vertex_shader_bit)
  {
    flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
  }
  if (f & ac_pipeline_stage_pixel_shader_bit)
  {
    flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
  }
  if (f & ac_pipeline_stage_early_pixel_tests_bit)
  {
    flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
  }
  if (f & ac_pipeline_stage_late_pixel_tests_bit)
  {
    flags |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  }
  if (
    f & (ac_pipeline_stage_color_attachment_output_bit |
         ac_pipeline_stage_resolve_bit))
  {
    flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  }
  if (f & ac_pipeline_stage_compute_shader_bit)
  {
    flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
  }
  if (f & ac_pipeline_stage_transfer_bit)
  {
    flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
  }
  if (f & ac_pipeline_stage_bottom_of_pipe_bit)
  {
    flags |= VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  }
  if (f & ac_pipeline_stage_all_graphics_bit)
  {
    flags |= VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
  }
  if (f & ac_pipeline_stage_all_commands_bit)
  {
    flags |= VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
  }
  if (f & ac_pipeline_stage_acceleration_structure_build_bit)
  {
    flags |= VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR;
  }
  if (f & ac_pipeline_stage_raytracing_shader_bit)
  {
    flags |= VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR;
  }
  return flags;
}

static inline VkImageLayout
ac_image_layout_to_vk(ac_image_layout layout, ac_format format)
{
  switch (layout)
  {
  case ac_image_layout_undefined:
    return VK_IMAGE_LAYOUT_UNDEFINED;
  case ac_image_layout_general:
    return VK_IMAGE_LAYOUT_GENERAL;
  case ac_image_layout_color_write:
    return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
  case ac_image_layout_resolve:
    if (ac_format_depth_or_stencil(format))
    {
      return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    }
    else
    {
      return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    }
  case ac_image_layout_depth_stencil_write:
    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
  case ac_image_layout_depth_stencil_read:
    return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
  case ac_image_layout_shader_read:
    return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
  case ac_image_layout_transfer_src:
    return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
  case ac_image_layout_transfer_dst:
    return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
  case ac_image_layout_present_src:
    return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
  default:
    break;
  }

  AC_ASSERT(false);
  return (VkImageLayout)-1;
}

static inline VkRayTracingShaderGroupTypeKHR
ac_raytracing_group_type_to_vk(ac_raytracing_group_type type)
{
  switch (type)
  {
  case ac_raytracing_group_type_general:
    return VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
  case ac_raytracing_group_type_triangles:
    return VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
  case ac_raytracing_group_type_procedural:
    return VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
  default:
    break;
  }

  AC_ASSERT(false);
  return (VkRayTracingShaderGroupTypeKHR)-1;
}

static inline VkGeometryInstanceFlagsKHR
ac_as_instance_bits_to_vk(ac_as_instance_bits bits)
{
  VkGeometryInstanceFlagsKHR flags = 0;

  if (bits & ac_as_instance_triangle_facing_cull_disable_bit)
  {
    flags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR;
  }

  if (bits & ac_as_instance_triangle_flip_facing_bit)
  {
    flags |= VK_GEOMETRY_INSTANCE_TRIANGLE_FLIP_FACING_BIT_KHR;
  }

  if (bits & ac_as_instance_force_opaque_bit)
  {
    flags |= VK_GEOMETRY_INSTANCE_FORCE_OPAQUE_BIT_KHR;
  }

  if (bits & ac_as_instance_force_no_opaque_bit)
  {
    flags |= VK_GEOMETRY_INSTANCE_FORCE_NO_OPAQUE_BIT_KHR;
  }

  return flags;
}

static inline VkGeometryTypeKHR
ac_geometry_type_to_vk(ac_geometry_type type)
{
  switch (type)
  {
  case ac_geometry_type_triangles:
    return VK_GEOMETRY_TYPE_TRIANGLES_KHR;
  case ac_geometry_type_aabbs:
    return VK_GEOMETRY_TYPE_AABBS_KHR;
  default:
    break;
  }

  AC_ASSERT(false);
  return (VkGeometryTypeKHR)-1;
}

static inline VkGeometryFlagsKHR
ac_geometry_bits_to_vk(ac_geometry_bits bits)
{
  VkGeometryFlagsKHR flags = 0;

  if (bits & ac_geometry_opaque_bit)
  {
    flags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
  }

  if (bits & ac_geometry_no_duplicate_any_hit_invocation_bit)
  {
    flags |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
  }

  return flags;
}

static inline VkAccelerationStructureTypeKHR
ac_as_type_to_vk(ac_as_type type)
{
  switch (type)
  {
  case ac_as_type_bottom_level:
    return VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
  case ac_as_type_top_level:
    return VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
  default:
    break;
  }
  AC_ASSERT(false);
  return (VkAccelerationStructureTypeKHR)-1;
}

static inline VkFormat
ac_format_to_vk(ac_format format)
{
  switch (format)
  {
  case ac_format_undefined:
    return VK_FORMAT_UNDEFINED;
  case ac_format_r5g6b5_unorm:
    return VK_FORMAT_B5G6R5_UNORM_PACK16;
  case ac_format_r8_unorm:
    return VK_FORMAT_R8_UNORM;
  case ac_format_r8_snorm:
    return VK_FORMAT_R8_SNORM;
  case ac_format_r8_uint:
    return VK_FORMAT_R8_UINT;
  case ac_format_r8_sint:
    return VK_FORMAT_R8_SINT;
  case ac_format_r8_srgb:
    return VK_FORMAT_R8_SRGB;
  case ac_format_r8g8_unorm:
    return VK_FORMAT_R8G8_UNORM;
  case ac_format_r8g8_snorm:
    return VK_FORMAT_R8G8_SNORM;
  case ac_format_r8g8_uint:
    return VK_FORMAT_R8G8_UINT;
  case ac_format_r8g8_sint:
    return VK_FORMAT_R8G8_SINT;
  case ac_format_r8g8_srgb:
    return VK_FORMAT_R8G8_SRGB;
  case ac_format_r8g8b8a8_unorm:
    return VK_FORMAT_R8G8B8A8_UNORM;
  case ac_format_r8g8b8a8_snorm:
    return VK_FORMAT_R8G8B8A8_SNORM;
  case ac_format_r8g8b8a8_uint:
    return VK_FORMAT_R8G8B8A8_UINT;
  case ac_format_r8g8b8a8_sint:
    return VK_FORMAT_R8G8B8A8_SINT;
  case ac_format_r8g8b8a8_srgb:
    return VK_FORMAT_R8G8B8A8_SRGB;
  case ac_format_b8g8r8a8_srgb:
    return VK_FORMAT_B8G8R8A8_SRGB;
  case ac_format_b8g8r8a8_unorm:
    return VK_FORMAT_B8G8R8A8_UNORM;
  case ac_format_r16_unorm:
    return VK_FORMAT_R16_UNORM;
  case ac_format_r16_snorm:
    return VK_FORMAT_R16_SNORM;
  case ac_format_r16_uint:
    return VK_FORMAT_R16_UINT;
  case ac_format_r16_sint:
    return VK_FORMAT_R16_SINT;
  case ac_format_r16_sfloat:
    return VK_FORMAT_R16_SFLOAT;
  case ac_format_r16g16_unorm:
    return VK_FORMAT_R16G16_UNORM;
  case ac_format_r16g16_snorm:
    return VK_FORMAT_R16G16_SNORM;
  case ac_format_r16g16_uint:
    return VK_FORMAT_R16G16_UINT;
  case ac_format_r16g16_sint:
    return VK_FORMAT_R16G16_SINT;
  case ac_format_r16g16_sfloat:
    return VK_FORMAT_R16G16_SFLOAT;
  case ac_format_r16g16b16a16_unorm:
    return VK_FORMAT_R16G16B16A16_UNORM;
  case ac_format_r16g16b16a16_snorm:
    return VK_FORMAT_R16G16B16A16_SNORM;
  case ac_format_r16g16b16a16_uint:
    return VK_FORMAT_R16G16B16A16_UINT;
  case ac_format_r16g16b16a16_sint:
    return VK_FORMAT_R16G16B16A16_SINT;
  case ac_format_r16g16b16a16_sfloat:
    return VK_FORMAT_R16G16B16A16_SFLOAT;
  case ac_format_r32_uint:
    return VK_FORMAT_R32_UINT;
  case ac_format_r32_sint:
    return VK_FORMAT_R32_SINT;
  case ac_format_r32_sfloat:
    return VK_FORMAT_R32_SFLOAT;
  case ac_format_r32g32_uint:
    return VK_FORMAT_R32G32_UINT;
  case ac_format_r32g32_sint:
    return VK_FORMAT_R32G32_SINT;
  case ac_format_r32g32_sfloat:
    return VK_FORMAT_R32G32_SFLOAT;
  case ac_format_r32g32b32_uint:
    return VK_FORMAT_R32G32B32_UINT;
  case ac_format_r32g32b32_sint:
    return VK_FORMAT_R32G32B32_SINT;
  case ac_format_r32g32b32_sfloat:
    return VK_FORMAT_R32G32B32_SFLOAT;
  case ac_format_r32g32b32a32_uint:
    return VK_FORMAT_R32G32B32A32_UINT;
  case ac_format_r32g32b32a32_sint:
    return VK_FORMAT_R32G32B32A32_SINT;
  case ac_format_r32g32b32a32_sfloat:
    return VK_FORMAT_R32G32B32A32_SFLOAT;
  case ac_format_b10g10r10a2_unorm:
    return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
  case ac_format_b10g10r10a2_uint:
    return VK_FORMAT_A2R10G10B10_UINT_PACK32;
  case ac_format_r10g10b10a2_unorm:
    return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
  case ac_format_r10g10b10a2_uint:
    return VK_FORMAT_A2B10G10R10_UINT_PACK32;
  case ac_format_e5b9g9r9_ufloat:
    return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
  case ac_format_d16_unorm:
    return VK_FORMAT_D16_UNORM;
  case ac_format_d32_sfloat:
    return VK_FORMAT_D32_SFLOAT;
  case ac_format_s8_uint:
    return VK_FORMAT_S8_UINT;
  default:
    break;
  }
  return VK_FORMAT_UNDEFINED;
}

static inline ac_format
ac_format_from_vk(VkFormat format)
{
  switch (format)
  {
  case VK_FORMAT_UNDEFINED:
    return ac_format_undefined;
  case VK_FORMAT_B5G6R5_UNORM_PACK16:
    return ac_format_r5g6b5_unorm;
  case VK_FORMAT_R8_UNORM:
    return ac_format_r8_unorm;
  case VK_FORMAT_R8_SNORM:
    return ac_format_r8_snorm;
  case VK_FORMAT_R8_UINT:
    return ac_format_r8_uint;
  case VK_FORMAT_R8_SINT:
    return ac_format_r8_sint;
  case VK_FORMAT_R8_SRGB:
    return ac_format_r8_srgb;
  case VK_FORMAT_R8G8_UNORM:
    return ac_format_r8g8_unorm;
  case VK_FORMAT_R8G8_SNORM:
    return ac_format_r8g8_snorm;
  case VK_FORMAT_R8G8_UINT:
    return ac_format_r8g8_uint;
  case VK_FORMAT_R8G8_SINT:
    return ac_format_r8g8_sint;
  case VK_FORMAT_R8G8_SRGB:
    return ac_format_r8g8_srgb;
  case VK_FORMAT_R8G8B8A8_UNORM:
    return ac_format_r8g8b8a8_unorm;
  case VK_FORMAT_R8G8B8A8_SNORM:
    return ac_format_r8g8b8a8_snorm;
  case VK_FORMAT_R8G8B8A8_UINT:
    return ac_format_r8g8b8a8_uint;
  case VK_FORMAT_R8G8B8A8_SINT:
    return ac_format_r8g8b8a8_sint;
  case VK_FORMAT_R8G8B8A8_SRGB:
    return ac_format_r8g8b8a8_srgb;
  case VK_FORMAT_B8G8R8A8_UNORM:
    return ac_format_b8g8r8a8_unorm;
  case VK_FORMAT_B8G8R8A8_SRGB:
    return ac_format_b8g8r8a8_srgb;
  case VK_FORMAT_R16_UNORM:
    return ac_format_r16_unorm;
  case VK_FORMAT_R16_SNORM:
    return ac_format_r16_snorm;
  case VK_FORMAT_R16_UINT:
    return ac_format_r16_uint;
  case VK_FORMAT_R16_SINT:
    return ac_format_r16_sint;
  case VK_FORMAT_R16_SFLOAT:
    return ac_format_r16_sfloat;
  case VK_FORMAT_R16G16_UNORM:
    return ac_format_r16g16_unorm;
  case VK_FORMAT_R16G16_SNORM:
    return ac_format_r16g16_snorm;
  case VK_FORMAT_R16G16_UINT:
    return ac_format_r16g16_uint;
  case VK_FORMAT_R16G16_SINT:
    return ac_format_r16g16_sint;
  case VK_FORMAT_R16G16_SFLOAT:
    return ac_format_r16g16_sfloat;
  case VK_FORMAT_R16G16B16A16_UNORM:
    return ac_format_r16g16b16a16_unorm;
  case VK_FORMAT_R16G16B16A16_SNORM:
    return ac_format_r16g16b16a16_snorm;
  case VK_FORMAT_R16G16B16A16_UINT:
    return ac_format_r16g16b16a16_uint;
  case VK_FORMAT_R16G16B16A16_SINT:
    return ac_format_r16g16b16a16_sint;
  case VK_FORMAT_R16G16B16A16_SFLOAT:
    return ac_format_r16g16b16a16_sfloat;
  case VK_FORMAT_R32_UINT:
    return ac_format_r32_uint;
  case VK_FORMAT_R32_SINT:
    return ac_format_r32_sint;
  case VK_FORMAT_R32_SFLOAT:
    return ac_format_r32_sfloat;
  case VK_FORMAT_R32G32_UINT:
    return ac_format_r32g32_uint;
  case VK_FORMAT_R32G32_SINT:
    return ac_format_r32g32_sint;
  case VK_FORMAT_R32G32_SFLOAT:
    return ac_format_r32g32_sfloat;
  case VK_FORMAT_R32G32B32_UINT:
    return ac_format_r32g32b32_uint;
  case VK_FORMAT_R32G32B32_SINT:
    return ac_format_r32g32b32_sint;
  case VK_FORMAT_R32G32B32_SFLOAT:
    return ac_format_r32g32b32_sfloat;
  case VK_FORMAT_R32G32B32A32_UINT:
    return ac_format_r32g32b32a32_uint;
  case VK_FORMAT_R32G32B32A32_SINT:
    return ac_format_r32g32b32a32_sint;
  case VK_FORMAT_R32G32B32A32_SFLOAT:
    return ac_format_r32g32b32a32_sfloat;
  case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
    return ac_format_b10g10r10a2_unorm;
  case VK_FORMAT_A2R10G10B10_UINT_PACK32:
    return ac_format_b10g10r10a2_uint;
  case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
    return ac_format_r10g10b10a2_unorm;
  case VK_FORMAT_A2B10G10R10_UINT_PACK32:
    return ac_format_r10g10b10a2_uint;
  case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
    return ac_format_e5b9g9r9_ufloat;
  case VK_FORMAT_D16_UNORM:
    return ac_format_d16_unorm;
  case VK_FORMAT_D32_SFLOAT:
    return ac_format_d32_sfloat;
  case VK_FORMAT_S8_UINT:
    return ac_format_s8_uint;
  default:
    break;
  }
  return ac_format_undefined;
}

static inline ac_color_space
ac_color_space_from_vk(VkColorSpaceKHR color_space)
{
  switch (color_space)
  {
  case VK_COLOR_SPACE_HDR10_ST2084_EXT:
    return ac_color_space_hdr;
  case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
    return ac_color_space_srgb;
  default:
    break;
  }
  AC_ASSERT(false);
  return (ac_color_space)-1;
}

#endif
