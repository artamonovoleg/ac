#include "ac_private.h"

#if (AC_INCLUDE_RENDERER)

#include "renderer.h"

AC_API ac_result
ac_create_device(const ac_device_info* info, ac_device* p)
{
  AC_ASSERT(info);
  AC_ASSERT(info->wsi);
  AC_ASSERT(p);

  *p = NULL;

  ac_result (*create)(const ac_device_info*, ac_device*) = NULL;

#if (AC_PLATFORM_WINDOWS || AC_PLATFORM_XBOX)
  create = ac_d3d12_create_device;
#elif (AC_PLATFORM_APPLE)
  create = ac_mtl_create_device;
#elif (AC_PLATFORM_PS4)
  create = ac_ps4_create_device;
#elif (AC_PLATFORM_PS5)
  create = ac_ps5_create_device;
#elif (AC_PLATFORM_NINTENDO_SWITCH)
  create = ac_vk_create_device;
#elif (AC_PLATFORM_LINUX)
  create = ac_vk_create_device;
#endif

#if (AC_INCLUDE_VULKAN)
  if (info->force_vulkan)
  {
    create = ac_vk_create_device;
  }
#endif

  if (!create)
  {
    AC_DEBUGBREAK();
    return ac_result_unknown_error;
  }

  ac_result res = create(info, p);

  ac_device device = *p;

  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_device(device);
    ac_free(device);
    *p = NULL;
    return ac_result_unknown_error;
  }

  if (AC_INCLUDE_DEBUG)
  {
    device->debug_bits = info->debug_bits;
  }

  AC_ASSERT(device->props.api);
  AC_ASSERT(device->props.cbv_buffer_alignment);
  AC_ASSERT(device->props.image_row_alignment);
  AC_ASSERT(device->props.image_alignment);
  AC_ASSERT(device->props.max_sample_count);

  for (uint32_t q = 0; q < device->queue_count; ++q)
  {
    ac_queue queue = device->queues[q];
    queue->device = device;
    ac_create_mutex(&queue->mtx);
  }

  return res;
}

AC_API void
ac_destroy_device(ac_device device)
{
  if (!device)
  {
    return;
  }

  for (uint32_t i = 0; i < device->queue_count; ++i)
  {
    ac_destroy_mutex(device->queues[i]->mtx);
  }
  device->destroy_device(device);
  ac_free(device);
}

AC_API ac_result
ac_queue_wait_idle(ac_queue queue)
{
  AC_ASSERT(queue);

  ac_device device = queue->device;

  ac_result res;
  res = ac_mutex_lock(queue->mtx);
  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    return res;
  }
  res = device->queue_wait_idle(queue);
  ac_mutex_unlock(queue->mtx);
  return res;
}

AC_API ac_result
ac_queue_submit(ac_queue queue, const ac_queue_submit_info* info)
{
  AC_ASSERT(queue);
  AC_ASSERT(info);
  AC_ASSERT(info->cmd_count);
  AC_ASSERT(info->cmds);

  ac_device device = queue->device;

  ac_result res = ac_mutex_lock(queue->mtx);
  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    return res;
  }

  res = device->queue_submit(queue, info);
  ac_mutex_unlock(queue->mtx);
  return res;
}

AC_API ac_result
ac_queue_present(ac_queue queue, const ac_queue_present_info* info)
{
  AC_ASSERT(queue);
  AC_ASSERT(queue->type == ac_queue_type_graphics);
  AC_ASSERT(info);
  AC_ASSERT(info->swapchain);

  if (
    AC_INCLUDE_DEBUG &&
    (queue->device->debug_bits & ac_device_debug_validation_bit))
  {
    for (uint32_t i = 0; i < info->wait_fence_count; ++i)
    {
      AC_ASSERT(info->wait_fences[i]->bits & ac_fence_present_bit);
    }
  }

  ac_device device = queue->device;

  ac_result res;
  res = ac_mutex_lock(queue->mtx);
  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    return res;
  }
  res = device->queue_present(queue, info);
  ac_mutex_unlock(queue->mtx);
  return res;
}

AC_API ac_result
ac_create_fence(ac_device device, const ac_fence_info* info, ac_fence* p)
{
  AC_ASSERT(device);
  AC_ASSERT(p);

  ac_result res = device->create_fence(device, info, p);

  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_fence(device, *p);
    ac_free(*p);
    *p = NULL;
    return res;
  }

  (*p)->device = device;
  (*p)->bits = info->bits;

  return res;
}

AC_API void
ac_destroy_fence(ac_fence fence)
{
  if (!fence)
  {
    return;
  }
  ac_device device = fence->device;
  device->destroy_fence(device, fence);
  ac_free(fence);
}

AC_API ac_result
ac_get_fence_value(ac_fence fence, uint64_t* value)
{
  AC_ASSERT(fence);

  ac_device device = fence->device;
  return device->get_fence_value(device, fence, value);
}

AC_API ac_result
ac_signal_fence(ac_fence fence, uint64_t value)
{
  AC_ASSERT(fence);

  ac_device device = fence->device;

  return device->signal_fence(device, fence, value);
}

AC_API ac_result
ac_wait_fence(ac_fence fence, uint64_t value)
{
  AC_ASSERT(fence);

  ac_device device = fence->device;
  return device->wait_fence(device, fence, value);
}

