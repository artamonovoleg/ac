#include "ac_private.h"

#if (AC_INCLUDE_RG)

#include <stdio.h>
#include "render_graph.h"

typedef struct ac_rg_message_context {
  char                         string[1024];
  size_t                       object_count;
  ac_rg_validation_object_info objects[6];
} ac_rg_message_context;

static inline const char*
ac_pipeline_stage_bit_to_string(uint32_t type)
{
  switch ((ac_pipeline_stage_bit)type)
  {
  case ac_pipeline_stage_none:
    return "none";
  case ac_pipeline_stage_top_of_pipe_bit:
    return "top_of_pipe";
  case ac_pipeline_stage_draw_indirect_bit:
    return "draw_indirect";
  case ac_pipeline_stage_vertex_input_bit:
    return "vertex_input";
  case ac_pipeline_stage_vertex_shader_bit:
    return "vertex_shader";
  case ac_pipeline_stage_pixel_shader_bit:
    return "pixel_shader";
  case ac_pipeline_stage_early_pixel_tests_bit:
    return "early_fragment_tests";
  case ac_pipeline_stage_late_pixel_tests_bit:
    return "late_fragment_tests";
  case ac_pipeline_stage_color_attachment_output_bit:
    return "color_attachment_output";
  case ac_pipeline_stage_compute_shader_bit:
    return "compute_shader";
  case ac_pipeline_stage_transfer_bit:
    return "transfer";
  case ac_pipeline_stage_bottom_of_pipe_bit:
    return "bottom_of_pipe";
  case ac_pipeline_stage_all_graphics_bit:
    return "all_graphics";
  case ac_pipeline_stage_all_commands_bit:
    return "all_commands";
  case ac_pipeline_stage_acceleration_structure_build_bit:
    return "acceleration_structure_build";
  case ac_pipeline_stage_raytracing_shader_bit:
    return "raytracing_shader";
  case ac_pipeline_stage_resolve_bit:
    return "resolve";
  default:
    break;
  }
  return "?";
}

static inline const char*
ac_image_layout_to_string(uint32_t type)
{
  switch ((ac_image_layout)type)
  {
  case ac_image_layout_undefined:
    return "undefined";
  case ac_image_layout_general:
    return "general";
  case ac_image_layout_color_write:
    return "color_write";
  case ac_image_layout_depth_stencil_write:
    return "depth_stencil_write";
  case ac_image_layout_depth_stencil_read:
    return "depth_stencil_read";
  case ac_image_layout_shader_read:
    return "shader_read";
  case ac_image_layout_transfer_src:
    return "transfer_src";
  case ac_image_layout_transfer_dst:
    return "transfer_dst";
  case ac_image_layout_present_src:
    return "present_src";
  case ac_image_layout_resolve:
    return "resolve";
  default:
    break;
  }
  return "?";
}

static inline const char*
ac_access_to_string(uint32_t type)
{
  switch ((ac_access_bit)type)
  {
  case ac_access_none:
    return "none";
  case ac_access_indirect_command_read_bit:
    return "indirect_command_read";
  case ac_access_index_read_bit:
    return "index_read";
  case ac_access_vertex_attribute_read_bit:
    return "vertex_attribute_read";
  case ac_access_uniform_read_bit:
    return "uniform_read";
  case ac_access_shader_read_bit:
    return "shader_read";
  case ac_access_shader_write_bit:
    return "shader_write";
  case ac_access_color_read_bit:
    return "color_read";
  case ac_access_color_attachment_bit:
    return "color_attachment";
  case ac_access_depth_stencil_read_bit:
    return "depth_stencil_read";
  case ac_access_depth_stencil_attachment_bit:
    return "depth_stencil_attachment";
  case ac_access_transfer_read_bit:
    return "transfer_read";
  case ac_access_transfer_write_bit:
    return "transfer_write";
  case ac_access_acceleration_structure_read_bit:
    return "acceleration_structure_read";
  case ac_access_acceleration_structure_write_bit:
    return "acceleration_structure_write";
  case ac_access_resolve_bit:
    return "resolve";
  default:
    break;
  }
  return "?";
}

