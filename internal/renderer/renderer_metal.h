#pragma once

#include "ac_private.h"

#if (AC_PLATFORM_APPLE)

#if (AC_PLATFORM_APPLE_MACOS)
#include <Cocoa/Cocoa.h>
#elif (AC_PLATFORM_APPLE_IOS)
#include <UIKit/UIKit.h>
#endif
#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>
#include <vk_mem_alloc/vk_mem_alloc.h>
#include <QuartzCore/CAMetalLayer.h>
#include <Metal/Metal.h>
#include "renderer.h"

#define AC_MTL_MAX_BINDINGS 30
#define AC_MTL_PUSH_CONSTANT_ALIGNMENT 16

#define AC_MTL_PUSH_CONSTANT_INDEX ((NSUInteger)ac_space_count)

#if (AC_INCLUDE_DEBUG)
#define AC_MTL_SET_OBJECT_NAME(object, name)                                   \
  do                                                                           \
  {                                                                            \
    if ((name) && (object))                                                    \
    {                                                                          \
      ((object).label = [NSString stringWithUTF8String:(name)]);               \
    }                                                                          \
  }                                                                            \
  while (false)
#else
#define AC_MTL_SET_OBJECT_NAME(object, name) ((void)0)
#endif

typedef struct ac_mtl_device {
  ac_device_internal  common;
  id<MTLDevice>       device;
  VmaAllocator        gpu_allocator;
  uint32_t            heap_count;
  uint32_t            heaps_capacity;
  __unsafe_unretained id<MTLHeap>* heaps;
} ac_mtl_device;

typedef struct ac_mtl_queue {
  ac_queue_internal   common;
  id<MTLCommandQueue> queue;
} ac_mtl_queue;

typedef struct ac_mtl_cmd_pool {
  ac_cmd_pool_internal common;
} ac_mtl_cmd_pool;

typedef struct ac_mtl_cmd {
  ac_cmd_internal              common;
  id<MTLCommandBuffer>         cmd;
  id<MTLRenderCommandEncoder>  render_encoder;
  id<MTLComputeCommandEncoder> compute_encoder;
  uint64_t                     index_offset;
  MTLIndexType                 index_type;
  __unsafe_unretained id<MTLBuffer> index_buffer;
} ac_mtl_cmd;

typedef struct ac_mtl_fence {
  ac_fence_internal  common;
  id<MTLSharedEvent> event;
} ac_mtl_fence;

typedef struct ac_mtl_sampler {
  ac_sampler_internal common;
  id<MTLSamplerState> sampler;
} ac_mtl_sampler;

typedef struct ac_mtl_image {
  ac_image_internal        common;
  id<MTLTexture>           texture;
  id<MTLTexture> __strong* mips;
  VmaAllocation            allocation;
} ac_mtl_image;

typedef struct ac_mtl_buffer {
  ac_buffer_internal common;
  id<MTLBuffer>      buffer;
  VmaAllocation      allocation;
} ac_mtl_buffer;

typedef struct ac_mtl_swapchain {
  ac_swapchain_internal             common;
  __unsafe_unretained CAMetalLayer* layer;
  id<CAMetalDrawable>               drawable;
} ac_mtl_swapchain;

typedef struct ac_mtl_shader {
  ac_shader_internal common;
  id<MTLFunction>    function;
} ac_mtl_shader;

typedef struct ac_mtl_dsl_space {
  bool            exist;
  uint32_t        ro_resource_count;
  uint32_t        rw_resource_count;
  struct hashmap* resource_map;
} ac_mtl_dsl_space;

typedef struct ac_mtl_dsl {
  ac_dsl_internal  common;
  ac_mtl_dsl_space spaces[ac_space_count];
  id<MTLFunction>  function;
} ac_mtl_dsl;

typedef struct ac_mtl_resource {
  uint32_t           reg;
  ac_descriptor_type type;
  uint32_t           index;
  bool               rw;
} ac_mtl_resource;

typedef struct ac_mtl_resources {
  struct hashmap*     resource_map;
  uint32_t            ro_resource_count;
  uint32_t            rw_resource_count;
  __unsafe_unretained id<MTLResource>* ro_resources;
  __unsafe_unretained id<MTLResource>* rw_resources;
} ac_mtl_resources;

