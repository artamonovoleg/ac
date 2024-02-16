#include "ac_private.h"

#if (AC_INCLUDE_RG)

#include "render_graph.h"

typedef struct ac_rg_execute_context {
  ac_rg_builder       builder;
  ac_rg_stage         info;
  ac_rg_graph_stage*  stage;
  bool                global_queue_labeled[ac_queue_type_count];
  ac_rg_builder_group group_queue_labels[ac_queue_type_count];
  uint32_t            queue_index;
} ac_rg_execute_context;

static uint64_t
ac_rg_pipeline_hash(const void* item, uint64_t seed0, uint64_t seed1)
{
  const ac_rg_pipeline* pipeline = item;
  return hashmap_murmur(&pipeline->info, sizeof(pipeline->info), seed0, seed1);
}

static int32_t
ac_rg_pipeline_compare(const void* a, const void* b, void* udata)
{
  AC_UNUSED(udata);
  const ac_rg_pipeline* pa = a;
  const ac_rg_pipeline* pb = b;
  return memcmp(&pa->info, &pb->info, sizeof(pa->info));
}

static void
ac_rg_destroy_pipelines(ac_rg_pipelines* pipelines)
{
  if (!pipelines->hashmap)
  {
    return;
  }

  size_t i = 0;
  void*  ptr;
  while (hashmap_iter(pipelines->hashmap, &i, &ptr))
  {
    ac_rg_pipeline* pipeline = ptr;

    ac_destroy_pipeline(pipeline->pipeline);
  }

  hashmap_free(pipelines->hashmap);
  pipelines->hashmap = NULL;
}

static void
ac_rg_destroy_fences(ac_rg_timelines* fences)
{
  for (size_t i = 0; i < AC_COUNTOF(fences->fences); ++i)
  {
    ac_destroy_fence(fences->fences[i]);
  }
}

static ac_result
ac_rg_create_fences(ac_device device, ac_rg_timelines* fences)
{
  for (uint32_t i = 0; i < AC_COUNTOF(fences->fences); ++i)
  {
    ac_result res =
      ac_create_fence(device, &(ac_fence_info) {0}, fences->fences + i);
    if (res != ac_result_success)
    {
      ac_rg_destroy_fences(fences);
      return res;
    }
  }
  return ac_result_success;
}

static ac_result
ac_rg_cmd_wait_timelines(ac_rg rg, ac_rg_cmd* cmd, bool frame_index)
{
  if (cmd->frame_states[frame_index] != ac_rg_frame_state_running)
  {
    return ac_result_success;
  }

  uint64_t* signaled_queue_times = rg->timelines.signaled_values.queue_times;

  for (size_t i = 0; i < AC_COUNTOF(rg->timelines.fences); ++i)
  {
    uint64_t value = cmd->signaling_values[frame_index].queue_times[i];
    if (value <= signaled_queue_times[i])
    {
      continue;
    }

    ac_result res = ac_wait_fence(rg->timelines.fences[i], value);
    if (res != ac_result_success)
    {
      return res;
    }

    if (signaled_queue_times[i] < value)
    {
      signaled_queue_times[i] = value;
    }
  }

  cmd->frame_states[frame_index] = ac_rg_frame_state_idle;

  return ac_result_success;
}

static void
ac_rg_destroy_cmd(ac_rg rg, ac_rg_cmd* rg_cmd)
{
  for (size_t frame = 0; frame < AC_MAX_FRAME_IN_FLIGHT; ++frame)
  {
    (void)ac_rg_cmd_wait_timelines(rg, rg_cmd, frame);
    for (size_t i = 0; i < AC_COUNTOF(rg_cmd->pools[frame]); ++i)
    {
      ac_rg_cmd_pool* pool = rg_cmd->pools[frame] + i;

      if (pool->pool)
      {
        for (uint32_t c = 0; c < array_size(pool->cmds); ++c)
        {
          ac_destroy_cmd(pool->cmds[c]);
        }

        ac_destroy_cmd_pool(pool->pool);
      }

      pool->acquired_count = 0;

      array_free(pool->cmds);
      pool->cmds = NULL;
    }
  }
}