static inline const char*
ac_queue_type_to_string(uint32_t type)
{
  switch ((ac_queue_type)type)
  {
  case ac_queue_type_graphics:
    return "graphics";
  case ac_queue_type_compute:
    return "compute";
  case ac_queue_type_transfer:
    return "transfer";
  default:
    break;
  }
  return "?";
}

static inline const char*
ac_image_usage_bit_to_string(uint32_t type)
{
  switch ((ac_image_usage_bit)type)
  {
  case ac_image_usage_srv_bit:
    return "srv";
  case ac_image_usage_uav_bit:
    return "uav";
  case ac_image_usage_attachment_bit:
    return "attachment";
  case ac_image_usage_transfer_src_bit:
    return "transfer_src";
  case ac_image_usage_transfer_dst_bit:
    return "transfer_dst";
  default:
    break;
  }
  return "?";
}

static inline const char*
ac_buffer_usage_bit_to_string(uint32_t type)
{
  switch ((ac_buffer_usage_bit)type)
  {
  case ac_buffer_usage_vertex_bit:
    return "vertex";
  case ac_buffer_usage_index_bit:
    return "index";
  case ac_buffer_usage_cbv_bit:
    return "cbv";
  case ac_buffer_usage_srv_bit:
    return "srv";
  case ac_buffer_usage_uav_bit:
    return "uav";
  case ac_buffer_usage_raytracing_bit:
    return "raytracing";
  case ac_buffer_usage_transfer_src_bit:
    return "transfer_src";
  case ac_buffer_usage_transfer_dst_bit:
    return "transfer_dst";
  default:
    break;
  }
  return "?";
}

static inline uint32_t
ac_highest_bit(uint32_t mask)
{
  mask |= (mask >> 1);
  mask |= (mask >> 2);
  mask |= (mask >> 4);
  mask |= (mask >> 8);
  mask |= (mask >> 16);
  return mask - (mask >> 1);
}

static inline void
ac_rg_print_bits(ac_file file, uint32_t mask, const char* (*str)(uint32_t mask))
{
  if (!mask)
  {
    ac_print(file, "%s", str(0));
    return;
  }

  for (;;)
  {
    uint32_t bit = ac_highest_bit(mask);
    mask &= ~bit;

    ac_print(file, "%s", str(bit));

    if (!mask)
    {
      break;
    }
    ac_print(file, " | ");
  }
}

static inline void
ac_rg_write_bits(
  int32_t  len,
  char*    buf,
  uint32_t mask,
  const char* (*str)(uint32_t mask))
{
  if (!mask)
  {
    snprintf(buf, (size_t)len, "%s", str(0));
    return;
  }

  for (;;)
  {
    uint32_t bit = ac_highest_bit(mask);
    mask &= ~bit;

    int32_t d = snprintf(buf, (size_t)len, "%s", str(bit));
    buf += d;
    len -= d;

    if (!mask)
    {
      break;
    }

    d = snprintf(buf, (size_t)len, " | ");
    buf += d;
    len -= d;
  }
}

static inline void
ac_rg_message_push(
  ac_rg_message_context*       ctx,
  const void*                  handle,
  ac_rg_validation_object_type type,
  const char*                  name)
{
  ctx->objects[ctx->object_count++] = (ac_rg_validation_object_info) {
    .type = type,
    .handle = handle,
    .name = name,
  };
}

static inline void
ac_rg_message_push_stage(
  ac_rg_message_context*    ctx,
  const ac_rg_builder_stage stage)
{
  ac_rg_message_push(ctx, stage, ac_rg_object_type_stage, stage->info.name);
}