typedef struct ac_mtl_db_space {
  uint32_t               set_count;
  ac_mtl_resources       resources;
  id<MTLArgumentEncoder> arg_enc;
  uint32_t               arg_buffer_offset;
} ac_mtl_db_space;

typedef struct ac_mtl_descriptor_buffer {
  ac_descriptor_buffer_internal common;
  ac_mtl_db_space               spaces[ac_space_count];
  id<MTLBuffer>                 arg_buffer;
} ac_mtl_descriptor_buffer;

typedef struct ac_mtl_pipeline {
  ac_pipeline_internal common;
  bool                 has_non_fragment_pc;
  union {
    struct {
      id<MTLRenderPipelineState> graphics_pipeline;
      id<MTLDepthStencilState>   depth_state;
      MTLCullMode                cull_mode;
      MTLPrimitiveType           primitive_type;
      MTLWinding                 winding;
      float                      depth_bias;
      float                      slope_scale;
      bool                       has_fragment_pc;
      MTLSize                    object_workgroup;
      MTLSize                    mesh_workgroup;
    };
    struct {
      id<MTLComputePipelineState> compute_pipeline;
      MTLSize                     workgroup;
    };
  };
} ac_mtl_pipeline;

typedef struct ac_mtl_as {
  ac_as_internal                      common;
  id<MTLAccelerationStructure>        as;
  MTLAccelerationStructureDescriptor* descriptor;
  VmaAllocation                       allocation;
} ac_mtl_as;

typedef struct VkDeviceMemory_T {
  id<MTLHeap> heap;
} VkDeviceMemory_T;

static inline NSString*
ac_mtl_cmd_error_to_string(MTLCommandBufferError code)
{
  AC_OBJC_BEGIN_ARP();

  switch (code)
  {
  case MTLCommandBufferErrorNone:
    return @"none";
  case MTLCommandBufferErrorInternal:
    return @"internal";
  case MTLCommandBufferErrorTimeout:
    return @"timeout";
  case MTLCommandBufferErrorPageFault:
    return @"page fault";
  case MTLCommandBufferErrorNotPermitted:
    return @"not permitted";
  case MTLCommandBufferErrorOutOfMemory:
    return @"out of memory";
  case MTLCommandBufferErrorInvalidResource:
    return @"invalid resource";
  case MTLCommandBufferErrorMemoryless:
    return @"memory-less";
  default:
    break;
  }

  return [NSString stringWithFormat:@"<unknown> %zu", code];

  AC_OBJC_END_ARP();
}

static inline NSString*
ac_mtl_cmd_error_state_to_string(MTLCommandEncoderErrorState state)
{
  AC_OBJC_BEGIN_ARP();

  switch (state)
  {
  case MTLCommandEncoderErrorStateUnknown:
    return @"unknown";
  case MTLCommandEncoderErrorStateCompleted:
    return @"completed";
  case MTLCommandEncoderErrorStateAffected:
    return @"affected";
  case MTLCommandEncoderErrorStatePending:
    return @"pending";
  case MTLCommandEncoderErrorStateFaulted:
    return @"faulted";
  default:
    break;
  }
  return @"unknown";

  AC_OBJC_END_ARP();
}