static ac_result
ac_rg_create_cmd(ac_rg rg, ac_rg_cmd* rg_cmd, ac_device device)
{
  AC_ZEROP(rg_cmd);

  ac_result res = ac_result_success;

  for (size_t frame = 0;
       res == ac_result_success && frame < AC_MAX_FRAME_IN_FLIGHT;
       ++frame)
  {
    for (size_t i = 0; i < AC_COUNTOF(rg_cmd->pools[frame]); ++i)
    {
      ac_rg_cmd_pool* pool = rg_cmd->pools[frame] + i;

      res = ac_create_cmd_pool(
        device,
        &(ac_cmd_pool_info) {
          .queue = ac_device_get_queue(device, (ac_queue_type)i),
        },
        &pool->pool);
      if (res != ac_result_success)
      {
        break;
      }

      pool->acquired_count = 0;
    }
  }

  if (res != ac_result_success)
  {
    ac_rg_destroy_cmd(rg, rg_cmd);
    return res;
  }

  rg_cmd->frame_states[0] = ac_rg_frame_state_pending;

  return ac_result_success;
}

static ac_result
ac_rg_cmd_get_cmd(ac_rg_cmd* rg_cmd, ac_queue_type queue, ac_cmd* out)
{
  ac_rg_cmd_pool* pool = &rg_cmd->pools[rg_cmd->frame_running][queue];

  if (array_size(pool->cmds) == pool->acquired_count)
  {
    ac_cmd    cmd;
    ac_result res = ac_create_cmd(pool->pool, &cmd);
    if (res != ac_result_success)
    {
      return res;
    }

    array_append(pool->cmds, cmd);

    // TODO cmd debug name
    /*
        p_debug->name(
            p_d->device,
            VK_OBJECT_TYPE_COMMAND_BUFFER,
            cmd,
            "CmdData pool command buffer");
    */
  }

  *out = pool->cmds[pool->acquired_count++];
  return ac_result_success;
}

static ac_result
ac_rg_cmd_run_frame(ac_rg_cmd* rg_cmd, ac_device device)
{
  AC_UNUSED(device);

  // TODO shrink pool->cmds
  for (uint32_t i = 0; i < AC_COUNTOF(rg_cmd->pools[rg_cmd->frame_pending]);
       ++i)
  {
    ac_rg_cmd_pool* pool = rg_cmd->pools[rg_cmd->frame_pending] + i;

    pool->acquired_count = 0;

    ac_result res = ac_reset_cmd_pool(pool->pool);
    if (res != ac_result_success)
    {
      return res;
    }
  }

  rg_cmd->frame_states[rg_cmd->frame_pending] = ac_rg_frame_state_running;

  rg_cmd->frame_running = rg_cmd->frame_pending;
  rg_cmd->frame_pending = !rg_cmd->frame_running;

  return ac_result_success;
}

ac_result
ac_rg_cmd_acquire_frame(ac_rg rg, ac_rg_cmd* rg_cmd)
{
  ac_result res = ac_rg_cmd_wait_timelines(rg, rg_cmd, rg_cmd->frame_pending);
  if (res != ac_result_success)
  {
    return res;
  }

  rg_cmd->frame_states[rg_cmd->frame_pending] = ac_rg_frame_state_pending;
  return ac_result_success;
}