static inline void
ac_rg_message_push_use(
  ac_rg_message_context*       ctx,
  const ac_rg_builder_resource state)
{
  ac_rg_validation_object_type type = ac_rg_object_type_none;
  switch (state->resource->type)
  {
  case ac_rg_resource_type_image:
    type = ac_rg_object_type_image;
    break;
  case ac_rg_resource_type_buffer:
    type = ac_rg_object_type_buffer;
    break;
  default:
    break;
  }

  ac_rg_message_push(ctx, state, type, state->resource->name);
}

void
ac_rg_message(
  ac_rg_validation_callback*               callback,
  const ac_rg_validation_message_internal* info)
{
  if (!callback->function)
  {
    return;
  }

  ac_rg_validation_severity_bits severity = 0;
  ac_rg_validation_message_bits  type = 0;

  switch (info->id)
  {
  case ac_rg_message_id_not_a_message:
  case ac_rg_message_id_error_external_use_mask_not_supported:
    type = ac_rg_validation_message_general_bit;
    severity = ac_rg_validation_severity_error_bit;
    break;
  case ac_rg_message_id_info_image_samples_reduced:
    type = ac_rg_validation_message_general_bit;
    severity = ac_rg_validation_severity_info_bit;
    break;
  case ac_rg_message_id_error_null_argument:
  case ac_rg_message_id_error_image_create_info_zero:
  case ac_rg_message_id_error_attachment_does_not_match:
  case ac_rg_message_id_error_image_blit_samples:
  case ac_rg_message_id_error_external_resource_compatibility:
  case ac_rg_message_id_error_resource_referenced_twice_in_stage:
  case ac_rg_message_id_error_cyclic_stage_dependencies:
  case ac_rg_message_id_error_bad_resource_usage_and_commands_type:
  case ac_rg_message_id_error_bad_access_for_given_usage:
  case ac_rg_message_id_error_format_not_supported:
  case ac_rg_message_id_error_duplicated_token:
  case ac_rg_message_id_error_exported_multiple_times:
    type = ac_rg_validation_message_validation_bit;
    severity = ac_rg_validation_severity_error_bit;
    break;
  }

  if (!(callback->type_filter_mask & type))
  {
    return;
  }
  if (!(callback->severity_filter_mask & severity))
  {
    return;
  }

  ac_rg_message_context ctx = {0};

  if (info->stage)
  {
    ac_rg_message_push_stage(&ctx, info->stage);
  }
  if (info->state1)
  {
    ac_rg_message_push_use(&ctx, info->state1);
  }
  if (info->state2)
  {
    ac_rg_message_push_use(&ctx, info->state2);
  }

  if (info->null_obj_type)
  {
    ac_rg_message_push(&ctx, NULL, info->null_obj_type, info->null_obj_name);
  }

  const char* str_message_type = "";

  switch (type)
  {
  case ac_rg_validation_message_general_bit:
    str_message_type = "general";
    break;
  case ac_rg_validation_message_validation_bit:
    str_message_type = "validation";
    break;
  case ac_rg_validation_message_performance_bit:
    str_message_type = "performance";
    break;
  default:
    break;
  }

  char description_buffer[1024] = {'\0'};

  const char* description = NULL;

  switch (info->id)
  {
  case ac_rg_message_id_not_a_message:
  {
    description = "not a message";
    break;
  }
  case ac_rg_message_id_error_null_argument:
  {
    description = "this argument must not be null";
    break;
  }
  case ac_rg_message_id_error_external_use_mask_not_supported:
  {
    AC_ASSERT(info->state1);

    char use1buf[256];

    bool image = info->state1->resource->type == ac_rg_resource_type_image;

    ac_rg_write_bits(
      sizeof(use1buf),
      use1buf,
      (uint32_t)info->metadata,
      image ? ac_image_usage_bit_to_string : ac_buffer_usage_bit_to_string);

    snprintf(
      description_buffer,
      sizeof description_buffer,
      "external resource is created with restrictive usage mask [ %s ]",
      use1buf);
    description = description_buffer;
    break;
  }
  case ac_rg_message_id_error_format_not_supported:
  {
    description =
      "image format (ac_image_format) properties of this ac_device does not "
      "allow "
      "to use it for ...";
    break;
  }
  case ac_rg_message_id_error_image_create_info_zero:
  {
    description =
      "ac_image_info fields must not be 0: 'format', 'width', 'height', "
      "'depth', 'samples'";
    break;
  }
  case ac_rg_message_id_error_attachment_does_not_match:
  {
    description =
      "attachments in the render pass must have same size/samples properties";
    break;
  }
  case ac_rg_message_id_error_image_blit_samples:
  {
    description = "for image blit new ImageCreateInfo.samples must be 1";
    break;
  }
  case ac_rg_message_id_error_external_resource_compatibility:
  {
    description = "external resource is not compatible";
    break;
  }
  case ac_rg_message_id_info_image_samples_reduced:
  {
    description =
      "ac_device does not support ac_image_info::samples, fallback to "
      "supported samples";
    break;
  }
  case ac_rg_message_id_error_resource_referenced_twice_in_stage:
  {
    description = "resource is added twice to the stage";
    break;
  }
  case ac_rg_message_id_error_cyclic_stage_dependencies:
  {
    description =
      "failed to resolve stage ordering because of cyclic dependencies";
    break;
  }
  case ac_rg_message_id_error_bad_resource_usage_and_commands_type:
  {
    description = "bad resource usage for a given stage commands type";
    break;
  }
  case ac_rg_message_id_error_bad_access_for_given_usage:
  {
    description = "bad access for a given usage";
    break;
  }
  case ac_rg_message_id_error_duplicated_token:
  {
    snprintf(
      description_buffer,
      sizeof description_buffer,
      "token %llu used several times",
      (long long unsigned int)info->metadata);
    description = description_buffer;
    break;
  }
  default:
  {
    break;
  }
  }

  int32_t len = snprintf(
    ctx.string,
    sizeof ctx.string,
    "[ ac_rg ] %s : %s",
    str_message_type,
    description);

  // TODO: add meaningfully information
  for (size_t i = 0; i < ctx.object_count; ++i)
  {
    const ac_rg_validation_object_info* obj = &ctx.objects[i];
    len += snprintf(
      ctx.string + len,
      sizeof(ctx.string) - (size_t)len,
      "\n\tobject %i: type %x, handle %13p, name '%s'",
      (int)i,
      (int)obj->type,
      obj->handle,
      obj->name);

    if (!obj->handle)
    {
      continue;
    }

    if (obj->type == ac_rg_object_type_image)
    {
      const ac_rg_builder_resource_internal* state = obj->handle;

      char buffer[256];

      ac_rg_write_bits(
        sizeof buffer,
        buffer,
        state->resource->image_info.usage,
        ac_image_usage_bit_to_string);

      len += snprintf(
        ctx.string + len,
        sizeof(ctx.string) - (size_t)len,
        ", usage: '%s'",
        buffer);
    }
  }

  AC_UNUSED(len);

  ctx.string[(sizeof ctx.string) - 1] = 0;

  ac_rg_validation_message message = {
    .message = ctx.string,
    .object_count = ctx.object_count,
    .objects = ctx.objects,
  };

  callback->function(severity, type, &message, callback->user_data);
}

