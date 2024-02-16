#pragma once

#include "ac_private.h"

#if (AC_INCLUDE_RG)

// TODO: REMOVE ME!!!
#define AC_RG_TODO 0
#define AC_RG_BARRIERS_PRINTF 0
#define AC_RG_TESTME() AC_DEBUGBREAK()
#define AC_RG_IMPLEMENTME(x)                                                   \
  AC_RG_TESTME();                                                              \
  AC_ASSERT(0)

#include "renderer/renderer.h"

#if (AC_INCLUDE_DEBUG)
#define AC_RG_MESSAGE ac_rg_message
#else
#define AC_RG_MESSAGE(...) ((void)0)
#endif

struct ac_rg_time;
struct ac_rg_timeline;

struct ac_rg_storage_buffer;
struct ac_rg_storage_image;
struct ac_rg_storage;

struct ac_rg_graph_resource;
struct ac_rg_graph_resource_use;

struct ac_rg_graph_stage_dependency;
struct ac_rg_graph_stage_barrier;
struct ac_rg_graph_stage_subpass;
struct ac_rg_graph_stage;

struct ac_rg_common_pass_instance;
struct ac_rg_common_passes;

struct ac_rg_builder_timeline_state;

struct ac_rg_validation_message_internal;

struct ac_rg_cmd_fences;
struct ac_rg_cmd_pool;
struct ac_rg_cmd;

typedef uint32_t ac_usage_bits;

typedef enum ac_rg_message_id {
  ac_rg_message_id_not_a_message = 0,
  ac_rg_message_id_info_image_samples_reduced = 1,
  ac_rg_message_id_error_null_argument = 2,
  ac_rg_message_id_error_external_use_mask_not_supported = 3,
  ac_rg_message_id_error_format_not_supported = 4,
  ac_rg_message_id_error_image_create_info_zero = 5,
  ac_rg_message_id_error_attachment_does_not_match = 6,
  ac_rg_message_id_error_image_blit_samples = 7,
  ac_rg_message_id_error_external_resource_compatibility = 8,
  ac_rg_message_id_error_resource_referenced_twice_in_stage = 9,
  ac_rg_message_id_error_cyclic_stage_dependencies = 10,
  ac_rg_message_id_error_bad_resource_usage_and_commands_type = 11,
  ac_rg_message_id_error_bad_access_for_given_usage = 12,
  ac_rg_message_id_error_duplicated_token = 13,
  ac_rg_message_id_error_exported_multiple_times = 14,
} ac_rg_message_id;

typedef enum ac_rg_common_pipeline_type {
  ac_rg_common_pipeline_type_image_blit_compute = 0,
  ac_rg_common_pipeline_type_count = 1,
} ac_rg_common_pipeline_type;

typedef enum ac_rg_resource_type {
  ac_rg_resource_type_undefined = 0,
  ac_rg_resource_type_image = 1,
  ac_rg_resource_type_buffer = 2,
} ac_rg_resource_type;

typedef struct ac_rg_time {
  uint64_t      timeline;
  ac_queue_type queue_type;
  uint32_t      queue_index;
} ac_rg_time;

typedef struct ac_rg_timeline {
  uint64_t queue_times[ac_queue_type_count];
} ac_rg_timeline;

typedef struct ac_rg_resource_use_specifics {
  ac_rg_resource_access      access_read;
  ac_rg_resource_access      access_write;
  ac_image_layout            image_layout;
  ac_image_subresource_range image_range;
  ac_rg_time                 time;
} ac_rg_resource_use_specifics;

typedef struct ac_rg_resource_internal {
  ac_rg_resource_type          type;
  uint32_t                     reference_count;
  ac_rg_resource_use_specifics last_use_specifics;
  ac_rg_time                   release_time;
  union {
    ac_buffer buffer;
    ac_image  image;
    void*     buffer_or_image;
  };
  union {
    ac_buffer_info buffer_info;
    ac_image_info  image_info;
  };
} ac_rg_resource_internal;

typedef struct ac_rg_common_pass_instance_info {
  ac_rg_common_pipeline_type pipeline;
  ac_queue_type              commands_type;
  ac_filter                  blit_filter;
} ac_rg_common_pass_instance_info;

