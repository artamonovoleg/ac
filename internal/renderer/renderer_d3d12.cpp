#include "ac_private.h"

#if (AC_INCLUDE_RENDERER) && (AC_PLATFORM_WINDOWS || AC_PLATFORM_XBOX)

#include "renderer_d3d12.h"
#include "renderer_d3d12_helpers.h"

extern "C"
{
static int32_t
ac_d3d12_binding_handle_compare(const void* a, const void* b, void* udata)
{
  AC_UNUSED(udata);
  const ac_d3d12_binding_handle* b0 =
    static_cast<const ac_d3d12_binding_handle*>(a);
  const ac_d3d12_binding_handle* b1 =
    static_cast<const ac_d3d12_binding_handle*>(b);

  return !(
    b0->reg == b1->reg && b0->space == b1->space && b0->type == b1->type);
}

static uint64_t
ac_d3d12_binding_handle_hash(const void* item, uint64_t seed0, uint64_t seed1)
{
  const ac_d3d12_binding_handle* b =
    static_cast<const ac_d3d12_binding_handle*>(item);
  struct {
    uint32_t           space;
    uint32_t           reg;
    ac_descriptor_type type;
  } tmp = {
    b->space,
    b->reg,
    b->type,
  };
  return hashmap_murmur(&tmp, sizeof(tmp), seed0, seed1);
}

static uint64_t
ac_d3d12_get_binding_handle(
  ac_d3d12_descriptor_buffer* db,
  uint32_t                    space,
  uint32_t                    reg,
  ac_descriptor_type          type)
{
  ac_d3d12_binding_handle bh = {};
  bh.reg = reg;
  bh.space = space;
  bh.type = type;

  ac_d3d12_binding_handle* out =
    static_cast<ac_d3d12_binding_handle*>(hashmap_get(db->handles, &bh));
  if (!out)
  {
    AC_ERROR(
      "[ renderer ] [ d3d12 ]: failed to locate descriptor space: %d register: "
      "%d",
      space,
      reg);
  }

  return out->handle;
}

static inline ac_result
ac_d3d12_create_buffer2(
  ac_d3d12_device*      device,
  const ac_buffer_info* info,
  D3D12_RESOURCE_STATES state,
  ID3D12Resource**      buffer,
  D3D12MA::Allocation** allocation)
{
  D3D12_RESOURCE_DESC1 resource_desc = {};
  resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  resource_desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
  resource_desc.Width = info->size;
  resource_desc.Height = 1;
  resource_desc.DepthOrArraySize = 1;
  resource_desc.MipLevels = 1;
  resource_desc.Format = DXGI_FORMAT_UNKNOWN;
  resource_desc.SampleDesc.Count = 1;
  resource_desc.SampleDesc.Quality = 0;
  resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

  D3D12MA::ALLOCATION_DESC alloc_desc = {};
  alloc_desc.HeapType = ac_memory_usage_to_d3d12(info->memory_usage);

  if (info->usage & ac_buffer_usage_cbv_bit)
  {
    resource_desc.Width = AC_ALIGN_UP(
      resource_desc.Width,
      device->common.props.cbv_buffer_alignment);
  }

#if (AC_D3D12_USE_ENHANCED_BARRIERS)
  if (device->enhanced_barriers)
  {
    AC_D3D12_RIF(device->allocator->CreateResource3(
      &alloc_desc,
      &resource_desc,
      D3D12_BARRIER_LAYOUT_UNDEFINED,
      NULL,
      0,
      NULL,
      allocation,
      AC_IID_PPV_ARGS(buffer)));
  }
  else
#endif
  {
    D3D12_RESOURCE_DESC resource_desc2 = {};
    resource_desc2.Dimension = resource_desc.Dimension;
    resource_desc2.Alignment = resource_desc.Alignment;
    resource_desc2.Width = resource_desc.Width;
    resource_desc2.Height = resource_desc.Height;
    resource_desc2.DepthOrArraySize = resource_desc.DepthOrArraySize;
    resource_desc2.MipLevels = resource_desc.MipLevels;
    resource_desc2.Format = resource_desc.Format;
    resource_desc2.SampleDesc = resource_desc.SampleDesc;
    resource_desc2.Layout = resource_desc.Layout;
    resource_desc2.Flags = resource_desc.Flags;

    AC_D3D12_RIF(device->allocator->CreateResource(
      &alloc_desc,
      &resource_desc2,
      state,
      NULL,
      allocation,
      AC_IID_PPV_ARGS(buffer)));
  }

  AC_D3D12_SET_OBJECT_NAME((*buffer), info->name);
  AC_D3D12_SET_OBJECT_NAME((*allocation), info->name);

  return ac_result_success;
}

static ac_result
ac_d3d12_create_cmd_pool(
  ac_device               device_handle,
  const ac_cmd_pool_info* info,
  ac_cmd_pool*            cmd_pool_handle)
{
  AC_INIT_INTERNAL(cmd_pool, ac_d3d12_cmd_pool);

  AC_FROM_HANDLE(device, ac_d3d12_device);

  AC_D3D12_RIF(device->device->CreateCommandAllocator(
    ac_queue_type_to_d3d12(info->queue->type),
    AC_IID_PPV_ARGS(&cmd_pool->command_allocator)));

  return ac_result_success;
}

static void
ac_d3d12_destroy_cmd_pool(ac_device device_handle, ac_cmd_pool cmd_pool_handle)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(cmd_pool, ac_d3d12_cmd_pool);

  AC_D3D12_SAFE_RELEASE(cmd_pool->command_allocator);
}

static ac_result
ac_d3d12_reset_cmd_pool(ac_device device_handle, ac_cmd_pool pool_handle)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(pool, ac_d3d12_cmd_pool);

  AC_D3D12_RIF(pool->command_allocator->Reset());

  return ac_result_success;
}

static ac_result
ac_d3d12_create_cmd(
  ac_device   device_handle,
  ac_cmd_pool cmd_pool_handle,
  ac_cmd*     cmd_handle)
{
  AC_FROM_HANDLE(device, ac_d3d12_device);
  AC_FROM_HANDLE(cmd_pool, ac_d3d12_cmd_pool);

  AC_INIT_INTERNAL(cmd, ac_d3d12_cmd);

  AC_D3D12_RIF(device->device->CreateCommandList(
    0,
    ac_queue_type_to_d3d12(cmd_pool->common.queue->type),
    cmd_pool->command_allocator,
    nullptr,
    AC_IID_PPV_ARGS(&cmd->cmd)));

  AC_D3D12_RIF(cmd->cmd->Close());

  return ac_result_success;
}

static void
ac_d3d12_destroy_cmd(
  ac_device   device_handle,
  ac_cmd_pool cmd_pool_handle,
  ac_cmd      cmd_handle)
{
  AC_UNUSED(device_handle);
  AC_UNUSED(cmd_pool_handle);

  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_D3D12_SAFE_RELEASE(cmd->cmd);
}

static ac_result
ac_d3d12_create_fence(
  ac_device            device_handle,
  const ac_fence_info* info,
  ac_fence*            fence_handle)
{
  AC_INIT_INTERNAL(fence, ac_d3d12_fence);

  AC_FROM_HANDLE(device, ac_d3d12_device);

  return ac_d3d12_create_fence2(device, info, fence);
}

static void
ac_d3d12_destroy_fence(ac_device device_handle, ac_fence fence_handle)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(fence, ac_d3d12_fence);

  ac_d3d12_destroy_fence2(fence);
}

static ac_result
ac_d3d12_get_fence_value(
  ac_device device_handle,
  ac_fence  fence_handle,
  uint64_t* value)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(fence, ac_d3d12_fence);

  *value = fence->fence->GetCompletedValue();

  return ac_result_success;
}

static ac_result
ac_d3d12_signal_fence(
  ac_device device_handle,
  ac_fence  fence_handle,
  uint64_t  value)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(fence, ac_d3d12_fence);

  AC_D3D12_RIF(fence->fence->Signal(value));

  return ac_result_success;
}

static ac_result
ac_d3d12_wait_fence(
  ac_device device_handle,
  ac_fence  fence_handle,
  uint64_t  value)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(fence, ac_d3d12_fence);

  if (fence->fence->GetCompletedValue() < value)
  {
    AC_D3D12_RIF(fence->fence->SetEventOnCompletion(value, fence->event));
    if (WaitForSingleObject(fence->event, INFINITE))
    {
      return ac_result_device_lost;
    }
  }

  return ac_result_success;
}

static ac_result
ac_d3d12_queue_wait_idle(ac_queue queue_handle)
{
  AC_FROM_HANDLE(queue, ac_d3d12_queue);

  ac_d3d12_fence* fence = &queue->fence;

  AC_D3D12_RIF(queue->queue->Signal(fence->fence, fence->value + 1));
  fence->value++;

  if (fence->fence->GetCompletedValue() < fence->value)
  {
    AC_D3D12_RIF(
      fence->fence->SetEventOnCompletion(fence->value, fence->event));
    if (WaitForSingleObject(fence->event, INFINITE))
    {
      return ac_result_device_lost;
    }
  }

  return ac_result_success;
}

static ac_result
ac_d3d12_queue_submit(ac_queue queue_handle, const ac_queue_submit_info* info)
{
  AC_FROM_HANDLE(queue, ac_d3d12_queue);

  for (uint32_t i = 0; i < info->wait_fence_count; ++i)
  {
    ac_fence_submit_info* fi = &info->wait_fences[i];
    AC_FROM_HANDLE2(fence, fi->fence, ac_d3d12_fence);
    uint64_t value;
    if (fence->common.bits & ac_fence_present_bit)
    {
      value = fence->value;
    }
    else
    {
      value = fi->value;
    }
    AC_D3D12_RIF(queue->queue->Wait(fence->fence, value));
  }

  ID3D12CommandList** cmds = static_cast<ID3D12CommandList**>(
    ac_alloc(info->cmd_count * sizeof(ID3D12CommandList*)));

  for (uint32_t i = 0; i < info->cmd_count; ++i)
  {
    AC_FROM_HANDLE2(cmd, info->cmds[i], ac_d3d12_cmd);
    cmds[i] = cmd->cmd;
  }

  queue->queue->ExecuteCommandLists(info->cmd_count, cmds);

  for (uint32_t i = 0; i < info->signal_fence_count; ++i)
  {
    ac_fence_submit_info* fi = &info->signal_fences[i];

    AC_FROM_HANDLE2(fence, fi->fence, ac_d3d12_fence);
    uint64_t value;
    if (fence->common.bits & ac_fence_present_bit)
    {
      value = fence->value + 1;
      fence->value++;
    }
    else
    {
      value = fi->value;
    }

    if (FAILED(queue->queue->Signal(fence->fence, value)))
    {
      ac_free(cmds);
      return ac_result_unknown_error;
    }
  }

  ac_free(cmds);
  return ac_result_success;
}

static ac_result
ac_d3d12_begin_cmd(ac_cmd cmd_handle)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_FROM_HANDLE2(pool, cmd->common.pool, ac_d3d12_cmd_pool);
  cmd->db = NULL;

  AC_D3D12_RIF(cmd->cmd->Reset(pool->command_allocator, NULL));

  return ac_result_success;
}

static ac_result
ac_d3d12_end_cmd(ac_cmd cmd_handle)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_D3D12_RIF(cmd->cmd->Close());
  return ac_result_success;
}