AC_API ac_result
ac_create_swapchain(
  ac_device                device,
  const ac_swapchain_info* info,
  ac_swapchain*            p)
{
  AC_ASSERT(device);
  AC_ASSERT(info);
  AC_ASSERT(info->wsi);
  AC_ASSERT(info->queue);
  AC_ASSERT(info->width);
  AC_ASSERT(info->height);
  AC_ASSERT(info->min_image_count >= 2 && info->min_image_count <= 3);
  AC_ASSERT(p);

  ac_result res = device->create_swapchain(device, info, p);
  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_swapchain(device, *p);
    ac_free(*p);
    *p = NULL;
  }

  (*p)->device = device;

  return res;
}

AC_API void
ac_destroy_swapchain(ac_swapchain swapchain)
{
  if (!swapchain)
  {
    return;
  }

  ac_device device = swapchain->device;
  device->destroy_swapchain(device, swapchain);

  for (uint32_t i = 0; i < swapchain->image_count; ++i)
  {
    ac_free(swapchain->images[i]);
  }
  ac_free(swapchain->images);
  ac_free(swapchain);
}

AC_API ac_result
ac_create_cmd_pool(
  ac_device               device,
  const ac_cmd_pool_info* info,
  ac_cmd_pool*            p)
{
  AC_ASSERT(device);
  AC_ASSERT(info);
  AC_ASSERT(info->queue);
  AC_ASSERT(p);

  ac_result res = device->create_cmd_pool(device, info, p);

  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_cmd_pool(device, *p);
    ac_free(*p);
    *p = NULL;
    return res;
  }

  (*p)->queue = info->queue;

  return res;
}

AC_API void
ac_destroy_cmd_pool(ac_cmd_pool cmd_pool)
{
  if (!cmd_pool)
  {
    return;
  }

  ac_device device = cmd_pool->queue->device;
  device->destroy_cmd_pool(device, cmd_pool);
  ac_free(cmd_pool);
}

AC_API ac_result
ac_reset_cmd_pool(ac_cmd_pool pool)
{
  AC_ASSERT(pool);

  ac_device device = pool->queue->device;
  return device->reset_cmd_pool(device, pool);
}

AC_API ac_result
ac_create_cmd(ac_cmd_pool cmd_pool, ac_cmd* p)
{
  AC_ASSERT(cmd_pool);
  AC_ASSERT(p);

  ac_device device = cmd_pool->queue->device;

  ac_result res = device->create_cmd(device, cmd_pool, p);

  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_cmd(device, cmd_pool, *p);
    ac_free(*p);
    *p = NULL;
    return res;
  }

  (*p)->device = device;
  (*p)->pool = cmd_pool;

  return res;
}

AC_API void
ac_destroy_cmd(ac_cmd cmd)
{
  if (!cmd)
  {
    return;
  }

  ac_device device = cmd->device;
  device->destroy_cmd(device, cmd->pool, cmd);
  ac_free(cmd);
}

AC_API ac_result
ac_begin_cmd(ac_cmd cmd)
{
  AC_ASSERT(cmd);

  ac_device device = cmd->device;

  cmd->pipeline = NULL;
  cmd->db = NULL;
  for (uint32_t i = 0; i < ac_space_count; ++i)
  {
    cmd->sets[i] = (uint32_t)-1;
  }
  return device->begin_cmd(cmd);
}

AC_API ac_result
ac_end_cmd(ac_cmd cmd)
{
  AC_ASSERT(cmd);

  ac_device device = cmd->device;

  cmd->pipeline = NULL;
  cmd->db = NULL;
  for (uint32_t i = 0; i < ac_space_count; ++i)
  {
    cmd->sets[i] = (uint32_t)-1;
  }
  return device->end_cmd(cmd);
}

AC_API ac_result
ac_acquire_next_image(ac_swapchain swapchain, ac_fence fence)
{
  AC_ASSERT(swapchain);
  AC_ASSERT(fence);
  AC_ASSERT(fence->bits & ac_fence_present_bit);

  ac_device device = swapchain->device;
  return device->acquire_next_image(device, swapchain, fence);
}

AC_API ac_result
ac_create_shader(ac_device device, const ac_shader_info* info, ac_shader* p)
{
  AC_ASSERT(device);
  AC_ASSERT(info);
  AC_ASSERT(p);

  ac_result res = device->create_shader(device, info, p);

  if (res == ac_result_success)
  {
    uint32_t    size;
    const void* reflection = ac_shader_compiler_get_bytecode(
      info->code,
      &size,
      ac_shader_bytecode_reflection);

    if (reflection)
    {
      (*p)->reflection = ac_calloc(size);
      memcpy((*p)->reflection, reflection, size);
    }
    else
    {
      AC_DEBUGBREAK();
      res = ac_result_invalid_argument;
    }
  }

  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_shader(device, *p);
    ac_free(*p);
    *p = NULL;
    return res;
  }

  (*p)->device = device;
  (*p)->stage = info->stage;

  return res;
}

AC_API void
ac_destroy_shader(ac_shader shader)
{
  if (!shader)
  {
    return;
  }

  ac_device device = shader->device;
  device->destroy_shader(device, shader);
  ac_free(shader->reflection);
  ac_free(shader);
}

