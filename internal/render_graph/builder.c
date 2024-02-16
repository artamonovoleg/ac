#include "ac_private.h"

#if (AC_INCLUDE_RG)

#include "render_graph.h"

typedef enum ac_rg_builder_stage_image_slot {
  ac_rg_builder_stage_image_slot_default = 0,
  ac_rg_builder_stage_image_slot_resolve = 1,
} ac_rg_builder_stage_image_slot;

typedef enum ac_rg_resource_use_add_mode {
  ac_rg_resource_use_add_mode_create = 0,
  ac_rg_resource_use_add_mode_edit = 1,
  ac_rg_resource_use_add_mode_continue = 2,
} ac_rg_resource_use_add_mode;

static inline void
ac_rg_destroy_resource_state(ac_rg_builder_resource state)
{
  array_free(state->uses);
  ac_free(state);
}

static inline void
ac_rg_destroy_resource_internal(ac_rg_graph_resource* resource)
{
  for (size_t i = 0; i < array_size(resource->states); ++i)
  {
    ac_rg_destroy_resource_state(resource->states[i]);
  }
  array_free(resource->states);

  ac_free(resource);
}

static inline void
ac_rg_destroy_builder_stage(ac_rg_builder_stage stage)
{
  array_free(stage->resource_states);
  array_free(stage->resolve_dst);
  ac_free(stage);
}

#if AC_RG_TODO
[[nodiscard]] VkFormat inline format_fallback(VkFormat format)
{
  switch (format)
  {
  case VK_FORMAT_R8G8B8_UINT:
    return VK_FORMAT_R8G8B8A8_UINT;
  case VK_FORMAT_R8G8B8_UNORM:
    return VK_FORMAT_R8G8B8A8_UNORM;
  case VK_FORMAT_R8G8B8_SRGB:
    return VK_FORMAT_R8G8B8A8_SRGB;

  case VK_FORMAT_R16G16B16_SFLOAT:
    return VK_FORMAT_R16G16B16A16_SFLOAT;

  case VK_FORMAT_R32G32B32_SFLOAT:
    return VK_FORMAT_R32G32B32A32_SFLOAT;

  default:
    return VK_FORMAT_UNDEFINED;
  }
}