static ac_result
ac_d3d12_create_shader(
  ac_device             device_handle,
  const ac_shader_info* info,
  ac_shader*            shader_handle)
{
  AC_UNUSED(device_handle);
  AC_INIT_INTERNAL(shader, ac_d3d12_shader);

  uint32_t    size = 0;
  const void* bytecode =
    ac_shader_compiler_get_bytecode(info->code, &size, ac_shader_bytecode_dxil);

  if (!bytecode)
  {
    return ac_result_unknown_error;
  }

  void* stage = ac_calloc(size);
  memcpy(stage, bytecode, size);

  shader->bytecode.BytecodeLength = size;
  shader->bytecode.pShaderBytecode = stage;

  return ac_result_success;
}

static void
ac_d3d12_destroy_shader(ac_device device_handle, ac_shader shader_handle)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(shader, ac_d3d12_shader);

  ac_free(ac_const_cast(shader->bytecode.pShaderBytecode));
}

static ac_result
ac_d3d12_create_dsl(
  ac_device                   device_handle,
  const ac_dsl_info_internal* info,
  ac_dsl*                     dsl_handle)
{
  AC_INIT_INTERNAL(dsl, ac_d3d12_dsl);

  AC_FROM_HANDLE(device, ac_d3d12_device);

  // TODO: cleanup later, root params can be writen in reflection
  bool sampler_ranges[ac_space_count] = {};
  bool resource_ranges[ac_space_count] = {};

  dsl->handles = hashmap_new(
    sizeof(ac_d3d12_binding_handle),
    0,
    0,
    0,
    ac_d3d12_binding_handle_hash,
    ac_d3d12_binding_handle_compare,
    NULL,
    NULL);

  for (uint32_t i = 0; i < info->binding_count; ++i)
  {
    ac_shader_binding* b = &info->bindings[i];
    ac_d3d12_space*    space = &dsl->spaces[b->space];

    ac_d3d12_binding_handle bh = {};
    bh.reg = b->reg;
    bh.space = b->space;
    bh.type = static_cast<ac_descriptor_type>(b->type);

    if (bh.type == ac_descriptor_type_sampler)
    {
      sampler_ranges[b->space] = true;
      bh.handle = space->sampler.stride;
      space->sampler.stride += b->descriptor_count;
    }
    else
    {
      resource_ranges[b->space] = true;
      bh.handle = space->resource.stride;
      space->resource.stride += b->descriptor_count;
    }

    hashmap_set(dsl->handles, &bh);
  }

  uint32_t param_count = 0;

  for (uint32_t i = 0; i < ac_space_count; ++i)
  {
    ac_d3d12_space* space = &dsl->spaces[i];

    if (sampler_ranges[i])
    {
      space->sampler.root_param = param_count;
      param_count++;
    }

    if (resource_ranges[i])
    {
      space->resource.root_param = param_count;
      param_count++;
    }
  }

  dsl->push_constant = static_cast<uint32_t>(-1);

  for (uint32_t i = 0; i < info->info.shader_count; ++i)
  {
    if (ac_shader_compiler_has_push_constant(info->info.shaders[i]->reflection))
    {
      dsl->push_constant = param_count;
    }
  }

  {
    AC_FROM_HANDLE2(shader, info->info.shaders[0], ac_d3d12_shader);
    AC_D3D12_RIF(device->device->CreateRootSignature(
      0,
      shader->bytecode.pShaderBytecode,
      shader->bytecode.BytecodeLength,
      AC_IID_PPV_ARGS(&dsl->signature)));
  }

  AC_D3D12_SET_OBJECT_NAME(dsl->signature, info->info.name);

  return ac_result_success;
};

static void
ac_d3d12_destroy_dsl(ac_device device_handle, ac_dsl dsl_handle)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(dsl, ac_d3d12_dsl);

  hashmap_free(dsl->handles);
  AC_D3D12_SAFE_RELEASE(dsl->signature);
}

static ac_result
ac_d3d12_create_descriptor_buffer(
  ac_device                        device_handle,
  const ac_descriptor_buffer_info* info,
  ac_descriptor_buffer*            db_handle)
{
  AC_INIT_INTERNAL(db, ac_d3d12_descriptor_buffer);

  AC_FROM_HANDLE(device, ac_d3d12_device);
  AC_FROM_HANDLE2(dsl, info->dsl, ac_d3d12_dsl);

  uint64_t sampler_count = 0;
  uint64_t resource_count = 0;
  for (uint32_t i = 0; i < ac_space_count; ++i)
  {
    ac_d3d12_space* space = &dsl->spaces[i];
    space->sampler.offset = sampler_count;
    space->resource.offset = resource_count;

    sampler_count += space->sampler.stride * info->max_sets[i];
    resource_count += space->resource.stride * info->max_sets[i];
  }

  if (sampler_count)
  {
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};

    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.NumDescriptors = static_cast<UINT>(sampler_count);

    AC_D3D12_RIF(device->device->CreateDescriptorHeap(
      &heap_desc,
      AC_IID_PPV_ARGS(&db->sampler_heap)));

    AC_D3D12_SET_OBJECT_NAME(db->sampler_heap, info->name);
  }

  if (resource_count)
  {
    D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};

    heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    heap_desc.NumDescriptors = static_cast<UINT>(resource_count);

    AC_D3D12_RIF(device->device->CreateDescriptorHeap(
      &heap_desc,
      AC_IID_PPV_ARGS(&db->resource_heap)));

    AC_D3D12_SET_OBJECT_NAME(db->resource_heap, info->name);
  }

  db->handles = hashmap_new(
    sizeof(ac_d3d12_binding_handle),
    0,
    0,
    0,
    ac_d3d12_binding_handle_hash,
    ac_d3d12_binding_handle_compare,
    NULL,
    NULL);

  size_t                   iter = 0;
  ac_d3d12_binding_handle* binding;
  while (hashmap_iter(dsl->handles, &iter, reinterpret_cast<void**>(&binding)))
  {
    hashmap_set(db->handles, binding);
  }

  memcpy(db->spaces, dsl->spaces, sizeof(dsl->spaces));
  db->signature = dsl->signature;
  db->signature->AddRef();

  return ac_result_success;
}

static void
ac_d3d12_destroy_descriptor_buffer(
  ac_device            device_handle,
  ac_descriptor_buffer db_handle)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(db, ac_d3d12_descriptor_buffer);

  AC_D3D12_SAFE_RELEASE(db->sampler_heap);
  AC_D3D12_SAFE_RELEASE(db->resource_heap);
  AC_D3D12_SAFE_RELEASE(db->signature);

  if (db->handles)
  {
    hashmap_free(db->handles);
  }
}

static ac_result
ac_d3d12_create_graphics_pipeline(
  ac_device               device_handle,
  const ac_pipeline_info* info,
  ac_pipeline*            pipeline_handle)
{
  AC_INIT_INTERNAL(pipeline, ac_d3d12_pipeline);

  AC_FROM_HANDLE(device, ac_d3d12_device);
  AC_FROM_HANDLE2(dsl, info->graphics.dsl, ac_d3d12_dsl);

  const ac_graphics_pipeline_info* graphics = &info->graphics;

  pipeline->push_constant = dsl->push_constant;

  D3D12_STREAM_OUTPUT_DESC stream_output;
  AC_ZERO(stream_output);

  DXGI_SAMPLE_DESC sample_desc;
  AC_ZERO(sample_desc);
  sample_desc.Count = graphics->samples;
  sample_desc.Quality = 0;

  D3D12_BLEND_DESC blend_state;
  AC_ZERO(blend_state);
  blend_state.AlphaToCoverageEnable = FALSE;
  blend_state.IndependentBlendEnable = FALSE;

  for (uint32_t i = 0; i < graphics->color_attachment_count; ++i)
  {
    D3D12_RENDER_TARGET_BLEND_DESC* out = &blend_state.RenderTarget[i];

    const ac_blend_attachment_state* in =
      &graphics->blend_state_info.attachment_states[i];

    out->BlendEnable = (in->src_factor != ac_blend_factor_zero) ||
                       (in->src_alpha_factor != ac_blend_factor_zero) ||
                       (in->dst_factor != ac_blend_factor_zero) ||
                       (in->dst_alpha_factor != ac_blend_factor_zero);
    out->LogicOpEnable = FALSE;
    out->SrcBlend = ac_blend_factor_to_d3d12(in->src_factor);
    out->DestBlend = ac_blend_factor_to_d3d12(in->dst_factor);
    out->BlendOp = ac_blend_op_to_d3d12(in->op);
    out->SrcBlendAlpha = ac_blend_factor_to_d3d12(in->src_alpha_factor);
    out->DestBlendAlpha = ac_blend_factor_to_d3d12(in->dst_alpha_factor);
    out->BlendOpAlpha = ac_blend_op_to_d3d12(in->alpha_op);
    out->LogicOp = D3D12_LOGIC_OP_NOOP;
    out->RenderTargetWriteMask = (~graphics->color_attachment_discard_bits[i]) &
                                 (ac_channel_r_bit | ac_channel_g_bit |
                                  ac_channel_b_bit | ac_channel_a_bit);

    if (
      i > 0 && blend_state.RenderTarget[i - 1].BlendEnable != out->BlendEnable)
    {
      blend_state.IndependentBlendEnable = TRUE;
    }
  }

  D3D12_RASTERIZER_DESC rs_state;
  AC_ZERO(rs_state);
  rs_state.FillMode =
    ac_polygon_mode_to_d3d12(graphics->rasterizer_info.polygon_mode);
  rs_state.CullMode =
    ac_cull_mode_to_d3d12(graphics->rasterizer_info.cull_mode);
  rs_state.FrontCounterClockwise =
    (graphics->rasterizer_info.front_face != ac_front_face_clockwise);
  rs_state.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  rs_state.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  rs_state.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  rs_state.DepthClipEnable = TRUE;
  if (graphics->rasterizer_info.depth_bias_enable)
  {
    rs_state.DepthBias = graphics->rasterizer_info.depth_bias_constant_factor;
    rs_state.DepthBiasClamp = 0.0f;
    rs_state.SlopeScaledDepthBias =
      graphics->rasterizer_info.depth_bias_slope_factor;
  }
  rs_state.MultisampleEnable = graphics->samples > 1;
  rs_state.AntialiasedLineEnable = FALSE;
  rs_state.ForcedSampleCount = 0;
  rs_state.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  D3D12_DEPTH_STENCIL_DESC depth_stencil_state;
  AC_ZERO(depth_stencil_state);
  depth_stencil_state.DepthEnable = graphics->depth_state_info.depth_test;
  depth_stencil_state.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
  if (graphics->depth_state_info.depth_write)
  {
    depth_stencil_state.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  }
  depth_stencil_state.DepthFunc =
    ac_compare_op_to_d3d12(graphics->depth_state_info.compare_op);
  // depth_stencil_state.StencilEnable;
  // depth_stencil_state.StencilReadMask;
  // depth_stencil_state.StencilWriteMask;
  // depth_stencil_state.FrontFace;
  // depth_stencil_state.BackFace;

  // TODO:
  depth_stencil_state.StencilEnable = false;

  ac_vertex_binding_info bindings[AC_MAX_VERTEX_BINDING_COUNT];
  AC_ZERO(bindings);

  for (uint32_t i = 0; i < graphics->vertex_layout.binding_count; ++i)
  {
    const ac_vertex_binding_info* in = &graphics->vertex_layout.bindings[i];
    bindings[in->binding] = *in;
  }

  D3D12_INPUT_ELEMENT_DESC input_elements[AC_MAX_VERTEX_ATTRIBUTE_COUNT];
  AC_ZERO(input_elements);

  for (uint32_t i = 0; i < graphics->vertex_layout.attribute_count; ++i)
  {
    const ac_vertex_attribute_info* in = &graphics->vertex_layout.attributes[i];
    const ac_vertex_binding_info*   binding = &bindings[in->binding];

    D3D12_INPUT_ELEMENT_DESC* out = &input_elements[i];
    ac_attribute_semantic_to_d3d12(
      in->semantic,
      &out->SemanticName,
      &out->SemanticIndex);
    out->Format = ac_format_to_d3d12(in->format);
    out->InputSlot = binding->binding;
    out->AlignedByteOffset = in->offset;
    out->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    out->InstanceDataStepRate = 0;
    if (binding->input_rate == ac_input_rate_instance)
    {
      out->InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA;
      out->InstanceDataStepRate = 1;
    }

    pipeline->vertex_strides[binding->binding] = binding->stride;
  }

  D3D12_INPUT_LAYOUT_DESC input_layout;
  AC_ZERO(input_layout);
  input_layout.NumElements = graphics->vertex_layout.attribute_count;
  input_layout.pInputElementDescs = input_elements;

  AC_FROM_HANDLE2(vertex_shader, graphics->vertex_shader, ac_d3d12_shader);

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pipe_desc = {};
  pipe_desc.pRootSignature = dsl->signature;
  pipe_desc.VS = vertex_shader->bytecode;
  if (graphics->pixel_shader)
  {
    AC_FROM_HANDLE2(pixel_shader, graphics->pixel_shader, ac_d3d12_shader);
    pipe_desc.PS = pixel_shader->bytecode;
  }
  pipe_desc.StreamOutput = stream_output;
  pipe_desc.BlendState = blend_state;
  pipe_desc.SampleMask = UINT_MAX;
  pipe_desc.RasterizerState = rs_state;
  pipe_desc.DepthStencilState = depth_stencil_state;
  pipe_desc.InputLayout = input_layout;
  pipe_desc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
  pipe_desc.PrimitiveTopologyType =
    ac_primitive_topology_type_to_d3d12(graphics->topology);
  pipe_desc.NumRenderTargets = graphics->color_attachment_count;
  for (uint32_t i = 0; i < graphics->color_attachment_count; ++i)
  {
    pipe_desc.RTVFormats[i] =
      ac_format_to_d3d12_attachment(graphics->color_attachment_formats[i]);
  }
  pipe_desc.DSVFormat =
    ac_format_to_d3d12_attachment(graphics->depth_stencil_format);
  pipe_desc.SampleDesc = sample_desc;
  pipe_desc.NodeMask = 0;
  pipe_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

  AC_D3D12_RIF(device->device->CreateGraphicsPipelineState(
    &pipe_desc,
    AC_IID_PPV_ARGS(&pipeline->pipeline)));

  AC_D3D12_SET_OBJECT_NAME(pipeline->pipeline, info->name);

  pipeline->topology = ac_primitive_topology_to_d3d12(graphics->topology);
  pipeline->signature = dsl->signature;
  pipeline->signature->AddRef();

  return ac_result_success;
}