AC_API ac_result
ac_create_rg(ac_device device, ac_rg* out_graph)
{
  *out_graph = NULL;

  ac_rg rg = ac_calloc(sizeof *rg);
  if (!rg)
  {
    return ac_result_out_of_host_memory;
  }

  rg->callback.severity_filter_mask = UINT32_MAX;
  rg->callback.type_filter_mask = UINT32_MAX;
#if (AC_INCLUDE_DEBUG)
  if (device->debug_bits)
  {
    rg->callback.function = ac_rg_default_validation_callback;
  }
#endif

  rg->device = device;

  ac_result res = ac_result_success;

  if (res == ac_result_success)
  {
    res = ac_rg_create_fences(device, &rg->timelines);
  }

  if (res == ac_result_success)
  {
    res = ac_rg_create_common_passes(&rg->common_passes, device);
  }

  if (res != ac_result_success)
  {
    ac_destroy_rg(rg);
    rg = NULL;
  }

  *out_graph = rg;
  return res;
}

AC_API void
ac_destroy_rg(ac_rg rg)
{
  if (!rg)
  {
    return;
  }

  AC_ASSERT(rg->graph_count == 0);

  ac_rg_destroy_storage(rg);
  ac_rg_destroy_common_passes(&rg->common_passes);
  ac_rg_destroy_fences(&rg->timelines);
  ac_rg_destroy_pipelines(&rg->pipelines);

  ac_free(rg);
}

AC_API ac_result
ac_rg_create_graph(
  ac_rg                   rg,
  const ac_rg_graph_info* info,
  ac_rg_graph*            out_graph)
{
  *out_graph = NULL;

  AC_ASSERT(rg);
  AC_ASSERT(info);
  AC_ASSERT(out_graph);
  AC_ASSERT(info->cb_build);

  ac_rg_builder builder = ac_calloc(sizeof *builder);
  if (!builder)
  {
    return ac_result_out_of_host_memory;
  }

  builder->info = *info;
  builder->rg = rg;
  builder->write_barrier_nodes = AC_RG_BARRIERS_PRINTF;

  ac_result res = ac_rg_create_cmd(rg, &builder->cmd, builder->rg->device);
  if (res != ac_result_success)
  {
    ac_free(builder);
    return res;
  }

  ++rg->graph_count;

  *out_graph = (ac_rg_graph)builder;
  return ac_result_success;
}

AC_API ac_result
ac_rg_graph_wait_idle(ac_rg_graph graph)
{
  ac_rg_builder builder = (ac_rg_builder)graph;

  for (uint8_t i = 0; i < AC_MAX_FRAME_IN_FLIGHT; ++i)
  {
    ac_result res = ac_rg_cmd_wait_timelines(builder->rg, &builder->cmd, i);
    if (res != ac_result_success)
    {
      return res;
    }
  }

  return ac_result_success;
}

AC_API void
ac_rg_destroy_graph(ac_rg_graph graph)
{
  if (!graph)
  {
    return;
  }

  ac_rg_builder builder = (ac_rg_builder)graph;

  AC_ASSERT(builder->rg->graph_count);

  ac_rg_destroy_cmd(builder->rg, &builder->cmd);
  --builder->rg->graph_count;

  ac_rg_destroy_builder(builder);
  ac_free(builder);
}

static inline ac_rg_graph_resource*
ac_rg_search_resource(
  ac_rg_graph_resource_reference* resources,
  ac_rg_resource_type             type,
  uint64_t                        token,
  ac_rg_graph_resource_use**      out_use)
{
  AC_MAYBE_UNUSED(type);

  for (size_t index = 0; index < array_size(resources); ++index)
  {
    *out_use = ac_rg_graph_resource_dereference_use(resources[index]);
    if ((*out_use)->token == token)
    {
      AC_ASSERT(resources[index].resource->type == type);
      return resources[index].resource;
    }
  }

  return NULL;
}