AC_API ac_result
ac_create_dsl(ac_device device, const ac_dsl_info* info, ac_dsl* p)
{
  AC_ASSERT(device);
  AC_ASSERT(info);
  AC_ASSERT(info->shader_count);
  AC_ASSERT(info->shaders);
  AC_ASSERT(p);

  *p = NULL;

  ac_dsl_info_internal info_internal = {
    .info = *info,
  };

  AC_RIF(ac_shader_compiler_get_bindings(
    info->shaders[0]->reflection,
    &info_internal.binding_count,
    NULL));

  if (info_internal.binding_count)
  {
    info_internal.bindings =
      ac_calloc(info_internal.binding_count * sizeof(ac_shader_binding));
    if (ac_shader_compiler_get_bindings(
          info->shaders[0]->reflection,
          &info_internal.binding_count,
          info_internal.bindings))
    {
      ac_free(info_internal.bindings);
      return ac_result_unknown_error;
    }
  }

  ac_result res = device->create_dsl(device, &info_internal, p);

  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_dsl(device, *p);
    ac_free(*p);
    *p = NULL;
    return res;
  }

  (*p)->device = device;
  (*p)->binding_count = info_internal.binding_count;
  (*p)->bindings = info_internal.bindings;

  return res;
}

AC_API void
ac_destroy_dsl(ac_dsl dsl)
{
  if (!dsl)
  {
    return;
  }

  ac_device device = dsl->device;
  device->destroy_dsl(device, dsl);
  ac_free(dsl->bindings);
  ac_free(dsl);
}

AC_API ac_result
ac_create_descriptor_buffer(
  ac_device                        device,
  const ac_descriptor_buffer_info* info,
  ac_descriptor_buffer*            p)
{
  AC_ASSERT(device);
  AC_ASSERT(info);
  AC_ASSERT(info->dsl);
  AC_ASSERT(p);

  ac_result res = device->create_descriptor_buffer(device, info, p);

  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_descriptor_buffer(device, *p);
    ac_free(*p);
    *p = NULL;
    return res;
  }

  (*p)->device = device;

  return res;
}

AC_API void
ac_destroy_descriptor_buffer(ac_descriptor_buffer buffer)
{
  if (!buffer)
  {
    return;
  }

  ac_device device = buffer->device;
  device->destroy_descriptor_buffer(device, buffer);
  ac_free(buffer);
}

AC_API ac_result
ac_create_pipeline(
  ac_device               device,
  const ac_pipeline_info* info,
  ac_pipeline*            p)
{
  AC_ASSERT(device);
  AC_ASSERT(info);
  AC_ASSERT(p);

  ac_result res = ac_result_unknown_error;
  switch (info->type)
  {
  case ac_pipeline_type_graphics:
    AC_ASSERT(info->graphics.dsl);
    AC_ASSERT(info->graphics.vertex_shader);
    res = device->create_graphics_pipeline(device, info, p);
    break;
  case ac_pipeline_type_mesh:
    AC_ASSERT(info->mesh.dsl);
    AC_ASSERT(info->mesh.mesh_shader);
    res = device->create_mesh_pipeline(device, info, p);
    break;
  case ac_pipeline_type_compute:
    AC_ASSERT(info->compute.dsl);
    AC_ASSERT(info->compute.shader);
    res = device->create_compute_pipeline(device, info, p);
    break;
  case ac_pipeline_type_raytracing:
    res = device->create_raytracing_pipeline(device, info, p);
    break;
  default:
    AC_ASSERT(false);
    return res;
  }

  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_pipeline(device, *p);
    ac_free(*p);
    *p = NULL;
    return ac_result_unknown_error;
  }

  (*p)->type = info->type;
  (*p)->device = device;

  return res;
}

AC_API void
ac_destroy_pipeline(ac_pipeline pipeline)
{
  if (!pipeline)
  {
    return;
  }

  ac_device device = pipeline->device;
  device->destroy_pipeline(device, pipeline);
  ac_free(pipeline);
}

AC_API void
ac_cmd_begin_rendering(ac_cmd cmd, const ac_rendering_info* info)
{
  AC_ASSERT(cmd);
  AC_ASSERT(info);
  AC_ASSERT(info->depth_attachment.image || info->color_attachments[0].image);

  ac_device device = cmd->device;

  device->cmd_begin_rendering(cmd, info);
}

AC_API void
ac_cmd_end_rendering(ac_cmd cmd)
{
  AC_ASSERT(cmd);

  ac_device device = cmd->device;

  device->cmd_end_rendering(cmd);
}