static ac_result
ac_d3d12_create_mesh_pipeline(
  ac_device               device_handle,
  const ac_pipeline_info* info,
  ac_pipeline*            pipeline_handle)
{
#if (AC_D3D12_USE_MESH_SHADERS)
  AC_INIT_INTERNAL(pipeline, ac_d3d12_pipeline);

  AC_FROM_HANDLE(device, ac_d3d12_device);
  AC_FROM_HANDLE2(dsl, info->mesh.dsl, ac_d3d12_dsl);

  const ac_mesh_pipeline_info* mesh = &info->mesh;

  pipeline->push_constant = dsl->push_constant;

  DXGI_SAMPLE_DESC sample_desc;
  AC_ZERO(sample_desc);
  sample_desc.Count = mesh->samples;
  sample_desc.Quality = 0;

  D3D12_BLEND_DESC blend_state;
  AC_ZERO(blend_state);
  blend_state.AlphaToCoverageEnable = FALSE;
  blend_state.IndependentBlendEnable = FALSE;

  for (uint32_t i = 0; i < mesh->color_attachment_count; ++i)
  {
    D3D12_RENDER_TARGET_BLEND_DESC* out = &blend_state.RenderTarget[i];

    const ac_blend_attachment_state* in =
      &mesh->blend_state_info.attachment_states[i];

    out->BlendEnable = (in->src_factor != ac_blend_factor_zero) ||
                       (in->src_alpha_factor != ac_blend_factor_zero) ||
                       (in->dst_factor != ac_blend_factor_zero) ||
                       (in->dst_alpha_factor != ac_blend_factor_zero);
    out->LogicOpEnable = FALSE;
    out->SrcBlend = ac_blend_factor_to_d3d12(in->src_factor);
    out->DestBlend = ac_blend_factor_to_d3d12(in->dst_factor);
    out->BlendOp = ac_blend_op_to_d3d12(in->op);
    out->SrcBlendAlpha = ac_blend_factor_to_d3d12(in->src_alpha_factor);
    out->DestBlendAlpha = ac_blend_factor_to_d3d12(in->dst_alpha_factor);
    out->BlendOpAlpha = ac_blend_op_to_d3d12(in->alpha_op);
    out->LogicOp = D3D12_LOGIC_OP_NOOP;
    out->RenderTargetWriteMask = (~mesh->color_attachment_discard_bits[i]) &
                                 (ac_channel_r_bit | ac_channel_g_bit |
                                  ac_channel_b_bit | ac_channel_a_bit);

    if (
      i > 0 && blend_state.RenderTarget[i - 1].BlendEnable != out->BlendEnable)
    {
      blend_state.IndependentBlendEnable = TRUE;
    }
  }

  D3D12_RASTERIZER_DESC rs_state;
  AC_ZERO(rs_state);
  rs_state.FillMode =
    ac_polygon_mode_to_d3d12(mesh->rasterizer_info.polygon_mode);
  rs_state.CullMode = ac_cull_mode_to_d3d12(mesh->rasterizer_info.cull_mode);
  rs_state.FrontCounterClockwise =
    (mesh->rasterizer_info.front_face != ac_front_face_clockwise);
  rs_state.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  rs_state.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  rs_state.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  rs_state.DepthClipEnable = TRUE;
  if (mesh->rasterizer_info.depth_bias_enable)
  {
    rs_state.DepthBias = mesh->rasterizer_info.depth_bias_constant_factor;
    rs_state.DepthBiasClamp = 0.0f;
    rs_state.SlopeScaledDepthBias =
      mesh->rasterizer_info.depth_bias_slope_factor;
  }
  rs_state.MultisampleEnable = mesh->samples > 1;
  rs_state.AntialiasedLineEnable = FALSE;
  rs_state.ForcedSampleCount = 0;
  rs_state.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  D3D12_DEPTH_STENCIL_DESC depth_stencil_state;
  AC_ZERO(depth_stencil_state);
  depth_stencil_state.DepthEnable = mesh->depth_state_info.depth_test;
  depth_stencil_state.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
  if (mesh->depth_state_info.depth_write)
  {
    depth_stencil_state.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  }
  depth_stencil_state.DepthFunc =
    ac_compare_op_to_d3d12(mesh->depth_state_info.compare_op);
  // depth_stencil_state.StencilEnable;
  // depth_stencil_state.StencilReadMask;
  // depth_stencil_state.StencilWriteMask;
  // depth_stencil_state.FrontFace;
  // depth_stencil_state.BackFace;

  // TODO:
  depth_stencil_state.StencilEnable = false;

  D3DX12_MESH_SHADER_PIPELINE_STATE_DESC desc = {};
  desc.pRootSignature = dsl->signature;

  AC_FROM_HANDLE2(mesh_shader, mesh->mesh_shader, ac_d3d12_shader);
  desc.MS = mesh_shader->bytecode;

  if (mesh->task_shader)
  {
    AC_FROM_HANDLE2(task_shader, mesh->task_shader, ac_d3d12_shader);
    desc.AS = task_shader->bytecode;
  }

  if (mesh->pixel_shader)
  {
    AC_FROM_HANDLE2(pixel_shader, mesh->pixel_shader, ac_d3d12_shader);
    desc.PS = pixel_shader->bytecode;
  }
  desc.BlendState = blend_state;
  desc.SampleMask = UINT_MAX;
  desc.RasterizerState = rs_state;
  desc.DepthStencilState = depth_stencil_state;
  desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_UNDEFINED;
  desc.NumRenderTargets = mesh->color_attachment_count;
  for (uint32_t i = 0; i < mesh->color_attachment_count; ++i)
  {
    desc.RTVFormats[i] =
      ac_format_to_d3d12_attachment(mesh->color_attachment_formats[i]);
  }
  desc.DSVFormat = ac_format_to_d3d12_attachment(mesh->depth_stencil_format);
  desc.SampleDesc = sample_desc;
  desc.NodeMask = 0;
  desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

  CD3DX12_PIPELINE_MESH_STATE_STREAM stream =
    CD3DX12_PIPELINE_MESH_STATE_STREAM(desc);

  D3D12_PIPELINE_STATE_STREAM_DESC stream_desc;
  stream_desc.pPipelineStateSubobjectStream = &stream;
  stream_desc.SizeInBytes = sizeof(stream);

  AC_D3D12_RIF(device->device->CreatePipelineState(
    &stream_desc,
    AC_IID_PPV_ARGS(&pipeline->pipeline)));

  AC_D3D12_SET_OBJECT_NAME(pipeline->pipeline, info->name);

  pipeline->signature = dsl->signature;
  pipeline->signature->AddRef();

  return ac_result_success;
#else
  AC_UNUSED(device_handle);
  AC_UNUSED(info);
  *pipeline_handle = NULL;
  return ac_result_unknown_error;
#endif
}

static ac_result
ac_d3d12_create_compute_pipeline(
  ac_device               device_handle,
  const ac_pipeline_info* info,
  ac_pipeline*            pipeline_handle)
{
  AC_INIT_INTERNAL(pipeline, ac_d3d12_pipeline);

  AC_FROM_HANDLE(device, ac_d3d12_device);
  AC_FROM_HANDLE2(dsl, info->compute.dsl, ac_d3d12_dsl);
  AC_FROM_HANDLE2(shader, info->compute.shader, ac_d3d12_shader);

  pipeline->push_constant = dsl->push_constant;

  D3D12_COMPUTE_PIPELINE_STATE_DESC pipe_desc = {};
  pipe_desc.pRootSignature = dsl->signature;
  pipe_desc.CS = shader->bytecode;
  pipe_desc.NodeMask = 0;
  pipe_desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

  AC_D3D12_RIF(device->device->CreateComputePipelineState(
    &pipe_desc,
    AC_IID_PPV_ARGS(&pipeline->pipeline)));

  AC_D3D12_SET_OBJECT_NAME(pipeline->pipeline, info->name);

  pipeline->signature = dsl->signature;
  pipeline->signature->AddRef();

  return ac_result_success;
}

static ac_result
ac_d3d12_create_raytracing_pipeline(
  ac_device               device_handle,
  const ac_pipeline_info* info,
  ac_pipeline*            pipeline_handle)
{
  AC_UNUSED(device_handle);
  AC_UNUSED(info);
  AC_INIT_INTERNAL(pipeline, ac_d3d12_pipeline);

  return ac_result_unknown_error;
}

static void
ac_d3d12_destroy_pipeline(ac_device device_handle, ac_pipeline pipeline_handle)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(pipeline, ac_d3d12_pipeline);
  AC_D3D12_SAFE_RELEASE(pipeline->signature);
  AC_D3D12_SAFE_RELEASE(pipeline->pipeline);
}