static inline ac_mtl_resource*
ac_mtl_update_resources(
  ac_mtl_resources*          resources,
  uint32_t                   set,
  const ac_descriptor_write* write)
{
  AC_OBJC_BEGIN_ARP();

  uint32_t            resource_index = 0;
  bool                rw = false;
  __unsafe_unretained id<MTLResource>* rs = NULL;

  ac_mtl_resource* p_resource = NULL;

  {
    ac_mtl_resource resource;
    AC_ZERO(resource);
    resource.reg = write->reg;
    resource.type = write->type;

    p_resource = hashmap_get(resources->resource_map, &resource);

    if (!p_resource)
    {
      return NULL;
    }

    resource_index = p_resource->index;
    rw = p_resource->rw;
  }

  if (rw && resources->rw_resource_count)
  {
    rs = resources->rw_resources + resources->rw_resource_count * set +
         resource_index;
  }
  else if (resources->ro_resource_count)
  {
    rs = resources->ro_resources + resources->ro_resource_count * set +
         resource_index;
  }

  for (uint32_t i = 0; i < write->count; ++i)
  {
    switch (write->type)
    {
    case ac_descriptor_type_srv_image:
    {
      AC_FROM_HANDLE2(image, write->descriptors[i].image, ac_mtl_image);
      rs[i] = image->texture;
      break;
    }
    case ac_descriptor_type_uav_image:
    {
      AC_FROM_HANDLE2(image, write->descriptors[i].image, ac_mtl_image);
      rs[i] = image->mips[write->descriptors[i].level];
      break;
    }
    case ac_descriptor_type_cbv_buffer:
    case ac_descriptor_type_srv_buffer:
    case ac_descriptor_type_uav_buffer:
    {
      AC_FROM_HANDLE2(buffer, write->descriptors[i].buffer, ac_mtl_buffer);
      rs[i] = buffer->buffer;
      break;
    }
    case ac_descriptor_type_as:
    {
      AC_FROM_HANDLE2(as, write->descriptors[i].as, ac_mtl_as);
      rs[i] = as->as;
      break;
    }
    default:
    {
      break;
    }
    }
  }

  return p_resource;

  AC_OBJC_END_ARP();
}

static int32_t
ac_mtl_resource_compare(const void* a, const void* b, void* udata)
{
  AC_UNUSED(udata);
  const ac_mtl_resource* b0 = a;
  const ac_mtl_resource* b1 = b;

  if (b0->reg == b1->reg && b0->type == b1->type)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}

static uint64_t
ac_mtl_resource_hash(const void* item, uint64_t seed0, uint64_t seed1)
{
  const ac_mtl_resource* b = item;
  struct {
    uint32_t           reg;
    ac_descriptor_type type;
  } tmp = {
    .reg = b->reg,
    .type = b->type,
  };
  return hashmap_murmur(&tmp, sizeof(tmp), seed0, seed1);
}

static inline MTLResourceOptions
ac_memory_usage_to_mtl(ac_memory_usage usage)
{
  switch (usage)
  {
  case ac_memory_usage_gpu_only:
    return MTLResourceStorageModePrivate;
  case ac_memory_usage_cpu_to_gpu:
    return MTLResourceStorageModeShared | MTLResourceCPUCacheModeWriteCombined;
  case ac_memory_usage_gpu_to_cpu:
    return MTLResourceStorageModeShared | MTLResourceCPUCacheModeDefaultCache;
  default:
    break;
  }

  AC_ASSERT(false);
  return (MTLResourceOptions)-1;
}