AC_API void
ac_cmd_barrier(
  ac_cmd                   cmd,
  uint32_t                 buffer_barrier_count,
  const ac_buffer_barrier* buffer_barriers,
  uint32_t                 image_barrier_count,
  const ac_image_barrier*  image_barriers)
{
  AC_ASSERT(cmd);
  AC_ASSERT(buffer_barrier_count || image_barrier_count);

  ac_device device = cmd->device;

  if (
    AC_INCLUDE_DEBUG &&
    cmd->pool->queue->device->debug_bits & ac_device_debug_validation_bit)
  {
    for (uint32_t i = 0; i < buffer_barrier_count; ++i)
    {
      const ac_buffer_barrier* b = &buffer_barriers[i];
      AC_ASSERT(b->buffer);
      uint64_t size = (b->size == AC_WHOLE_SIZE) ? b->buffer->size : b->size;
      AC_MAYBE_UNUSED(size);
      AC_ASSERT(b->offset < size);
    }

    for (uint32_t i = 0; i < image_barrier_count; ++i)
    {
      const ac_image_barrier* b = &image_barriers[i];
      AC_MAYBE_UNUSED(b);
      AC_ASSERT(b->image);
    }
  }

  device->cmd_barrier(
    cmd,
    buffer_barrier_count,
    buffer_barriers,
    image_barrier_count,
    image_barriers);
}

AC_API void
ac_cmd_set_scissor(
  ac_cmd   cmd,
  uint32_t x,
  uint32_t y,
  uint32_t width,
  uint32_t height)
{
  AC_ASSERT(cmd);

  ac_device device = cmd->device;

  device->cmd_set_scissor(cmd, x, y, width, height);
}

AC_API void
ac_cmd_set_viewport(
  ac_cmd cmd,
  float  x,
  float  y,
  float  width,
  float  height,
  float  min_depth,
  float  max_depth)
{
  AC_ASSERT(cmd);

  ac_device device = cmd->device;

  device->cmd_set_viewport(cmd, x, y, width, height, min_depth, max_depth);
}

AC_API void
ac_cmd_bind_pipeline(ac_cmd cmd, ac_pipeline pipeline)
{
  AC_ASSERT(cmd);
  AC_ASSERT(pipeline);

  if (cmd->pipeline == pipeline)
  {
    return;
  }

  ac_device device = cmd->device;

  device->cmd_bind_pipeline(cmd, pipeline);

  cmd->pipeline = pipeline;
  cmd->db = NULL;
  for (uint32_t i = 0; i < ac_space_count; ++i)
  {
    cmd->sets[i] = (uint32_t)-1;
  }
}

AC_API void
ac_cmd_draw_mesh_tasks(
  ac_cmd   cmd,
  uint32_t group_count_x,
  uint32_t group_count_y,
  uint32_t group_count_z)
{
  AC_ASSERT(cmd);
  AC_ASSERT(cmd->pipeline);
  AC_ASSERT(cmd->pipeline->type == ac_pipeline_type_mesh);
  AC_ASSERT(group_count_x);
  AC_ASSERT(group_count_y);
  AC_ASSERT(group_count_z);

  ac_device device = cmd->device;

  device->cmd_draw_mesh_tasks(cmd, group_count_x, group_count_y, group_count_z);
}

AC_API void
ac_cmd_draw(
  ac_cmd   cmd,
  uint32_t vertex_count,
  uint32_t instance_count,
  uint32_t first_vertex,
  uint32_t first_instance)
{
  AC_ASSERT(cmd);
  AC_ASSERT(cmd->pipeline);
  AC_ASSERT(cmd->pipeline->type == ac_pipeline_type_graphics);
  AC_ASSERT(vertex_count);
  AC_ASSERT(instance_count);

  ac_device device = cmd->device;

  device
    ->cmd_draw(cmd, vertex_count, instance_count, first_vertex, first_instance);
}

AC_API void
ac_cmd_draw_indexed(
  ac_cmd   cmd,
  uint32_t index_count,
  uint32_t instance_count,
  uint32_t first_index,
  int32_t  first_vertex,
  uint32_t first_instance)
{
  AC_ASSERT(cmd);
  AC_ASSERT(cmd->pipeline);
  AC_ASSERT(cmd->pipeline->type == ac_pipeline_type_graphics);
  AC_ASSERT(index_count);
  AC_ASSERT(instance_count);

  ac_device device = cmd->device;

  device->cmd_draw_indexed(
    cmd,
    index_count,
    instance_count,
    first_index,
    first_vertex,
    first_instance);
}

AC_API void
ac_cmd_bind_vertex_buffer(
  ac_cmd    cmd,
  uint32_t  binding,
  ac_buffer buffer,
  uint64_t  offset)
{
  AC_ASSERT(cmd);
  AC_ASSERT(cmd->pipeline);
  AC_ASSERT(cmd->pipeline->type == ac_pipeline_type_graphics);
  AC_ASSERT(buffer);
  AC_ASSERT(offset < buffer->size);
  AC_ASSERT(buffer->usage & ac_buffer_usage_vertex_bit);

  ac_device device = cmd->device;

  device->cmd_bind_vertex_buffer(cmd, binding, buffer, offset);
}

AC_API void
ac_cmd_bind_index_buffer(
  ac_cmd        cmd,
  ac_buffer     buffer,
  uint64_t      offset,
  ac_index_type index_type)
{
  AC_ASSERT(cmd);
  AC_ASSERT(cmd->pipeline);
  AC_ASSERT(cmd->pipeline->type == ac_pipeline_type_graphics);
  AC_ASSERT(buffer);
  AC_ASSERT(offset <= buffer->size);
  AC_ASSERT(buffer->usage & ac_buffer_usage_index_bit);

  ac_device device = cmd->device;

  device->cmd_bind_index_buffer(cmd, buffer, offset, index_type);
}

