#include "ac_private.h"

#if (AC_INCLUDE_RG)

#include "render_graph.h"
#include "shaders/compiled/blit.h"

static uint32_t
ac_rg_group_extent_coverage(uint32_t extent, uint32_t group_size)
{
  uint32_t group = extent / group_size;
  if (group * group_size < extent)
  {
    group += 1;
  }
  return group;
}

static const char*
ac_rg_common_pass_name(ac_rg_common_pipeline_type type)
{
  const char* name = "render graph common";

  switch (type)
  {
  case ac_rg_common_pipeline_type_image_blit_compute:
    name = "render graph compute image blit";
    break;
  case ac_rg_common_pipeline_type_count:
    break;
  default:
    AC_ASSERT(false);
    break;
  }

  return name;
}

ac_result
ac_rg_create_common_passes(ac_rg_common_passes* common_passes, ac_device device)
{
  AC_ZEROP(common_passes);

  ac_result res = ac_result_success;

  for (uint32_t i = 0; i < ac_rg_common_pipeline_type_count; ++i)
  {
    const char* name = ac_rg_common_pass_name((ac_rg_common_pipeline_type)i);

    {
      ac_shader_info shader_info = {
        .name = name,
      };

      switch ((ac_rg_common_pipeline_type)i)
      {
      case ac_rg_common_pipeline_type_image_blit_compute:
        shader_info.stage = ac_shader_stage_compute;
        shader_info.code = blit_cs[0];
        break;
      case ac_rg_common_pipeline_type_count:
        break;
      default:
        AC_ASSERT(false);
        break;
      }

      res = ac_create_shader(device, &shader_info, &common_passes->shaders[i]);
    }

    if (res == ac_result_success)
    {
      ac_dsl_info dsl_info = {
        .shader_count = 1,
        .shaders = &common_passes->shaders[i],
        .name = name,
      };
      res = ac_create_dsl(device, &dsl_info, &common_passes->dummy_dsls[i]);
    }

    if (res == ac_result_success)
    {
      ac_pipeline_info info = {
        .type = ac_pipeline_type_compute,
        .name = name,
        .compute =
          {
            .shader = common_passes->shaders[i],
            .dsl = common_passes->dummy_dsls[i],
          },
      };

      res = ac_create_pipeline(device, &info, &common_passes->pipelines[i]);
    }

    if (res != ac_result_success)
    {
      break;
    }
  }

  if (res == ac_result_success)
  {
    ac_sampler_info sampler_info = {
      .mag_filter = ac_filter_nearest,
      .min_filter = ac_filter_nearest,
      .address_mode_u = ac_sampler_address_mode_clamp_to_edge,
      .address_mode_v = ac_sampler_address_mode_clamp_to_edge,
      .address_mode_w = ac_sampler_address_mode_clamp_to_edge,
    };
    res =
      ac_create_sampler(device, &sampler_info, &common_passes->sampler_nearest);
  }

  if (res == ac_result_success)
  {
    ac_sampler_info sampler_info = {
      .mag_filter = ac_filter_linear,
      .min_filter = ac_filter_linear,
      .address_mode_u = ac_sampler_address_mode_clamp_to_edge,
      .address_mode_v = ac_sampler_address_mode_clamp_to_edge,
      .address_mode_w = ac_sampler_address_mode_clamp_to_edge,
    };
    res =
      ac_create_sampler(device, &sampler_info, &common_passes->sampler_linear);
  }

  if (res != ac_result_success)
  {
    ac_rg_destroy_common_passes(common_passes);
    return res;
  }

  return ac_result_success;
}

void
ac_rg_destroy_common_passes(ac_rg_common_passes* common_passes)
{
  for (uint32_t i = 0; i < ac_rg_common_pipeline_type_count; ++i)
  {
    ac_destroy_shader(common_passes->shaders[i]);
  }

  for (uint32_t i = 0; i < ac_rg_common_pipeline_type_count; ++i)
  {
    ac_destroy_dsl(common_passes->dummy_dsls[i]);
  }

  for (uint32_t i = 0; i < ac_rg_common_pipeline_type_count; ++i)
  {
    ac_destroy_pipeline(common_passes->pipelines[i]);
  }

  ac_destroy_sampler(common_passes->sampler_nearest);
  ac_destroy_sampler(common_passes->sampler_linear);

  AC_ZEROP(common_passes);
}