static inline MTLPixelFormat
ac_format_to_mtl(ac_format format)
{
  switch (format)
  {
  case ac_format_r8_unorm:
    return MTLPixelFormatR8Unorm;
  case ac_format_r8_snorm:
    return MTLPixelFormatR8Snorm;
  case ac_format_r8_uint:
    return MTLPixelFormatR8Uint;
  case ac_format_r8_sint:
    return MTLPixelFormatR8Sint;
  case ac_format_r8_srgb:
    return MTLPixelFormatR8Unorm_sRGB;
  case ac_format_r5g6b5_unorm:
    return MTLPixelFormatB5G6R5Unorm;
  case ac_format_r8g8_unorm:
    return MTLPixelFormatRG8Unorm;
  case ac_format_r8g8_snorm:
    return MTLPixelFormatRG8Snorm;
  case ac_format_r8g8_uint:
    return MTLPixelFormatRG8Uint;
  case ac_format_r8g8_sint:
    return MTLPixelFormatRG8Sint;
  case ac_format_r8g8_srgb:
    return MTLPixelFormatRG8Unorm_sRGB;
  case ac_format_r8g8b8a8_unorm:
    return MTLPixelFormatRGBA8Unorm;
  case ac_format_r8g8b8a8_snorm:
    return MTLPixelFormatRGBA8Snorm;
  case ac_format_r8g8b8a8_uint:
    return MTLPixelFormatRGBA8Uint;
  case ac_format_r8g8b8a8_sint:
    return MTLPixelFormatRGBA8Sint;
  case ac_format_r8g8b8a8_srgb:
    return MTLPixelFormatRGBA8Unorm_sRGB;
  case ac_format_b8g8r8a8_unorm:
    return MTLPixelFormatBGRA8Unorm;
  case ac_format_b8g8r8a8_srgb:
    return MTLPixelFormatBGRA8Unorm_sRGB;
  case ac_format_b10g10r10a2_unorm:
    return MTLPixelFormatBGR10A2Unorm;
  case ac_format_r10g10b10a2_unorm:
    return MTLPixelFormatRGB10A2Unorm;
  case ac_format_r10g10b10a2_uint:
    return MTLPixelFormatRGB10A2Uint;
  case ac_format_r16_unorm:
    return MTLPixelFormatR16Unorm;
  case ac_format_r16_snorm:
    return MTLPixelFormatR16Snorm;
  case ac_format_r16_uint:
    return MTLPixelFormatR16Uint;
  case ac_format_r16_sint:
    return MTLPixelFormatR16Sint;
  case ac_format_r16_sfloat:
    return MTLPixelFormatR16Float;
  case ac_format_r16g16_unorm:
    return MTLPixelFormatRG16Unorm;
  case ac_format_r16g16_snorm:
    return MTLPixelFormatRG16Snorm;
  case ac_format_r16g16_uint:
    return MTLPixelFormatRG16Uint;
  case ac_format_r16g16_sint:
    return MTLPixelFormatRG16Sint;
  case ac_format_r16g16_sfloat:
    return MTLPixelFormatRG16Float;
  case ac_format_r16g16b16a16_unorm:
    return MTLPixelFormatRGBA16Unorm;
  case ac_format_r16g16b16a16_snorm:
    return MTLPixelFormatRGBA16Snorm;
  case ac_format_r16g16b16a16_uint:
    return MTLPixelFormatRGBA16Uint;
  case ac_format_r16g16b16a16_sint:
    return MTLPixelFormatRGBA16Sint;
  case ac_format_r16g16b16a16_sfloat:
    return MTLPixelFormatRGBA16Float;
  case ac_format_r32_uint:
    return MTLPixelFormatR32Uint;
  case ac_format_r32_sint:
    return MTLPixelFormatR32Sint;
  case ac_format_r32_sfloat:
    return MTLPixelFormatR32Float;
  case ac_format_r32g32_uint:
    return MTLPixelFormatRG32Uint;
  case ac_format_r32g32_sint:
    return MTLPixelFormatRG32Sint;
  case ac_format_r32g32_sfloat:
    return MTLPixelFormatRG32Float;
  case ac_format_r32g32b32a32_uint:
    return MTLPixelFormatRGBA32Uint;
  case ac_format_r32g32b32a32_sint:
    return MTLPixelFormatRGBA32Sint;
  case ac_format_r32g32b32a32_sfloat:
    return MTLPixelFormatRGBA32Float;
  case ac_format_e5b9g9r9_ufloat:
    return MTLPixelFormatRGB9E5Float;
  case ac_format_d16_unorm:
    return MTLPixelFormatDepth16Unorm;
  case ac_format_d32_sfloat:
    return MTLPixelFormatDepth32Float;
  case ac_format_s8_uint:
    return MTLPixelFormatStencil8;
  default:
    break;
  }

  return MTLPixelFormatInvalid;
}

static inline MTLLoadAction
ac_attachment_load_op_to_mtl(ac_attachment_load_op load_op)
{
  switch (load_op)
  {
  case ac_attachment_load_op_load:
    return MTLLoadActionLoad;
  case ac_attachment_load_op_clear:
    return MTLLoadActionClear;
  case ac_attachment_load_op_dont_care:
    return MTLLoadActionDontCare;
  default:
    AC_ASSERT(false);
    return (MTLLoadAction)-1;
  }
}

static inline MTLStoreAction
ac_attachment_store_op_to_mtl(ac_attachment_store_op store_op)
{
  switch (store_op)
  {
  case ac_attachment_store_op_store:
    return MTLStoreActionStore;
  case ac_attachment_store_op_none:
  case ac_attachment_store_op_dont_care:
    return MTLStoreActionDontCare;
  default:
    AC_ASSERT(false);
    break;
  }
}