AC_API void
ac_cmd_copy_buffer(
  ac_cmd    cmd,
  ac_buffer src,
  uint64_t  src_offset,
  ac_buffer dst,
  uint64_t  dst_offset,
  uint64_t  size)
{
  AC_ASSERT(cmd);
  AC_ASSERT(src);
  AC_ASSERT(dst);
  AC_ASSERT(dst_offset + size <= dst->size);
  AC_ASSERT(src->usage & ac_buffer_usage_transfer_src_bit);
  AC_ASSERT(dst->usage & ac_buffer_usage_transfer_dst_bit);

  ac_device device = cmd->device;

  device->cmd_copy_buffer(cmd, src, src_offset, dst, dst_offset, size);
}

AC_API void
ac_cmd_copy_buffer_to_image(
  ac_cmd                      cmd,
  ac_buffer                   src,
  ac_image                    dst,
  const ac_buffer_image_copy* copy)
{
  AC_ASSERT(cmd);
  AC_ASSERT(src);
  AC_ASSERT(dst);
  AC_ASSERT(copy);
  AC_ASSERT(copy->width);
  AC_ASSERT(copy->height);
  AC_ASSERT(src->usage & ac_buffer_usage_transfer_src_bit);
  AC_ASSERT(
    src->memory_usage == ac_memory_usage_cpu_to_gpu ||
    src->memory_usage == ac_memory_usage_gpu_only);
  AC_ASSERT(dst->usage & ac_image_usage_transfer_dst_bit);

  ac_device device = cmd->device;

  device->cmd_copy_buffer_to_image(cmd, src, dst, copy);
}

AC_API void
ac_cmd_copy_image(
  ac_cmd               cmd,
  ac_image             src,
  ac_image             dst,
  const ac_image_copy* copy)
{
  AC_ASSERT(cmd);
  AC_ASSERT(src);
  AC_ASSERT(dst);
  AC_ASSERT(copy);
  AC_ASSERT(copy->width);
  AC_ASSERT(copy->height);
  AC_ASSERT(src->usage & ac_image_usage_transfer_src_bit);
  AC_ASSERT(dst->usage & ac_image_usage_transfer_dst_bit);

  ac_device device = cmd->device;

  device->cmd_copy_image(cmd, src, dst, copy);
}

AC_API void
ac_cmd_copy_image_to_buffer(
  ac_cmd                      cmd,
  ac_image                    src,
  ac_buffer                   dst,
  const ac_buffer_image_copy* copy)
{
  AC_ASSERT(cmd);
  AC_ASSERT(src);
  AC_ASSERT(dst);
  AC_ASSERT(copy);
  AC_ASSERT(copy->width);
  AC_ASSERT(copy->height);
  AC_ASSERT(src->usage & ac_buffer_usage_transfer_src_bit);
  AC_ASSERT(dst->usage & ac_image_usage_transfer_dst_bit);
  AC_ASSERT(
    dst->memory_usage == ac_memory_usage_gpu_to_cpu ||
    dst->memory_usage == ac_memory_usage_gpu_only);

  ac_device device = cmd->device;

  device->cmd_copy_image_to_buffer(cmd, src, dst, copy);
}

AC_API void
ac_cmd_bind_set(
  ac_cmd               cmd,
  ac_descriptor_buffer db,
  ac_space             space,
  uint32_t             set)
{
  AC_ASSERT(cmd);
  AC_ASSERT(db);
  AC_ASSERT(cmd->pipeline);

  if (cmd->db == db && cmd->sets[space] == set)
  {
    return;
  }

  ac_device device = cmd->device;

  device->cmd_bind_set(cmd, db, space, set);
  cmd->db = db;
  cmd->sets[space] = set;
}

AC_API void
ac_cmd_dispatch(
  ac_cmd   cmd,
  uint32_t group_count_x,
  uint32_t group_count_y,
  uint32_t group_count_z)
{
  AC_ASSERT(cmd);
  AC_ASSERT(cmd->pipeline);
  AC_ASSERT(cmd->pipeline->type == ac_pipeline_type_compute);
  AC_ASSERT(group_count_x);
  AC_ASSERT(group_count_y);
  AC_ASSERT(group_count_z);

  ac_device device = cmd->device;

  device->cmd_dispatch(cmd, group_count_x, group_count_y, group_count_z);
}

AC_API void
ac_cmd_trace_rays(
  ac_cmd   cmd,
  ac_sbt   sbt,
  uint32_t width,
  uint32_t height,
  uint32_t depth)
{
  AC_ASSERT(cmd);
  AC_ASSERT(sbt);
  AC_ASSERT(width);
  AC_ASSERT(height);
  AC_ASSERT(depth);

  ac_device device = cmd->device;

  device->cmd_trace_rays(cmd, sbt, width, height, depth);
}

AC_API void
ac_cmd_build_as(ac_cmd cmd, ac_as_build_info* info)
{
  AC_ASSERT(cmd);
  AC_ASSERT(info);

  ac_device device = cmd->device;

  device->cmd_build_as(cmd, info);
}