void
ac_rg_default_validation_callback(
  ac_rg_validation_severity_bits  severity_bits,
  ac_rg_validation_message_bits   type_bits,
  const ac_rg_validation_message* message,
  void*                           user_data)
{
  AC_UNUSED(type_bits);
  AC_UNUSED(user_data);

  if (severity_bits & ac_rg_validation_severity_error_bit)
  {
    AC_ERROR("%s", message->message);
    AC_DEBUGBREAK();
  }
  else if (severity_bits & ac_rg_validation_severity_warning_bit)
  {
    AC_WARN("%s", message->message);
  }
  else if (severity_bits & ac_rg_validation_severity_info_bit)
  {
    AC_INFO("%s", message->message);
  }
  else if (severity_bits & ac_rg_validation_severity_debug_bit)
  {
    AC_DEBUG("%s", message->message);
  }

  if (severity_bits & ac_rg_validation_severity_error_bit)
  {
    AC_DEBUGBREAK();
  }
}

static inline void
ac_rg_print_barrier(ac_file file, ac_rg_graph_stage_barrier* barriers)
{
  if (!array_size(barriers->buffers) && !array_size(barriers->images))
  {
    ac_print(file, "\tempty\n");
    return;
  }

  for (uint32_t i = 0; i < array_size(barriers->images); ++i)
  {
    ac_image_barrier*     barrier = &barriers->images[i];
    ac_rg_graph_resource* image = barriers->image_nodes[i];

    ac_print(
      file,
      "\t|- image: \"%s\" (ac handle %p) %ux%ux%ux%ux%u \n",
      image->image_info.name,
      (void*)image->image,
      (unsigned)image->image_info.width,
      (unsigned)image->image_info.height,
      (unsigned)image->image_info.layers,
      (unsigned)image->image_info.levels,
      (unsigned)image->image_info.samples);

    ac_print(
      file,
      "\t|\tlayout: %s  >>>  %s\n",
      ac_image_layout_to_string((uint32_t)barrier->old_layout),
      ac_image_layout_to_string((uint32_t)barrier->new_layout));

    ac_print(file, "\t|\tstages: ");
    ac_rg_print_bits(file, barrier->src_stage, ac_pipeline_stage_bit_to_string);
    ac_print(file, "  >>>  ");
    ac_rg_print_bits(file, barrier->dst_stage, ac_pipeline_stage_bit_to_string);
    ac_print(file, "\n");

    ac_print(file, "\t|\taccess: ");
    ac_rg_print_bits(file, barrier->src_access, ac_access_to_string);
    ac_print(file, "  >>>  ");
    ac_rg_print_bits(file, barrier->dst_access, ac_access_to_string);
    ac_print(file, "\n");

    if (barrier->src_queue != NULL || barrier->dst_queue != NULL)
    {
      ac_print(
        file,
        "\t|\tqueue:  %s  >>>  %s\n",
        ac_queue_type_to_string((uint32_t)barrier->src_queue->type),
        ac_queue_type_to_string((uint32_t)barrier->dst_queue->type));
    }

    if (barrier->range.layers != 0 || barrier->range.levels != 0)
    {
      ac_image_subresource_range range =
        ac_rg_graph_calculate_range(&barrier->range, &image->image_info);

      ac_print(
        file,
        "\t|\tmip   levels [%u;%u]\n\t|\tarray layers [%u;%u]\n",
        range.base_level,
        (range.base_level + range.levels) - 1,
        range.base_layer,
        (range.base_layer + range.layers) - 1);
    }
  }
  ac_print(file, "\n");
}