static ac_result
ac_d3d12_create_image(
  ac_device            device_handle,
  const ac_image_info* info,
  ac_image*            image_handle)
{
  AC_INIT_INTERNAL(image, ac_d3d12_image);
  AC_FROM_HANDLE(device, ac_d3d12_device);

  D3D12_RESOURCE_DESC1 resource_desc = {};
  resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  resource_desc.Alignment =
    info->samples > 1 ? D3D12_DEFAULT_MSAA_RESOURCE_PLACEMENT_ALIGNMENT : 0ull;
  resource_desc.Width = info->width;
  resource_desc.Height = info->height;
  resource_desc.DepthOrArraySize = AC_MAX(1u, info->layers);
  resource_desc.MipLevels = info->levels;
  resource_desc.Format = ac_format_to_d3d12(info->format);
  resource_desc.SampleDesc.Count = info->samples;
  resource_desc.SampleDesc.Quality = 0;
  resource_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

  D3D12MA::ALLOCATION_DESC alloc_desc = {};
  alloc_desc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

  D3D12_CLEAR_VALUE clear_value = {};
  clear_value.Format = ac_format_to_d3d12_attachment(info->format);
  D3D12_CLEAR_VALUE* p_clear_value = NULL;

  if (info->usage & ac_image_usage_attachment_bit)
  {
    clear_value.Color[0] = info->clear_value.color[0];
    clear_value.Color[1] = info->clear_value.color[1];
    clear_value.Color[2] = info->clear_value.color[2];
    clear_value.Color[3] = info->clear_value.color[3];
    clear_value.DepthStencil.Depth = info->clear_value.depth;
    clear_value.DepthStencil.Stencil = info->clear_value.stencil;
    p_clear_value = &clear_value;

    if (ac_format_depth_or_stencil(info->format))
    {
      resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
    }
    else
    {
      resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
    }
  }

  if (info->usage & ac_image_usage_uav_bit)
  {
    resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
  }

#if (AC_D3D12_USE_ENHANCED_BARRIERS)

  if (device->enhanced_barriers)
  {
    AC_D3D12_RIF(device->allocator->CreateResource3(
      &alloc_desc,
      &resource_desc,
      D3D12_BARRIER_LAYOUT_UNDEFINED,
      p_clear_value,
      0,
      NULL,
      &image->allocation,
      AC_IID_PPV_ARGS(&image->resource)));
  }
  else
#endif
  {
    D3D12_RESOURCE_DESC resource_desc2 = {};
    resource_desc2.Dimension = resource_desc.Dimension;
    resource_desc2.Alignment = resource_desc.Alignment;
    resource_desc2.Width = resource_desc.Width;
    resource_desc2.Height = resource_desc.Height;
    resource_desc2.DepthOrArraySize = resource_desc.DepthOrArraySize;
    resource_desc2.MipLevels = resource_desc.MipLevels;
    resource_desc2.Format = resource_desc.Format;
    resource_desc2.SampleDesc = resource_desc.SampleDesc;
    resource_desc2.Layout = resource_desc.Layout;
    resource_desc2.Flags = resource_desc.Flags;

    AC_D3D12_RIF(device->allocator->CreateResource(
      &alloc_desc,
      &resource_desc2,
      D3D12_RESOURCE_STATE_COMMON,
      p_clear_value,
      &image->allocation,
      AC_IID_PPV_ARGS(&image->resource)));
  }

  AC_D3D12_SET_OBJECT_NAME(image->resource, info->name);
  AC_D3D12_SET_OBJECT_NAME(image->allocation, info->name);

  image->handle = AC_D3D12_INVALID_HANDLE;

  if (info->usage & ac_image_usage_attachment_bit)
  {
    if (ac_format_depth_or_stencil(info->format))
    {
      image->handle = ac_d3d12_cpu_heap_alloc_handle(&device->dsv_heap);

      D3D12_DEPTH_STENCIL_VIEW_DESC view_desc = {};
      view_desc.Format = ac_format_to_d3d12_attachment(info->format);

      switch (info->type)
      {
      case ac_image_type_1d:
        view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
        break;
      case ac_image_type_1d_array:
        view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
        break;
      case ac_image_type_2d:
        view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        if (info->samples > 1)
        {
          view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        }
        break;
      case ac_image_type_2d_array:
        if (info->samples > 1)
        {
          view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
          view_desc.Texture2DMSArray.ArraySize = info->layers;
          view_desc.Texture2DMSArray.FirstArraySlice = 0;
        }
        else
        {
          view_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
          view_desc.Texture2DArray.ArraySize = info->layers;
        }
        break;
      default:
        return ac_result_invalid_argument;
      }

      device->device->CreateDepthStencilView(
        image->resource,
        &view_desc,
        ac_d3d12_get_cpu_handle(
          device->dsv_heap.heap,
          device->dsv_descriptor_size,
          image->handle));
    }
    else
    {
      image->handle = ac_d3d12_cpu_heap_alloc_handle(&device->rtv_heap);
      device->device->CreateRenderTargetView(
        image->resource,
        nullptr,
        ac_d3d12_get_cpu_handle(
          device->rtv_heap.heap,
          device->rtv_descriptor_size,
          image->handle));
    }
  }

  return ac_result_success;
}

static void
ac_d3d12_destroy_image(ac_device device_handle, ac_image image_handle)
{
  AC_FROM_HANDLE(device, ac_d3d12_device);
  AC_FROM_HANDLE(image, ac_d3d12_image);

  if (image->common.usage & ac_image_usage_attachment_bit)
  {
    if (ac_format_depth_or_stencil(image->common.format))
    {
      ac_d3d12_cpu_heap_free_handle(&device->dsv_heap, image->handle);
    }
    else
    {
      ac_d3d12_cpu_heap_free_handle(&device->rtv_heap, image->handle);
    }
  }

  AC_D3D12_SAFE_RELEASE(image->resource);
  AC_D3D12_SAFE_RELEASE(image->allocation);
}

static ac_result
ac_d3d12_create_buffer(
  ac_device             device_handle,
  const ac_buffer_info* info,
  ac_buffer*            buffer_handle)
{
  AC_INIT_INTERNAL(buffer, ac_d3d12_buffer);

  AC_FROM_HANDLE(device, ac_d3d12_device);

  D3D12_RESOURCE_STATES state = D3D12_RESOURCE_STATE_GENERIC_READ;
  if (info->usage & ac_buffer_usage_uav_bit)
  {
    state |= D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
  }

  return ac_d3d12_create_buffer2(
    device,
    info,
    state,
    &buffer->resource,
    &buffer->allocation);
}

static void
ac_d3d12_destroy_buffer(ac_device device_handle, ac_buffer buffer_handle)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(buffer, ac_d3d12_buffer);

  if (buffer->resource && buffer->common.mapped_memory)
  {
    buffer->resource->Unmap(0, NULL);
  }

  AC_D3D12_SAFE_RELEASE(buffer->resource);
  AC_D3D12_SAFE_RELEASE(buffer->allocation);
}

static ac_result
ac_d3d12_map_memory(ac_device device_handle, ac_buffer buffer_handle)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(buffer, ac_d3d12_buffer);
  AC_D3D12_RIF(buffer->resource->Map(0, NULL, &buffer->common.mapped_memory));
  return ac_result_success;
}

static void
ac_d3d12_unmap_memory(ac_device device_handle, ac_buffer buffer_handle)
{
  AC_UNUSED(device_handle);
  AC_FROM_HANDLE(buffer, ac_d3d12_buffer);
  buffer->resource->Unmap(0, NULL);
}

static ac_result
ac_d3d12_create_sampler(
  ac_device              device_handle,
  const ac_sampler_info* info,
  ac_sampler*            sampler_handle)
{
  AC_UNUSED(device_handle);
  AC_INIT_INTERNAL(sampler, ac_d3d12_sampler);

  D3D12_SAMPLER_DESC desc = {};
  desc.Filter = ac_filter_to_d3d12(
    info->min_filter,
    info->mag_filter,
    info->mipmap_mode,
    info->anisotropy_enable,
    info->compare_enable);
  desc.AddressU = ac_address_mode_to_d3d12(info->address_mode_u);
  desc.AddressV = ac_address_mode_to_d3d12(info->address_mode_v);
  desc.AddressW = ac_address_mode_to_d3d12(info->address_mode_w);
  desc.MipLODBias = info->mip_lod_bias;
  desc.MaxAnisotropy = static_cast<UINT>(info->max_anisotropy);
  desc.ComparisonFunc = ac_compare_op_to_d3d12(info->compare_op);
  desc.BorderColor[0] = 0.0f;
  desc.BorderColor[1] = 0.0f;
  desc.BorderColor[2] = 0.0f;
  desc.BorderColor[3] = 0.0f;
  desc.MinLOD = info->min_lod;
  desc.MaxLOD = info->max_lod;

  sampler->desc = desc;

  return ac_result_success;
}

static void
ac_d3d12_destroy_sampler(ac_device device_handle, ac_sampler sampler_handle)
{
  AC_UNUSED(device_handle);
  AC_UNUSED(sampler_handle);
}