static inline MTLTextureUsage
ac_image_usage_bits_to_mtl(ac_image_usage_bits bits)
{
  MTLTextureUsage usage = 0;
  if (bits & ac_image_usage_srv_bit)
  {
    usage |= MTLTextureUsageShaderRead;
  }
  if (bits & ac_image_usage_uav_bit)
  {
    usage |=
      (MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite |
       MTLTextureUsagePixelFormatView);
  }
  if (bits & ac_image_usage_attachment_bit)
  {
    usage |= MTLTextureUsageRenderTarget;
  }
  return usage;
}

static inline MTLVertexFormat
ac_format_to_mtl_vertex_format(ac_format format)
{
  switch (format)
  {
  case ac_format_r8g8_uint:
    return MTLVertexFormatUChar2;
  case ac_format_r8g8b8a8_uint:
    return MTLVertexFormatUChar4;
  case ac_format_r8g8_sint:
    return MTLVertexFormatChar2;
  case ac_format_r8g8b8a8_sint:
    return MTLVertexFormatChar4;

  case ac_format_r8g8_unorm:
    return MTLVertexFormatUChar2Normalized;
  case ac_format_r8g8b8a8_unorm:
    return MTLVertexFormatUChar4Normalized;

  case ac_format_r8g8_snorm:
    return MTLVertexFormatChar2Normalized;
  case ac_format_r8g8b8a8_snorm:
    return MTLVertexFormatChar4Normalized;

  case ac_format_r16g16_uint:
    return MTLVertexFormatUShort2;
  case ac_format_r16g16b16a16_uint:
    return MTLVertexFormatUShort4;

  case ac_format_r16g16_sint:
    return MTLVertexFormatShort2;
  case ac_format_r16g16b16a16_sint:
    return MTLVertexFormatShort4;

  case ac_format_r16g16_unorm:
    return MTLVertexFormatUShort2Normalized;
  case ac_format_r16g16b16a16_unorm:
    return MTLVertexFormatUShort4Normalized;

  case ac_format_r16g16_snorm:
    return MTLVertexFormatShort2Normalized;
  case ac_format_r16g16b16a16_snorm:
    return MTLVertexFormatShort4Normalized;

  case ac_format_r16g16_sfloat:
    return MTLVertexFormatHalf2;
  case ac_format_r16g16b16a16_sfloat:
    return MTLVertexFormatHalf4;

  case ac_format_r32_sfloat:
    return MTLVertexFormatFloat;
  case ac_format_r32g32_sfloat:
    return MTLVertexFormatFloat2;
  case ac_format_r32g32b32_sfloat:
    return MTLVertexFormatFloat3;
  case ac_format_r32g32b32a32_sfloat:
    return MTLVertexFormatFloat4;

  case ac_format_r32_sint:
    return MTLVertexFormatInt;
  case ac_format_r32g32_sint:
    return MTLVertexFormatInt2;
  case ac_format_r32g32b32_sint:
    return MTLVertexFormatInt3;
  case ac_format_r32g32b32a32_sint:
    return MTLVertexFormatInt4;

  case ac_format_r32_uint:
    return MTLVertexFormatUInt;
  case ac_format_r32g32_uint:
    return MTLVertexFormatUInt2;
  case ac_format_r32g32b32_uint:
    return MTLVertexFormatUInt3;
  case ac_format_r32g32b32a32_uint:
    return MTLVertexFormatUInt4;

    // case ...
    // return MTLVertexFormatInt1010102Normalized;
  case ac_format_r10g10b10a2_unorm:
  case ac_format_b10g10r10a2_unorm:
    return MTLVertexFormatUInt1010102Normalized;

  case ac_format_r8_uint:
    return MTLVertexFormatUChar;
  case ac_format_r8_sint:
    return MTLVertexFormatChar;
  case ac_format_r8_unorm:
    return MTLVertexFormatUCharNormalized;
  case ac_format_r8_snorm:
    return MTLVertexFormatCharNormalized;

  case ac_format_r16_uint:
    return MTLVertexFormatUShort;
  case ac_format_r16_sint:
    return MTLVertexFormatShort;
  case ac_format_r16_unorm:
    return MTLVertexFormatUShortNormalized;
  case ac_format_r16_snorm:
    return MTLVertexFormatShortNormalized;
  default:
    break;
  }

  AC_ASSERT(false);
  return MTLVertexFormatInvalid;
}