void
ac_rg_print_stage_barriers(ac_rg_graph_stage* stage)
{
  ac_print(NULL, "%s: ", ac_queue_type_to_string((uint32_t)stage->queue_type));

  for (size_t i = 0; i < array_size(stage->subpasses); ++i)
  {
    ac_print(
      NULL,
      "%s'%s'%s",
      i == 0 ? "" : " + ",
      stage->subpasses[i].stage_info.name,
      i == array_size(stage->subpasses) - 1 ? "\n" : "");
  }

  ac_print(NULL, "\tpre-barrier:\n");
  ac_rg_print_barrier(NULL, &stage->barrier_beg);
  ac_print(NULL, "\tposs-barrier:\n");
  ac_rg_print_barrier(NULL, &stage->barrier_end);
}

static inline void
ac_rg_print_fence(
  ac_file                     file,
  ac_rg_timelines*            timelines,
  const ac_fence_submit_info* info)
{
  const char* fence_name = NULL;
  for (uint32_t qi = 0; qi < ac_queue_type_count; ++qi)
  {
    if (timelines->fences[qi] == info->fence)
    {
      fence_name = ac_queue_type_to_string(qi);
      break;
    }
  }

  if (fence_name)
  {
    ac_print(file, "\t|\t%s %llu; stages: ", fence_name, info->value);
  }
  else if (info->fence->bits & ac_fence_present_bit)
  {
    ac_print(file, "\t|\t%p %s; stages: ", info->fence, "present");
  }
  else
  {
    ac_print(file, "\t|\t%p %llu; stages: ", info->fence, info->value);
  }

  ac_rg_print_bits(file, info->stages, ac_pipeline_stage_bit_to_string);
  ac_print(file, "\n");
}