AC_API ac_image
ac_rg_stage_get_image_subresource(
  ac_rg_stage*                user_stage,
  uint64_t                    token,
  ac_image_subresource_range* out_range)
{
  ac_rg_stage_internal* stage = (ac_rg_stage_internal*)user_stage;

  ac_rg_graph_resource_use* use;

  ac_rg_graph_resource* resource = ac_rg_search_resource(
    stage->pass->attachments,
    ac_rg_resource_type_image,
    token,
    &use);

  if (!resource)
  {
    resource = ac_rg_search_resource(
      stage->pass->images,
      ac_rg_resource_type_image,
      token,
      &use);
  }

  if (resource)
  {
    AC_ASSERT(
      (use->specifics.image_range.layers == 0 &&
       use->specifics.image_range.levels == 0) ||
      out_range);
    if (out_range)
    {
      *out_range = use->specifics.image_range;
    }
    return resource->image;
  }
  return NULL;
}

AC_API ac_image
ac_rg_stage_get_image(ac_rg_stage* user_stage, uint64_t token)
{
  return ac_rg_stage_get_image_subresource(user_stage, token, NULL);
}

AC_API ac_buffer
ac_rg_stage_get_buffer(ac_rg_stage* user_stage, uint64_t token)
{
  ac_rg_graph_resource_use* use;

  ac_rg_stage_internal* stage = (ac_rg_stage_internal*)user_stage;
  return ac_rg_search_resource(
           stage->pass->buffers,
           ac_rg_resource_type_buffer,
           token,
           &use)
    ->buffer;
}

AC_API ac_result
ac_rg_stage_get_pipeline(
  ac_rg_stage*               stage,
  const ac_rg_pipeline_info* info,
  ac_pipeline*               p)
{
  *p = NULL;

  ac_rg_stage_internal*      stage_internal = (ac_rg_stage_internal*)stage;
  ac_rg                      rg = stage_internal->rg;
  ac_rg_graph_stage*         graph_stage = stage_internal->stage;
  ac_rg_graph_stage_subpass* pass = stage_internal->pass;

  if (!pass)
  {
    AC_ASSERT(0);
    return ac_result_bad_usage;
  }

  if (!rg->pipelines.hashmap)
  {
    rg->pipelines.hashmap = hashmap_new(
      sizeof(ac_rg_pipeline),
      20,
      0,
      0,
      ac_rg_pipeline_hash,
      ac_rg_pipeline_compare,
      NULL,
      NULL);
    if (!rg->pipelines.hashmap)
    {
      return ac_result_out_of_host_memory;
    }
  }

  ac_rg_pipeline pipeline;
  AC_ZERO(pipeline);

  ac_pipeline_info* dst = &pipeline.info;

  if (info->type == ac_pipeline_type_graphics)
  {
    dst->type = ac_pipeline_type_graphics;
    ac_graphics_pipeline_info* graphics = &pipeline.info.graphics;

    graphics->vertex_shader = info->vertex_shader;
    graphics->pixel_shader = info->pixel_shader;
    graphics->dsl = info->dsl;

    graphics->color_attachment_count =
      graph_stage->rendering.color_attachment_count;

    if (
      info->vertex_layout.binding_count && info->vertex_layout.attribute_count)
    {
      graphics->vertex_layout.binding_count = info->vertex_layout.binding_count;
      for (uint32_t i = 0; i < info->vertex_layout.binding_count; ++i)
      {
        graphics->vertex_layout.bindings[i] = info->vertex_layout.bindings[i];
      }

      graphics->vertex_layout.attribute_count =
        info->vertex_layout.attribute_count;
      for (size_t i = 0; i < info->vertex_layout.attribute_count; ++i)
      {
        graphics->vertex_layout.attributes[i] =
          info->vertex_layout.attributes[i];
      }
    }

    graphics->rasterizer_info = info->rasterizer_info;
    graphics->topology = info->topology;

    for (uint32_t i = 0; i < graphics->color_attachment_count; ++i)
    {
      graphics->color_attachment_formats[i] =
        ac_image_get_format(graph_stage->rendering.color_attachments[i].image);
      graphics->color_attachment_discard_bits[i] =
        info->color_attachment_discard_bits[i];
      graphics->blend_state_info.attachment_states[i] =
        info->blend_state_info.attachment_states[i];
    }

    if (graph_stage->rendering.depth_attachment.image)
    {
      graphics->depth_stencil_format =
        ac_image_get_format(graph_stage->rendering.depth_attachment.image);

      graphics->depth_state_info = info->depth_state_info;

      for (size_t i = 0; i < array_size(pass->attachments); ++i)
      {
        ac_rg_graph_resource_reference* reference = &pass->attachments[i];

        if (ac_format_depth_or_stencil(reference->resource->image_info.format))
        {
          ac_rg_graph_resource_use* use =
            ac_rg_graph_resource_dereference_use(*reference);
          if (
            graphics->depth_state_info.depth_write &&
            !use->specifics.access_write.access)
          {
            AC_ASSERT(false);
            return ac_result_bad_usage;
          }

          if (
            graphics->depth_state_info.depth_test &&
            !use->specifics.access_read.access)
          {
            AC_ASSERT(false);
            return ac_result_bad_usage;
          }
          break;
        }
      }
    }

    if (graphics->color_attachment_count)
    {
      graphics->samples =
        ac_image_get_samples(graph_stage->rendering.color_attachments[0].image);
    }
    else if (graph_stage->rendering.depth_attachment.image)
    {
      graphics->samples =
        ac_image_get_samples(graph_stage->rendering.depth_attachment.image);
    }
    else
    {
      graphics->samples = 1;
    }
  }
  else if (info->type == ac_pipeline_type_compute)
  {
    dst->type = ac_pipeline_type_compute;

    dst->compute.dsl = info->dsl;
    dst->compute.shader = info->compute_shader;
  }
  else
  {
    AC_DEBUG("rt pipelines unimplemented");
    return ac_result_unknown_error;
  }

  {
    ac_rg_pipeline* old_pipeline =
      hashmap_get(rg->pipelines.hashmap, &pipeline);
    if (old_pipeline)
    {
      *p = old_pipeline->pipeline;
      return ac_result_success;
    }
  }

  dst->name = info->name;
  if (!dst->name)
  {
    dst->name = pass->stage_info.name;
  }

  ac_result res =
    ac_create_pipeline(rg->device, &pipeline.info, &pipeline.pipeline);
  if (res != ac_result_success)
  {
    return res;
  }
  dst->name = NULL;

  (void)hashmap_set(rg->pipelines.hashmap, &pipeline);
  if (hashmap_oom(rg->pipelines.hashmap))
  {
    ac_destroy_pipeline(pipeline.pipeline);
    return ac_result_out_of_host_memory;
  }

  *p = pipeline.pipeline;
  return ac_result_success;
}