static void
ac_d3d12_update_descriptor(
  ac_device                  device_handle,
  ac_descriptor_buffer       db_handle,
  ac_space                   space_index,
  uint32_t                   set,
  const ac_descriptor_write* write)
{
  AC_FROM_HANDLE(device, ac_d3d12_device);
  AC_FROM_HANDLE(db, ac_d3d12_descriptor_buffer);

  const ac_d3d12_space* space = &db->spaces[space_index];

  uint64_t              handle = 0;
  uint32_t              descriptor_size = 0;
  ID3D12DescriptorHeap* heap = NULL;

  if (write->type == ac_descriptor_type_sampler)
  {
    handle += space->sampler.offset + space->sampler.stride * set;
    heap = db->sampler_heap;
    descriptor_size = device->sampler_descriptor_size;
  }
  else
  {
    handle += space->resource.offset + space->resource.stride * set;
    heap = db->resource_heap;
    descriptor_size = device->resource_descriptor_size;
  }

  handle +=
    ac_d3d12_get_binding_handle(db, space_index, write->reg, write->type);

  for (uint32_t j = 0; j < write->count; ++j)
  {
    const ac_descriptor* d = &write->descriptors[j];

    switch (write->type)
    {
    case ac_descriptor_type_sampler:
    {
      AC_FROM_HANDLE2(sampler, d->sampler, ac_d3d12_sampler);

      D3D12_CPU_DESCRIPTOR_HANDLE dst =
        ac_d3d12_get_cpu_handle(heap, descriptor_size, handle + j);
      device->device->CreateSampler(&sampler->desc, dst);
      break;
    }
    case ac_descriptor_type_uav_image:
    {
      ID3D12Resource*             resource = NULL;
      D3D12_CPU_DESCRIPTOR_HANDLE dst =
        ac_d3d12_get_cpu_handle(heap, descriptor_size, handle + j);
      D3D12_UNORDERED_ACCESS_VIEW_DESC view_desc = {};
      view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;

      if (d->image)
      {
        AC_FROM_HANDLE2(image, d->image, ac_d3d12_image);

        resource = image->resource;
        view_desc.Format = ac_format_to_d3d12(image->common.format);

        switch (image->common.type)
        {
        case ac_image_type_1d:
          view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
          break;
        case ac_image_type_1d_array:
          view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
          break;
        case ac_image_type_2d:
          view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
          view_desc.Texture2D.MipSlice = d->level;
          break;
        case ac_image_type_2d_array:
        case ac_image_type_cube:
          view_desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
          view_desc.Texture2DArray.ArraySize = image->common.layers;
          view_desc.Texture2DArray.MipSlice = d->level;
          break;
        default:
          AC_ASSERT(false);
          break;
        }
      }

      device->device
        ->CreateUnorderedAccessView(resource, NULL, &view_desc, dst);

      break;
    }
    case ac_descriptor_type_srv_image:
    {
      ID3D12Resource*             resource = NULL;
      D3D12_CPU_DESCRIPTOR_HANDLE dst =
        ac_d3d12_get_cpu_handle(heap, descriptor_size, handle + j);
      D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = {};
      view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;

      if (d->image)
      {
        AC_FROM_HANDLE2(image, d->image, ac_d3d12_image);

        resource = image->resource;

        view_desc.Format = ac_format_to_d3d12_srv(image->common.format);
        view_desc.Shader4ComponentMapping =
          D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        switch (image->common.type)
        {
        case ac_image_type_1d:
          view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
          break;
        case ac_image_type_1d_array:
          view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
          break;
        case ac_image_type_2d:
          view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
          view_desc.Texture2D.MipLevels = image->common.levels;
          if (image->common.samples > 1)
          {
            view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMS;
          }
          break;
        case ac_image_type_2d_array:
          if (image->common.samples > 1)
          {
            view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY;
            view_desc.Texture2DMSArray.ArraySize = image->common.layers;
            view_desc.Texture2DMSArray.FirstArraySlice = 0;
          }
          else
          {
            view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
            view_desc.Texture2DArray.ArraySize = image->common.layers;
            view_desc.Texture2DArray.MipLevels = image->common.levels;
          }
          break;
        case ac_image_type_cube:
          view_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
          view_desc.TextureCube.MipLevels = image->common.levels;
          break;
        default:
          AC_ASSERT(false);
          break;
        }
      }

      device->device->CreateShaderResourceView(resource, &view_desc, dst);

      break;
    }
    case ac_descriptor_type_uav_buffer:
    {
      ID3D12Resource*             resource = NULL;
      D3D12_CPU_DESCRIPTOR_HANDLE dst =
        ac_d3d12_get_cpu_handle(heap, descriptor_size, handle + j);

      D3D12_UNORDERED_ACCESS_VIEW_DESC view_desc = {};
      view_desc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;

      if (d->buffer)
      {
        AC_FROM_HANDLE2(buffer, d->buffer, ac_d3d12_buffer);

        resource = buffer->resource;

        uint64_t range = (d->range == AC_WHOLE_SIZE)
                           ? (buffer->common.size - d->offset)
                           : d->range;

        view_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
        view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
        view_desc.Buffer.FirstElement = (d->offset / 4);
        view_desc.Buffer.NumElements = static_cast<UINT>(range / 4);
        view_desc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_RAW;
      }

      device->device
        ->CreateUnorderedAccessView(resource, NULL, &view_desc, dst);

      break;
    }
    case ac_descriptor_type_srv_buffer:
    {
      ID3D12Resource*             resource = NULL;
      D3D12_CPU_DESCRIPTOR_HANDLE dst =
        ac_d3d12_get_cpu_handle(heap, descriptor_size, handle + j);

      D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = {};
      view_desc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;

      if (d->buffer)
      {
        AC_FROM_HANDLE2(buffer, d->buffer, ac_d3d12_buffer);

        resource = buffer->resource;

        uint64_t range = (d->range == AC_WHOLE_SIZE)
                           ? (buffer->common.size - d->offset)
                           : d->range;

        view_desc.Shader4ComponentMapping =
          D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        view_desc.Format = DXGI_FORMAT_R32_TYPELESS;
        view_desc.Buffer.FirstElement = (d->offset / 4);
        view_desc.Buffer.NumElements = static_cast<UINT>(range / 4);
        view_desc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_RAW;
      }

      device->device->CreateShaderResourceView(resource, &view_desc, dst);

      break;
    }
    case ac_descriptor_type_cbv_buffer:
    {
      AC_FROM_HANDLE2(buffer, d->buffer, ac_d3d12_buffer);

      uint64_t size = (d->range == AC_WHOLE_SIZE)
                        ? (buffer->common.size - d->offset)
                        : d->range;

      size = AC_ALIGN_UP(size, device->common.props.cbv_buffer_alignment);

      D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc = {};
      D3D12_CPU_DESCRIPTOR_HANDLE     dst =
        ac_d3d12_get_cpu_handle(heap, descriptor_size, handle + j);
      cbv_desc.BufferLocation =
        buffer->resource->GetGPUVirtualAddress() + d->offset;
      cbv_desc.SizeInBytes = static_cast<UINT>(size);

      device->device->CreateConstantBufferView(&cbv_desc, dst);
      break;
    }
#if (AC_D3D12_USE_RAYTRACING)
    case ac_descriptor_type_as:
    {
      AC_FROM_HANDLE2(as, d->as, ac_d3d12_as);

      D3D12_CPU_DESCRIPTOR_HANDLE dst =
        ac_d3d12_get_cpu_handle(heap, descriptor_size, handle + j);

      D3D12_SHADER_RESOURCE_VIEW_DESC view_desc = {};
      view_desc.ViewDimension =
        D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE;
      view_desc.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
      view_desc.Format = DXGI_FORMAT_UNKNOWN;
      view_desc.RaytracingAccelerationStructure.Location =
        as->resource->GetGPUVirtualAddress();

      device->device->CreateShaderResourceView(NULL, &view_desc, dst);
      break;
    }
#endif
    default:
    {
      AC_ASSERT(false);
      break;
    }
    }
  }
}

static ac_result
ac_d3d12_create_sbt(
  ac_device          device_handle,
  const ac_sbt_info* info,
  ac_sbt*            sbt_handle)
{
  AC_UNUSED(device_handle);
  AC_UNUSED(info);
  // TODO: IMPLEMENT
  AC_INIT_INTERNAL(sbt, ac_d3d12_sbt);

  return ac_result_success;
}

static void
ac_d3d12_destroy_sbt(ac_device device_handle, ac_sbt sbt_handle)
{
  AC_UNUSED(device_handle);
  AC_UNUSED(sbt_handle);
}

static ac_result
ac_d3d12_create_blas(
  ac_device         device_handle,
  const ac_as_info* info,
  ac_as*            as_handle)
{
  AC_MAYBE_UNUSED(device_handle);
  AC_MAYBE_UNUSED(info);
  AC_MAYBE_UNUSED(as_handle);

#if (AC_D3D12_USE_RAYTRACING)
  AC_INIT_INTERNAL(as, ac_d3d12_as);
  AC_FROM_HANDLE(device, ac_d3d12_device);

  as->count = info->count;
  as->geometries = static_cast<D3D12_RAYTRACING_GEOMETRY_DESC*>(
    ac_calloc(info->count * sizeof(D3D12_RAYTRACING_GEOMETRY_DESC)));

  for (uint32_t i = 0; i < info->count; ++i)
  {
    const ac_as_geometry*           src = &info->geometries[i];
    D3D12_RAYTRACING_GEOMETRY_DESC* dst = &as->geometries[i];

    dst->Type = ac_geometry_type_to_d3d12(src->type);
    dst->Flags = ac_geometry_bits_to_d3d12(src->bits);

    switch (src->type)
    {
    case ac_geometry_type_triangles:
    {
      const ac_as_geometry_triangles*           src_triangles = &src->triangles;
      D3D12_RAYTRACING_GEOMETRY_TRIANGLES_DESC* dst_triangles = &dst->Triangles;

      if (src_triangles->transform.buffer)
      {
        AC_FROM_HANDLE2(buf, src_triangles->transform.buffer, ac_d3d12_buffer);
        dst_triangles->Transform3x4 = buf->resource->GetGPUVirtualAddress() +
                                      src_triangles->transform.offset;
      }

      dst_triangles->IndexFormat =
        src_triangles->index_type == ac_index_type_u16 ? DXGI_FORMAT_R16_UINT
                                                       : DXGI_FORMAT_R32_UINT;
      dst_triangles->VertexFormat =
        ac_format_to_d3d12(src_triangles->vertex_format);
      dst_triangles->IndexCount = src_triangles->index_count;
      dst_triangles->VertexCount = src_triangles->vertex_count;

      {
        AC_FROM_HANDLE2(buf, src_triangles->index.buffer, ac_d3d12_buffer);
        dst_triangles->IndexBuffer =
          buf->resource->GetGPUVirtualAddress() + src_triangles->index.offset;
      }
      {
        AC_FROM_HANDLE2(buf, src_triangles->vertex.buffer, ac_d3d12_buffer);
        dst_triangles->VertexBuffer.StartAddress =
          buf->resource->GetGPUVirtualAddress() + src_triangles->vertex.offset;
        dst_triangles->VertexBuffer.StrideInBytes =
          src_triangles->vertex_stride;
      }
      break;
    }
    case ac_geometry_type_aabbs:
    {
      const ac_as_geometry_aabs*            src_aabbs = &src->aabs;
      D3D12_RAYTRACING_GEOMETRY_AABBS_DESC* dst_aabbs = &dst->AABBs;

      dst_aabbs->AABBCount = 1;
      AC_FROM_HANDLE2(buf, src_aabbs->buffer, ac_d3d12_buffer);
      dst_aabbs->AABBs.StartAddress =
        buf->resource->GetGPUVirtualAddress() + src_aabbs->offset;
      dst_aabbs->AABBs.StrideInBytes = src_aabbs->stride;

      break;
    }
    default:
    {
      return ac_result_invalid_argument;
    }
    }
  }

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS desc = {};
  desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  desc.Flags =
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  desc.NumDescs = as->count;
  desc.Type = ac_as_type_to_d3d12(info->type);
  desc.pGeometryDescs = as->geometries;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild = {};
  device->device->GetRaytracingAccelerationStructurePrebuildInfo(
    &desc,
    &prebuild);

  ac_buffer_info buffer_info = {};
  buffer_info.memory_usage = ac_memory_usage_gpu_only;
  buffer_info.usage = ac_buffer_usage_none;
  buffer_info.size = prebuild.ResultDataMaxSizeInBytes;
  buffer_info.name = info->name;

  AC_RIF(ac_d3d12_create_buffer2(
    device,
    &buffer_info,
    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
    &as->resource,
    &as->allocation));

  as->common.scratch_size = prebuild.ScratchDataSizeInBytes;

  return ac_result_success;
#else
  return ac_result_unknown_error;
#endif
}

static ac_result
ac_d3d12_create_tlas(
  ac_device         device_handle,
  const ac_as_info* info,
  ac_as*            as_handle)
{
  AC_MAYBE_UNUSED(device_handle);
  AC_MAYBE_UNUSED(info);
  AC_MAYBE_UNUSED(as_handle);

#if (AC_D3D12_USE_RAYTRACING)
  AC_INIT_INTERNAL(as, ac_d3d12_as);
  AC_FROM_HANDLE(device, ac_d3d12_device);

  AC_FROM_HANDLE2(instances_buffer, info->instances_buffer, ac_d3d12_buffer);

  as->count = info->count;
  as->instances = instances_buffer->resource->GetGPUVirtualAddress() +
                  info->instances_buffer_offset;

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS desc = {};
  desc.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  desc.Flags =
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  desc.NumDescs = as->count;
  desc.Type = ac_as_type_to_d3d12(info->type);
  desc.InstanceDescs = as->instances;

  D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO prebuild = {};
  device->device->GetRaytracingAccelerationStructurePrebuildInfo(
    &desc,
    &prebuild);

  ac_buffer_info buffer_info = {};
  buffer_info.memory_usage = ac_memory_usage_gpu_only;
  buffer_info.usage = ac_buffer_usage_none;
  buffer_info.size = prebuild.ResultDataMaxSizeInBytes;
  buffer_info.name = info->name;

  AC_RIF(ac_d3d12_create_buffer2(
    device,
    &buffer_info,
    D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE,
    &as->resource,
    &as->allocation));

  as->common.scratch_size = prebuild.ScratchDataSizeInBytes;

  return ac_result_success;
#else
  return ac_result_unknown_error;
#endif
}