AC_API void
ac_cmd_push_constants(ac_cmd cmd, uint32_t size, const void* data)
{
  AC_ASSERT(cmd);
  AC_ASSERT(cmd->pipeline);
  AC_ASSERT(data);
  AC_ASSERT(size <= AC_MAX_PUSH_CONSTANT_RANGE);
  AC_ASSERT(size % 4 == 0);

  ac_device device = cmd->device;

  device->cmd_push_constants(cmd, size, data);
}

AC_API ac_result
ac_create_buffer(ac_device device, const ac_buffer_info* info, ac_buffer* p)
{
  AC_ASSERT(device);
  AC_ASSERT(info);
  AC_ASSERT(info->size);
  AC_ASSERT(p);

  if (AC_INCLUDE_DEBUG)
  {
    if (
      (info->usage & ac_buffer_usage_srv_bit) ||
      (info->usage & ac_buffer_usage_uav_bit))
    {
      AC_ASSERT((info->size % 4) == 0);
    }
  }

  ac_result res = device->create_buffer(device, info, p);

  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_buffer(device, *p);
    ac_free(*p);
    *p = NULL;
    return res;
  }

  (*p)->device = device;
  (*p)->size = info->size;
  (*p)->usage = info->usage;
  (*p)->memory_usage = info->memory_usage;

  return res;
}

AC_API void
ac_destroy_buffer(ac_buffer buffer)
{
  if (!buffer)
  {
    return;
  }

  ac_device device = buffer->device;
  device->destroy_buffer(device, buffer);
  ac_free(buffer);
}

AC_API ac_result
ac_buffer_map_memory(ac_buffer buffer)
{
  AC_ASSERT(buffer);
  AC_ASSERT(buffer->mapped_memory == NULL);

  ac_device device = buffer->device;

  return device->map_memory(device, buffer);
}

AC_API void
ac_buffer_unmap_memory(ac_buffer buffer)
{
  AC_ASSERT(buffer);
  AC_ASSERT(buffer->mapped_memory);

  ac_device device = buffer->device;
  device->unmap_memory(device, buffer);
  buffer->mapped_memory = NULL;
}

AC_API void
ac_cmd_begin_debug_label(ac_cmd cmd, const char* name, const float color[4])
{
  if (!AC_INCLUDE_DEBUG)
  {
    return;
  }

  AC_ASSERT(cmd);
  AC_ASSERT(name);
  AC_UNUSED(color);

  ac_device device = cmd->device;

  if (cmd->pool->queue->device->debug_bits)
  {
    device->cmd_begin_debug_label(cmd, name, color);
  }
}

AC_API void
ac_cmd_end_debug_label(ac_cmd cmd)
{
  if (!AC_INCLUDE_DEBUG)
  {
    return;
  }

  AC_ASSERT(cmd);

  ac_device device = cmd->device;

  if (cmd->pool->queue->device->debug_bits)
  {
    device->cmd_end_debug_label(cmd);
  }
}

AC_API void
ac_cmd_insert_debug_label(ac_cmd cmd, const char* name, const float color[4])
{
  if (!AC_INCLUDE_DEBUG)
  {
    return;
  }

  AC_ASSERT(cmd);
  AC_ASSERT(name);
  AC_UNUSED(color);

  ac_device device = cmd->device;

  if (cmd->pool->queue->device->debug_bits)
  {
    device->cmd_insert_debug_label(cmd, name, color);
  }
}

AC_API void
ac_device_begin_capture(ac_device device)
{
  if (!AC_INCLUDE_DEBUG)
  {
    return;
  }

  AC_ASSERT(device);

  if (device->debug_bits)
  {
    device->begin_capture(device);
  }
}

AC_API void
ac_device_end_capture(ac_device device)
{
  if (!AC_INCLUDE_DEBUG)
  {
    return;
  }

  AC_ASSERT(device);

  if (device->debug_bits)
  {
    device->end_capture(device);
  }
}

AC_API ac_result
ac_create_sampler(ac_device device, const ac_sampler_info* info, ac_sampler* p)
{
  AC_ASSERT(device);
  AC_ASSERT(info);
  AC_ASSERT(p);
  AC_ASSERT(
    (!info->anisotropy_enable && info->max_anisotropy == 0) ||
    (info->max_anisotropy >= 1 && info->max_anisotropy <= 16));

  ac_result res = device->create_sampler(device, info, p);
  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_sampler(device, *p);
    ac_free(*p);
    *p = NULL;
  }

  (*p)->device = device;

  return res;
}

AC_API void
ac_destroy_sampler(ac_sampler sampler)
{
  if (!sampler)
  {
    return;
  }

  ac_device device = sampler->device;
  device->destroy_sampler(device, sampler);
  ac_free(sampler);
}

AC_API ac_result
ac_create_image(ac_device device, const ac_image_info* info, ac_image* p)
{
  AC_ASSERT(device);
  AC_ASSERT(info);
  AC_ASSERT(info->width > 0);
  AC_ASSERT(info->height > 0);
  AC_ASSERT(info->layers > 0);
  AC_ASSERT(info->levels > 0);
  AC_ASSERT(info->samples > 0);
  AC_ASSERT(p);
  AC_ASSERT(
    ((info->usage & ac_image_usage_attachment_bit) == 0) ||
    (info->levels == 1));

  ac_result res = device->create_image(device, info, p);

  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_image(device, *p);
    ac_free(*p);
    *p = NULL;
    return res;
  }

  (*p)->device = device;
  (*p)->width = info->width;
  (*p)->height = info->height;
  (*p)->format = info->format;
  (*p)->samples = info->samples;
  (*p)->levels = info->levels;
  (*p)->layers = info->layers;
  (*p)->usage = info->usage;
  (*p)->type = info->type;

  return res;
}