static void
ac_rg_execute_stage(ac_rg_execute_context* ctx, uint32_t subpass)
{
  static const float red[4] = {1, 0.5, 0.5, 1};
  static const float green[4] = {0.5, 1, 0.5, 1};
  static const float blue[4] = {0.5, 0.5, 1, 1};
  static const float gray[4] = {0.5, 0.5, 0.5, 1};

  const float* color = gray;
  switch (ctx->stage->queue_type)
  {
  case ac_queue_type_graphics:
    color = red;
    break;
  case ac_queue_type_compute:
    color = green;
    break;
  case ac_queue_type_transfer:
    color = blue;
    break;
  case ac_queue_type_count:
    break;
  }

  ac_rg_graph_stage_subpass* pass = &ctx->stage->subpasses[subpass];

  ac_rg_builder_stage_info* cb = &pass->stage_info;

  if (cb->group != ctx->group_queue_labels[ctx->queue_index])
  {
    if (ctx->group_queue_labels[ctx->queue_index])
    {
      ac_cmd_end_debug_label(ctx->info.cmd);
    }

    ctx->group_queue_labels[ctx->queue_index] = cb->group;

    if (cb->group)
    {
      ac_rg_builder_group group =
        &ctx->builder->groups[(uint64_t)cb->group - 1];
      if (group->info.name)
      {
        float color2[4] = {
          color[0] * 0.7f,
          color[1] * 0.7f,
          color[2] * 0.7f,
          1,
        };
        ac_cmd_begin_debug_label(ctx->info.cmd, group->info.name, color2);
      }
      else
      {
        ctx->group_queue_labels[ctx->queue_index] = 0;
      }
    }
  }

  if (cb->name)
  {
    ac_cmd_begin_debug_label(ctx->info.cmd, cb->name, color);
  }

  if (cb->cb_cmd)
  {
    ac_rg_stage_internal user_stage = {
      .info = ctx->info,
      .pass = pass,
      .stage = ctx->stage,
      .rg = ctx->builder->rg,
    };

    user_stage.info.metadata = cb->metadata;

    cb->cb_cmd(&user_stage.info, cb->user_data);
  }

  if (cb->name)
  {
    ac_cmd_end_debug_label(ctx->info.cmd);
  }
}