static void
ac_d3d12_destroy_as(ac_device device_handle, ac_as as_handle)
{
  AC_UNUSED(device_handle);
  AC_MAYBE_UNUSED(as_handle);

#if (AC_D3D12_USE_RAYTRACING)
  AC_FROM_HANDLE(as, ac_d3d12_as);

  AC_D3D12_SAFE_RELEASE(as->resource);
  AC_D3D12_SAFE_RELEASE(as->allocation);

  if (as->common.type == ac_as_type_bottom_level)
  {
    ac_free(as->geometries);
  }
#endif
}

static void
ac_d3d12_write_as_instances(
  ac_device             device_handle,
  uint32_t              count,
  const ac_as_instance* instances,
  void*                 mem)
{
  AC_UNUSED(device_handle);
  AC_MAYBE_UNUSED(count);
  AC_MAYBE_UNUSED(instances);
  AC_MAYBE_UNUSED(mem);

#if (AC_D3D12_USE_RAYTRACING)
  D3D12_RAYTRACING_INSTANCE_DESC* dst_instances =
    static_cast<D3D12_RAYTRACING_INSTANCE_DESC*>(mem);

  for (uint32_t i = 0; i < count; ++i)
  {
    const ac_as_instance*           src = &instances[i];
    D3D12_RAYTRACING_INSTANCE_DESC* dst = &dst_instances[i];

    AC_FROM_HANDLE2(as, src->as, ac_d3d12_as);

    memcpy(dst->Transform, src->transform.matrix, sizeof(dst->Transform));

    dst->InstanceID = src->instance_index;
    dst->InstanceMask = src->mask;
    dst->InstanceContributionToHitGroupIndex = src->instance_sbt_offset;
    dst->Flags = ac_as_instance_bits_to_d3d12(src->bits);
    dst->AccelerationStructure = as->resource->GetGPUVirtualAddress();
  }
#endif
}

static void
ac_d3d12_cmd_barrier(
  ac_cmd                   cmd_handle,
  uint32_t                 buffer_barrier_count,
  const ac_buffer_barrier* buffer_barriers,
  uint32_t                 image_barrier_count,
  const ac_image_barrier*  image_barriers)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
#if (AC_D3D12_USE_ENHANCED_BARRIERS)
  AC_FROM_HANDLE2(device, cmd_handle->pool->queue->device, ac_d3d12_device);

  if (device->enhanced_barriers)
  {
    D3D12_BUFFER_BARRIER*  dst_buffer_barriers = NULL;
    D3D12_TEXTURE_BARRIER* dst_image_barriers = NULL;

    if (buffer_barrier_count)
    {
      dst_buffer_barriers = static_cast<D3D12_BUFFER_BARRIER*>(
        ac_alloc(buffer_barrier_count * sizeof(D3D12_BUFFER_BARRIER)));
    }

    if (image_barrier_count)
    {
      dst_image_barriers = static_cast<D3D12_TEXTURE_BARRIER*>(
        ac_alloc(image_barrier_count * sizeof(D3D12_TEXTURE_BARRIER)));
    }

    D3D12_BARRIER_GROUP groups[2];
    AC_ZERO(groups);

    uint32_t group_count = 0;

    for (uint32_t i = 0; i < buffer_barrier_count; ++i)
    {
      const ac_buffer_barrier* in = &buffer_barriers[i];
      AC_FROM_HANDLE2(buffer, in->buffer, ac_d3d12_buffer);
      D3D12_BUFFER_BARRIER* out = &dst_buffer_barriers[i];
      AC_ZEROP(out);
      out->AccessBefore = ac_access_bits_to_d3d12(in->src_access);
      out->AccessAfter = ac_access_bits_to_d3d12(in->dst_access);
      out->SyncBefore = ac_pipeline_stage_bits_to_d3d12(in->src_stage, true);
      out->SyncAfter = ac_pipeline_stage_bits_to_d3d12(in->dst_stage, false);
      out->pResource = buffer->resource;
      out->Offset = in->offset;
      out->Size = in->size;
    }

    if (buffer_barrier_count)
    {
      groups[group_count].Type = D3D12_BARRIER_TYPE_BUFFER;
      groups[group_count].NumBarriers = buffer_barrier_count;
      groups[group_count].pBufferBarriers = dst_buffer_barriers;
      group_count++;
    }

    for (uint32_t i = 0; i < image_barrier_count; ++i)
    {
      const ac_image_barrier* in = &image_barriers[i];
      AC_FROM_HANDLE2(image, in->image, ac_d3d12_image);

      D3D12_BARRIER_SUBRESOURCE_RANGE range = {};
      range.IndexOrFirstMipLevel = in->range.base_level;
      range.NumMipLevels = in->range.levels;
      range.FirstArraySlice = in->range.base_layer;
      range.NumArraySlices = in->range.layers;
      range.FirstPlane = 0;
      range.NumPlanes = 1;

      D3D12_TEXTURE_BARRIER* out = &dst_image_barriers[i];
      AC_ZEROP(out);
      out->AccessBefore = ac_access_bits_to_d3d12(in->src_access);
      out->AccessAfter = ac_access_bits_to_d3d12(in->dst_access);
      out->SyncBefore = ac_pipeline_stage_bits_to_d3d12(in->src_stage, true);
      out->SyncAfter = ac_pipeline_stage_bits_to_d3d12(in->dst_stage, false);
      out->LayoutBefore = ac_image_layout_to_d3d12(in->old_layout);
      out->LayoutAfter = ac_image_layout_to_d3d12(in->new_layout);
      out->pResource = image->resource;
      out->Subresources = range;

      if (in->range.layers == AC_WHOLE_LAYERS)
      {
        out->Subresources.NumArraySlices = image->common.layers;
        out->Subresources.FirstArraySlice = 0;
      }
      if (in->range.levels == AC_WHOLE_LEVELS)
      {
        out->Subresources.NumMipLevels = image->common.levels;
        out->Subresources.IndexOrFirstMipLevel = 0;
      }

      if (
        (out->LayoutAfter == D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ) &&
        (out->AccessAfter & D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE))
      {
        out->LayoutAfter = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
      }

      if (
        (out->LayoutAfter == D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE) &&
        (out->AccessAfter & D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ))
      {
        out->AccessAfter &= ~D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
        out->AccessAfter |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
      }

      if (
        (out->LayoutBefore == D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_READ) &&
        (out->AccessBefore & D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE))
      {
        out->LayoutBefore = D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE;
      }

      if (
        (out->LayoutBefore == D3D12_BARRIER_LAYOUT_DEPTH_STENCIL_WRITE) &&
        (out->AccessBefore & D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ))
      {
        out->AccessBefore &= ~D3D12_BARRIER_ACCESS_DEPTH_STENCIL_READ;
        out->AccessBefore |= D3D12_BARRIER_ACCESS_DEPTH_STENCIL_WRITE;
      }
    }

    if (image_barrier_count)
    {
      groups[group_count].Type = D3D12_BARRIER_TYPE_TEXTURE;
      groups[group_count].NumBarriers = image_barrier_count;
      groups[group_count].pTextureBarriers = dst_image_barriers;
      group_count++;
    }

    cmd->cmd->Barrier(group_count, groups);

    ac_free(dst_buffer_barriers);
    ac_free(dst_image_barriers);
  }
  else
#endif
  {
    D3D12_RESOURCE_BARRIER* barriers =
      static_cast<D3D12_RESOURCE_BARRIER*>(ac_alloc(
        (buffer_barrier_count + image_barrier_count) *
        sizeof(D3D12_RESOURCE_BARRIER)));

    uint32_t barrier_count = 0;

    for (uint32_t i = 0; i < buffer_barrier_count; ++i)
    {
      const ac_buffer_barrier* src = &buffer_barriers[i];
      AC_FROM_HANDLE2(buffer, src->buffer, ac_d3d12_buffer);
      D3D12_RESOURCE_BARRIER* dst = &barriers[barrier_count];

      dst->Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
      dst->Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      dst->UAV.pResource = buffer->resource;

      barrier_count++;
    }

    for (uint32_t i = 0; i < image_barrier_count; ++i)
    {
      const ac_image_barrier* src = &image_barriers[i];
      AC_FROM_HANDLE2(image, src->image, ac_d3d12_image);
      D3D12_RESOURCE_BARRIER* dst = &barriers[barrier_count];

      dst->Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
      dst->Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
      dst->Transition.pResource = image->resource;
      dst->Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
      dst->Transition.StateBefore = ac_image_layout_to_d3d12_resource_states(
        src->old_layout,
        cmd_handle->pool->queue->type);
      dst->Transition.StateAfter = ac_image_layout_to_d3d12_resource_states(
        src->new_layout,
        cmd_handle->pool->queue->type);

      if (dst->Transition.StateBefore == dst->Transition.StateAfter)
      {
        continue;
      }

      barrier_count++;
    }

    if (barrier_count)
    {
      cmd->cmd->ResourceBarrier(barrier_count, barriers);
    }

    ac_free(barriers);
  }
}

static void
ac_d3d12_cmd_begin_rendering(ac_cmd cmd_handle, const ac_rendering_info* info)
{
  AC_FROM_HANDLE2(device, cmd_handle->pool->queue->device, ac_d3d12_device);
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);

  D3D12_CPU_DESCRIPTOR_HANDLE  color_attachments[AC_MAX_ATTACHMENT_COUNT] = {};
  D3D12_CPU_DESCRIPTOR_HANDLE  depth;
  D3D12_CPU_DESCRIPTOR_HANDLE* p_depth = NULL;

  for (uint32_t i = 0; i < info->color_attachment_count; ++i)
  {
    const ac_attachment_info* in = &info->color_attachments[i];
    AC_FROM_HANDLE2(image, in->image, ac_d3d12_image);

    D3D12_CPU_DESCRIPTOR_HANDLE handle = ac_d3d12_get_cpu_handle(
      device->rtv_heap.heap,
      device->rtv_descriptor_size,
      image->handle);

    color_attachments[i] = handle;

    if (in->load_op == ac_attachment_load_op_clear)
    {
      cmd->cmd->ClearRenderTargetView(handle, in->clear_value.color, 0, NULL);
    }

    if (in->resolve_image)
    {
      AC_DEBUG(
        "[ renderer ] [ d3d12 ] : this code path is never tested. if you "
        "encounter any problems with resolve most likely it's broken");

      AC_FROM_HANDLE2(resolve_image, in->resolve_image, ac_d3d12_image);

      cmd->cmd->ResolveSubresourceRegion(
        resolve_image->resource,
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        0,
        0,
        image->resource,
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        NULL,
        ac_format_to_d3d12_attachment(image->common.format),
        D3D12_RESOLVE_MODE_AVERAGE);
    }
  }

  if (info->depth_attachment.image)
  {
    const ac_attachment_info* in = &info->depth_attachment;
    AC_FROM_HANDLE2(image, in->image, ac_d3d12_image);

    D3D12_CPU_DESCRIPTOR_HANDLE handle = ac_d3d12_get_cpu_handle(
      device->dsv_heap.heap,
      device->dsv_descriptor_size,
      image->handle);

    depth = handle;
    p_depth = &depth;

    if (in->load_op == ac_attachment_load_op_clear)
    {
      cmd->cmd->ClearDepthStencilView(
        handle,
        D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
        in->clear_value.depth,
        in->clear_value.stencil,
        0,
        NULL);
    }

    if (in->resolve_image)
    {
      AC_DEBUG(
        "[ renderer ] [ d3d12 ] : this code path is never tested. if you "
        "encounter any problems with resolve most likely it's broken");

      AC_FROM_HANDLE2(resolve_image, in->resolve_image, ac_d3d12_image);

      cmd->cmd->ResolveSubresourceRegion(
        resolve_image->resource,
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        0,
        0,
        image->resource,
        D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
        NULL,
        ac_format_to_d3d12_attachment(image->common.format),
        D3D12_RESOLVE_MODE_MIN);
    }
  }

  cmd->cmd->OMSetRenderTargets(
    info->color_attachment_count,
    color_attachments,
    false,
    p_depth);
}