static inline MTLAttributeFormat
ac_format_to_mtl_attribute_format(ac_format format)
{
  switch (format)
  {
  case ac_format_r8g8_uint:
    return MTLAttributeFormatUChar2;
  case ac_format_r8g8b8a8_uint:
    return MTLAttributeFormatUChar4;
  case ac_format_r8g8_sint:
    return MTLAttributeFormatChar2;
  case ac_format_r8g8b8a8_sint:
    return MTLAttributeFormatChar4;

  case ac_format_r8g8_unorm:
    return MTLAttributeFormatUChar2Normalized;
  case ac_format_r8g8b8a8_unorm:
    return MTLAttributeFormatUChar4Normalized;

  case ac_format_r8g8_snorm:
    return MTLAttributeFormatChar2Normalized;
  case ac_format_r8g8b8a8_snorm:
    return MTLAttributeFormatChar4Normalized;

  case ac_format_r16g16_uint:
    return MTLAttributeFormatUShort2;
  case ac_format_r16g16b16a16_uint:
    return MTLAttributeFormatUShort4;

  case ac_format_r16g16_sint:
    return MTLAttributeFormatShort2;
  case ac_format_r16g16b16a16_sint:
    return MTLAttributeFormatShort4;

  case ac_format_r16g16_unorm:
    return MTLAttributeFormatUShort2Normalized;
  case ac_format_r16g16b16a16_unorm:
    return MTLAttributeFormatUShort4Normalized;

  case ac_format_r16g16_snorm:
    return MTLAttributeFormatShort2Normalized;
  case ac_format_r16g16b16a16_snorm:
    return MTLAttributeFormatShort4Normalized;

  case ac_format_r16g16_sfloat:
    return MTLAttributeFormatHalf2;
  case ac_format_r16g16b16a16_sfloat:
    return MTLAttributeFormatHalf4;

  case ac_format_r32_sfloat:
    return MTLAttributeFormatFloat;
  case ac_format_r32g32_sfloat:
    return MTLAttributeFormatFloat2;
  case ac_format_r32g32b32_sfloat:
    return MTLAttributeFormatFloat3;
  case ac_format_r32g32b32a32_sfloat:
    return MTLAttributeFormatFloat4;

  case ac_format_r32_sint:
    return MTLAttributeFormatInt;
  case ac_format_r32g32_sint:
    return MTLAttributeFormatInt2;
  case ac_format_r32g32b32_sint:
    return MTLAttributeFormatInt3;
  case ac_format_r32g32b32a32_sint:
    return MTLAttributeFormatInt4;

  case ac_format_r32_uint:
    return MTLAttributeFormatUInt;
  case ac_format_r32g32_uint:
    return MTLAttributeFormatUInt2;
  case ac_format_r32g32b32_uint:
    return MTLAttributeFormatUInt3;
  case ac_format_r32g32b32a32_uint:
    return MTLAttributeFormatUInt4;

    // case ...
    // return MTLVertexFormatInt1010102Normalized;
  case ac_format_r10g10b10a2_unorm:
  case ac_format_b10g10r10a2_unorm:
    return MTLAttributeFormatUInt1010102Normalized;

  case ac_format_r8_uint:
    return MTLAttributeFormatUChar;
  case ac_format_r8_sint:
    return MTLAttributeFormatChar;
  case ac_format_r8_unorm:
    return MTLAttributeFormatUCharNormalized;
  case ac_format_r8_snorm:
    return MTLAttributeFormatCharNormalized;

  case ac_format_r16_uint:
    return MTLAttributeFormatUShort;
  case ac_format_r16_sint:
    return MTLAttributeFormatShort;
  case ac_format_r16_unorm:
    return MTLAttributeFormatUShortNormalized;
  case ac_format_r16_snorm:
    return MTLAttributeFormatShortNormalized;
  default:
    break;
  }

  AC_ASSERT(false);
  return MTLAttributeFormatInvalid;
}

static inline MTLVertexStepFunction
ac_input_rate_to_mtl(ac_input_rate input_rate)
{
  switch (input_rate)
  {
  case ac_input_rate_vertex:
    return MTLVertexStepFunctionPerVertex;
  case ac_input_rate_instance:
    return MTLVertexStepFunctionPerInstance;
  default:
    break;
  }
  AC_ASSERT(false);
  return (MTLVertexStepFunction)-1;
}