static ac_result
ac_rg_common_pass_cmd(ac_rg_stage* stage, void* user_data)
{
  ac_rg_common_pass_instance* instance = user_data;

  ac_image input = ac_rg_stage_get_image(stage, 0);
  ac_image output = ac_rg_stage_get_image(stage, 1);

  ac_pipeline pipeline = instance->passes->pipelines[instance->info.pipeline];

  ac_cmd_bind_pipeline(stage->cmd, pipeline);

  {
    ac_descriptor descriptor_input = {
      .image = input,
    };

    ac_descriptor descriptor_output = {
      .image = output,
    };

    ac_sampler sampler;
    if (instance->info.blit_filter == ac_filter_nearest)
    {
      sampler = instance->passes->sampler_nearest;
    }
    else
    {
      sampler = instance->passes->sampler_linear;
    }

    ac_descriptor descriptor_sampler = {
      .sampler = sampler,
    };

    ac_descriptor_write write[] = {
      {
        .descriptors = &descriptor_input,
        .count = 1,
        .reg = 0,
        .type = ac_descriptor_type_srv_image,
      },
      {
        .descriptors = &descriptor_output,
        .count = 1,
        .reg = 0,
        .type = ac_descriptor_type_uav_image,
      },
      {
        .descriptors = &descriptor_sampler,
        .count = 1,
        .reg = 0,
        .type = ac_descriptor_type_sampler,
      },
    };

    ac_update_set(
      instance->db,
      ac_space0,
      stage->frame,
      AC_COUNTOF(write),
      write);
  }

  ac_cmd_bind_set(stage->cmd, instance->db, ac_space0, stage->frame);

  uint8_t wg[3];
  AC_RIF(ac_shader_get_workgroup(
    instance->passes->shaders[instance->info.pipeline],
    wg));

  ac_cmd_dispatch(
    stage->cmd,
    ac_rg_group_extent_coverage(ac_image_get_width(output), wg[0]),
    ac_rg_group_extent_coverage(ac_image_get_height(output), wg[1]),
    ac_rg_group_extent_coverage(1, wg[2]));

  return ac_result_success;
}

void
ac_rg_destroy_common_pass_instance(ac_rg_common_pass_instance* instance)
{
  if (!instance)
  {
    return;
  }

  ac_destroy_descriptor_buffer(instance->db);
  ac_destroy_dsl(instance->dsl);

  ac_free(instance);
}

ac_result
ac_rg_create_common_pass_instance(
  ac_device                              device,
  ac_rg_common_passes*                   passes,
  const ac_rg_common_pass_instance_info* info,
  ac_rg_common_pass_instance**           out_instance)
{
  *out_instance = NULL;

  ac_rg_common_pass_instance* instance = ac_calloc(sizeof *instance);
  if (!instance)
  {
    return ac_result_out_of_host_memory;
  }

  instance->info = *info;
  instance->passes = passes;

  ac_result res = ac_result_success;

  if (res == ac_result_success)
  {
    ac_dsl_info dsl_info = {
      .shader_count = 1,
      .shaders = &passes->shaders[info->pipeline],
      .name = ac_rg_common_pass_name(instance->info.pipeline),
    };
    res = ac_create_dsl(device, &dsl_info, &instance->dsl);
  }

  if (res == ac_result_success)
  {
    ac_descriptor_buffer_info db_info = {
      .dsl = instance->dsl,
      .max_sets[0] = AC_MAX_FRAME_IN_FLIGHT,
      .name = ac_rg_common_pass_name(instance->info.pipeline),
    };

    res = ac_create_descriptor_buffer(device, &db_info, &instance->db);
  }

  if (res != ac_result_success)
  {
    ac_rg_destroy_common_pass_instance(instance);
    return res;
  }

  *out_instance = instance;
  return ac_result_success;
}

ac_rg_builder_resource
ac_rg_common_pass_build_image_blit(
  ac_rg_builder               builder,
  ac_rg_common_pass_instance* instance,
  ac_rg_builder_group         group,
  const char*                 name,
  ac_queue_type               execute_queue,
  ac_filter                   filter,
  ac_rg_builder_resource      in_image,
  ac_rg_builder_resource      blit_dst)
{
  instance->info.blit_filter = filter;

  // TODO: while (dst_size < src_size / 2)

  if (!name)
  {
    name = ac_rg_common_pass_name(instance->info.pipeline);
  }

  ac_rg_builder_stage stage = ac_rg_builder_create_stage(
    builder,
    &(ac_rg_builder_stage_info) {
      .name = name,
      .queue = execute_queue,
      .commands = ac_queue_type_compute,
      .cb_cmd = ac_rg_common_pass_cmd,
      .user_data = instance,
      .group = group,
    });

  ac_rg_builder_stage_use_resource(
    builder,
    stage,
    &(ac_rg_builder_stage_use_resource_info) {
      .resource = in_image,
      .usage_bits = ac_image_usage_srv_bit,
      .token = 0,
    });

  return ac_rg_builder_stage_use_resource(
    builder,
    stage,
    &(ac_rg_builder_stage_use_resource_info) {
      .resource = blit_dst,
      .usage_bits = ac_image_usage_uav_bit,
      .access_write.access = ac_access_shader_write_bit,
      .token = 1,
    });
}

#endif