static void
ac_d3d12_cmd_end_rendering(ac_cmd cmd_handle)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);

  cmd->cmd->OMSetRenderTargets(0, NULL, false, NULL);
}

static void
ac_d3d12_cmd_set_scissor(
  ac_cmd   cmd_handle,
  uint32_t x,
  uint32_t y,
  uint32_t width,
  uint32_t height)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);

  D3D12_RECT rc = {};
  rc.left = static_cast<LONG>(x);
  rc.top = static_cast<LONG>(y);
  rc.right = static_cast<LONG>(x + width);
  rc.bottom = static_cast<LONG>(y + height);

  cmd->cmd->RSSetScissorRects(1, &rc);
}

static void
ac_d3d12_cmd_set_viewport(
  ac_cmd cmd_handle,
  float  x,
  float  y,
  float  width,
  float  height,
  float  min_depth,
  float  max_depth)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);

  D3D12_VIEWPORT viewport = {};
  viewport.TopLeftX = x;
  viewport.TopLeftY = y;
  viewport.Width = width;
  viewport.Height = height;
  viewport.MinDepth = min_depth;
  viewport.MaxDepth = max_depth;

  cmd->cmd->RSSetViewports(1, &viewport);
}

static void
ac_d3d12_cmd_bind_pipeline(ac_cmd cmd_handle, ac_pipeline pipeline_handle)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_FROM_HANDLE(pipeline, ac_d3d12_pipeline);

  switch (pipeline->common.type)
  {
  case ac_pipeline_type_graphics:
  case ac_pipeline_type_mesh:
    cmd->cmd->SetGraphicsRootSignature(pipeline->signature);
    break;
  case ac_pipeline_type_compute:
  case ac_pipeline_type_raytracing:
    cmd->cmd->SetComputeRootSignature(pipeline->signature);
    break;
  default:
    AC_ASSERT(false);
    break;
  }

  cmd->cmd->SetPipelineState(pipeline->pipeline);

  if (pipeline->common.type == ac_pipeline_type_graphics)
  {
    cmd->cmd->IASetPrimitiveTopology(pipeline->topology);
  }
}

static void
ac_d3d12_cmd_draw_mesh_tasks(
  ac_cmd   cmd_handle,
  uint32_t group_count_x,
  uint32_t group_count_y,
  uint32_t group_count_z)
{
  AC_MAYBE_UNUSED(cmd_handle);
  AC_MAYBE_UNUSED(group_count_x);
  AC_MAYBE_UNUSED(group_count_y);
  AC_MAYBE_UNUSED(group_count_z);

#if (AC_D3D12_USE_MESH_SHADERS)
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  cmd->cmd->DispatchMesh(group_count_x, group_count_y, group_count_z);
#endif
}

static void
ac_d3d12_cmd_draw(
  ac_cmd   cmd_handle,
  uint32_t vertex_count,
  uint32_t instance_count,
  uint32_t first_vertex,
  uint32_t first_instance)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  cmd->cmd
    ->DrawInstanced(vertex_count, instance_count, first_vertex, first_instance);
}

static void
ac_d3d12_cmd_draw_indexed(
  ac_cmd   cmd_handle,
  uint32_t index_count,
  uint32_t instance_count,
  uint32_t first_index,
  int32_t  first_vertex,
  uint32_t first_instance)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  cmd->cmd->DrawIndexedInstanced(
    index_count,
    instance_count,
    first_index,
    first_vertex,
    first_instance);
}

static void
ac_d3d12_cmd_bind_vertex_buffer(
  ac_cmd         cmd_handle,
  uint32_t       binding,
  ac_buffer      buffer_handle,
  const uint64_t offset)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_FROM_HANDLE(buffer, ac_d3d12_buffer);
  AC_FROM_HANDLE2(pipeline, cmd_handle->pipeline, ac_d3d12_pipeline);

  D3D12_VERTEX_BUFFER_VIEW view = {};
  view.BufferLocation = buffer->resource->GetGPUVirtualAddress() + offset;
  view.SizeInBytes = static_cast<UINT>(buffer->common.size - offset);
  view.StrideInBytes = pipeline->vertex_strides[binding];

  cmd->cmd->IASetVertexBuffers(binding, 1, &view);
}

static void
ac_d3d12_cmd_bind_index_buffer(
  ac_cmd         cmd_handle,
  ac_buffer      buffer_handle,
  const uint64_t offset,
  ac_index_type  index_type)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_FROM_HANDLE(buffer, ac_d3d12_buffer);

  D3D12_INDEX_BUFFER_VIEW view = {};
  view.BufferLocation = buffer->resource->GetGPUVirtualAddress() + offset;
  view.SizeInBytes = static_cast<UINT>(buffer->common.size - offset);
  view.Format = DXGI_FORMAT_R16_UINT;
  if (index_type == ac_index_type_u32)
  {
    view.Format = DXGI_FORMAT_R32_UINT;
  }

  cmd->cmd->IASetIndexBuffer(&view);
}

static void
ac_d3d12_cmd_copy_buffer(
  ac_cmd    cmd_handle,
  ac_buffer src_handle,
  uint64_t  src_offset,
  ac_buffer dst_handle,
  uint64_t  dst_offset,
  uint64_t  size)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_FROM_HANDLE(src, ac_d3d12_buffer);
  AC_FROM_HANDLE(dst, ac_d3d12_buffer);

  cmd->cmd->CopyBufferRegion(
    dst->resource,
    dst_offset,
    src->resource,
    src_offset,
    size);
}

static void
ac_d3d12_cmd_copy_buffer_to_image(
  ac_cmd                      cmd_handle,
  ac_buffer                   src_handle,
  ac_image                    dst_handle,
  const ac_buffer_image_copy* copy)
{
  AC_FROM_HANDLE2(device, cmd_handle->pool->queue->device, ac_d3d12_device);
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_FROM_HANDLE(src, ac_d3d12_buffer);
  AC_FROM_HANDLE(dst, ac_d3d12_image);

  uint32_t subresource = ac_d3d12_get_subresource_index(
    copy->level,
    copy->layer,
    dst->common.levels);

  D3D12_RESOURCE_DESC desc = dst->resource->GetDesc();

  D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp = {};

  device->device->GetCopyableFootprints(
    &desc,
    subresource,
    1,
    copy->buffer_offset,
    &fp,
    NULL,
    NULL,
    NULL);

  D3D12_TEXTURE_COPY_LOCATION src_location = {};
  src_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  src_location.pResource = src->resource;
  src_location.PlacedFootprint = fp;

  D3D12_TEXTURE_COPY_LOCATION dst_location = {};
  dst_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  dst_location.pResource = dst->resource;
  dst_location.SubresourceIndex = subresource;

  D3D12_BOX box = {};
  box.left = 0;
  box.right = copy->width;
  box.top = 0;
  box.bottom = copy->height;
  box.front = 0;
  box.back = 1;

  cmd->cmd->CopyTextureRegion(
    &dst_location,
    copy->x,
    copy->y,
    0,
    &src_location,
    &box);
}

static void
ac_d3d12_cmd_copy_image(
  ac_cmd               cmd_handle,
  ac_image             src_handle,
  ac_image             dst_handle,
  const ac_image_copy* copy)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_FROM_HANDLE(src, ac_d3d12_image);
  AC_FROM_HANDLE(dst, ac_d3d12_image);

  D3D12_TEXTURE_COPY_LOCATION src_location = {};
  src_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  src_location.pResource = src->resource;
  src_location.SubresourceIndex = ac_d3d12_get_subresource_index(
    copy->src.level,
    copy->src.layer,
    src->common.levels);

  D3D12_TEXTURE_COPY_LOCATION dst_location = {};
  dst_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  dst_location.pResource = dst->resource;
  dst_location.SubresourceIndex = ac_d3d12_get_subresource_index(
    copy->dst.level,
    copy->dst.layer,
    dst->common.levels);

  D3D12_BOX box = {};
  box.left = copy->src.x;
  box.right = copy->width;
  box.top = copy->src.y;
  box.bottom = copy->height;
  box.front = 0;
  box.back = 1;

  cmd->cmd->CopyTextureRegion(
    &dst_location,
    copy->dst.x,
    copy->dst.y,
    0,
    &src_location,
    &box);
}

static void
ac_d3d12_cmd_copy_image_to_buffer(
  ac_cmd                      cmd_handle,
  ac_image                    src_handle,
  ac_buffer                   dst_handle,
  const ac_buffer_image_copy* copy)
{
  AC_FROM_HANDLE2(device, cmd_handle->pool->queue->device, ac_d3d12_device);
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_FROM_HANDLE(src, ac_d3d12_image);
  AC_FROM_HANDLE(dst, ac_d3d12_buffer);

  uint32_t subresource = ac_d3d12_get_subresource_index(
    copy->level,
    copy->layer,
    src->common.levels);

  D3D12_RESOURCE_DESC desc = src->resource->GetDesc();

  D3D12_PLACED_SUBRESOURCE_FOOTPRINT fp = {};

  device->device->GetCopyableFootprints(
    &desc,
    subresource,
    1,
    copy->buffer_offset,
    &fp,
    NULL,
    NULL,
    NULL);

  D3D12_TEXTURE_COPY_LOCATION src_location = {};
  src_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  src_location.pResource = src->resource;
  src_location.SubresourceIndex = subresource;

  D3D12_TEXTURE_COPY_LOCATION dst_location = {};
  dst_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  dst_location.pResource = dst->resource;
  dst_location.PlacedFootprint = fp;

  D3D12_BOX box = {};
  box.left = copy->x;
  box.right = copy->width;
  box.top = copy->y;
  box.bottom = copy->height;
  box.front = 0;
  box.back = 1;

  cmd->cmd->CopyTextureRegion(
    &dst_location,
    copy->x,
    copy->y,
    0,
    &src_location,
    &box);
}

static void
ac_d3d12_cmd_dispatch(
  ac_cmd   cmd_handle,
  uint32_t group_count_x,
  uint32_t group_count_y,
  uint32_t group_count_z)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  cmd->cmd->Dispatch(group_count_x, group_count_y, group_count_z);
}

static void
ac_d3d12_cmd_trace_rays(
  ac_cmd   cmd_handle,
  ac_sbt   sbt_handle,
  uint32_t width,
  uint32_t height,
  uint32_t depth)
{
  AC_MAYBE_UNUSED(cmd_handle);
  AC_MAYBE_UNUSED(sbt_handle);
  AC_MAYBE_UNUSED(width);
  AC_MAYBE_UNUSED(height);
  AC_MAYBE_UNUSED(depth);

#if (AC_D3D12_USE_RAYTRACING)
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_FROM_HANDLE(sbt, ac_d3d12_sbt);

  D3D12_DISPATCH_RAYS_DESC rays_desc = {};
  rays_desc.RayGenerationShaderRecord = sbt->raygen;
  rays_desc.MissShaderTable = sbt->miss;
  rays_desc.HitGroupTable = sbt->hit;
  rays_desc.CallableShaderTable = sbt->callable;
  rays_desc.Width = width;
  rays_desc.Height = height;
  rays_desc.Depth = depth;

  cmd->cmd->DispatchRays(&rays_desc);
#endif
}