static void
ac_rg_stage_cmd(ac_rg_execute_context* ctx)
{
  if (ctx->stage->is_render_pass)
  {
    ac_cmd_begin_rendering(ctx->info.cmd, &ctx->stage->rendering);
  }

  for (uint32_t si = 0; si < array_size(ctx->stage->subpasses); ++si)
  {
#if AC_RG_TODO
    if (si > 0 && stage->is_render_pass)
    {
      vkCmdNextSubpass(info->cmd, VK_SUBPASS_CONTENTS_INLINE);
    }
#endif
    ac_rg_execute_stage(ctx, si);
  }

  if (ctx->stage->is_render_pass)
  {
    ac_cmd_end_rendering(ctx->info.cmd);
  }
}

static inline void
ac_rg_cmd_barrier(ac_cmd cmd, ac_rg_graph_stage_barrier* barrier)
{
  uint32_t nb = (uint32_t)array_size(barrier->buffers);
  uint32_t ni = (uint32_t)array_size(barrier->images);

  if (nb == 0 && ni == 0)
  {
    return;
  }

  ac_cmd_barrier(cmd, nb, barrier->buffers, ni, barrier->images);
}

static inline void
ac_rg_update_resource_specifics(
  const ac_rg_graph_resource_reference* use_ref,
  uint64_t                              sem_value_offsets[ac_queue_type_count])
{
  ac_rg_resource rgr = use_ref->resource->rg_resource;
  if (rgr)
  {
    ac_rg_graph_resource_use* use =
      ac_rg_graph_resource_dereference_use(*use_ref);
    rgr->last_use_specifics = use->specifics;
    rgr->last_use_specifics.time.timeline +=
      sem_value_offsets[use->specifics.time.queue_index];
    rgr->release_time = rgr->last_use_specifics.time;
  }
}

static void
ac_rg_on_stage_submit_success(
  ac_rg_builder          builder,
  ac_rg_execute_context* ctx,
  uint64_t               sem_value_offsets[ac_queue_type_count])
{
  for (uint32_t subpass_index = 0;
       subpass_index < array_size(ctx->stage->subpasses);
       ++subpass_index)
  {
    ac_rg_graph_stage_subpass* subpass = &ctx->stage->subpasses[subpass_index];

    for (size_t i = 0; i < array_size(subpass->attachments); ++i)
    {
      ac_rg_update_resource_specifics(
        &subpass->attachments[i],
        sem_value_offsets);
    }

    for (size_t i = 0; i < array_size(subpass->images); ++i)
    {
      ac_rg_update_resource_specifics(&subpass->images[i], sem_value_offsets);
    }

    for (size_t i = 0; i < array_size(subpass->buffers); ++i)
    {
      ac_rg_update_resource_specifics(&subpass->images[i], sem_value_offsets);
    }

    ac_rg_builder_stage_info* builder_stage_info = &subpass->stage_info;

    if (builder_stage_info->cb_submit)
    {
      ctx->info.metadata = builder_stage_info->metadata;

      ac_rg_stage_internal user_stage = {
        .info = ctx->info,
        .pass = subpass,
        .stage = ctx->stage,
        .rg = builder->rg,
      };

      builder_stage_info->cb_submit(
        &user_stage.info,
        builder_stage_info->user_data);
    }
  }
}