static inline MTLSamplerMinMagFilter
ac_filter_to_mtl(ac_filter filter)
{
  switch (filter)
  {
  case ac_filter_linear:
    return MTLSamplerMinMagFilterLinear;
  case ac_filter_nearest:
    return MTLSamplerMinMagFilterNearest;
  default:
    break;
  }
  AC_ASSERT(false);
  return (MTLSamplerMinMagFilter)-1;
}

static inline MTLSamplerMipFilter
ac_sampler_mipmap_mode_to_mtl(ac_sampler_mipmap_mode mode)
{
  switch (mode)
  {
  case ac_sampler_mipmap_mode_linear:
    return MTLSamplerMipFilterLinear;
  case ac_sampler_mipmap_mode_nearest:
    return MTLSamplerMipFilterNearest;
  default:
    break;
  }
  AC_ASSERT(false);
  return (MTLSamplerMipFilter)-1;
}

static inline MTLSamplerAddressMode
ac_sampler_address_mode_to_mtl(ac_sampler_address_mode mode)
{
  switch (mode)
  {
  case ac_sampler_address_mode_repeat:
    return MTLSamplerAddressModeRepeat;
  case ac_sampler_address_mode_mirrored_repeat:
    return MTLSamplerAddressModeMirrorRepeat;
  case ac_sampler_address_mode_clamp_to_edge:
    return MTLSamplerAddressModeClampToEdge;
  case ac_sampler_address_mode_clamp_to_border:
    return MTLSamplerAddressModeClampToBorderColor;
  default:
    break;
  }
  AC_ASSERT(false);
  return (MTLSamplerAddressMode)-1;
}

static inline MTLCompareFunction
ac_compare_op_to_mtl(ac_compare_op op)
{
  switch (op)
  {
  case ac_compare_op_never:
    return MTLCompareFunctionNever;
  case ac_compare_op_less:
    return MTLCompareFunctionLess;
  case ac_compare_op_equal:
    return MTLCompareFunctionEqual;
  case ac_compare_op_less_or_equal:
    return MTLCompareFunctionLessEqual;
  case ac_compare_op_greater:
    return MTLCompareFunctionGreater;
  case ac_compare_op_not_equal:
    return MTLCompareFunctionNotEqual;
  case ac_compare_op_greater_or_equal:
    return MTLCompareFunctionGreaterEqual;
  case ac_compare_op_always:
    return MTLCompareFunctionAlways;
  default:
    break;
  }
  AC_ASSERT(false);
  return (MTLCompareFunction)-1;
}

static inline MTLPrimitiveTopologyClass
ac_mtl_determine_primitive_topology_class(
  ac_polygon_mode       mode,
  ac_primitive_topology topology)
{
  switch (mode)
  {
  case ac_polygon_mode_line:
    return MTLPrimitiveTopologyClassLine;
  case ac_polygon_mode_fill:
    if (topology == ac_primitive_topology_point_list)
    {
      return MTLPrimitiveTopologyClassPoint;
    }
    if (
      topology == ac_primitive_topology_line_list ||
      topology == ac_primitive_topology_line_strip)
    {
      return MTLPrimitiveTopologyClassLine;
    }
    else
    {
      return MTLPrimitiveTopologyClassTriangle;
    }
  default:
    break;
  }
  AC_ASSERT(false);
  return (MTLPrimitiveTopologyClass)-1;
}

static inline MTLPrimitiveType
ac_primitive_topology_to_mtl(ac_primitive_topology topology)
{
  switch (topology)
  {
  case ac_primitive_topology_point_list:
    return MTLPrimitiveTypePoint;
  case ac_primitive_topology_line_list:
    return MTLPrimitiveTypeLine;
  case ac_primitive_topology_line_strip:
    return MTLPrimitiveTypeLineStrip;
  case ac_primitive_topology_triangle_list:
    return MTLPrimitiveTypeTriangle;
  case ac_primitive_topology_triangle_strip:
    return MTLPrimitiveTypeTriangleStrip;
  default:
    break;
  }
  AC_ASSERT(false);
  return (MTLPrimitiveType)-1;
}