typedef struct ac_rg_common_pass_instance {
  ac_rg_common_pass_instance_info info;
  ac_dsl                          dsl;
  ac_descriptor_buffer            db;
  struct ac_rg_common_passes*     passes;
} ac_rg_common_pass_instance;

typedef struct ac_rg_common_passes {
  ac_pipeline pipelines[ac_rg_common_pipeline_type_count];
  ac_dsl      dummy_dsls[ac_rg_common_pipeline_type_count];
  ac_shader   shaders[ac_rg_common_pipeline_type_count];
  ac_sampler  sampler_nearest;
  ac_sampler  sampler_linear;
} ac_rg_common_passes;

typedef struct ac_rg_storage_common_pass_instance {
  uint32_t                    use_count;
  uint32_t                    unused_times;
  ac_rg_common_pass_instance* instance;
} ac_rg_storage_common_pass_instance;

typedef struct ac_rg_storage {
  array_t(ac_rg_resource) resources;
  array_t(ac_rg_storage_common_pass_instance) common_pass_instances;
} ac_rg_storage;

typedef struct ac_rg_builder_timeline_state {
  uint32_t state;
  uint32_t use_count;
} ac_rg_builder_timeline_state;

typedef struct ac_rg_graph_resource_connection {
  bool                 is_active;
  ac_image_layout      image_layout;
  ac_fence_submit_info wait;
  ac_fence_submit_info signal;
} ac_rg_graph_resource_connection;

typedef struct ac_rg_graph_resource {
  const char*                  name;
  ac_rg_resource_type          type;
  ac_rg_builder_timeline_state timeline_state;
  array_t(ac_rg_builder_resource) states;
  bool do_clear;
  union {
    ac_image  image;
    ac_buffer buffer;
    void*     buffer_or_image;
  };
  union {
    ac_image_info  image_info;
    ac_buffer_info buffer_info;
  };
  bool                            used_twice_in_stage;
  ac_rg_resource                  rg_resource;
  ac_rg_graph_resource_connection import;
  ac_rg_graph_resource_connection export;
} ac_rg_graph_resource;

typedef struct ac_rg_graph_resource_use {
  ac_rg_resource_use_specifics specifics;
  ac_rg_builder_stage          builder_stage;
  ac_usage_bits                usage_bits;
  struct ac_rg_graph_stage*    graph_stage;
  uint64_t                     token;
} ac_rg_graph_resource_use;

typedef struct ac_rg_builder_resource_internal {
  bool                   is_exported;
  bool                   is_imported;
  bool                   is_read_only;
  ac_rg_graph_resource*  resource;
  ac_rg_builder_resource image_resolve_dst;
  array_t(ac_rg_graph_resource_use) uses;
} ac_rg_builder_resource_internal;

typedef struct ac_rg_graph_stage_extent {
  uint32_t width;
  uint32_t height;
  uint32_t samples;
  uint32_t layers;
} ac_rg_graph_stage_extent;

typedef struct ac_rg_graph_stage_dependency {
  uint64_t               timeline;
  ac_pipeline_stage_bits wait_stage_bits;
} ac_rg_graph_stage_dependency;

typedef struct ac_rg_graph_stage_barrier {
  array_t(ac_buffer_barrier) buffers;
  array_t(ac_image_barrier) images;
  array_t(ac_rg_graph_resource*) buffer_nodes;
  array_t(ac_rg_graph_resource*) image_nodes;
} ac_rg_graph_stage_barrier;

typedef struct ac_rg_graph_resource_reference {
  ac_rg_graph_resource* resource;
  int32_t               state;
  size_t                use;
} ac_rg_graph_resource_reference;

typedef struct ac_rg_graph_stage_subpass {
  ac_rg_builder_stage_info stage_info;
  uint32_t                 index;
  array_t(ac_rg_graph_resource_reference) attachments;
  array_t(ac_rg_graph_resource_reference) images;
  array_t(ac_rg_graph_resource_reference) buffers;
} ac_rg_graph_stage_subpass;