AC_API void
ac_destroy_image(ac_image image)
{
  if (!image)
  {
    return;
  }

  ac_device device = image->device;
  device->destroy_image(device, image);
  ac_free(image);
}

AC_API void
ac_update_set(
  ac_descriptor_buffer       db,
  ac_space                   space,
  uint32_t                   set,
  uint32_t                   count,
  const ac_descriptor_write* writes)
{
  AC_ASSERT(db);
  AC_ASSERT(count);
  AC_ASSERT(writes);

  ac_device device = db->device;

  for (uint32_t i = 0; i < count; ++i)
  {
    const ac_descriptor_write* w = &writes[i];

    if (
      AC_INCLUDE_DEBUG && (device->debug_bits & ac_device_debug_validation_bit))
    {
      for (uint32_t ii = 0; ii < w->count; ++ii)
      {
        const ac_descriptor* d = &w->descriptors[ii];
        AC_MAYBE_UNUSED(d);

        switch (w->type)
        {
        case ac_descriptor_type_srv_image:
        {
          AC_ASSERT(d->image);
          AC_ASSERT(d->image->usage & ac_image_usage_srv_bit);
          break;
        }
        case ac_descriptor_type_uav_image:
        {
          AC_ASSERT(d->image);
          AC_ASSERT(d->image->usage & ac_image_usage_uav_bit);
          break;
        }
        case ac_descriptor_type_cbv_buffer:
        {
          ac_device_properties props = ac_device_get_properties(device);
          AC_MAYBE_UNUSED(props);

          AC_ASSERT(d->buffer);
          AC_ASSERT(d->buffer->usage & ac_buffer_usage_cbv_bit);
          AC_ASSERT(d->buffer->size > d->offset);
          AC_ASSERT((d->offset % props.cbv_buffer_alignment) == 0);
          AC_ASSERT((d->range == AC_WHOLE_SIZE) || (d->range <= UINT32_MAX));
          break;
        }
        case ac_descriptor_type_srv_buffer:
        {
          AC_ASSERT(d->buffer);
          AC_ASSERT(d->buffer->usage & ac_buffer_usage_srv_bit);
          AC_ASSERT((d->offset % 4) == 0);
          // NOTE: AC_WHOLE_SIZE handled in buffer creation
          AC_ASSERT((d->range % 4) == 0);
          AC_ASSERT(d->buffer->size > d->offset);
          break;
        }
        case ac_descriptor_type_uav_buffer:
        {
          AC_ASSERT(d->buffer);
          AC_ASSERT(d->buffer->usage & ac_buffer_usage_uav_bit);
          AC_ASSERT((d->offset % 4) == 0);
          // NOTE: AC_WHOLE_SIZE handled in buffer creation
          AC_ASSERT((d->range % 4) == 0);
          AC_ASSERT(d->buffer->size > d->offset);
          break;
        }
        case ac_descriptor_type_sampler:
        {
          AC_ASSERT(d->sampler);
          break;
        }
        case ac_descriptor_type_as:
        {
          AC_ASSERT(d->as);
          break;
        }
        default:
        {
          AC_ASSERT(false);
          break;
        }
        }
      }
    }

    device->update_descriptor(device, db, space, set, w);
  }
}

AC_API ac_result
ac_create_sbt(ac_device device, const ac_sbt_info* info, ac_sbt* p)
{
  AC_ASSERT(device);
  AC_ASSERT(info);
  AC_ASSERT(p);

  ac_result res = device->create_sbt(device, info, p);

  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_sbt(device, *p);
    ac_free(*p);
    *p = NULL;
  }

  (*p)->device = device;

  return res;
}

AC_API void
ac_destroy_sbt(ac_sbt sbt)
{
  if (!sbt)
  {
    return;
  }

  ac_device device = sbt->device;
  device->destroy_sbt(device, sbt);
  ac_free(sbt);
}

AC_API ac_result
ac_create_as(ac_device device, const ac_as_info* info, ac_as* p)
{
  AC_ASSERT(device);
  AC_ASSERT(info);
  AC_ASSERT(p);

  *p = NULL;

  ac_result res;

  switch (info->type)
  {
  case ac_as_type_bottom_level:
  {
    res = device->create_blas(device, info, p);
    break;
  }
  case ac_as_type_top_level:
  {
    res = device->create_tlas(device, info, p);
    break;
  }
  default:
  {
    AC_DEBUGBREAK();
    return ac_result_invalid_argument;
  }
  }

  if ((*p)->scratch_size == 0)
  {
    AC_DEBUGBREAK();
    res = ac_result_unknown_error;
  }

  if (res != ac_result_success)
  {
    AC_DEBUGBREAK();
    device->destroy_as(device, *p);
    ac_free(*p);
    *p = NULL;
    return res;
  }

  (*p)->device = device;
  (*p)->type = info->type;

  return res;
}