[[nodiscard]] inline bool
is_image_format_suitable(VkFormatFeatureFlags features, VkImageUsageFlags usage)
{
  if (
    usage & VK_IMAGE_USAGE_TRANSFER_SRC_BIT and
    not(features & VK_FORMAT_FEATURE_TRANSFER_SRC_BIT))
  {
    return false;
  }

  if (
    usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT and
    not(features & VK_FORMAT_FEATURE_TRANSFER_DST_BIT))
  {
    return false;
  }

  if (
    usage & VK_IMAGE_USAGE_SAMPLED_BIT and
    not(features & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT))
  {
    return false;
  }

  if (
    usage & VK_IMAGE_USAGE_STORAGE_BIT and
    not(features & VK_FORMAT_FEATURE_STORAGE_IMAGE_BIT))
  {
    return false;
  }

  if (
    usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT and
    not(features & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
  {
    return false;
  }

  if (
    usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT and
    not(features & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT))
  {
    return false;
  }

  if (
    usage & VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT and
    not(features & VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT))
  {
    return false;
  }

  return true;
}
#endif

static inline bool
ac_rg_check_format(
  ac_rg_builder         builder,
  ac_rg_graph_resource* image,
  ac_format             format)
{
  while (format != ac_format_undefined)
  {
#if AC_RG_TODO
    VkFormatProperties fp;
    vkGetPhysicalDeviceFormatProperties(p_graph->physical_device, format, &fp);

    if (is_image_format_suitable(fp.optimalTilingFeatures, p_image->usage))
    {
      image->info.format = format;
      image->info.format_features = fp.optimalTilingFeatures;
      image->info.aspect_mask = format_aspect_mask(format);
      return 1;
    }

    format = format_fallback(format);
#else
    AC_UNUSED(builder);
    image->image_info.format = format;
    return 1;
#endif
  }

  return 0;
}

static ac_rg_graph_resource_use*
ac_rg_builder_find_resource_use(
  ac_rg_builder_resource state,
  ac_rg_builder_stage    stage)
{
  for (size_t ui = 0; ui < array_size(state->uses); ++ui)
  {
    ac_rg_graph_resource_use* use = &state->uses[ui];
    if (use->builder_stage != stage)
    {
      continue;
    }

    return use;
  }

  AC_ASSERT(false);
  return NULL;
}

static ac_rg_builder_resource
ac_rg_add_resource_use(
  ac_rg_builder                                builder,
  ac_rg_resource_use_add_mode                  add_mode,
  ac_rg_builder_stage                          stage,
  ac_rg_graph_resource*                        resource,
  ac_rg_builder_resource                       state,
  ac_rg_builder_stage_use_resource_info const* info,
  ac_image_layout                              layout)
{
  if (AC_INCLUDE_DEBUG && stage && info && layout != ac_image_layout_resolve)
  {
    ac_rg_graph_resource_use* use2 = NULL;
    for (size_t i = 0; i < array_size(stage->resource_states); ++i)
    {
      use2 = ac_rg_builder_find_resource_use(stage->resource_states[i], stage);
      if (use2->token != info->token)
      {
        continue;
      }

      AC_RG_MESSAGE(
        &builder->rg->callback,
        &(ac_rg_validation_message_internal) {
          .id = ac_rg_message_id_error_duplicated_token,
          .stage = stage,
          .state1 = state,
          .metadata = info->token,
        });
      builder->result = ac_result_bad_usage;
      return NULL;
    }
  }

  uint32_t insert_index = 0;

  switch (add_mode)
  {
  case ac_rg_resource_use_add_mode_create:
    AC_ASSERT(state == NULL);
    break;
  case ac_rg_resource_use_add_mode_continue:
  {
    if (!info)
    {
      state = NULL;
      break;
    }

    if (info->access_write.access)
    {
      if (state->is_read_only || state != array_back(resource->states))
      {
        switch (resource->type)
        {
        case ac_rg_resource_type_image:
        {
          ac_rg_builder_resource blit_dst = ac_rg_builder_create_resource(
            builder,
            &(ac_rg_builder_create_resource_info) {
              .image_info = &resource->image_info});

          ac_rg_builder_group group = NULL;
          if (stage)
          {
            group = stage->info.group;
          }

          state = ac_rg_builder_blit_image(
            builder,
            group,
            "implicit copy from read only image",
            state,
            blit_dst,
            ac_filter_nearest);
          break;
        }
        default:
          // TODO buffers
          AC_ASSERT(0);
        }

        if (!state)
        {
          return NULL;
        }
        resource = state->resource;
      }

      insert_index = (uint32_t)array_size(resource->states);
      state = NULL;
    }
    break;
  }
  case ac_rg_resource_use_add_mode_edit:
  {
    for (insert_index = 0; insert_index < array_size(resource->states);
         ++insert_index)
    {
      if (resource->states[insert_index] == state)
      {
        break;
      }
    }

    if (array_size(state->uses) && state->uses[0].specifics.access_write.access)
    {
      state = NULL;
      break;
    }
    break;
  }
  }

  if (!state)
  {
    state = ac_calloc(sizeof *state);
    if (!state)
    {
      builder->result = ac_result_out_of_host_memory;
      return NULL;
    }

    state->resource = resource;

    array_insert(resource->states, insert_index, state);
  }

  if (stage && info)
  {
    ac_rg_graph_resource_use use;
    AC_ZERO(use);

    use.builder_stage = stage;
    use.usage_bits = info->usage_bits;
    use.token = info->token;

    use.specifics.access_read = info->access_read;
    use.specifics.access_write = info->access_write;
    use.specifics.image_layout = layout;
    if (info->image_range)
    {
      use.specifics.image_range = *info->image_range;
    }

    array_append(state->uses, use);
  }

  return state;
}

static ac_rg_builder_resource
ac_rg_builder_add_image_use(
  ac_rg_builder                                builder,
  ac_rg_resource_use_add_mode                  add_mode,
  ac_rg_builder_stage                          stage,
  ac_rg_builder_stage_use_resource_info const* info,
  ac_rg_builder_stage_image_slot               slot)
{
  ac_rg_builder_resource state = info->resource;
  ac_rg_graph_resource*  resource = state->resource;

  if (
    state && resource->image_info.samples > 1 &&
    info->usage_bits != ac_image_usage_attachment_bit)
  {
    ac_rg_builder_resource resolved =
      ac_rg_builder_resolve_image(builder, state);
    if (!resolved)
    {
      return NULL;
    }
    resource = resolved->resource;
    state = resolved;
  }

  ac_image_info* image_info = &resource->image_info;

  if (
    info->usage_bits & ac_image_usage_attachment_bit &&
    array_size(stage->resource_states))
  {
    ac_rg_builder_resource    attachment = stage->resource_states[0];
    ac_rg_graph_resource_use* use =
      ac_rg_builder_find_resource_use(attachment, stage);
    ac_image_info* attachment_info = &attachment->resource->image_info;

    if (
      use->usage_bits & ac_image_usage_attachment_bit &&
      (image_info->width != attachment_info->width ||
       image_info->height != attachment_info->height ||
       image_info->samples != attachment_info->samples))
    {
      AC_RG_MESSAGE(
        &builder->rg->callback,
        &(ac_rg_validation_message_internal) {
          .id = ac_rg_message_id_error_attachment_does_not_match,
          .stage = stage,
          .state1 = attachment,
          .null_obj_name = image_info->name,
          .null_obj_type = ac_rg_object_type_image,
        });

      builder->result = ac_result_bad_usage;
      return NULL;
    }
  }

  ac_rg_resource_access r = info->access_read;
  ac_rg_resource_access w = info->access_write;

  ac_image_layout layout = ac_image_layout_undefined;

  ac_rg_builder_stage_use_resource_info new_info = *info;

  ac_rg_message_id message;
  AC_MAYBE_UNUSED(message);

  switch (slot)
  {
  case ac_rg_builder_stage_image_slot_default:
  {
    if (stage->info.commands == ac_queue_type_compute)
    {
      r.stages |= ac_pipeline_stage_compute_shader_bit;
      w.stages |= ac_pipeline_stage_compute_shader_bit;
    }

    switch ((ac_image_usage_bits)info->usage_bits)
    {
    case ac_image_usage_attachment_bit:
    {
      if (stage->info.commands != ac_queue_type_graphics)
      {
        message = ac_rg_message_id_error_bad_resource_usage_and_commands_type;
        goto VALIDATION_ERROR;
      }

      bool read =
        (info->access_attachment & ac_rg_attachment_access_read_bit) != 0;
      bool write =
        (info->access_attachment & ac_rg_attachment_access_write_bit) != 0;

      if (ac_format_depth_or_stencil(image_info->format))
      {
        layout = ac_image_layout_depth_stencil_read;
        if (read)
        {
          r.stages |= ac_pipeline_stage_early_pixel_tests_bit |
                      ac_pipeline_stage_late_pixel_tests_bit;
          r.access |= ac_access_depth_stencil_read_bit;
        }
        if (write)
        {
          layout = ac_image_layout_depth_stencil_write;
          w.stages |= ac_pipeline_stage_early_pixel_tests_bit |
                      ac_pipeline_stage_late_pixel_tests_bit;
          w.access |= ac_access_depth_stencil_attachment_bit;
        }
      }
      else
      {
        layout = ac_image_layout_color_write;

        if (read)
        {
          r.stages |= ac_pipeline_stage_color_attachment_output_bit;
          r.access |= ac_access_color_read_bit;
        }
        if (write)
        {
          w.stages |= ac_pipeline_stage_color_attachment_output_bit;
          w.access |= ac_access_color_attachment_bit;
        }
      }

      AC_ASSERT(write || read);
      break;
    }
    case ac_image_usage_srv_bit:
    {
      if (
        stage->info.commands != ac_queue_type_graphics &&
        stage->info.commands != ac_queue_type_compute)
      {
        message = ac_rg_message_id_error_bad_resource_usage_and_commands_type;
        goto VALIDATION_ERROR;
      }

      layout = ac_image_layout_shader_read;
      r.access |= ac_access_shader_read_bit;

      if (
        (stage->info.commands == ac_queue_type_graphics && !r.stages) ||
        w.access)
      {
        message = ac_rg_message_id_error_bad_access_for_given_usage;
        goto VALIDATION_ERROR;
      }

      break;
    }
    case ac_image_usage_uav_bit:
    {
      if (
        stage->info.commands != ac_queue_type_graphics &&
        stage->info.commands != ac_queue_type_compute)
      {
        message = ac_rg_message_id_error_bad_resource_usage_and_commands_type;
        goto VALIDATION_ERROR;
      }

      layout = ac_image_layout_general;
      AC_ASSERT((r.stages && r.access) || (w.stages && w.access));

      if ((!r.stages || !r.access) && (!w.stages || !w.access))
      {
        message = ac_rg_message_id_error_bad_access_for_given_usage;
        goto VALIDATION_ERROR;
      }
      break;
    }
    case ac_image_usage_transfer_src_bit:
    case ac_image_usage_transfer_dst_bit:
    {
      if (stage->info.commands != ac_queue_type_transfer)
      {
        message = ac_rg_message_id_error_bad_resource_usage_and_commands_type;
        goto VALIDATION_ERROR;
      }

      AC_ASSERT(
        (r.access == 0 || (r.access == ac_access_transfer_read_bit &&
                           r.stages == ac_pipeline_stage_transfer_bit)));

      AC_ASSERT(
        (w.access == 0 || (w.access == ac_access_transfer_write_bit &&
                           w.stages == ac_pipeline_stage_transfer_bit)));

      AC_ASSERT((r.access > 0) + (w.access > 0) == 1);

      layout = ac_image_layout_transfer_src;
      if (w.access)
      {
        layout = ac_image_layout_transfer_dst;
      }

      break;
    }
    default:
    {
      AC_ASSERT(false);
      break;
    }
    }
    AC_ASSERT(r.access || w.access);
    AC_ASSERT(layout != ac_image_layout_undefined);

    break;
  }
  case ac_rg_builder_stage_image_slot_resolve:
  {
    layout = ac_image_layout_resolve;
    w.stages |= ac_pipeline_stage_resolve_bit;
    w.access |= ac_access_resolve_bit;

    new_info.usage_bits = ac_image_usage_attachment_bit;

    break;
  }
  default:
  {
    AC_ASSERT(false);
    break;
  }

  VALIDATION_ERROR:
    AC_RG_MESSAGE(
      &builder->rg->callback,
      &(ac_rg_validation_message_internal) {
        .id = message,
        .stage = stage,
        .null_obj_name = image_info->name,
        .null_obj_type = ac_rg_object_type_image,
      });
    builder->result = ac_result_bad_usage;
    return NULL;
  }

  new_info.access_read = r;
  new_info.access_write = w;

  for (size_t i = 0; i < array_size(stage->resource_states); ++i)
  {
    ac_rg_builder_resource state2 = stage->resource_states[i];
    if (state2->resource != resource)
    {
      continue;
    }

    if (resource->type == ac_rg_resource_type_image)
    {
      ac_rg_graph_resource_use* use =
        ac_rg_builder_find_resource_use(state2, stage);

      ac_rg_resource_use_specifics* ss = &use->specifics;
      if (
        (ss->image_range.levels != 0 && new_info.image_range->levels != 0 &&
         (ss->image_range.base_level >=
            new_info.image_range->base_level + new_info.image_range->levels ||
          ss->image_range.base_level + ss->image_range.levels <=
            new_info.image_range->base_level)) ||
        (ss->image_range.layers != 0 && new_info.image_range->layers != 0 &&
         (ss->image_range.base_level >=
            new_info.image_range->base_level + new_info.image_range->layers ||
          ss->image_range.base_level + ss->image_range.layers <=
            new_info.image_range->base_level)))
      {
        if (ss->access_write.access || new_info.access_write.access)
        {
          resource->used_twice_in_stage = true;
        }
        continue;
      }
    }

    AC_RG_MESSAGE(
      &builder->rg->callback,
      &(ac_rg_validation_message_internal) {
        .id = ac_rg_message_id_error_resource_referenced_twice_in_stage,
        .stage = stage,
        .state1 = state,
        .null_obj_name = resource->image_info.name,
        .null_obj_type = ac_rg_object_type_image,
      });

    builder->result = ac_result_bad_usage;
    return NULL;
  }

  state = ac_rg_add_resource_use(
    builder,
    add_mode,
    stage,
    resource,
    state,
    &new_info,
    layout);

  if (!state)
  {
    return NULL;
  }

  switch (slot)
  {
  case ac_rg_builder_stage_image_slot_default:
    if (new_info.usage_bits & ac_image_usage_attachment_bit)
    {
      uint32_t index = stage->attachment_count++;
      array_insert(stage->resource_states, index, state);
    }
    else
    {
      array_append(stage->resource_states, state);
    }
    break;
  case ac_rg_builder_stage_image_slot_resolve:
    array_append(stage->resolve_dst, state);
    break;
  default:
    AC_ASSERT(false);
    break;
  }

  return state;
}

static ac_rg_builder_resource
ac_rg_builder_add_buffer_use(
  ac_rg_builder                                builder,
  ac_rg_builder_stage                          stage,
  ac_rg_builder_stage_use_resource_info const* info)
{
  ac_rg_message_id message = ac_rg_message_id_not_a_message;
  AC_MAYBE_UNUSED(message);

  ac_rg_builder_resource new_state = NULL;

  ac_rg_builder_resource state = info->resource;
  ac_rg_graph_resource*  resource = state->resource;

  const ac_rg_resource_access r = info->access_read;
  const ac_rg_resource_access w = info->access_write;

  bool read = r.access > 0;
  bool write = w.access > 0;

  switch (info->usage_bits)
  {
  case ac_buffer_usage_none:
  {
    if (write || read)
    {
      message = ac_rg_message_id_error_bad_access_for_given_usage;
      goto VALIDATION_ERROR;
    }
    break;
  }
  case ac_buffer_usage_srv_bit:
  {
    if (write)
    {
      message = ac_rg_message_id_error_bad_access_for_given_usage;
      goto VALIDATION_ERROR;
    }
    break;
  }
  case ac_buffer_usage_uav_bit:
  {
    break;
  }
  case ac_buffer_usage_vertex_bit:
  {
    if (
      write || !(r.stages & ac_pipeline_stage_vertex_input_bit) ||
      !(r.access & ac_access_vertex_attribute_read_bit))
    {
      message = ac_rg_message_id_error_bad_access_for_given_usage;
      goto VALIDATION_ERROR;
    }
    break;
  }
  case ac_buffer_usage_index_bit:
  {
    if (
      write || !(r.stages & ac_pipeline_stage_vertex_input_bit) ||
      !(r.access & ac_access_index_read_bit))
    {
      message = ac_rg_message_id_error_bad_access_for_given_usage;
      goto VALIDATION_ERROR;
    }
    break;
  }
  case ac_buffer_usage_vertex_bit | ac_buffer_usage_index_bit:
  {
    if (
      write || !(r.stages & ac_pipeline_stage_vertex_input_bit) ||
      !(r.access &
        (ac_access_vertex_attribute_read_bit | ac_access_index_read_bit)))
    {
      message = ac_rg_message_id_error_bad_access_for_given_usage;
      goto VALIDATION_ERROR;
    }
    break;
  }
  case ac_buffer_usage_transfer_src_bit:
  case ac_buffer_usage_transfer_dst_bit:
  {
    AC_ASSERT(
      (r.access == 0 || (r.access == ac_access_transfer_read_bit &&
                         r.stages == ac_pipeline_stage_transfer_bit)));

    AC_ASSERT(
      (w.access == 0 || (w.access == ac_access_transfer_write_bit &&
                         w.stages == ac_pipeline_stage_transfer_bit)));

    AC_ASSERT((r.access > 0) + (w.access > 0) == 1);
    break;
  }
  default:
  {
    AC_ASSERT(false);
    break;
  }
  }

  for (size_t i = 0; stage && i < array_size(stage->resource_states); ++i)
  {
    ac_rg_builder_resource state1 = stage->resource_states[i];

    if (state1->resource == resource)
    {
      AC_RG_MESSAGE(
        &builder->rg->callback,
        &(ac_rg_validation_message_internal) {
          .id = ac_rg_message_id_error_resource_referenced_twice_in_stage,
          .stage = stage,
          .state1 = state1,
          .null_obj_name = resource->buffer_info.name,
          .null_obj_type = ac_rg_object_type_buffer,
        });

      builder->result = ac_result_bad_usage;
      return NULL;
    }
  }

  new_state = ac_rg_add_resource_use(
    builder,
    ac_rg_resource_use_add_mode_continue,
    stage,
    resource,
    state,
    info,
    ac_image_layout_undefined);

  if (!new_state)
  {
    return NULL;
  }

  if (stage)
  {
    array_append(stage->resource_states, new_state);
  }

  return new_state;

VALIDATION_ERROR:
  AC_RG_MESSAGE(
    &builder->rg->callback,
    &(ac_rg_validation_message_internal) {
      .id = message,
      .stage = stage,
      .null_obj_name = resource->buffer_info.name,
      .null_obj_type = ac_rg_object_type_image,
    });
  builder->result = ac_result_bad_usage;
  return NULL;
}

static inline bool
ac_rg_builder_check_nonnull(ac_rg_builder builder, const void* pointer)
{
  if (pointer != NULL)
  {
    return builder->result == ac_result_success;
  }

  if (builder && builder->result == ac_result_success)
  {
    AC_RG_MESSAGE(
      &builder->rg->callback,
      &(ac_rg_validation_message_internal) {
        .id = ac_rg_message_id_error_null_argument,
      });

    builder->result = ac_result_bad_usage;
  }

  return 0;
}

#if AC_RG_TODO
[[nodiscard]] RgImage*
RgBuilder::buffer_copy_to_image(RgBuffer* src, const RgImageCreateInfo& info)
{
  if (not stage_ref_check(p, src, p))
  {
    return nullptr;
  }

  if (not image_create_info_validation(
        p,
        nullptr,
        &info,
        RgImageUsage::TRANSFER_DST))
  {
    return nullptr;
  }

  // TODO format fallback later can lead to data corruption.
  uint32_t format_size = vkw::format_size(info.format);
  jassert_release(format_size > 0, "format size is wrong");

  if (info.samples != 1)
  {
    ::message(
      &p->callback,
      {
        .id = MessageId::ERROR_BUFFER_COPY_TO_IMAGE_SAMPLES,
        .p_use1 = src,
      });

    p->result = RgResult::ERROR_USAGE;
    return nullptr;
  }

  RgStageCreateInfo copy_stage_info {
    .name = "vkCmdCopyBufferToImage",
    .queue_type = jen::QueueType::TRANSFER_GPU_LOCAL,
    .pf_on_cmd = cmd_copy_buffer_to_image,
  };

  VkDeviceSize ntexels =
    info.extent.width * info.extent.height * info.extent.depth * info.layers;

  if (cast_buffer(src->p_resource)->info.size != ntexels)
  {
    ::message(
      &p->callback,
      {
        .id = MessageId::ERROR_BUFFER_COPY_TO_IMAGE_SIZE,
        .p_use1 = src,
        .null_obj_name = info.name,
        .null_obj_type = RgObjectType::IMAGE,
      });

    p->result = RgResult::ERROR_USAGE;
    return nullptr;
  }

  RgStage* stage = new_stage(copy_stage_info);

  RgResourceUse use_read {
    .stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT,
    .access_mask = VK_ACCESS_TRANSFER_READ_BIT,
  };

  buffer_read(stage, src, RgBufferUsage::TRANSFER_SRC, use_read);
  return image_produce(stage, info, RgImageUsage::TRANSFER_DST);
}
#endif

AC_API ac_rg_builder_resource
ac_rg_builder_create_resource(
  ac_rg_builder                             builder,
  ac_rg_builder_create_resource_info const* info)
{
  if (!ac_rg_builder_check_nonnull(builder, info))
  {
    return NULL;
  }

  AC_ASSERT(
    (info->buffer_info != NULL) + (info->image_info != NULL) +
      (info->import_rg_resource != NULL) + (info->import_connection != NULL) ==
    1);

  if (info->import_rg_resource)
  {
    for (size_t i = 0; i < array_size(builder->resource_mappings); ++i)
    {
      ac_rg_builder_resource_mapping mapping = builder->resource_mappings[i];
      if (mapping.resource != info->import_rg_resource)
      {
        continue;
      }

      ac_rg_builder_resource state = mapping.history->states[0];
      if (info->import_bits == ac_rg_import_allow_write_bit)
      {
        state->is_read_only = false;
      }
      return state;
    }
  }

  ac_rg_graph_resource* resource = ac_calloc(sizeof *resource);
  if (!resource)
  {
    builder->result = ac_result_out_of_host_memory;
    return NULL;
  }

  array_append(builder->resources, resource);

  bool is_image = info->image_info ||
                  (info->import_rg_resource && info->import_rg_resource->type ==
                                                 ac_rg_resource_type_image) ||
                  (info->import_connection && info->import_connection->image);

  if (is_image)
  {
    if (info->import_rg_resource)
    {
      resource->image_info = info->import_rg_resource->image_info;
    }
    else if (info->import_connection)
    {
      resource->image_info = ac_image_get_info(info->import_connection->image);
    }
    else
    {
      resource->image_info = *info->image_info;
    }

#if AC_RG_TODO
    uint8_t max = builder->max_samples_supported;
#else
    uint8_t max = 4;
#endif

    if (resource->image_info.samples > max)
    {
      resource->image_info.samples = max;

      AC_RG_MESSAGE(
        &builder->rg->callback,
        &(ac_rg_validation_message_internal) {
          .id = ac_rg_message_id_info_image_samples_reduced,
          .null_obj_name = resource->image_info.name,
          .null_obj_type = ac_rg_object_type_image,
        });
    }

    resource->image_info.usage = 0;

    if (
      resource->image_info.format == 0 || resource->image_info.width == 0 ||
      resource->image_info.height == 0 || resource->image_info.samples == 0)
    {
      AC_RG_MESSAGE(
        &builder->rg->callback,
        &(ac_rg_validation_message_internal) {
          .id = ac_rg_message_id_error_image_create_info_zero,
          .null_obj_name = resource->image_info.name,
          .null_obj_type = ac_rg_object_type_image,
        });
      builder->result = ac_result_bad_usage;
      return NULL;
    }

    resource->type = ac_rg_resource_type_image;
    resource->name = resource->image_info.name;
    resource->do_clear = info->do_clear;

#if AC_RG_TODO
    VkFormatProperties fp;
    vkGetPhysicalDeviceFormatProperties(
      p_graph->physical_device,
      info.format,
      &fp);
    image->info.format_features = fp.optimalTilingFeatures;
    p_image->info.aspect_mask = format_aspect_mask(info.format);
#endif
  }
  else
  {
    if (info->import_rg_resource)
    {
      resource->buffer_info = info->import_rg_resource->buffer_info;
    }
    else if (info->import_connection)
    {
      resource->buffer_info =
        ac_buffer_get_info(info->import_connection->buffer);
    }
    else
    {
      resource->buffer_info = *info->buffer_info;
    }

    if (info->do_clear)
    {
      // Buffer clearing not supported;
      AC_ASSERT(0);
      builder->result = ac_result_unknown_error;
      return NULL;
    }

    resource->type = ac_rg_resource_type_buffer;
    resource->name = resource->image_info.name;
  }

  ac_rg_builder_resource state = ac_rg_add_resource_use(
    builder,
    ac_rg_resource_use_add_mode_create,
    NULL,
    resource,
    NULL,
    NULL,
    ac_image_layout_undefined);

  if (state && (info->import_connection || info->import_rg_resource))
  {
    state->is_read_only = info->import_bits != ac_rg_import_allow_write_bit;
    state->is_imported = true;

    if (info->import_connection)
    {
      switch (state->resource->type)
      {
      case ac_rg_resource_type_image:
      {
        resource->image = info->import_connection->image;
        break;
      }
      case ac_rg_resource_type_buffer:
      {
        resource->buffer = info->import_connection->buffer;
        break;
      }
      default:
      {
        AC_DEBUGBREAK();
        return NULL;
      }
      }

      resource->import.is_active = true;
      resource->import.signal = info->import_connection->signal;
      resource->import.wait = info->import_connection->wait;
      resource->import.image_layout = info->import_connection->image_layout;
    }

    if (info->import_rg_resource)
    {
      resource->rg_resource = info->import_rg_resource;
      resource->buffer_or_image = resource->rg_resource->buffer;
    }

    ac_rg_builder_resource_mapping mapping = {
      .resource = info->import_rg_resource,
      .history = state->resource,
    };
    array_append(builder->resource_mappings, mapping);
  }

  return state;
}

AC_API ac_rg_builder_resource
ac_rg_builder_stage_use_resource(
  ac_rg_builder                                builder,
  ac_rg_builder_stage                          stage,
  ac_rg_builder_stage_use_resource_info const* info)
{
  if (
    !ac_rg_builder_check_nonnull(builder, stage) ||
    !ac_rg_builder_check_nonnull(builder, info) ||
    !ac_rg_builder_check_nonnull(builder, info->resource))
  {
    return NULL;
  }

  switch (info->resource->resource->type)
  {
  case ac_rg_resource_type_buffer:
  {
    return ac_rg_builder_add_buffer_use(builder, stage, info);
  }
  case ac_rg_resource_type_image:
  {
    return ac_rg_builder_add_image_use(
      builder,
      ac_rg_resource_use_add_mode_continue,
      stage,
      info,
      ac_rg_builder_stage_image_slot_default);
  }
  default:
  {
    AC_ASSERT(false);
  }
  }

  return NULL;
}

#if AC_RG_TODO
static ac_format
merge_format(ac_format f1, ac_format f2)
{
  if (f1 == f2)
  {
    return f1;
  }

  // formats not same, try fallbacks
  ac_format fc1 = format_fallback(f1);
  if (fc1 != ac_format_undefined)
  {
    f1 = fc1;
  }

  ac_format fc2 = format_fallback(f2);
  if (fc2 != ac_format_undefined)
  {
    f2 = fc2;
  }

  if (f1 == f2)
  {
    return f1;
  }

  return ac_format_undefined;
}
#endif

AC_API ac_rg_builder_resource
ac_rg_builder_blit_image(
  ac_rg_builder          builder,
  ac_rg_builder_group    group,
  const char*            name,
  ac_rg_builder_resource src_state,
  ac_rg_builder_resource dst_state,
  ac_filter              filter)
{
  if (
    !ac_rg_builder_check_nonnull(builder, src_state) ||
    !ac_rg_builder_check_nonnull(builder, dst_state))
  {
    return NULL;
  }

  if (src_state->resource->type != ac_rg_resource_type_image)
  {
    return NULL;
  }
  if (dst_state->resource->type != ac_rg_resource_type_image)
  {
    return NULL;
  }

  const ac_image_info* dst_info = &dst_state->resource->image_info;

  if (dst_info->samples != 1)
  {
    AC_RG_MESSAGE(
      &builder->rg->callback,
      &(ac_rg_validation_message_internal) {
        .id = ac_rg_message_id_error_image_blit_samples,
        .state1 = src_state,
      });

    builder->result = ac_result_bad_usage;
    return NULL;
  }

  ac_rg_graph_resource* src_resource = src_state->resource;

  ac_image_info const* src_info = &src_resource->image_info;

#if AC_RG_TODO
  VkFormat format = merge_format(old_info.format, new_info.format);

  if (format == VK_FORMAT_UNDEFINED)
  {
    goto BLIT;
  }
#endif

  if (dst_info->width != src_info->width)
  {
    goto BLIT;
  }
  if (dst_info->height != src_info->height)
  {
    goto BLIT;
  }

#if AC_RG_TODO
  if (ac_rg_check_format(p, src_image, format))
  {
    return src_image_use;
  }

  return NULL;
#endif

BLIT:;

  ac_rg_common_pass_instance_info pass_info = {
    .pipeline = ac_rg_common_pipeline_type_image_blit_compute,
    .commands_type = ac_queue_type_compute,
  };

  ac_rg_common_pass_instance* pass;

  ac_result res = ac_rg_storage_get_instance(
    &builder->rg->storage,
    &builder->rg->common_passes,
    builder->rg->device,
    &pass_info,
    &pass);
  if (res != ac_result_success)
  {
    builder->result = res;
    return NULL;
  }

  ac_queue_type queue = ac_queue_type_graphics;

  if (array_size(src_state->uses))
  {
    queue = src_state->uses[0].builder_stage->info.queue;
    if (queue == ac_queue_type_transfer)
    {
      queue = ac_queue_type_compute;
    }
  }

  return ac_rg_common_pass_build_image_blit(
    builder,
    pass,
    group,
    name,
    queue,
    filter,
    src_state,
    dst_state);
}

#if AC_RG_TODO
[[nodiscard]] RgImage*
RgBuilder::image_copy(RgImage* p_src, const RgImageCreateInfo& new_info)
{
  if (not stage_ref_check(p, p_src, p_src))
  {
    return nullptr;
  }

  auto& old_info = cast_image(p_src->p_resource)->info;

  VkFormat format = merge_format(old_info.format, new_info.format);
  if (format != old_info.format)
  {
    goto EUSAGE;
  }

  // TODO format fallback later can lead to wrong vulkan usage.

  if (
    jl::cmp_bytes(old_info.extent, new_info.extent) != 0 or
    old_info.layers != new_info.layers or old_info.levels != new_info.levels or
    old_info.samples != new_info.samples)
  {
  EUSAGE:
    ::message(
      &p->callback,
      {
        .id = MessageId::ERROR_IMAGE_COPY_WRONG_IMAGE_PROPERTIES,
        .p_use1 = p_src,
      });

    p->result = RgResult::ERROR_USAGE;
    return nullptr;
  }

  RgStageCreateInfo copy_stage_info {
    .name = "vkCmdCopyImage",
    .queue_type = jen::QueueType::GRAPHICS,
    .pf_on_cmd = cmd_copy_image,
  };

  RgStage* p_stage;
  p_stage = new_stage(copy_stage_info);
  image_use(p_stage, p_src, RgImageUsage::TRANSFER_SRC, RgAccessFlag::READ);
  return image_produce(p_stage, new_info, RgImageUsage::TRANSFER_DST);
}

[[nodiscard]] RgBuffer*
RgBuilder::image_copy_to_buffer(
  RgImage* p_src,
  uint32_t layer_offset,
  uint32_t n_layers)
{
  if (not stage_ref_check(p, p_src, p))
  {
    return nullptr;
  }

  auto& info = cast_image(p_src->p_resource)->info;

  // TODO format fallback later can lead to data corruption.
  uint32_t format_size = vkw::format_size(info.format);
  jassert_release(format_size > 0, "format size is wrong");

  if (info.samples != 1)
  {
    ::message(
      &p->callback,
      {
        .id = MessageId::ERROR_IMAGE_COPY_TO_BUFFER_SAMPLES,
        .p_use1 = p_src,
      });

    p->result = RgResult::ERROR_USAGE;
    return nullptr;
  }

  union {
    void* p;
    struct {
      uint32_t offset;
      uint32_t count;
    } layer;
  } data {.p = p};

  if (info.layers < n_layers)
  {
    n_layers = info.layers;
  }

  data.layer = {
    .offset = layer_offset,
    .count = n_layers,
  };

  RgStageCreateInfo copy_stage_info {
    .name = "vkCmdCopyImageToBuffer",
    .queue_type = jen::QueueType::TRANSFER_GPU_LOCAL,
    .pf_on_cmd = cmd_copy_image_to_buffer,
    .p_cb_data = data.p,
  };

  VkDeviceSize ntexels =
    info.extent.width * info.extent.height * info.extent.depth * n_layers;

  RgBufferCreateInfo create_info {.size = format_size * ntexels};

  RgStage* p_stage;
  p_stage = new_stage(copy_stage_info);
  image_use(p_stage, p_src, RgImageUsage::TRANSFER_SRC, RgAccessFlag::READ);
  return buffer_produce(p_stage, create_info, RgBufferUsage::TRANSFER_DST, {});
}
#endif

AC_API ac_image_info
ac_rg_builder_get_image_info(ac_rg_builder_resource state)
{
  if (!state)
  {
    return (ac_image_info) {0};
  }

  ac_image_info info = state->resource->image_info;

  for (size_t i = 0; i < array_size(state->resource->states); ++i)
  {
    ac_rg_builder_resource history = state->resource->states[i];
    if (array_size(history->uses))
    {
      ac_image_subresource_range range = history->uses[0].specifics.image_range;

      info.layers = AC_MAX(info.layers, range.base_layer + range.layers);
      info.levels = AC_MAX(info.levels, range.base_level + range.levels);
    }

    if (history == state)
    {
      break;
    }
  }

  if (info.layers == 0)
  {
    info.layers = 1;
  }
  if (info.levels == 0)
  {
    info.levels = 1;
  }

  return info;
}

AC_API void
ac_rg_builder_export_resource(
  ac_rg_builder                             builder,
  ac_rg_builder_export_resource_info const* info)
{
  if (
    !ac_rg_builder_check_nonnull(builder, info) ||
    !ac_rg_builder_check_nonnull(builder, info->resource))
  {
    return;
  }

  if ((info->connection != NULL) + (info->rg_resource != NULL) != 1)
  {
    builder->result = ac_result_bad_usage;
    return;
  }

  ac_rg_builder_deferred_resource_export export;
  AC_ZERO(export);

  export.state = info->resource;

  ac_rg_graph_resource* resource = export.state->resource;

  if (info->rg_resource)
  {
    export.rg_resource = info->rg_resource;

    if (info->rg_resource->type == ac_rg_resource_type_image)
    {
      export.image_info = info->rg_resource->image_info;
    }
    else
    {
      export.buffer_info = info->rg_resource->buffer_info;
    }
  }

  if (info->connection)
  {
    export.connection = *info->connection;

    if (resource->type == ac_rg_resource_type_image)
    {
      export.image_info = ac_image_get_info(info->connection->image);
    }
    else
    {
      export.buffer_info = ac_buffer_get_info(info->connection->buffer);
    }

    if (export.connection.image_layout == ac_image_layout_undefined)
    {
      builder->result = ac_result_bad_usage;
      return;
    }

    const ac_fence_submit_info* signal = &export.connection.signal;
    if (
      signal->fence && (signal->fence->bits & ac_fence_present_bit) &&
      signal->value != 0)
    {
      builder->result = ac_result_bad_usage;
      return;
    }
  }

  switch (resource->type)
  {
  case ac_rg_resource_type_buffer:
  {
    if (export.buffer_info.size != resource->buffer_info.size)
    {
      AC_RG_MESSAGE(
        &builder->rg->callback,
        &(ac_rg_validation_message_internal) {
          .id = ac_rg_message_id_error_external_resource_compatibility,
          .state1 = export.state,
          .null_obj_name = export.buffer_info.name,
          .null_obj_type = ac_rg_object_type_buffer,
        });

      builder->result = ac_result_bad_usage;
      return;
    }
    break;
  }
  case ac_rg_resource_type_image:
  {
    if (
      ac_format_depth_or_stencil(export.image_info.format) !=
      ac_format_depth_or_stencil(resource->image_info.format))
    {
      AC_RG_MESSAGE(
        &builder->rg->callback,
        &(ac_rg_validation_message_internal) {
          .id = ac_rg_message_id_error_external_resource_compatibility,
          .state1 = export.state,
          .null_obj_name = export.image_info.name,
          .null_obj_type = ac_rg_object_type_image,
        });

      builder->result = ac_result_bad_usage;
      return;
    }

    export.image_info = export.image_info;
    break;
  }
  default:
  {
    AC_ASSERT(false);
    return;
  }
  }

  export.state->is_exported = true;
  array_append(builder->deferred_exports, export);
}

AC_API ac_rg_builder_resource
ac_rg_builder_resolve_image(
  ac_rg_builder          builder,
  ac_rg_builder_resource src_state)
{
  if (!ac_rg_builder_check_nonnull(builder, src_state))
  {
    return NULL;
  }

  ac_rg_graph_resource* src_resource = src_state->resource;

  if (src_resource->image_info.samples == 1)
  {
    return src_state;
  }

  if (src_state->image_resolve_dst)
  {
    return src_state->image_resolve_dst;
  }

  if (
    array_size(src_state->uses) &&
    src_state->uses[0].usage_bits == ac_image_usage_attachment_bit)
  {
    ac_image_info new_info = src_resource->image_info;
    new_info.samples = 1;

    ac_rg_builder_create_resource_info create_info = {
      .image_info = &new_info,
    };

    src_state->image_resolve_dst =
      ac_rg_builder_create_resource(builder, &create_info);

    ac_rg_builder_stage_use_resource_info use_info = {
      .resource = src_state->image_resolve_dst,
    };

    return ac_rg_builder_add_image_use(
      builder,
      ac_rg_resource_use_add_mode_edit,
      src_state->uses[0].builder_stage,
      &use_info,
      ac_rg_builder_stage_image_slot_resolve);
  }

  // TODO resolve by inserting render pass or without render pass
  AC_ASSERT(0);
  builder->result = ac_result_unknown_error;
  return NULL;
}

AC_API ac_rg_builder_group
ac_rg_builder_create_group(
  ac_rg_builder                   builder,
  ac_rg_builder_group_info const* info)
{
  ac_rg_builder_group_internal group = {.info = *info};
  array_append(builder->groups, group);
  return (ac_rg_builder_group)array_size(builder->groups);
}

AC_API ac_rg_builder_stage
ac_rg_builder_create_stage(
  ac_rg_builder                   builder,
  ac_rg_builder_stage_info const* info)
{
  if (!ac_rg_builder_check_nonnull(builder, info))
  {
    return NULL;
  }

  ac_rg_builder_stage stage = ac_calloc(sizeof *stage);
  if (!stage)
  {
    builder->result = ac_result_out_of_host_memory;
    return NULL;
  }

  stage->info = *info;

  array_append(builder->stages, stage);
  return stage;
}

static inline bool
ac_rg_resource_use_is_useful(
  ac_rg_builder_stage    stage,
  ac_rg_builder_resource state)
{
  ac_rg_graph_resource_use* use = ac_rg_builder_find_resource_use(state, stage);
  AC_ASSERT(use);
  return (array_size(state->uses) > 1 && use->specifics.access_write.access) ||
         state->is_exported || array_back(state->resource->states) != state;
}

static inline uint32_t
ac_rg_graph_state_use_index(
  ac_rg_builder_resource state,
  ac_rg_builder_stage    stage)
{
  for (uint32_t i = 0; i < array_size(state->uses); ++i)
  {
    if (state->uses[i].builder_stage != stage)
    {
      continue;
    }
    return i;
  }

  AC_ASSERT(0);
  return 0;
}

static void
ac_rg_remove_resource_use(
  ac_rg_builder_resource state,
  ac_rg_builder_stage    stage)
{
  ac_rg_graph_resource* resource = state->resource;

  size_t use_count = array_size(state->uses);

  if (stage)
  {
    array_remove(state->uses, ac_rg_graph_state_use_index(state, stage));
  }

  if (use_count > 1)
  {
    return;
  }

  for (size_t i = 0; i < array_size(resource->states); ++i)
  {
    if (resource->states[i] != state)
    {
      continue;
    }
    array_remove(resource->states, i);
    break;
  }

  ac_rg_destroy_resource_state(state);
}

static void
ac_rg_builder_apply_zero_state(
  ac_rg_builder         builder,
  ac_rg_graph_resource* resource)
{
  uint32_t state_count = (uint32_t)array_size(resource->states);
  if (!state_count)
  {
    return;
  }

  ac_rg_builder_resource first_state = resource->states[0];

  bool first_use_state_index = state_count > 1 &&
                               array_size(first_state->uses) == 0 &&
                               !first_state->is_exported;

  bool insert_clear = resource->do_clear;

  if (insert_clear)
  {
    ac_rg_builder_resource clear_state =
      resource->states[first_use_state_index];

    if (
      array_size(clear_state->uses) &&
      clear_state->uses[0].builder_stage->info.commands ==
        ac_queue_type_graphics)
    {
      insert_clear = false;
    }
  }

  if (!insert_clear)
  {
    if (first_use_state_index > 0)
    {
      if (state_count > 0 && resource->states[0]->is_imported)
      {
        resource->states[1]->is_imported = true;
      }

      ac_rg_destroy_resource_state(resource->states[0]);
      array_remove(resource->states, 0);
    }
    return;
  }

  // TODO decide how to clear and where to clear (compute/graphics)
  // Maybe even avoid clearing each frame.
  ac_rg_builder_stage_info stage_info = {
    .name = "image clear",
    .queue = ac_queue_type_graphics,
    .commands = ac_queue_type_graphics,
  };
  ac_rg_builder_stage stage = ac_rg_builder_create_stage(builder, &stage_info);

  ac_rg_builder_add_image_use(
    builder,
    ac_rg_resource_use_add_mode_edit,
    stage,
    &(ac_rg_builder_stage_use_resource_info) {
      .resource = resource->states[0],
      .usage_bits = ac_image_usage_attachment_bit,
      .access_attachment = ac_rg_attachment_access_write_bit,
      .token = 0,
    },
    ac_rg_builder_stage_image_slot_default);
}

static ac_result
ac_rg_builder_apply_zero_states(ac_rg_builder builder)
{
  uint64_t count = array_size(builder->resources);
  for (size_t i = 0; i < count; ++i)
  {
    ac_rg_graph_resource* resource = builder->resources[i];
    ac_rg_builder_apply_zero_state(builder, resource);
  }

  return builder->result;
}

static void
ac_rg_builder_remove_unused_stages_and_resources(ac_rg_builder builder)
{
  bool culled;

  do
  {
    culled = 0;

    for (size_t i = 0; i < array_size(builder->stages);)
    {
      ac_rg_builder_stage stage = builder->stages[i];

      bool useful = false;
      for (size_t j = 0; j < array_size(stage->resolve_dst); ++j)
      {
        ac_rg_builder_resource resolve_state = stage->resolve_dst[j];

        if (ac_rg_resource_use_is_useful(stage, resolve_state))
        {
          useful = true;
          continue;
        }

        array_remove(stage->resolve_dst, j--);

        for (size_t ai = 0; ai < array_size(stage->resource_states); ++ai)
        {
          ac_rg_builder_resource resolve_src = stage->resource_states[ai];
          if (resolve_src->image_resolve_dst != resolve_state)
          {
            continue;
          }
          resolve_src->image_resolve_dst = NULL;
          break;
        }

        ac_rg_remove_resource_use(resolve_state, NULL);
      }

      if (useful)
      {
        goto CONTINUE;
      }

      for (size_t j = 0; j < array_size(stage->resource_states); ++j)
      {
        if (ac_rg_resource_use_is_useful(stage, stage->resource_states[j]))
        {
          goto CONTINUE;
        }
      }

      culled = 1;

      for (size_t j = 0; j < array_size(stage->resource_states); ++j)
      {
        ac_rg_remove_resource_use(stage->resource_states[j], stage);
      }

      for (size_t j = 0; j < array_size(stage->resolve_dst); ++j)
      {
        ac_rg_remove_resource_use(stage->resolve_dst[j], stage);
      }

      ac_rg_destroy_builder_stage(stage);
      array_remove(builder->stages, i);
      continue;
    CONTINUE:
      ++i;
    }
  }
  while (culled);

  uint64_t count = array_size(builder->resources);
  uint64_t new_count = 0;
  for (uint64_t i = 0; i < count; ++i)
  {
    ac_rg_graph_resource* resource = builder->resources[i];

    bool remove = array_size(resource->states) == 0;
    if (remove)
    {
      ac_rg_destroy_resource_internal(resource);
    }
    else
    {
      if (i != new_count)
      {
        builder->resources[new_count] = resource;
      }
      ++new_count;
    }
  }

  if (new_count != count)
  {
    array_resize(builder->resources, new_count);
  }
}

static void
ac_rg_builder_determine_create_info(
  ac_rg_builder         builder,
  ac_rg_graph_resource* resource)
{
  AC_UNUSED(builder);

  uint32_t usage = 0;
  uint16_t levels = 1;
  uint16_t layers = 1;

  for (size_t si = 0; si < array_size(resource->states); ++si)
  {
    ac_rg_builder_resource state = resource->states[si];

    for (size_t ui = 0; ui < array_size(state->uses); ++ui)
    {
      ac_rg_graph_resource_use* use = &state->uses[ui];

      usage |= use->usage_bits;

      levels = AC_MAX(
        use->specifics.image_range.base_level +
          use->specifics.image_range.levels,
        levels);
      layers = AC_MAX(
        use->specifics.image_range.base_layer +
          use->specifics.image_range.layers,
        layers);
    }
  }

  switch (resource->type)
  {
  case ac_rg_resource_type_image:
  {
    resource->image_info.levels = AC_MAX(levels, resource->image_info.levels);
    resource->image_info.layers = AC_MAX(layers, resource->image_info.layers);
    resource->image_info.usage = usage;

    if (!ac_rg_check_format(builder, resource, resource->image_info.format))
    {
      AC_RG_MESSAGE(
        &builder->rg->callback,
        &(ac_rg_validation_message_internal) {
          .id = ac_rg_message_id_error_format_not_supported,
          .null_obj_name = resource->image_info.name,
          .null_obj_type = ac_rg_object_type_image,
        });

      builder->result = ac_result_format_not_supported;
    }
    break;
  }
  case ac_rg_resource_type_buffer:
  {
    resource->buffer_info.usage = usage;
    break;
  }
  default:
  {
    AC_ASSERT(false);
    break;
  }
  }
}

static ac_result
ac_rg_builder_determine_resources_create_info(ac_rg_builder builder)
{
  uint64_t count = array_size(builder->resources);
  for (size_t i = 0; i < count; ++i)
  {
    ac_rg_builder_determine_create_info(builder, builder->resources[i]);
  }

  return builder->result;
}

static ac_result
ac_rg_builder_apply_export(ac_rg_builder builder)
{
  for (size_t i = 0; i < array_size(builder->deferred_exports); ++i)
  {
    ac_rg_builder_deferred_resource_export* export =
      &builder->deferred_exports[i];
    ac_rg_builder_resource state = export->state;
    ac_rg_graph_resource*  resource = state->resource;
    bool is_last_state = array_back(resource->states) == state;

    state->is_exported = false;

    bool is_compatible = true;

    ac_rg_builder_resource blit_dst = NULL;

    if (export->rg_resource)
    {
      bool mapped = false;
      for (size_t ii = 0; ii < array_size(builder->resource_mappings); ++ii)
      {
        ac_rg_builder_resource_mapping mapping = builder->resource_mappings[ii];
        if (mapping.resource != export->rg_resource)
        {
          continue;
        }
        mapped = true;
        if (mapping.history == resource)
        {
          break;
        }

        ac_rg_builder_resource last_state =
          mapping.history->states[array_size(mapping.history->states) - 1];
        if (last_state->is_read_only)
        {
          last_state->is_read_only = false;
        }

        if (resource->export.is_active)
        {
          AC_RG_MESSAGE(
            &builder->rg->callback,
            &(ac_rg_validation_message_internal) {
              .id = ac_rg_message_id_error_exported_multiple_times,
              .state1 = export->state,
              .state2 = mapping.history->states[0],
              .resource = resource->rg_resource,
            });

          builder->result = ac_result_bad_usage;
          return builder->result;
        }

        is_compatible = false;
        blit_dst = last_state;
        break;
      }

      if (!mapped)
      {
        ac_rg_builder_resource_mapping mapping = {
          .resource = export->rg_resource,
          .history = resource,
        };
        array_append(builder->resource_mappings, mapping);
      }
    }

    switch (state->resource->type)
    {
    case ac_rg_resource_type_image:
    {
      const ac_image_info* exported_info = &export->image_info;

      do
      {
        if (!is_last_state)
        {
          is_compatible = false;
        }

        if (resource->export.is_active)
        {
          is_compatible = false;
          break;
        }

        ac_image_info* src_info = &resource->image_info;

        if (src_info->format != exported_info->format)
        {
#if AC_RG_TODO
          VkFormat format = format_fallback(src_info.format);
          if (format != exported_image->format)
#endif
          {
            is_compatible = false;
            break;
          }
        }

        if (
          src_info->width != exported_info->width ||
          src_info->height != exported_info->height ||
          src_info->layers > exported_info->layers ||
          src_info->levels > exported_info->levels ||
          src_info->samples != exported_info->samples)
        {
          is_compatible = false;
          break;
        }

        if ((resource->image_info.usage -
             (resource->image_info.usage & exported_info->usage)))
        {
          is_compatible = false;
        }
      }
      while (0);

      if (is_compatible)
      {
        break;
      }

      // TODO use copy instead of blit when possible

      if (!blit_dst)
      {
        blit_dst = ac_rg_builder_create_resource(
          builder,
          &(ac_rg_builder_create_resource_info) {.image_info = exported_info});
      }

      state = ac_rg_builder_blit_image(
        builder,
        NULL,
        "implicit copy for image export",
        state,
        blit_dst,
        ac_filter_linear);
      if (!state)
      {
        return builder->result;
      }

      ac_rg_builder_apply_zero_state(builder, state->resource);

      // update image create info after adding blit passess
      ac_rg_builder_determine_create_info(builder, resource);
      ac_rg_builder_determine_create_info(builder, state->resource);

      resource = state->resource;

      // image was created with incompatible use mask (again)
      if (
        resource->image_info.usage -
        (resource->image_info.usage & exported_info->usage))
      {
        AC_RG_MESSAGE(
          &builder->rg->callback,
          &(ac_rg_validation_message_internal) {
            .id = ac_rg_message_id_error_external_use_mask_not_supported,
            .state1 = export->state,
            .state2 = state,
            .metadata = exported_info->usage,
          });

        builder->result = ac_result_exported_image_not_supported;
        return builder->result;
      }

      if (resource->image_info.format != exported_info->format)
      {
        // format is changed during building, because it is unsupported
        AC_RG_MESSAGE(
          &builder->rg->callback,
          &(ac_rg_validation_message_internal) {
            .id = ac_rg_message_id_error_format_not_supported,
            .state1 = export->state,
            .state2 = state,
          });

        builder->result = ac_result_format_not_supported;
        return builder->result;
      }

      break;
    }
    case ac_rg_resource_type_buffer:
    {
      // TODO buffer copy
      if (!is_last_state || resource->export.is_active)
      {
        AC_ASSERT(false);
        builder->result = ac_result_unknown_error;
        return builder->result;
      }
      break;
    }
    default:
    {
      AC_ASSERT(false);
      break;
    }
    }

    resource->export.is_active = true;
    resource->export.signal = export->connection.signal;
    resource->export.wait = export->connection.wait;
    resource->export.image_layout = export->connection.image_layout;

    if (export->rg_resource)
    {
      resource->rg_resource = export->rg_resource;
      resource->buffer_or_image = export->rg_resource->buffer_or_image;
    }
    else
    {
      switch (state->resource->type)
      {
      case ac_rg_resource_type_image:
      {
        resource->image = export->connection.image;
        break;
      }
      case ac_rg_resource_type_buffer:
      {
        resource->buffer = export->connection.buffer;
        break;
      }
      default:
      {
        AC_DEBUGBREAK();
        return ac_result_unknown_error;
      }
      }
    }
    state->is_exported = true;
  }

  return ac_result_success;
}

static bool
ac_rg_builder_resource_is_time(
  ac_rg_builder_resource state,
  ac_rg_builder_stage    stage)
{
  ac_rg_builder_timeline_state* timeline = &state->resource->timeline_state;

  uint32_t state_index = ac_rg_state_index(state);

  if (timeline->state != state_index)
  {
    if (
      state_index < array_size(state->resource->states) &&
      state->resource->used_twice_in_stage)
    {
      ac_rg_builder_resource next_state = state->resource->states[state_index];
      if (next_state->uses[0].builder_stage != stage)
      {
        return false;
      }
    }
    else
    {
      return false;
    }
#if 0
		size_t ui = 0;
		for ( ; ui < array_size( state->uses ); ++ui )
		{
			ac_rg_graph_resource_use* use = &state->uses[ ui ];
			if ( use->builder_stage != stage )
			{
				continue;
			}

			break;
		}

		size_t use_count = 1;
		for ( ; ui < array_size( state->uses ); ++ui )
		{
			ac_rg_graph_resource_use* use = &state->uses[ ui ];
			if ( use->builder_stage != stage )
				break;
			++use_count;
		}

		uint32_t end_state_index = state_index;
#endif
  }

  if (timeline->use_count > 0)
  {
    return true;
  }

  ac_rg_graph_resource_use* first_use = &state->uses[0];

  if (!first_use->specifics.access_write.access)
  {
    return true;
  }

  uint32_t use_index = ac_rg_graph_state_use_index(state, stage);
  if (use_index == 0)
  {
    return true;
  }

  return false;
}

static void
ac_rg_builder_resource_advance_time(
  ac_rg_builder_resource state,
  ac_rg_builder_stage    stage)
{
  ac_rg_builder_timeline_state* timeline = &state->resource->timeline_state;

  // Correct use order to follow stages order
  uint32_t ui1 = timeline->use_count;
  uint32_t ui2 = ac_rg_graph_state_use_index(state, stage);

  ac_rg_graph_resource_use use = state->uses[ui1];
  state->uses[ui1] = state->uses[ui2];
  state->uses[ui2] = use;

  ++timeline->use_count;

  if (timeline->use_count >= array_size(state->uses))
  {
    ++timeline->state;
    timeline->use_count = 0;
  }
}

static bool
ac_rg_builder_stage_is_time(ac_rg_builder builder, ac_rg_builder_stage stage)
{
  for (size_t j = 0; j < array_size(stage->resource_states); ++j)
  {
    if (!ac_rg_builder_resource_is_time(stage->resource_states[j], stage))
    {
      return false;
    }
  }

  for (size_t j = 0; j < array_size(stage->resolve_dst); ++j)
  {
    if (!ac_rg_builder_resource_is_time(stage->resolve_dst[j], stage))
    {
      return false;
    }
  }

  for (size_t j = 0; j < array_size(stage->resource_states); ++j)
  {
    ac_rg_builder_resource_advance_time(stage->resource_states[j], stage);
  }

  for (size_t j = 0; j < array_size(stage->resolve_dst); ++j)
  {
    ac_rg_builder_resource_advance_time(stage->resolve_dst[j], stage);
  }

  array_append(builder->timeline, stage);
  return true;
}

static ac_result
ac_rg_builder_sort_stages(ac_rg_builder builder)
{
  // timeline already initialized
  AC_ASSERT(array_size(builder->timeline) == 0);

  array_reserve(builder->timeline, array_size(builder->stages));

  for (size_t i = 0; i < array_size(builder->resources); ++i)
  {
    ac_rg_graph_resource* resource = builder->resources[i];

    resource->timeline_state = (ac_rg_builder_timeline_state) {
      .state = 0,
      .use_count = 0,
    };

#if 0
		while ( resource->timeline_state.state < array_size( resource->states ) &&
		        array_size(
		          resource->states[ resource->timeline_state.state ]->uses ) == 0 )
		{
			++resource->timeline_state.state;
		}
#endif
  }

  array_t(ac_rg_builder_stage) unused_stages[ac_queue_type_count];
  AC_ZERO(unused_stages);

  for (uint32_t i = 0; i < array_size(builder->stages); ++i)
  {
    ac_rg_builder_stage stage = builder->stages[i];
    array_append(unused_stages[stage->info.commands], stage);
  }

  bool stage_used[ac_queue_type_count];
  bool single_stage_used;

  do
  {
    single_stage_used = false;

    for (int qi = 0; qi < ac_queue_type_count; ++qi)
    {
      do
      {
        stage_used[qi] = false;

        for (uint32_t si = 0; si < array_size(unused_stages[qi]);)
        {
          if (!ac_rg_builder_stage_is_time(builder, unused_stages[qi][si]))
          {
            ++si;
            continue;
          }

          stage_used[qi] = true;
          single_stage_used = true;
          array_remove(unused_stages[qi], si);
        }
      }
      while (stage_used[qi]);
    }
  }
  while (single_stage_used);

  for (int qi = 0; qi < ac_queue_type_count; ++qi)
  {
    size_t unresolved_stage_count = array_size(unused_stages[qi]);
    if (unresolved_stage_count)
    {
      AC_RG_MESSAGE(
        &builder->rg->callback,
        &(ac_rg_validation_message_internal) {
          .id = ac_rg_message_id_error_cyclic_stage_dependencies,
        });

      builder->result = ac_result_bad_usage;
      return ac_result_bad_usage;
    }
    array_free(unused_stages[qi]);
  }

  if (array_size(builder->timeline) == 0)
  {
    array_free(builder->timeline);
    return ac_result_canceled;
  }
  return ac_result_success;
}

static inline void
ac_rg_destroy_graph_stage_barrier(ac_rg_graph_stage_barrier* barrier)
{
  array_free(barrier->buffers);
  array_free(barrier->images);

  array_free(barrier->buffer_nodes);
  array_free(barrier->image_nodes);
}

static inline void
ac_rg_destroy_graph_stage_subpass(ac_rg_graph_stage_subpass* subpass)
{
  array_free(subpass->images);
  array_free(subpass->buffers);
  array_free(subpass->attachments);
}

static inline void
ac_rg_destroy_graph_stage(ac_rg_graph_stage* stage)
{
  array_free(stage->fences_signal);
  array_free(stage->fences_wait);

  for (size_t si = 0; si < array_size(stage->subpasses); ++si)
  {
    ac_rg_destroy_graph_stage_subpass(&stage->subpasses[si]);
  }
  array_free(stage->subpasses);

  ac_rg_destroy_graph_stage_barrier(&stage->barrier_beg);
  ac_rg_destroy_graph_stage_barrier(&stage->barrier_end);

  ac_free(stage);
}

static inline void
ac_rg_destroy_graph_stages(ac_rg_builder builder)
{
  for (size_t qi = 0; qi < ac_queue_type_count; ++qi)
  {
    array_t(ac_rg_graph_stage*)* stages = &builder->stage_queues[qi];

    for (size_t si = 0; si < array_size(*stages); ++si)
    {
      ac_rg_destroy_graph_stage((*stages)[si]);
    }

    array_free(*stages);
    *stages = NULL;
  }
}

ac_rg_graph_resource_reference
ac_rg_graph_resource_next_use(ac_rg_graph_resource_reference use)
{
  if (use.state < 0)
  {
    use.state = 0;
    use.use = 0;
    return use;
  }

  ac_rg_builder_resource state = use.resource->states[use.state];

  if (use.use + 1 < array_size(state->uses))
  {
    ++use.use;
    return use;
  }

  if ((size_t)(use.state + 1) < array_size(use.resource->states))
  {
    ++use.state;
    use.use = 0;
    return use;
  }

  use.resource = 0;
  return use;
}

ac_rg_graph_resource_reference
ac_rg_graph_resource_prev_use(ac_rg_graph_resource_reference use)
{
  if (use.state < 0)
  {
    use.resource = NULL;
    return use;
  }

  if (use.use != 0)
  {
    --use.use;
    return use;
  }

  if (use.state == 0)
  {
    use.resource = NULL;
    return use;
  }

  --use.state;
  use.use = array_size(use.resource->states[use.state]->uses);
  if (use.use == 0)
  {
    use.resource = NULL;
  }
  else
  {
    --use.use;
  }
  return use;
}

static void
ac_rg_graph_stage_add_fence(
  array_t(ac_fence_submit_info) * array,
  ac_fence_submit_info   info,
  ac_pipeline_stage_bits extra_stages)
{
  info.stages |= extra_stages;

  for (size_t i = 0; i < array_size(*array); ++i)
  {
    ac_fence_submit_info* fence = (*array) + i;
    if (fence->fence != info.fence)
    {
      continue;
    }

    if (fence->value == info.value)
    {
      fence->stages |= info.stages;
      return;
    }
    break;
  }

  array_append(*array, info);
}

static void
ac_rg_graph_add_external_sync(
  ac_rg_graph_stage*             stage,
  ac_rg_graph_resource_reference use_reference)
{
  ac_rg_graph_resource* resource = use_reference.resource;

  if (
    (resource->import.wait.fence || resource->export.wait.fence) &&
    !ac_rg_graph_resource_prev_use(use_reference).resource)
  {
    ac_rg_graph_resource_use* use =
      ac_rg_graph_resource_dereference_use(use_reference);

    ac_pipeline_stage_bits extra_stages =
      use->specifics.access_read.stages | use->specifics.access_write.stages;

    if (resource->import.wait.fence)
    {
      ac_rg_graph_stage_add_fence(
        &stage->fences_wait,
        resource->import.wait,
        extra_stages);
    }
    if (resource->export.wait.fence)
    {
      ac_rg_graph_stage_add_fence(
        &stage->fences_wait,
        resource->export.wait,
        extra_stages);
    }
  }

  if (
    (resource->import.signal.fence || resource->export.signal.fence) &&
    !ac_rg_graph_resource_next_use(use_reference).resource)
  {
    ac_rg_graph_resource_use* use =
      ac_rg_graph_resource_dereference_use(use_reference);

    ac_pipeline_stage_bits extra_stages =
      use->specifics.access_read.stages | use->specifics.access_write.stages;

    if (resource->import.signal.fence)
    {
      ac_rg_graph_stage_add_fence(
        &stage->fences_signal,
        resource->import.signal,
        extra_stages);
    }
    if (resource->export.signal.fence)
    {
      ac_rg_graph_stage_add_fence(
        &stage->fences_signal,
        resource->export.signal,
        extra_stages);
    }
  }
}

static ac_rg_graph_resource_reference
ac_rg_graph_compute_resource_reference(
  ac_rg_builder_resource state,
  ac_rg_builder_stage    builder_stage,
  ac_rg_graph_stage*     graph_stage)
{
  for (size_t si = 0; si < array_size(state->resource->states); ++si)
  {
    ac_rg_builder_resource state2 = state->resource->states[si];
    if (state != state2)
    {
      continue;
    }

    for (size_t ui = 0; ui < array_size(state->uses); ++ui)
    {
      ac_rg_graph_resource_use* use = &state->uses[ui];
      if (use->builder_stage != builder_stage)
      {
        continue;
      }

      AC_ASSERT(!use->graph_stage);

      use->graph_stage = graph_stage;

      return (ac_rg_graph_resource_reference) {
        .resource = state->resource,
        .state = (int32_t)si,
        .use = ui,
      };
    }
  }

  AC_ASSERT(false);

  return (ac_rg_graph_resource_reference) {
    .resource = state->resource,
    .state = -1,
    .use = 0,
  };
}

static void
ac_rg_create_graph_subpass(
  ac_rg_graph_stage*         graph_stage,
  ac_rg_builder_stage        builder_stage,
  uint32_t                   index,
  ac_rg_graph_stage_subpass* subpass)
{
  *subpass = (ac_rg_graph_stage_subpass) {
    .stage_info = builder_stage->info,
    .index = index,
  };

  for (size_t i = 0; i < array_size(builder_stage->resource_states); ++i)
  {
    ac_rg_builder_resource state = builder_stage->resource_states[i];

    ac_rg_graph_resource_reference reference =
      ac_rg_graph_compute_resource_reference(state, builder_stage, graph_stage);

    ac_rg_graph_resource_use* use =
      ac_rg_graph_resource_dereference_use(reference);

    use->specifics.time.queue_index = graph_stage->queue_index;
    use->specifics.time.queue_type = graph_stage->queue_type;
    use->specifics.time.timeline = graph_stage->stage_index;

    switch (state->resource->type)
    {
    case ac_rg_resource_type_image:
    {
      if (use->usage_bits & ac_image_usage_attachment_bit)
      {
        array_append(subpass->attachments, reference);
      }
      else
      {
        array_append(subpass->images, reference);
      }
      if (reference.use == 0 && state->image_resolve_dst)
      {
        state->image_resolve_dst->uses[0].graph_stage = graph_stage;
      }
      break;
    }
    case ac_rg_resource_type_buffer:
    {
      array_append(subpass->buffers, reference);
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

static ac_result
ac_rg_create_graph_stage(
  ac_rg_builder              builder,
  ac_rg_builder_stage const* src_stages_begin,
  ac_rg_builder_stage const* src_stages_end,
  uint32_t                   stage_index,
  ac_rg_graph_stage**        dst)
{
  ac_rg_graph_stage* stage = ac_calloc(sizeof *stage);
  if (!stage)
  {
    return ac_result_out_of_host_memory;
  }

  stage->queue_type = (**src_stages_begin).info.queue;
  stage->queue_index = builder->rg->device->queue_map[stage->queue_type];

  stage->stage_index = stage_index;

  stage->is_render_pass =
    (**src_stages_begin).info.commands == ac_queue_type_graphics;

  for (ac_rg_builder_stage const* src_stage = src_stages_begin;
       src_stage < src_stages_end;
       ++src_stage)
  {
    uint32_t subpass_index = (uint32_t)(src_stage - src_stages_begin);

    ac_rg_graph_stage_subpass subpass;
    ac_rg_create_graph_subpass(stage, *src_stage, subpass_index, &subpass);

    array_append(stage->subpasses, subpass);
  }

  *dst = stage;
  return ac_result_success;
}

static ac_result
ac_rg_graph_acquire_resource(
  ac_rg_builder         builder,
  const ac_rg_timeline* available_timeline,
  ac_rg_graph_resource* resource)
{
  if (resource->buffer_or_image)
  {
    if (resource->rg_resource)
    {
      resource->import.image_layout =
        resource->rg_resource->last_use_specifics.image_layout;

      ac_rg_graph_resource_use* last_use =
        &array_back(array_back(resource->states)->uses);

      if (resource->export.is_active)
      {
        resource->export.image_layout = last_use->specifics.image_layout;
      }
    }

    return ac_result_success;
  }

  void* src_info = NULL;

  switch (resource->type)
  {
  case ac_rg_resource_type_buffer:
  {
    src_info = &resource->buffer_info;
    break;
  }
  case ac_rg_resource_type_image:
  {
    // Mark transient if image is used only inside render pass
#if AC_RG_TODO
    if (image->info.usage == ac_rg_image_usage_attachment)
    {
      ac_rg_graph_stage* stage =
        array_front(array_front(image->base.states).uses)->graph_stage;

      for (size_t i = 0; i < array_size(image->base.states); ++i)
      {
        ac_rg_builder_resource states = &image->base.states[i];
        for (size_t j = 0; j < array_size(states->uses); ++j)
        {
          ac_rg_graph_resource_use* use = states->uses[j];
          if (use->graph_stage != stage)
          {
            goto BREAK;
          }
        }
      }

      image->info.usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
    }
  BREAK:;
#endif

    src_info = &resource->image_info;
    break;
  }
  default:
  {
    AC_ASSERT(false);
    break;
  }
  }

  // TODO fill this timeline and implement aliased resource pre-barriers
  ac_rg_timeline available_timeline_inside_this_pass = *available_timeline;

  ac_rg_graph_resource_use* last_use =
    &array_back(array_back(resource->states)->uses);
  ac_rg_graph_stage* last_stage = last_use->graph_stage;

  ac_rg_time release_time = {
    .timeline = available_timeline->queue_times[last_stage->queue_index] +
                last_stage->stage_index + 1,
    .queue_type = last_stage->queue_type,
    .queue_index = last_stage->queue_index,
  };

  ac_rg_resource rg_resource = NULL;

  ac_result result = ac_rg_storage_get_resource(
    builder->rg,
    resource->type,
    src_info,
    &available_timeline_inside_this_pass,
    release_time,
    &rg_resource);
  if (rg_resource)
  {
    resource->buffer_or_image = rg_resource->buffer_or_image;
  }
  return result;
}

static void
ac_rg_fill_render_pass_info(ac_rg_graph_stage* stage)
{
  ac_rendering_info* rendering = &stage->rendering;

  for (uint32_t si = 0; si < array_size(stage->subpasses); ++si)
  {
    ac_rg_graph_stage_subpass* subpass = &stage->subpasses[si];

    rendering->color_attachment_count = 0;
    for (uint32_t ai = 0; ai < array_size(subpass->attachments); ++ai)
    {
      ac_rg_graph_resource_reference this_use = subpass->attachments[ai];

      ac_rg_graph_resource* image = this_use.resource;

      ac_attachment_info* attachment;

      if (ac_format_depth_or_stencil(image->image_info.format))
      {
        attachment = &rendering->depth_attachment;
      }
      else
      {
        AC_ASSERT(
          rendering->color_attachment_count + 1 <= AC_MAX_ATTACHMENT_COUNT);

        attachment =
          &rendering->color_attachments[rendering->color_attachment_count++];
      }

      attachment->image = image->image;

      ac_rg_builder_resource state =
        ac_rg_graph_resource_dereference_state(this_use);

      {
        if (state->image_resolve_dst)
        {
          attachment->resolve_image = state->image_resolve_dst->resource->image;
        }
      }

      if (si == 0)
      {
        attachment->load_op = ac_attachment_load_op_load;

        if (!state->is_imported && this_use.state == 0 && this_use.use == 0)
        {
          if (image->do_clear)
          {
            attachment->load_op = ac_attachment_load_op_clear;
            attachment->clear_value = image->image_info.clear_value;
          }
          else
          {
            attachment->load_op = ac_attachment_load_op_dont_care;
          }
        }
      }

      if (si == 0)
      {
        attachment->store_op = ac_attachment_store_op_none;
      }

      ac_rg_graph_resource_use* use = &state->uses[this_use.use];

      if (use->specifics.access_write.access)
      {
        if (attachment->store_op == ac_attachment_store_op_none)
        {
          attachment->store_op = ac_attachment_store_op_dont_care;
        }
      }

      if (attachment->store_op == ac_attachment_store_op_dont_care)
      {
        if (state->is_exported)
        {
          attachment->store_op = ac_attachment_store_op_store;
        }

        if (
          si == array_size(stage->subpasses) - 1 &&
          ac_rg_graph_resource_next_use(this_use).resource)
        {
          attachment->store_op = ac_attachment_store_op_store;
        }
      }
    }
  }
}

typedef struct ac_rg_dependency {
  bool execution;
  bool memory;
} ac_rg_dependency;

static inline ac_rg_dependency
ac_rg_graph_resource_use_dependnecy(
  const ac_rg_resource_use_specifics* this_use,
  const ac_rg_resource_use_specifics* next_use)
{
  const ac_rg_resource_access* this_r = &this_use->access_read;
  const ac_rg_resource_access* this_w = &this_use->access_write;
  const ac_rg_resource_access* next_w = &next_use->access_write;

  ac_rg_dependency dependency = {0};

  if (this_w->access != 0)
  {
    dependency.execution = 1;
    dependency.memory = 1;
  }
  else if (this_r->access != 0)
  {
    if (next_w->access != 0)
    {
      dependency.execution = 1;
    }
  }

  return dependency;
}

typedef struct ac_rg_barrier_info {
  ac_pipeline_stage_bits src_scope;
  ac_pipeline_stage_bits dst_scope;
  ac_access_bits         src_access;
  ac_access_bits         dst_access;
} ac_rg_barrier_info;

static ac_rg_barrier_info
ac_rg_graph_resource_barrier_info(
  const ac_rg_resource_use_specifics* this_use,
  const ac_rg_resource_use_specifics* next_use,
  bool                                layout_change)
{
  const ac_rg_resource_access* this_r = &this_use->access_read;
  const ac_rg_resource_access* this_w = &this_use->access_write;
  const ac_rg_resource_access* next_r = &next_use->access_read;
  const ac_rg_resource_access* next_w = &next_use->access_write;

  ac_rg_dependency dependency = {0};
  if (!layout_change)
  {
    dependency = ac_rg_graph_resource_use_dependnecy(this_use, next_use);
  }
  else
  {
    dependency.execution = 1;
    dependency.memory = 1;
  }

  ac_rg_barrier_info info = {0};

  if (!dependency.execution)
  {
    return info;
  }

  info.src_scope = this_r->stages | this_w->stages;
  info.dst_scope = next_r->stages | next_w->stages;

  if (dependency.memory)
  {
    // Vulkan version
    // info.src_access = this_w->access;
    // D3D12 requires to tell about reads as well
    info.src_access = this_r->access | this_w->access;
    info.dst_access = next_r->access | next_w->access;
  }

  return info;
}

typedef struct ac_rg_graph_barrier_ctx {
  ac_rg_barrier_info         sync;
  ac_rg_graph_resource*      resource;
  ac_image_layout            src_layout;
  ac_image_layout            dst_layout;
  ac_queue                   src_queue;
  ac_queue                   dst_queue;
  ac_image_subresource_range range;
} ac_rg_graph_barrier_ctx;

static void
ac_rg_graph_barrier(
  ac_rg_builder              builder,
  ac_rg_graph_stage_barrier* dst_barrier,
  ac_rg_graph_barrier_ctx*   ctx)
{
  // useless barrier
  AC_ASSERT(
    (ctx->sync.src_access && ctx->sync.dst_access) ||
    ctx->src_layout != ctx->dst_layout);

  switch (ctx->resource->type)
  {
  case ac_rg_resource_type_buffer:
  {
    ac_buffer_barrier barrier = {
      .buffer = ctx->resource->buffer,
      .src_stage = ctx->sync.src_scope,
      .dst_stage = ctx->sync.dst_scope,
      .src_access = ctx->sync.src_access,
      .dst_access = ctx->sync.dst_access,
      .src_queue = ctx->src_queue,
      .dst_queue = ctx->dst_queue,
      .offset = 0,
      .size = ctx->resource->buffer_info.size,
    };

    array_append(dst_barrier->buffers, barrier);

    if (builder->write_barrier_nodes)
    {
      array_append(dst_barrier->buffer_nodes, ctx->resource);
    }

    break;
  }
  case ac_rg_resource_type_image:
  {
    AC_ASSERT(ctx->dst_layout != ac_image_layout_undefined);

    ac_image_barrier barrier = {
      .image = ctx->resource->image,
      .src_stage = ctx->sync.src_scope,
      .dst_stage = ctx->sync.dst_scope,
      .src_access = ctx->sync.src_access,
      .dst_access = ctx->sync.dst_access,
      .old_layout = ctx->src_layout,
      .new_layout = ctx->dst_layout,
      .src_queue = ctx->src_queue,
      .dst_queue = ctx->dst_queue,
      .range = ctx->range,
    };

    array_append(dst_barrier->images, barrier);

    if (builder->write_barrier_nodes)
    {
      array_append(dst_barrier->image_nodes, ctx->resource);
    }

    break;
  }
  default:
  {
    AC_ASSERT("wrong resource type" && false);
    break;
  }
  }
}

static void
ac_rg_graph_add_stage_dependency(
  ac_rg_builder                  builder,
  ac_rg_graph_resource_reference this_use_reference)
{
  if (this_use_reference.state < 0)
  {
    return;
  }

  ac_rg_resource rg_resource = this_use_reference.resource->rg_resource;

  ac_rg_graph_resource_reference prev_use_reference =
    ac_rg_graph_resource_prev_use(this_use_reference);

  ac_rg_graph_resource_use* prev_use =
    ac_rg_graph_resource_dereference_use(prev_use_reference);

  ac_rg_resource_use_specifics* prev_specifics = &prev_use->specifics;
  if (!prev_specifics && rg_resource)
  {
    prev_specifics = &rg_resource->last_use_specifics;
  }

  if (
    !prev_specifics || (!prev_specifics->access_read.access &&
                        !prev_specifics->access_write.access))
  {
    return;
  }

  ac_rg_graph_resource_use* this_use =
    ac_rg_graph_resource_dereference_use(this_use_reference);

  ac_rg_resource_use_specifics* this_specifics = &this_use->specifics;

  uint32_t qi = prev_specifics->time.queue_index;
  if (qi == this_specifics->time.queue_index)
  {
    return;
  }

  uint64_t time = prev_specifics->time.timeline;
  if (!rg_resource)
  {
    time += builder->rg->timelines.signaling_values.queue_times[qi];
  }

  ac_rg_graph_stage* this_stage = this_use->graph_stage;

  ac_rg_graph_stage_dependency* dependency = &this_stage->dependencies[qi];
  if (dependency->timeline < time)
  {
    dependency->timeline = time;
  }

  dependency->wait_stage_bits |= this_specifics->access_read.stages;
  dependency->wait_stage_bits |= this_specifics->access_write.stages;
  AC_ASSERT(dependency->wait_stage_bits);
}

static void
ac_rg_graph_barrier_after_stage(
  ac_rg_builder                 builder,
  ac_rg_resource_use_specifics* this_use_specifics,
  ac_rg_graph_stage*            this_stage,
  ac_rg_graph_resource_use*     next_use,
  ac_rg_graph_stage*            next_stage,
  ac_rg_graph_barrier_ctx*      ctx)
{
  ctx->src_layout = this_use_specifics->image_layout;

  bool is_new_layout = ctx->src_layout != ctx->dst_layout;
  if (next_stage)
  {
    ctx->sync = ac_rg_graph_resource_barrier_info(
      this_use_specifics,
      &next_use->specifics,
      is_new_layout);
  }

  if (
    next_stage && this_stage &&
    this_stage->queue_type != next_stage->queue_type)
  {
    ctx->src_queue =
      ac_device_get_queue(builder->rg->device, this_stage->queue_type);
    ctx->dst_queue =
      ac_device_get_queue(builder->rg->device, next_stage->queue_type);

    ac_rg_barrier_info barrier_release = ctx->sync;
    barrier_release.dst_scope = ac_pipeline_stage_bottom_of_pipe_bit;
    barrier_release.dst_access = 0;

    ac_rg_barrier_info barrier_acquire = ctx->sync;
    barrier_acquire.src_scope = ac_pipeline_stage_top_of_pipe_bit;
    barrier_acquire.src_access = 0;

    ctx->sync = barrier_release;
    ac_rg_graph_barrier(builder, &this_stage->barrier_end, ctx);

    ctx->sync = barrier_acquire;
    ac_rg_graph_barrier(builder, &next_stage->barrier_beg, ctx);
  }
  else if (is_new_layout || (ctx->sync.dst_access && ctx->sync.src_access))
  {
    ac_rg_graph_stage_barrier* dst;
    if (next_stage)
    {
      dst = &next_stage->barrier_beg;
    }
    else
    {
      dst = &this_stage->barrier_end;
    }
    ac_rg_graph_barrier(builder, dst, ctx);
  }
}

static inline ac_image_subresource_range
ac_rg_graph_range_intersection(
  ac_image_subresource_range const* range1,
  ac_image_subresource_range const* range2)
{
  uint16_t layer_beg = AC_MAX(range1->base_layer, range2->base_layer);
  uint16_t layer_end = AC_MIN(
    range1->base_layer + range1->layers,
    range2->base_layer + range2->layers);

  if (layer_beg > layer_end)
  {
    layer_beg = layer_end;
  }

  uint16_t level_beg = AC_MAX(range1->base_level, range2->base_level);
  uint16_t level_end = AC_MIN(
    range1->base_level + range1->levels,
    range2->base_level + range2->levels);

  if (level_beg > level_end)
  {
    level_beg = level_end;
  }

  ac_image_subresource_range range = {
    .base_layer = layer_beg,
    .layers = layer_end - layer_beg,
    .base_level = level_beg,
    .levels = level_end - level_beg,
  };

  return range;
}

static void
ac_rg_graph_resource_sync(
  ac_rg_builder                  builder,
  ac_rg_graph_resource_reference this_use_reference)
{
  ac_rg_graph_add_stage_dependency(builder, this_use_reference);

  ac_rg_graph_resource_use* this_use =
    ac_rg_graph_resource_dereference_use(this_use_reference);

  ac_rg_graph_add_external_sync(this_use->graph_stage, this_use_reference);

  ac_rg_graph_resource_reference prev_use_reference =
    ac_rg_graph_resource_prev_use(this_use_reference);

  ac_rg_resource_use_specifics* this_specifics = &this_use->specifics;

  if (
    !prev_use_reference.resource &&
    (this_use_reference.resource->rg_resource ||
     (this_use_reference.resource->import.image_layout !=
      this_specifics->image_layout)))
  {
    // Add barrier to sync external usage or to transition from initial state
    ac_rg_graph_stage*            external_stage = NULL;
    ac_rg_resource_use_specifics* external_specifics = NULL;

    // TODO queue ownership transfer for imported resources

    ac_rg_graph_barrier_ctx ctx;
    AC_ZERO(ctx);
    ctx.resource = this_use_reference.resource;
    ctx.dst_layout = this_use->specifics.image_layout;

    ac_rg_resource_use_specifics import_specifics;

    if (ctx.resource->rg_resource)
    {
      external_specifics = &ctx.resource->rg_resource->last_use_specifics;
    }
    else
    {
      AC_ZERO(import_specifics);
      import_specifics.image_layout = ctx.resource->import.image_layout;

      external_specifics = &import_specifics;
    }

    ac_rg_graph_barrier_after_stage(
      builder,
      external_specifics,
      external_stage,
      this_use,
      this_use->graph_stage,
      &ctx);
  }

  ac_rg_graph_barrier_ctx ctx;
  AC_ZERO(ctx);
  ctx.resource = this_use_reference.resource;

  ac_rg_graph_resource_use* next_use = NULL;
  ac_rg_graph_stage*        next_stage = NULL;

  size_t   subresource_count = 0;
  uint8_t* subresource_states = NULL;

  bool allocated_levels = false;

  ac_rg_graph_resource_reference next_use_reference = this_use_reference;
  do
  {
    next_use_reference = ac_rg_graph_resource_next_use(next_use_reference);
    if (next_use_reference.resource)
    {
      next_use = ac_rg_graph_resource_dereference_use(next_use_reference);
      next_stage = next_use->graph_stage;
      ctx.dst_layout = next_use->specifics.image_layout;

      ctx.range = ac_rg_graph_calculate_range(
        &next_use->specifics.image_range,
        &ctx.resource->image_info);
    }
    else if (ctx.resource->export.is_active)
    {
      next_stage = NULL;
      ctx.sync.src_scope = this_specifics->access_write.stages;
      ctx.sync.src_access = this_specifics->access_write.access;
      ctx.dst_layout = ctx.resource->export.image_layout;

      ctx.range = ac_rg_graph_calculate_range(
        &this_specifics->image_range,
        &ctx.resource->image_info);
    }
    else
    {
      break;
    }

    bool keep_going = false;

    if (ctx.resource->type == ac_rg_resource_type_image)
    {
      ac_image_subresource_range this_range = ac_rg_graph_calculate_range(
        &this_specifics->image_range,
        &ctx.resource->image_info);

      bool layer_problem = ctx.range.base_layer != this_range.base_layer ||
                           ctx.range.layers != this_range.layers;

      bool level_problem = ctx.range.base_level != this_range.base_level ||
                           ctx.range.levels != this_range.levels;

      if (layer_problem || level_problem || subresource_states)
      {
        if (layer_problem && level_problem)
        {
          AC_RG_IMPLEMENTME("Need to solve 2D problem, too much complicated");
          return;
        }

        if (!subresource_states)
        {
          if (layer_problem)
          {
            subresource_count = this_range.layers;
            if (subresource_count == 0)
            {
              subresource_count = ctx.resource->image_info.layers;
            }
          }

          if (level_problem)
          {
            subresource_count = this_range.levels;
            if (subresource_count == 0)
            {
              subresource_count = ctx.resource->image_info.levels;
            }
          }

          subresource_states = ac_alloc((subresource_count + 7) / 8);
          memset(subresource_states, 0, (subresource_count + 7) / 8);

          allocated_levels = level_problem;
        }
        else
        {
          if (
            (layer_problem || level_problem) &&
            allocated_levels != level_problem)
          {
            AC_RG_IMPLEMENTME("Switch problem by extra barrier");
          }
        }

        ac_image_subresource_range next_range =
          ac_rg_graph_range_intersection(&this_range, &ctx.range);
        if (next_range.levels == 0 || next_range.layers == 0)
        {
          continue;
        }

        if (!allocated_levels)
        {
          ctx.range = next_range;
          ctx.range.layers = 0;

          for (uint16_t layer = next_range.base_layer;
               layer < next_range.base_layer + next_range.layers;
               ++layer)
          {
            uint32_t layer_index = (layer - this_range.base_layer);
            uint32_t index = layer_index / 8;
            uint32_t bit = 1 << layer_index % 8;
            if (subresource_states[index] & bit)
            {
              if (ctx.range.layers)
              {
                break;
              }
              ++ctx.range.base_layer;
              continue;
            }

            ++ctx.range.layers;
            subresource_states[index] |= bit;
          }

          if (ctx.range.layers == 0)
          {
            continue;
          }
        }
        else
        {
          ctx.range = next_range;
          ctx.range.levels = 0;

          for (uint16_t level = next_range.base_level;
               level < next_range.base_level + next_range.levels;
               ++level)
          {
            uint32_t level_index = (level - this_range.base_level);
            uint32_t index = level_index / 8;
            uint32_t bit = 1 << level_index % 8;
            if (subresource_states[index] & bit)
            {
              if (ctx.range.levels)
              {
                break;
              }
              ++ctx.range.base_level;
              continue;
            }

            ++ctx.range.levels;
            subresource_states[index] |= bit;
          }

          if (ctx.range.levels == 0)
          {
            continue;
          }
        }
      }

      for (uint32_t i = 0; !keep_going && i < subresource_count / 8; ++i)
      {
        uint8_t tmp = ~subresource_states[i];
        if (tmp)
        {
          keep_going = true;
        }
      }

      for (uint32_t i = 0; !keep_going && i < subresource_count % 8; ++i)
      {
        if ((~subresource_states[subresource_count / 8]) & (1 << i))
        {
          keep_going = true;
        }
      }
    }

    ac_rg_graph_barrier_after_stage(
      builder,
      &this_use->specifics,
      this_use->graph_stage,
      next_use,
      next_stage,
      &ctx);

    if (!keep_going)
    {
      break;
    }
  }
  while (next_stage);

  ac_free(subresource_states);
}

static void
ac_rg_initialize_stage_synchronisation(
  ac_rg_builder      builder,
  ac_rg_graph_stage* stage)
{
  for (size_t si = 0; si < array_size(stage->subpasses); ++si)
  {
    ac_rg_graph_stage_subpass* subpass = &stage->subpasses[si];
    for (size_t ui = 0; ui < array_size(subpass->images); ++ui)
    {
      ac_rg_graph_resource_sync(builder, subpass->images[ui]);
    }

    for (size_t ui = 0; ui < array_size(subpass->attachments); ++ui)
    {
      ac_rg_graph_resource_reference use = subpass->attachments[ui];
      ac_rg_graph_resource_sync(builder, use);

      ac_rg_builder_resource state =
        ac_rg_graph_resource_dereference_state(use);

      if (use.use == 0 && state->image_resolve_dst)
      {
        ac_rg_graph_resource_sync(
          builder,
          (ac_rg_graph_resource_reference) {
            .resource = state->image_resolve_dst->resource,
            .state = 0,
            .use = 0,
          });
      }
    }

    for (size_t ui = 0; ui < array_size(subpass->buffers); ++ui)
    {
      ac_rg_graph_resource_sync(builder, subpass->buffers[ui]);
    }
  }

  for (size_t bi = 0; bi < array_size(stage->barrier_beg.images); ++bi)
  {
    ac_image_barrier* image = &stage->barrier_beg.images[bi];
    // TODO check if Synchronisation 2 already does that
    if (image->src_stage == 0)
    {
      image->src_stage = ac_pipeline_stage_top_of_pipe_bit;
    }
    if (image->dst_stage == 0)
    {
      image->dst_stage = ac_pipeline_stage_bottom_of_pipe_bit;
    }
  }

  for (size_t bi = 0; bi < array_size(stage->barrier_end.images); ++bi)
  {
    ac_image_barrier* image = &stage->barrier_end.images[bi];

    if (image->src_stage == 0)
    {
      image->src_stage = ac_pipeline_stage_top_of_pipe_bit;
    }
    if (image->dst_stage == 0)
    {
      image->dst_stage = ac_pipeline_stage_bottom_of_pipe_bit;
    }
  }

  for (size_t bi = 0; bi < array_size(stage->barrier_beg.buffers); ++bi)
  {
    ac_buffer_barrier* buffer = &stage->barrier_beg.buffers[bi];

    if (buffer->src_stage == 0)
    {
      buffer->src_stage = ac_pipeline_stage_top_of_pipe_bit;
    }
    if (buffer->dst_stage == 0)
    {
      buffer->dst_stage = ac_pipeline_stage_bottom_of_pipe_bit;
    }
  }

  for (size_t bi = 0; bi < array_size(stage->barrier_end.buffers); ++bi)
  {
    ac_buffer_barrier* buffer = &stage->barrier_end.buffers[bi];

    if (buffer->src_stage == 0)
    {
      buffer->src_stage = ac_pipeline_stage_top_of_pipe_bit;
    }
    if (buffer->dst_stage == 0)
    {
      buffer->dst_stage = ac_pipeline_stage_bottom_of_pipe_bit;
    }
  }
}

static void
ac_rg_clear_synchronisation(ac_rg_builder builder)
{
  for (size_t qi = 0; qi < ac_queue_type_count; ++qi)
  {
    array_t(ac_rg_graph_stage*)* stages = &builder->stage_queues[qi];

    for (size_t si = 0; si < array_size(*stages); ++si)
    {
      ac_rg_graph_stage* stage = (*stages)[si];

      AC_ZERO(stage->dependencies);

      for (size_t i = 0; i < AC_COUNTOF(stage->dependencies); ++i)
      {
        stage->dependencies[i].wait_stage_bits = 0;
      }

      array_clear(stage->fences_wait);
      array_clear(stage->fences_signal);

      array_clear(stage->barrier_beg.images);
      array_clear(stage->barrier_beg.buffers);
      array_clear(stage->barrier_beg.image_nodes);
      array_clear(stage->barrier_beg.buffer_nodes);

      array_clear(stage->barrier_end.images);
      array_clear(stage->barrier_end.buffers);
      array_clear(stage->barrier_end.image_nodes);
      array_clear(stage->barrier_end.buffer_nodes);
    }
  }
}

static bool
ac_rg_check_can_merge_subpass(
  ac_rg_builder_stage stage1,
  ac_rg_builder_stage stage2)
{
  // TODO fix barriers when subpasses are merged to single stage
  if (!AC_RG_TODO)
  {
    return false;
  }

  size_t att_1_count = stage1->attachment_count;
  size_t att_2_count = stage2->attachment_count;
  if ((att_1_count == 0 && att_2_count == 0) || att_1_count != att_2_count)
  {
    return false;
  }

  for (size_t ui = 0; ui < att_1_count; ++ui)
  {
    ac_rg_builder_resource state1 = stage1->resource_states[ui];
    if (state1->image_resolve_dst)
    {
      return false;
    }

    ac_rg_builder_resource state2 = stage2->resource_states[ui];

    if (state1->resource != state2->resource)
    {
      return false;
    }
  }

  return true;
}

static ac_result
ac_rg_create_graph_stages(ac_rg_builder builder)
{
  array_t(ac_rg_builder_stage) timeline = builder->timeline;

  ac_rg_builder_stage const* pass_begin = timeline;

  size_t iend = array_size(timeline);

  for (size_t i = 0; i < iend; ++i)
  {
    if (
      i + 1 < iend &&
      ac_rg_check_can_merge_subpass(timeline[i], timeline[i + 1]))
    {
      continue;
    }

    uint32_t q_i = builder->rg->device->queue_map[(*pass_begin)->info.queue];

    array_t(ac_rg_graph_stage*)* queue = &builder->stage_queues[q_i];

    ac_rg_builder_stage* pass_end = timeline + i + 1;

    ac_rg_graph_stage* stage;

    ac_result res = ac_rg_create_graph_stage(
      builder,
      pass_begin,
      pass_end,
      (uint32_t)array_size(*queue),
      &stage);
    if (res != ac_result_success)
    {
      builder->result = res;
      ac_rg_destroy_graph_stages(builder);
      return res;
    }

    array_append(*queue, stage);

    pass_begin = pass_end;
  }

  return ac_result_success;
}

static ac_result
ac_rg_update_graph_stages(ac_rg_builder builder)
{
  for (size_t ri = 0; ri < array_size(builder->resources); ++ri)
  {
    ac_rg_graph_resource* resource = builder->resources[ri];

    ac_result res = ac_rg_graph_acquire_resource(
      builder,
      &builder->rg->timelines.signaling_values,
      resource);
    if (res != ac_result_success)
    {
      return res;
    }

    AC_ASSERT(resource->buffer_or_image);

    if (AC_INCLUDE_DEBUG)
    {
      for (size_t bri = 0; bri < ri; ++bri)
      {
        AC_ASSERT(
          builder->resources[bri]->buffer_or_image !=
          resource->buffer_or_image);
      }
    }
  }

  for (size_t qi = 0; qi < ac_queue_type_count; ++qi)
  {
    array_t(ac_rg_graph_stage*)* stages = &builder->stage_queues[qi];

    for (size_t si = 0; si < array_size(*stages); ++si)
    {
      ac_rg_graph_stage* stage = (*stages)[si];

      if (!stage->is_render_pass)
      {
        continue;
      }

      ac_rg_fill_render_pass_info(stage);
    }
  }

  ac_rg_clear_synchronisation(builder);

  for (size_t qi = 0; qi < ac_queue_type_count; ++qi)
  {
    array_t(ac_rg_graph_stage*)* stages = &builder->stage_queues[qi];

    for (size_t si = 0; si < array_size(*stages); ++si)
    {
      ac_rg_graph_stage* stage = (*stages)[si];
      ac_rg_initialize_stage_synchronisation(builder, stage);
    }
  }

  return ac_result_success;
}

static void
ac_rg_builder_clean(ac_rg_builder builder)
{
  array_clear(builder->resource_mappings);
  array_clear(builder->timeline);

  for (size_t i = 0; i < array_size(builder->stages); ++i)
  {
    ac_rg_destroy_builder_stage(builder->stages[i]);
  }
  array_clear(builder->stages);

  for (size_t i = 0; i < array_size(builder->resources); ++i)
  {
    ac_rg_destroy_resource_internal(builder->resources[i]);
  }
  array_clear(builder->resources);

  array_clear(builder->deferred_exports);
  array_clear(builder->groups);

  builder->result = ac_result_success;
}

void
ac_rg_destroy_builder(ac_rg_builder builder)
{
  ac_rg_destroy_graph_stages(builder);

  ac_rg_builder_clean(builder);

  array_free(builder->timeline);
  array_free(builder->stages);
  array_free(builder->resources);
  array_free(builder->deferred_exports);
}

ac_result
ac_rg_prepare_graph(ac_rg_builder builder)
{
  ac_rg_destroy_graph_stages(builder);
  ac_rg_builder_clean(builder);

  AC_ASSERT(builder->info.cb_build);

  ac_result res;

  res = builder->info.cb_build(builder, builder->info.user_data);
  if (res != ac_result_success)
  {
    return res;
  }

  if (builder->result != ac_result_success)
  {
    return builder->result;
  }

  res = ac_rg_builder_apply_zero_states(builder);
  if (res != ac_result_success)
  {
    return res;
  }

  ac_rg_builder_remove_unused_stages_and_resources(builder);

  res = ac_rg_builder_determine_resources_create_info(builder);
  if (res != ac_result_success)
  {
    return res;
  }

  res = ac_rg_builder_apply_export(builder);
  if (res != ac_result_success)
  {
    return res;
  }

  res = ac_rg_builder_apply_zero_states(builder);
  if (res != ac_result_success)
  {
    return res;
  }

  res = ac_rg_builder_sort_stages(builder);
  if (res != ac_result_success)
  {
    return res;
  }

  res = ac_rg_create_graph_stages(builder);
  if (res != ac_result_success)
  {
    return res;
  }

  res = ac_rg_cmd_acquire_frame(builder->rg, &builder->cmd);
  if (res != ac_result_success)
  {
    return res;
  }

  ac_rg_stage_internal cb;
  AC_ZERO(cb);
  cb.rg = builder->rg;
  cb.info.frame = builder->cmd.frame_pending;
  cb.info.device = builder->rg->device;
  for (int i = 0; i < AC_MAX_FRAME_IN_FLIGHT; ++i)
  {
    cb.info.frame_states[i] = builder->cmd.frame_states[i];
  }

  for (size_t group = 0; group < array_size(builder->groups); ++group)
  {
    builder->groups[group].prepared = false;
  }

  for (size_t qi = 0; qi < ac_queue_type_count; ++qi)
  {
    array_t(ac_rg_graph_stage*)* stages = &builder->stage_queues[qi];

    for (size_t si = 0; si < array_size(*stages); ++si)
    {
      ac_rg_graph_stage* stage = (*stages)[si];

      for (size_t spi = 0; spi < array_size(stage->subpasses); ++spi)
      {
        ac_rg_graph_stage_subpass* pass = &stage->subpasses[spi];

        ac_rg_builder_stage_info* pass_info = &pass->stage_info;

        if (pass_info->group)
        {
          ac_rg_builder_group group =
            &builder->groups[(size_t)pass_info->group - 1];

          if (group->info.cb_prepare && !group->prepared)
          {
            cb.stage = NULL;
            cb.info.metadata = 0;
            cb.pass = NULL;

            res = group->info.cb_prepare(&cb.info, group->info.user_data);
            if (res != ac_result_success)
            {
              return res;
            }

            group->prepared = true;
          }
        }

        if (pass_info->cb_prepare)
        {
          cb.stage = stage;
          cb.info.metadata = pass_info->metadata;
          cb.pass = pass;

          res = pass_info->cb_prepare(&cb.info, pass_info->user_data);
          if (res != ac_result_success)
          {
            return res;
          }
        }
      }
    }
  }

  return ac_rg_update_graph_stages(builder);
}

#endif