static ac_result
ac_rg_execute(ac_rg_builder builder)
{
  ac_device device = builder->rg->device;

  ac_result res = ac_rg_cmd_run_frame(&builder->cmd, device);
  if (res != ac_result_success)
  {
    return res;
  }

  ac_rg_cmd* rg_cmd = &builder->cmd;

  bool prev_frame_index = !rg_cmd->frame_running;

  ac_rg_timeline* curr_timeline =
    &builder->cmd.signaling_values[rg_cmd->frame_running];
  ac_rg_timeline* prev_timeline =
    &builder->cmd.signaling_values[prev_frame_index];

  int32_t executed_inds[ac_queue_type_count];
  for (size_t ii = 0; ii < AC_COUNTOF(executed_inds); ++ii)
  {
    executed_inds[ii] = -1;
  }

  bool done = 0;

  ac_rg_timelines* timelines = &builder->rg->timelines;

  uint64_t sem_value_offsets[ac_queue_type_count];
  for (size_t qi = 0; qi < ac_queue_type_count; ++qi)
  {
    sem_value_offsets[qi] = timelines->signaling_values.queue_times[qi] + 1;
  }

  array_t(ac_fence_submit_info) signals = NULL;
  array_t(ac_fence_submit_info) waits = NULL;

  bool wait_prev_frame[ac_queue_type_count] = {0};

  if (rg_cmd->frame_states[prev_frame_index] == ac_rg_frame_state_running)
  {
    for (size_t qi = 0; qi < ac_queue_type_count; ++qi)
    {
      wait_prev_frame[qi] = 1;
    }
  }

  ac_rg_execute_context ctx;
  AC_ZERO(ctx);
  ctx.builder = builder;

WHILE_NOT_DONE:

  done = 1;

  for (uint32_t qi = 0; qi < ac_queue_type_count; ++qi)
  {
    ac_rg_graph_stage** queue_stages = builder->stage_queues[qi];

    uint32_t stage_index = (uint32_t)(executed_inds[qi] + 1);

    if (stage_index >= array_size(queue_stages))
    {
      continue;
    }

    done = 0;

    ctx.queue_index = qi;
    ctx.stage = queue_stages[stage_index];

    array_clear(signals);
    array_clear(waits);

    for (uint32_t qi2 = 0; qi2 < ac_queue_type_count; ++qi2)
    {
      if (qi2 == qi)
      {
        continue;
      }

      ac_rg_graph_stage_dependency* dependency = &ctx.stage->dependencies[qi2];
      if (dependency->wait_stage_bits == 0)
      {
        continue;
      }

      if (
        executed_inds[qi2] <
        (int32_t)(dependency->timeline - sem_value_offsets[qi2]))
      {
        goto CONTINUE;
      }

      ac_fence_submit_info fence_info = {
        .fence = timelines->fences[qi2],
        .stages = dependency->wait_stage_bits,
        .value = dependency->timeline,
      };
      array_append(waits, fence_info);
    }

    if (AC_RG_BARRIERS_PRINTF)
    {
      ac_rg_print_stage_barriers(ctx.stage);
    }

    ac_cmd cmd;
    res = ac_rg_cmd_get_cmd(&builder->cmd, ctx.stage->queue_type, &cmd);
    if (res != ac_result_success)
    {
      return res;
    }

    res = ac_begin_cmd(cmd);
    if (res != ac_result_success)
    {
      goto CANCEL;
    }

    if (!ctx.global_queue_labeled[qi] && builder->info.name)
    {
      float color[4] = {0.2f, 0.5f, 0.7f, 1};
      ac_cmd_begin_debug_label(cmd, builder->info.name, color);
      ctx.global_queue_labeled[qi] = true;
    }

    ac_rg_cmd_barrier(cmd, &ctx.stage->barrier_beg);

    ctx.info = (ac_rg_stage) {
      .device = device,
      .cmd = cmd,
      .frame = rg_cmd->frame_running,
    };

    memcpy(
      ctx.info.frame_states,
      rg_cmd->frame_states,
      sizeof ctx.info.frame_states);

    ac_rg_stage_cmd(&ctx);

    ac_rg_cmd_barrier(cmd, &ctx.stage->barrier_end);

    if (stage_index + 1 == array_size(queue_stages))
    {
      if (ctx.group_queue_labels[qi])
      {
        ac_cmd_end_debug_label(cmd);
      }

      if (ctx.global_queue_labeled[qi])
      {
        ac_cmd_end_debug_label(cmd);
      }
    }

    res = ac_end_cmd(cmd);
    if (res != ac_result_success)
    {
      goto CANCEL;
    }

    {
      ac_fence_submit_info fence_info = {
        .fence = timelines->fences[qi],
        // TODO			  .stages = 0,
        .value = timelines->signaling_values.queue_times[qi] + 1,
      };
      array_append(signals, fence_info);
    }

    for (size_t fi = 0; fi < array_size(ctx.stage->fences_signal); ++fi)
    {
      array_append(signals, ctx.stage->fences_signal[fi]);
    }

    for (size_t fi = 0; fi < array_size(ctx.stage->fences_wait); ++fi)
    {
      array_append(waits, ctx.stage->fences_wait[fi]);
    }

    // TODO this wait seems useless
    if (wait_prev_frame[qi])
    {
      ac_fence_submit_info fence_info = {
        .fence = timelines->fences[qi],
        .stages = ac_pipeline_stage_top_of_pipe_bit,
        .value = prev_timeline->queue_times[qi],
      };
      array_append(waits, fence_info);
    }

    ac_queue_submit_info submit_info = {
      .cmd_count = 1,
      .cmds = &cmd,
      .wait_fence_count = (uint32_t)array_size(waits),
      .wait_fences = waits,
      .signal_fence_count = (uint32_t)array_size(signals),
      .signal_fences = signals,
    };

    if (AC_RG_BARRIERS_PRINTF)
    {
      ac_rg_print_stage_fences(ctx.stage, timelines, &submit_info);
    }

    res = ac_queue_submit(device->queues[qi], &submit_info);
    if (res != ac_result_success)
    {
      goto CANCEL;
    }

    wait_prev_frame[qi] = 0;

    ++executed_inds[qi];
    timelines->signaling_values.queue_times[qi] += 1;

    ac_rg_on_stage_submit_success(builder, &ctx, sem_value_offsets);

  CONTINUE:;
  }

  if (!done)
  {
    goto WHILE_NOT_DONE;
  }

  res = ac_result_success;

CANCEL:
  array_free(signals);
  array_free(waits);

  *curr_timeline = timelines->signaling_values;

  return res;
}

AC_API ac_result
ac_rg_graph_execute(ac_rg_graph graph)
{
  ac_rg_builder builder = (ac_rg_builder)graph;

  ac_result res = ac_rg_prepare_graph(builder);
  if (res != ac_result_success)
  {
    return res;
  }

  res = ac_rg_execute(builder);

  ac_rg_storage_cleanup(
    builder->rg,
    &builder->rg->timelines.signaled_values,
    &builder->rg->timelines.signaling_values);

  if (res != ac_result_success)
  {
    return res;
  }

  return ac_result_success;
}

AC_API void
ac_rg_set_validation_callback(
  ac_rg                            rg,
  const ac_rg_validation_callback* callback)
{
  rg->callback = *callback;
}

#endif
