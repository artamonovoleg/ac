#pragma once

#include "ac_private.h"

#if (AC_INCLUDE_RENDERER) && (AC_PLATFORM_WINDOWS || AC_PLATFORM_XBOX)

#if (AC_PLATFORM_WINDOWS)
#include "renderer_d3d12_windows.h"
#elif (AC_PLATFORM_XBOX)
#include "renderer_d3d12_xbox.h"
#endif

#if (AC_D3D12_USE_MESH_SHADERS)
#include <d3d12/d3dx12.h>
#endif

#define D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED
#include <d3d12_mem_alloc/d3d12_mem_alloc.hpp>
#include <renderdoc/renderdoc_app.h>

#include "renderer.h"

#if (AC_INCLUDE_DEBUG)
#define AC_D3D12_SET_OBJECT_NAME(object, _name)                                \
  do                                                                           \
  {                                                                            \
    if (_name)                                                                 \
    {                                                                          \
      AC_D3D12_CONVERT_NAME(name, _name);                                      \
      (object)->SetName((name));                                               \
    }                                                                          \
  }                                                                            \
  while (false)
#else
#define AC_D3D12_SET_OBJECT_NAME(object, name) ((void)0)
#endif

typedef HRESULT(WINAPI* ac_pfn_create_dxgi_factory2)(
  UINT                Flags,
  REFIID              riid,
  _COM_Outptr_ void** ppFactory);

typedef struct ac_d3d12_cpu_heap {
  ID3D12DescriptorHeap* heap;
  uint64_t*             stack;
  uint64_t              stack_size;
} ac_d3d12_cpu_heap;

typedef struct ac_d3d12_device {
  ac_device_internal          common;
  ID3D12Debug1*               debug;
  uint32_t                    factory_flags;
  ac_pfn_create_dxgi_factory2 create_factory;
  ID3D12InfoQueue*            info_queue;
  LUID                        adapter_luid;
#if (AC_D3D12_USE_RAYTRACING)
  ID3D12Device5* device;
#else
  ID3D12Device* device;
#endif
  DWORD                callback_cookie;
  D3D12MA::Allocator*  allocator;
  ac_d3d12_cpu_heap    rtv_heap;
  ac_d3d12_cpu_heap    dsv_heap;
  uint32_t             rtv_descriptor_size;
  uint32_t             dsv_descriptor_size;
  uint32_t             resource_descriptor_size;
  uint32_t             sampler_descriptor_size;
  bool                 enhanced_barriers;
  RENDERDOC_API_1_6_0* rdoc;
} ac_d3d12_device;

typedef struct ac_d3d12_fence {
  ac_fence_internal common;
  ID3D12Fence*      fence;
  HANDLE            event;
  UINT              value;
} ac_d3d12_fence;

typedef struct ac_d3d12_queue {
  ac_queue_internal   common;
  ID3D12CommandQueue* queue;
  ac_d3d12_fence      fence;
} ac_d3d12_queue;

typedef struct ac_d3d12_cmd_pool {
  ac_cmd_pool_internal    common;
  ID3D12CommandAllocator* command_allocator;
} ac_d3d12_cmd_pool;

typedef struct ac_d3d12_cmd {
  ac_cmd_internal                    common;
  struct ac_d3d12_descriptor_buffer* db;
#if (AC_D3D12_USE_ENHANCED_BARRIERS)
  ID3D12GraphicsCommandList7* cmd;
#elif (AC_D3D12_USE_RAYTRACING)
  ID3D12GraphicsCommandList5* cmd;
#else
  ID3D12GraphicsCommandList1* cmd;
#endif
} ac_d3d12_cmd;

typedef struct ac_d3d12_sampler {
  ac_sampler_internal common;
  D3D12_SAMPLER_DESC  desc;
} ac_d3d12_sampler;

typedef struct ac_d3d12_image {
  ac_image_internal    common;
  ID3D12Resource*      resource;
  D3D12MA::Allocation* allocation;
  uint64_t             handle;
} ac_d3d12_image;

typedef struct ac_d3d12_buffer {
  ac_buffer_internal   common;
  ID3D12Resource*      resource;
  D3D12MA::Allocation* allocation;
} ac_d3d12_buffer;

typedef struct ac_d3d12_swapchain {
  ac_swapchain_internal common;
  union {
    IDXGISwapChain1* swapchain;
    uint64_t         value;
  };
} ac_d3d12_swapchain;

typedef struct ac_d3d12_shader {
  ac_shader_internal    common;
  D3D12_SHADER_BYTECODE bytecode;
} ac_d3d12_shader;

typedef struct ac_d3d12_space {
  struct {
    uint64_t offset;
    uint32_t stride;
    uint32_t root_param;
  } sampler, resource;
} ac_d3d12_space;

typedef struct ac_d3d12_dsl {
  ac_dsl_internal      common;
  ID3D12RootSignature* signature;
  uint32_t             push_constant;
  struct hashmap*      handles;
  ac_d3d12_space       spaces[ac_space_count];
} ac_d3d12_dsl;

typedef struct ac_d3d12_descriptor_buffer {
  ac_descriptor_buffer_internal common;
  ID3D12RootSignature*          signature;
  struct hashmap*               handles;
  ID3D12DescriptorHeap*         resource_heap;
  ID3D12DescriptorHeap*         sampler_heap;
  ac_d3d12_space                spaces[ac_space_count];
} ac_d3d12_descriptor_buffer;

typedef struct ac_d3d12_pipeline {
  ac_pipeline_internal     common;
  ID3D12PipelineState*     pipeline;
  ID3D12RootSignature*     signature;
  D3D12_PRIMITIVE_TOPOLOGY topology;
  uint32_t                 vertex_strides[AC_MAX_VERTEX_BINDING_COUNT];
  uint32_t                 push_constant;
} ac_d3d12_pipeline;

typedef struct ac_d3d12_sbt {
  ac_sbt_internal                            common;
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE            raygen;
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE miss;
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE hit;
  D3D12_GPU_VIRTUAL_ADDRESS_RANGE_AND_STRIDE callable;
} ac_d3d12_sbt;

typedef struct ac_d3d12_as {
  ac_as_internal       common;
  ID3D12Resource*      resource;
  D3D12MA::Allocation* allocation;
  uint32_t             count;
  union {
    D3D12_RAYTRACING_GEOMETRY_DESC* geometries;
    D3D12_GPU_VIRTUAL_ADDRESS       instances;
  };
} ac_d3d12_as;

typedef struct ac_d3d12_binding_handle {
  uint32_t           reg;
  uint32_t           space;
  ac_descriptor_type type;
  uint64_t           handle;
} ac_d3d12_binding_handle;

#if defined(__cplusplus)
extern "C"
{
#endif

void
ac_d3d12_get_device_common_functions(ac_d3d12_device* device);

#if defined(__cplusplus)
}
#endif

#endif