static inline MTLDataType
ac_descriptor_type_to_mtl(ac_descriptor_type type)
{
  switch (type)
  {
  case ac_descriptor_type_cbv_buffer:
  case ac_descriptor_type_srv_buffer:
  case ac_descriptor_type_uav_buffer:
    return MTLDataTypePointer;
  case ac_descriptor_type_sampler:
    return MTLDataTypeSampler;
  case ac_descriptor_type_srv_image:
  case ac_descriptor_type_uav_image:
    return MTLDataTypeTexture;
  case ac_descriptor_type_as:
    return MTLDataTypeInstanceAccelerationStructure;
  default:
    break;
  }
  AC_ASSERT(false);
  return (MTLDataType)-1;
}

static inline MTLCullMode
ac_cull_mode_to_mtl(ac_cull_mode mode)
{
  switch (mode)
  {
  case ac_cull_mode_back:
    return MTLCullModeBack;
  case ac_cull_mode_front:
    return MTLCullModeFront;
  default:
    break;
  }
  return MTLCullModeNone;
}

static inline MTLWinding
ac_front_face_to_mtl(ac_front_face face)
{
  switch (face)
  {
  case ac_front_face_clockwise:
    return MTLWindingClockwise;
  case ac_front_face_counter_clockwise:
    return MTLWindingCounterClockwise;
  }
  return (MTLWinding)-1;
}

static inline MTLBlendFactor
ac_blend_factor_to_mtl(ac_blend_factor factor)
{
  switch (factor)
  {
  case ac_blend_factor_zero:
    return MTLBlendFactorZero;
  case ac_blend_factor_one:
    return MTLBlendFactorOne;
  case ac_blend_factor_src_color:
    return MTLBlendFactorSourceColor;
  case ac_blend_factor_one_minus_src_color:
    return MTLBlendFactorOneMinusSourceColor;
  case ac_blend_factor_dst_color:
    return MTLBlendFactorDestinationColor;
  case ac_blend_factor_one_minus_dst_color:
    return MTLBlendFactorOneMinusDestinationColor;
  case ac_blend_factor_src_alpha:
    return MTLBlendFactorSourceAlpha;
  case ac_blend_factor_one_minus_src_alpha:
    return MTLBlendFactorOneMinusSourceAlpha;
  case ac_blend_factor_dst_alpha:
    return MTLBlendFactorDestinationAlpha;
  case ac_blend_factor_one_minus_dst_alpha:
    return MTLBlendFactorOneMinusDestinationAlpha;
  case ac_blend_factor_src_alpha_saturate:
    return MTLBlendFactorSourceAlphaSaturated;
  default:
    AC_ASSERT(false);
    return (MTLBlendFactor)-1;
  }
}

static inline MTLBlendOperation
ac_blend_op_to_mtl(ac_blend_op op)
{
  switch (op)
  {
  case ac_blend_op_add:
    return MTLBlendOperationAdd;
  case ac_blend_op_subtract:
    return MTLBlendOperationSubtract;
  case ac_blend_op_reverse_subtract:
    return MTLBlendOperationReverseSubtract;
  case ac_blend_op_min:
    return MTLBlendOperationMin;
  case ac_blend_op_max:
    return MTLBlendOperationMax;
  default:
    AC_ASSERT(false);
    return (MTLBlendOperation)-1;
  }
}

static inline MTLAccelerationStructureInstanceOptions
ac_as_instance_bits_to_mtl(ac_as_instance_bits bits)
{
  MTLAccelerationStructureInstanceOptions options =
    MTLAccelerationStructureInstanceOptionNone;

  if (bits & ac_as_instance_triangle_facing_cull_disable_bit)
  {
    options |= MTLAccelerationStructureInstanceOptionDisableTriangleCulling;
  }

  if (bits & ac_as_instance_triangle_flip_facing_bit)
  {
    options |=
      MTLAccelerationStructureInstanceOptionTriangleFrontFacingWindingCounterClockwise;
  }

  if (bits & ac_as_instance_force_opaque_bit)
  {
    options |= MTLAccelerationStructureInstanceOptionOpaque;
  }

  if (bits & ac_as_instance_force_no_opaque_bit)
  {
    options |= MTLAccelerationStructureInstanceOptionNonOpaque;
  }

  return options;
}

#endif