void
ac_rg_print_stage_fences(
  ac_rg_graph_stage*          stage,
  ac_rg_timelines*            timelines,
  const ac_queue_submit_info* si)
{
  if (si->wait_fence_count == 0 && si->signal_fence_count == 0)
  {
    return;
  }

  ac_print(NULL, "%s: ", ac_queue_type_to_string((uint32_t)stage->queue_type));

  for (size_t i = 0; i < array_size(stage->subpasses); ++i)
  {
    ac_print(
      NULL,
      "%s'%s'%s",
      i == 0 ? "" : " + ",
      stage->subpasses[i].stage_info.name,
      i == array_size(stage->subpasses) - 1 ? "\n" : "");
  }

  if (si->wait_fence_count)
  {
    ac_print(NULL, "\ttimeline wait:\n");
    for (uint32_t i = 0; i < si->wait_fence_count; ++i)
    {
      ac_rg_print_fence(NULL, timelines, si->wait_fences + i);
    }
  }

  if (si->signal_fence_count)
  {
    ac_print(NULL, "\ttimeline signal:\n");
    for (uint32_t i = 0; i < si->signal_fence_count; ++i)
    {
      ac_rg_print_fence(NULL, timelines, si->signal_fences + i);
    }
  }

  ac_print(NULL, "\n");
}

static inline void
ac_rg_write_dot_stage_id(ac_file f, ac_rg_graph_stage* stage, uint32_t subpass)
{
  ac_print(f, "\"%p-%u\"", stage, subpass);
}

static inline void
ac_rg_write_dot_resource_id(ac_file f, ac_rg_builder_resource state)
{
  ac_print(f, "\"%p\"", state);
}

static inline void
ac_rg_write_dot_dependency(ac_file f)
{
  ac_print(f, " -> ");
}

static inline void
ac_rg_write_dot_resource_definition(ac_file f, ac_rg_builder_resource state)
{
  ac_rg_write_dot_resource_id(f, state);

  ac_rg_graph_resource* resource = state->resource;

  switch (resource->type)
  {
  case ac_rg_resource_type_image:
  {
    ac_image_info* info = &resource->image_info;
    ac_print(
      f,
      "[label=\"%s\n%ux%ux[%u], %u\"];\n",
      resource->name,
      info->width,
      info->height,
      info->layers,
      info->samples);
    break;
  }
  case ac_rg_resource_type_buffer:
  {
    ac_buffer_info* info = &resource->buffer_info;
    ac_print(f, "[label=\"%s\n %lu\"];\n", resource->name, info->size);
    break;
  }
  default:
  {
    AC_ASSERT(false);
    break;
  }
  }
}

static inline void
ac_rg_write_dot_resource_connection(
  ac_file                        f,
  ac_rg_graph_stage*             stage,
  uint32_t                       subpass,
  ac_rg_graph_resource_reference r)
{
  ac_rg_builder_resource    state = ac_rg_graph_resource_dereference_state(r);
  ac_rg_graph_resource_use* use = ac_rg_graph_resource_dereference_use(r);

  if (use->specifics.access_write.access)
  {
    ac_rg_write_dot_stage_id(f, stage, subpass);
    ac_rg_write_dot_dependency(f);
    ac_rg_write_dot_resource_id(f, state);
    ac_print(f, ";\n");

    ac_rg_graph_resource_reference prev = ac_rg_graph_resource_prev_use(r);

    if (prev.resource)
    {
      ac_rg_write_dot_resource_id(
        f,
        ac_rg_graph_resource_dereference_state(prev));
      ac_rg_write_dot_dependency(f);
      ac_rg_write_dot_stage_id(f, stage, subpass);
      ac_print(f, ";\n");
    }
  }
  else
  {
    ac_rg_write_dot_resource_id(f, state);
    ac_rg_write_dot_dependency(f);
    ac_rg_write_dot_stage_id(f, stage, subpass);
    ac_print(f, ";\n");
  }

  if (state->image_resolve_dst)
  {
    ac_rg_write_dot_resource_id(f, state);
    ac_rg_write_dot_dependency(f);
    ac_rg_write_dot_resource_id(f, state->image_resolve_dst);
    ac_print(f, ";\n");
  }
}