typedef struct ac_rg_graph_stage {
  ac_queue_type                queue_type;
  uint32_t                     queue_index;
  uint32_t                     stage_index;
  ac_rg_graph_stage_dependency dependencies[ac_queue_type_count];
  bool                         is_render_pass;
  ac_rendering_info            rendering;
  ac_rg_graph_stage_barrier    barrier_beg;
  ac_rg_graph_stage_barrier    barrier_end;
  array_t(ac_rg_graph_stage_subpass) subpasses;
  array_t(ac_fence_submit_info) fences_signal;
  array_t(ac_fence_submit_info) fences_wait;
} ac_rg_graph_stage;

typedef struct ac_rg_builder_deferred_resource_export {
  ac_rg_builder_resource    state;
  ac_rg_resource_connection connection;
  ac_rg_resource            rg_resource;
  union {
    ac_image_info  image_info;
    ac_buffer_info buffer_info;
  };
} ac_rg_builder_deferred_resource_export;

typedef struct ac_rg_builder_group_internal {
  ac_rg_builder_group_info info;
  bool                     prepared;
} ac_rg_builder_group_internal;

typedef struct ac_rg_builder_stage_internal {
  ac_rg_builder_stage_info info;
  uint32_t                 attachment_count;
  array_t(ac_rg_builder_resource) resource_states; // attachments first
  array_t(ac_rg_builder_resource) resolve_dst;
} ac_rg_builder_stage_internal;

typedef struct ac_rg_cmd_pool {
  ac_cmd_pool pool;
  ac_cmd*     cmds;
  size_t      acquired_count;
} ac_rg_cmd_pool;

typedef struct ac_rg_cmd {
  ac_rg_cmd_pool pools[AC_MAX_FRAME_IN_FLIGHT][ac_queue_type_count];
  ac_rg_timeline signaling_values[AC_MAX_FRAME_IN_FLIGHT];
  ac_frame_state frame_states[AC_MAX_FRAME_IN_FLIGHT];
  uint8_t        frame_pending;
  uint8_t        frame_running;
} ac_rg_cmd;

typedef struct ac_rg_builder_resource_mapping {
  ac_rg_resource        resource;
  ac_rg_graph_resource* history;
} ac_rg_builder_resource_mapping;

typedef struct ac_rg_builder_internal {
  ac_rg_graph_info info;
  ac_result        result;
  ac_rg            rg;
  bool             write_barrier_nodes;
  ac_rg_cmd        cmd;
  array_t(ac_rg_builder_resource_mapping) resource_mappings;
  array_t(ac_rg_graph_resource*) resources;
  array_t(ac_rg_builder_stage) stages;
  array_t(ac_rg_builder_deferred_resource_export) deferred_exports;
  array_t(ac_rg_builder_stage) timeline;
  array_t(ac_rg_graph_stage*) stage_queues[ac_queue_type_count];
  array_t(ac_rg_builder_group_internal) groups;
} ac_rg_builder_internal;

typedef struct ac_rg_stage_internal {
  ac_rg_stage                info;
  ac_rg_graph_stage_subpass* pass;
  ac_rg_graph_stage*         stage;
  ac_rg                      rg;
} ac_rg_stage_internal;

typedef struct ac_rg_validation_message_internal {
  ac_rg_message_id             id;
  const ac_rg_builder_stage    stage;
  const ac_rg_builder_resource state1;
  const ac_rg_builder_resource state2;
  const ac_rg_resource         resource;
  const char*                  null_obj_name;
  ac_rg_validation_object_type null_obj_type;
  uint64_t                     metadata;
} ac_rg_validation_message_internal;

typedef struct ac_rg_timelines {
  ac_fence       fences[ac_queue_type_count];
  ac_rg_timeline signaling_values;
  ac_rg_timeline signaled_values;
} ac_rg_timelines;

typedef struct ac_rg_pipeline {
  ac_pipeline_info info;
  ac_pipeline      pipeline;
} ac_rg_pipeline;

typedef struct ac_rg_pipelines {
  struct hashmap* hashmap;
} ac_rg_pipelines;

typedef struct ac_rg_internal {
  size_t                    graph_count;
  ac_rg_common_passes       common_passes;
  ac_rg_storage             storage;
  ac_device                 device;
  ac_rg_validation_callback callback;
  ac_rg_timelines           timelines;
  ac_rg_pipelines           pipelines;
} ac_rg_internal;