static void
ac_d3d12_cmd_build_as(ac_cmd cmd_handle, ac_as_build_info* info)
{
  AC_MAYBE_UNUSED(cmd_handle);
  AC_MAYBE_UNUSED(info);

#if (AC_D3D12_USE_RAYTRACING)
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);

  AC_FROM_HANDLE2(as, info->as, ac_d3d12_as);
  AC_FROM_HANDLE2(scratch, info->scratch_buffer, ac_d3d12_buffer);

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
  inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
  inputs.Flags =
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
  inputs.NumDescs = as->count;
  inputs.Type = ac_as_type_to_d3d12(as->common.type);

  switch (as->common.type)
  {
  case ac_as_type_bottom_level:
  {
    inputs.pGeometryDescs = as->geometries;
    break;
  }
  case ac_as_type_top_level:
  {
    inputs.InstanceDescs = as->instances;
    break;
  }
  }

  D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC desc = {};
  desc.Inputs = inputs;
  desc.ScratchAccelerationStructureData =
    scratch->resource->GetGPUVirtualAddress() + info->scratch_buffer_offset;
  desc.DestAccelerationStructureData = as->resource->GetGPUVirtualAddress();

  cmd->cmd->BuildRaytracingAccelerationStructure(&desc, 0, NULL);
#endif
}

static void
ac_d3d12_cmd_push_constants(ac_cmd cmd_handle, uint32_t size, const void* data)
{
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_FROM_HANDLE2(pipeline, cmd_handle->pipeline, ac_d3d12_pipeline);

  ac_d3d12_set_heaps(cmd, cmd->db);

  UINT idx = static_cast<UINT>(pipeline->push_constant);

  switch (pipeline->common.type)
  {
  case ac_pipeline_type_graphics:
  case ac_pipeline_type_mesh:
    cmd->cmd->SetGraphicsRoot32BitConstants(idx, size / 4, data, 0);
    break;
  case ac_pipeline_type_compute:
    cmd->cmd->SetComputeRoot32BitConstants(idx, size / 4, data, 0);
    break;
  default:
    AC_ASSERT(false);
    break;
  }
}

static void
ac_d3d12_cmd_bind_set(
  ac_cmd               cmd_handle,
  ac_descriptor_buffer db_handle,
  ac_space             space_index,
  uint32_t             set)
{
  AC_FROM_HANDLE2(device, cmd_handle->pool->queue->device, ac_d3d12_device);
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  AC_FROM_HANDLE(db, ac_d3d12_descriptor_buffer);

  ac_d3d12_set_heaps(cmd, db);

  const ac_d3d12_space* space = &db->spaces[space_index];

  uint32_t set_count = 0;
  struct {
    uint32_t                    root_param;
    D3D12_GPU_DESCRIPTOR_HANDLE table;
  } sets[2] = {};

  if (space->resource.stride)
  {
    sets[set_count].root_param = space->resource.root_param;
    sets[set_count].table = ac_d3d12_get_gpu_handle(
      db->resource_heap,
      device->resource_descriptor_size,
      space->resource.offset + space->resource.stride * set);

    set_count++;
  }

  if (space->sampler.stride)
  {
    sets[set_count].root_param = space->sampler.root_param;
    sets[set_count].table = ac_d3d12_get_gpu_handle(
      db->sampler_heap,
      device->sampler_descriptor_size,
      space->sampler.offset + space->sampler.stride * set);

    set_count++;
  }

  for (uint32_t s = 0; s < set_count; ++s)
  {
    switch (cmd->common.pipeline->type)
    {
    case ac_pipeline_type_graphics:
    case ac_pipeline_type_mesh:
      cmd->cmd->SetGraphicsRootDescriptorTable(
        sets[s].root_param,
        sets[s].table);
      break;
    default:
      cmd->cmd->SetComputeRootDescriptorTable(
        sets[s].root_param,
        sets[s].table);
      break;
    }
  }
}

static void
ac_d3d12_cmd_begin_debug_label(
  ac_cmd      cmd_handle,
  const char* name,
  const float color[4])
{
  AC_MAYBE_UNUSED(cmd_handle);
  AC_MAYBE_UNUSED(name);
  AC_MAYBE_UNUSED(color);

#if (AC_INCLUDE_DEBUG)
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  PIXBeginEvent(
    cmd->cmd,
    PIX_COLOR(
      static_cast<BYTE>(color[0] * 255),
      static_cast<BYTE>(color[1] * 255),
      static_cast<BYTE>(color[2] * 255)),
    name);
#endif
}

static void
ac_d3d12_cmd_end_debug_label(ac_cmd cmd_handle)
{
  AC_MAYBE_UNUSED(cmd_handle);
#if (AC_INCLUDE_DEBUG)
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);
  PIXEndEvent(cmd->cmd);
#endif
}

static void
ac_d3d12_cmd_insert_debug_label(
  ac_cmd      cmd_handle,
  const char* name,
  const float color[4])
{
  AC_MAYBE_UNUSED(cmd_handle);
  AC_MAYBE_UNUSED(name);
  AC_MAYBE_UNUSED(color);

#if (AC_INCLUDE_DEBUG)
  AC_FROM_HANDLE(cmd, ac_d3d12_cmd);

  PIXSetMarker(
    cmd->cmd,
    PIX_COLOR(
      static_cast<BYTE>(color[0] * 255),
      static_cast<BYTE>(color[1] * 255),
      static_cast<BYTE>(color[2] * 255)),
    name);
#endif
}

static void
ac_d3d12_begin_capture(ac_device device_handle)
{
  AC_FROM_HANDLE(device, ac_d3d12_device);

#if (AC_INCLUDE_DEBUG)
  PIXBeginCapture2(PIX_CAPTURE_GPU, NULL);
#endif

  if (device->rdoc)
  {
    if (!device->rdoc->IsFrameCapturing())
    {
      device->rdoc->StartFrameCapture(NULL, NULL);
    }
  }
}

static void
ac_d3d12_end_capture(ac_device device_handle)
{
  AC_FROM_HANDLE(device, ac_d3d12_device);

#if (AC_INCLUDE_DEBUG)
  PIXEndCapture(false);
#endif

  if (device->rdoc)
  {
    if (device->rdoc->IsFrameCapturing())
    {
      device->rdoc->EndFrameCapture(NULL, NULL);
    }
  }
}

void
ac_d3d12_get_device_common_functions(ac_d3d12_device* device)
{
  device->common.queue_wait_idle = ac_d3d12_queue_wait_idle;
  device->common.queue_submit = ac_d3d12_queue_submit;
  device->common.create_fence = ac_d3d12_create_fence;
  device->common.destroy_fence = ac_d3d12_destroy_fence;
  device->common.get_fence_value = ac_d3d12_get_fence_value;
  device->common.signal_fence = ac_d3d12_signal_fence;
  device->common.wait_fence = ac_d3d12_wait_fence;
  device->common.create_cmd_pool = ac_d3d12_create_cmd_pool;
  device->common.destroy_cmd_pool = ac_d3d12_destroy_cmd_pool;
  device->common.reset_cmd_pool = ac_d3d12_reset_cmd_pool;
  device->common.create_cmd = ac_d3d12_create_cmd;
  device->common.destroy_cmd = ac_d3d12_destroy_cmd;
  device->common.begin_cmd = ac_d3d12_begin_cmd;
  device->common.end_cmd = ac_d3d12_end_cmd;
  device->common.create_shader = ac_d3d12_create_shader;
  device->common.destroy_shader = ac_d3d12_destroy_shader;
  device->common.create_dsl = ac_d3d12_create_dsl;
  device->common.destroy_dsl = ac_d3d12_destroy_dsl;
  device->common.create_descriptor_buffer = ac_d3d12_create_descriptor_buffer;
  device->common.destroy_descriptor_buffer = ac_d3d12_destroy_descriptor_buffer;
  device->common.create_graphics_pipeline = ac_d3d12_create_graphics_pipeline;
  device->common.create_mesh_pipeline = ac_d3d12_create_mesh_pipeline;
  device->common.create_compute_pipeline = ac_d3d12_create_compute_pipeline;
  device->common.create_raytracing_pipeline =
    ac_d3d12_create_raytracing_pipeline;
  device->common.destroy_pipeline = ac_d3d12_destroy_pipeline;
  device->common.create_image = ac_d3d12_create_image;
  device->common.destroy_image = ac_d3d12_destroy_image;
  device->common.create_buffer = ac_d3d12_create_buffer;
  device->common.destroy_buffer = ac_d3d12_destroy_buffer;
  device->common.map_memory = ac_d3d12_map_memory;
  device->common.unmap_memory = ac_d3d12_unmap_memory;
  device->common.create_sampler = ac_d3d12_create_sampler;
  device->common.destroy_sampler = ac_d3d12_destroy_sampler;
  device->common.update_descriptor = ac_d3d12_update_descriptor;
  device->common.cmd_barrier = ac_d3d12_cmd_barrier;
  device->common.create_sbt = ac_d3d12_create_sbt;
  device->common.destroy_sbt = ac_d3d12_destroy_sbt;
  device->common.create_blas = ac_d3d12_create_blas;
  device->common.create_tlas = ac_d3d12_create_tlas;
  device->common.destroy_as = ac_d3d12_destroy_as;
  device->common.write_as_instances = ac_d3d12_write_as_instances;
  device->common.cmd_begin_rendering = ac_d3d12_cmd_begin_rendering;
  device->common.cmd_end_rendering = ac_d3d12_cmd_end_rendering;
  device->common.cmd_set_scissor = ac_d3d12_cmd_set_scissor;
  device->common.cmd_set_viewport = ac_d3d12_cmd_set_viewport;
  device->common.cmd_bind_pipeline = ac_d3d12_cmd_bind_pipeline;
  device->common.cmd_draw_mesh_tasks = ac_d3d12_cmd_draw_mesh_tasks;
  device->common.cmd_draw = ac_d3d12_cmd_draw;
  device->common.cmd_draw_indexed = ac_d3d12_cmd_draw_indexed;
  device->common.cmd_bind_vertex_buffer = ac_d3d12_cmd_bind_vertex_buffer;
  device->common.cmd_bind_index_buffer = ac_d3d12_cmd_bind_index_buffer;
  device->common.cmd_copy_buffer = ac_d3d12_cmd_copy_buffer;
  device->common.cmd_copy_buffer_to_image = ac_d3d12_cmd_copy_buffer_to_image;
  device->common.cmd_copy_image = ac_d3d12_cmd_copy_image;
  device->common.cmd_copy_image_to_buffer = ac_d3d12_cmd_copy_image_to_buffer;
  device->common.cmd_bind_set = ac_d3d12_cmd_bind_set;
  device->common.cmd_dispatch = ac_d3d12_cmd_dispatch;
  device->common.cmd_build_as = ac_d3d12_cmd_build_as;
  device->common.cmd_trace_rays = ac_d3d12_cmd_trace_rays;
  device->common.cmd_push_constants = ac_d3d12_cmd_push_constants;
  device->common.cmd_begin_debug_label = ac_d3d12_cmd_begin_debug_label;
  device->common.cmd_end_debug_label = ac_d3d12_cmd_end_debug_label;
  device->common.cmd_insert_debug_label = ac_d3d12_cmd_insert_debug_label;
  device->common.begin_capture = ac_d3d12_begin_capture;
  device->common.end_capture = ac_d3d12_end_capture;
}
}

#endif