AC_API ac_result
ac_rg_graph_write_digraph(ac_rg_graph graph, ac_file f)
{
  ac_rg_builder builder = (ac_rg_builder)graph;

  ac_print(f, "digraph {\n");

  for (size_t ri = 0; ri < array_size(builder->resources); ++ri)
  {
    ac_rg_graph_resource* resource = builder->resources[ri];

    for (size_t si = 0; si < array_size(resource->states); ++si)
    {
      ac_rg_builder_resource state = resource->states[si];

      for (size_t ui = 0; ui < array_size(state->uses); ++ui)
      {
        ac_rg_write_dot_resource_definition(f, state);

        if (state->image_resolve_dst)
        {
          ac_rg_write_dot_resource_definition(f, state->image_resolve_dst);
        }
      }
    }
  }

  for (size_t sq = 0; sq < ac_queue_type_count; ++sq)
  {
    array_t(ac_rg_graph_stage*) stage_queue = builder->stage_queues[sq];
    ac_print(f, "subgraph \"%p\" {\n", stage_queue);

    for (size_t si = 0; si < array_size(stage_queue); ++si)
    {
      ac_rg_graph_stage* stage = stage_queue[si];
      ac_print(f, "subgraph \"%p\" {\n", stage);

      for (size_t ssi = 0; ssi < array_size(stage->subpasses); ++ssi)
      {
        ac_rg_graph_stage_subpass* subpass = &stage->subpasses[ssi];

        ac_rg_write_dot_stage_id(f, stage, (uint32_t)ssi);
        ac_print(
          f,
          " [label=\"%s\n%s (%u)\npass %u:%u\" shape=box];\n",
          subpass->stage_info.name,
          ac_queue_type_to_string((uint32_t)subpass->stage_info.commands),
          stage->queue_index,
          stage->stage_index,
          (unsigned int)ssi);
      }
      ac_print(f, "}\n");
    }
    ac_print(f, "}\n");
  }

  for (size_t sq = 0; sq < ac_queue_type_count; ++sq)
  {
    array_t(ac_rg_graph_stage*) stage_queue = builder->stage_queues[sq];
    for (size_t si = 0; si < array_size(stage_queue); ++si)
    {
      ac_rg_graph_stage* stage = stage_queue[si];
      for (size_t ssi = 0; ssi < array_size(stage->subpasses); ++ssi)
      {
        ac_rg_graph_stage_subpass* subpass = &stage->subpasses[ssi];

        for (size_t i = 0; i < array_size(subpass->attachments); ++i)
        {
          ac_rg_write_dot_resource_connection(
            f,
            stage,
            (uint32_t)ssi,
            subpass->attachments[i]);
        }

        for (size_t i = 0; i < array_size(subpass->images); ++i)
        {
          ac_rg_write_dot_resource_connection(
            f,
            stage,
            (uint32_t)ssi,
            subpass->images[i]);
        }

        for (size_t i = 0; i < array_size(subpass->buffers); ++i)
        {
          ac_rg_write_dot_resource_connection(
            f,
            stage,
            (uint32_t)ssi,
            subpass->buffers[i]);
        }
      }
    }
  }

  ac_print(f, "}\n");
  uint8_t zero = 0;
  return ac_file_write(f, 1, &zero);
}

#endif