static inline uint32_t
ac_rg_state_index(const ac_rg_builder_resource state)
{
  ac_rg_graph_resource* resource = state->resource;

  for (uint32_t i = 0; i < array_size(resource->states); ++i)
  {
    if (resource->states[i] == state)
    {
      return i;
    }
  }
  AC_ASSERT(0);
  return ~0u;
}

ac_rg_graph_resource_reference
ac_rg_graph_resource_next_use(ac_rg_graph_resource_reference use);
ac_rg_graph_resource_reference
ac_rg_graph_resource_prev_use(ac_rg_graph_resource_reference use);

static inline ac_rg_builder_resource
ac_rg_graph_resource_dereference_state(ac_rg_graph_resource_reference use)
{
  if (use.state < 0)
  {
    return NULL;
  }
  return use.resource->states[use.state];
}

static inline ac_rg_graph_resource_use*
ac_rg_graph_resource_dereference_use(ac_rg_graph_resource_reference use)
{
  if (use.resource == NULL || use.state < 0)
  {
    return NULL;
  }
  AC_ASSERT((size_t)use.state < array_size(use.resource->states));
  AC_ASSERT(use.use < array_size(use.resource->states[use.state]->uses));
  return &use.resource->states[use.state]->uses[use.use];
}

static inline ac_image_subresource_range
ac_rg_graph_calculate_range(
  ac_image_subresource_range const* in,
  ac_image_info const*              info)
{
  ac_image_subresource_range range = *in;

  if (range.layers == 0)
  {
    range.base_layer = 0;
    range.layers = info->layers;
  }

  if (range.levels == 0)
  {
    range.base_level = 0;
    range.levels = info->levels;
  }

  return range;
}

ac_result
ac_rg_create_common_passes(
  ac_rg_common_passes* common_passes,
  ac_device            device);

void
ac_rg_destroy_common_passes(ac_rg_common_passes* common_passes);

ac_result
ac_rg_create_storage(ac_rg_storage* storage, ac_device device);

void
ac_rg_destroy_storage(ac_rg rg);

ac_result
ac_rg_storage_get_resource(
  ac_rg                 rg,
  ac_rg_resource_type   resource_type,
  const void*           resource_info,
  const ac_rg_timeline* acquire_time,
  ac_rg_time            release_time,
  ac_rg_resource*       out_resource);

ac_result
ac_rg_storage_get_instance(
  ac_rg_storage*                         storage,
  ac_rg_common_passes*                   passes,
  ac_device                              device,
  const ac_rg_common_pass_instance_info* info,
  ac_rg_common_pass_instance**           out);

void
ac_rg_storage_cleanup(
  ac_rg                 rg,
  const ac_rg_timeline* signaled_timline,
  const ac_rg_timeline* signaling_timeline);

void
ac_rg_storage_on_rebuild(ac_rg_storage* storage);

ac_result
ac_rg_cmd_acquire_frame(ac_rg rg, ac_rg_cmd* rg_cmd);

void
ac_rg_message(
  ac_rg_validation_callback*               callback,
  const ac_rg_validation_message_internal* info);

void
ac_rg_destroy_builder(ac_rg_builder builder);
ac_result
ac_rg_prepare_graph(ac_rg_builder builder);

void
ac_rg_print_stage_barriers(ac_rg_graph_stage* stage);

void
ac_rg_print_stage_fences(
  ac_rg_graph_stage*          stage,
  ac_rg_timelines*            timelines,
  const ac_queue_submit_info* si);

ac_result
ac_rg_create_common_pass_instance(
  ac_device                              device,
  ac_rg_common_passes*                   passes,
  const ac_rg_common_pass_instance_info* info,
  ac_rg_common_pass_instance**           out_instance);

void
ac_rg_destroy_common_pass_instance(ac_rg_common_pass_instance* instance);

ac_rg_builder_resource
ac_rg_common_pass_build_image_blit(
  ac_rg_builder               builder,
  ac_rg_common_pass_instance* instance,
  ac_rg_builder_group         group,
  const char*                 name,
  ac_queue_type               execute_queue,
  ac_filter                   filter,
  ac_rg_builder_resource      in_image,
  ac_rg_builder_resource      blit_dst);

void
ac_rg_default_validation_callback(
  ac_rg_validation_severity_bits  severity_bits,
  ac_rg_validation_message_bits   type_bits,
  const ac_rg_validation_message* message,
  void*                           user_data);

#endif