AC_API void
ac_destroy_as(ac_as as)
{
  if (!as)
  {
    return;
  }

  ac_device device = as->device;
  device->destroy_as(device, as);
  ac_free(as);
}

AC_API void
ac_write_as_instances(
  ac_device             device,
  uint32_t              count,
  const ac_as_instance* instances,
  void*                 mem)
{
  AC_ASSERT(device);
  AC_ASSERT(count);
  AC_ASSERT(instances);
  AC_ASSERT(mem);

  device->write_as_instances(device, count, instances, mem);
}

/*************************************************/
/*                 utils                         */
/*************************************************/

AC_API uint32_t
ac_format_size_bytes(ac_format format)
{
  return ac_format_size_bytes_internal(format);
}

AC_API bool
ac_format_has_depth_aspect(ac_format format)
{
  return ac_format_has_depth_aspect_internal(format);
}

AC_API bool
ac_format_has_stencil_aspect(ac_format format)
{
  return ac_format_has_depth_aspect_internal(format);
}

AC_API bool
ac_format_is_srgb(ac_format format)
{
  return ac_format_is_srgb_internal(format);
}

AC_API ac_format
ac_format_to_unorm(ac_format format)
{
  return ac_format_to_unorm_internal(format);
}

AC_API uint32_t
ac_format_channel_count(ac_format format)
{
  return ac_format_channel_count_internal(format);
}

/*************************************************/
/*                 getters                       */
/*************************************************/

AC_API bool
ac_device_support_raytracing(ac_device device)
{
  AC_ASSERT(device);
  return device->support_raytracing;
}

AC_API bool
ac_device_support_mesh_shaders(ac_device device)
{
  AC_ASSERT(device);
  return device->support_mesh_shaders;
}

AC_API ac_device_properties
ac_device_get_properties(ac_device device)
{
  AC_ASSERT(device);
  return device->props;
}

AC_API ac_queue
ac_device_get_queue(ac_device device, ac_queue_type type)
{
  AC_ASSERT(device);
  AC_ASSERT(type < ac_queue_type_count);
  return device->queues[device->queue_map[type]];
}

AC_API uint32_t
ac_image_get_width(ac_image image)
{
  AC_ASSERT(image);
  return image->width;
}

AC_API uint32_t
ac_image_get_height(ac_image image)
{
  AC_ASSERT(image);
  return image->height;
}

AC_API ac_format
ac_image_get_format(ac_image image)
{
  AC_ASSERT(image);
  return image->format;
}

AC_API uint8_t
ac_image_get_samples(ac_image image)
{
  AC_ASSERT(image);
  return image->samples;
}

AC_API uint16_t
ac_image_get_levels(ac_image image)
{
  AC_ASSERT(image);
  return image->levels;
}

AC_API uint16_t
ac_image_get_layers(ac_image image)
{
  AC_ASSERT(image);
  return image->layers;
}

AC_API ac_image_usage_bits
ac_image_get_usage(ac_image image)
{
  AC_ASSERT(image);
  return image->usage;
}

AC_API ac_image_type
ac_image_get_type(ac_image image)
{
  AC_ASSERT(image);
  return image->type;
}

AC_API ac_image_info
ac_image_get_info(ac_image image)
{
  AC_ASSERT(image);
  return (ac_image_info) {
    .width = image->width,
    .height = image->height,
    .format = image->format,
    .samples = image->samples,
    .levels = image->levels,
    .layers = image->layers,
    .usage = image->usage,
    .type = image->type,
  };
}

AC_API ac_image
ac_swapchain_get_image(ac_swapchain s)
{
  AC_ASSERT(s);
  AC_ASSERT(s->image_index < s->image_count);
  return s->images[s->image_index];
}

AC_API ac_color_space
ac_swapchain_get_color_space(ac_swapchain swapchain)
{
  AC_ASSERT(swapchain);
  return swapchain->color_space;
}

AC_API uint64_t
ac_buffer_get_size(ac_buffer buffer)
{
  AC_ASSERT(buffer);
  return buffer->size;
}

AC_API void*
ac_buffer_get_mapped_memory(ac_buffer buffer)
{
  AC_ASSERT(buffer);
  return buffer->mapped_memory;
}

AC_API ac_buffer_info
ac_buffer_get_info(ac_buffer buffer)
{
  AC_ASSERT(buffer);
  return (ac_buffer_info) {
    .size = buffer->size,
    .usage = buffer->usage,
    .memory_usage = buffer->memory_usage,
  };
}

AC_API ac_result
ac_shader_get_workgroup(ac_shader shader, uint8_t _wg[3])
{
  AC_ASSERT(shader);

  ac_shader_workgroup wg;
  AC_ZERO(wg);
  AC_RIF(ac_shader_compiler_get_workgroup(shader->reflection, &wg));

  _wg[0] = wg.x;
  _wg[1] = wg.y;
  _wg[2] = wg.z;

  return ac_result_success;
}

AC_API uint64_t
ac_as_get_scratch_size(ac_as as)
{
  AC_ASSERT(as);
  return as->scratch_size;
}

#endif
