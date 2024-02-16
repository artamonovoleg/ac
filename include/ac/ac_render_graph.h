#include "ac_base.h"

#if (AC_INCLUDE_RG)

#include "ac_renderer.h"

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-anonymous-struct"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

AC_DEFINE_HANDLE(ac_file);
AC_DEFINE_HANDLE(ac_rg_builder);
AC_DEFINE_HANDLE(ac_rg_builder_group);
AC_DEFINE_HANDLE(ac_rg_builder_stage);
AC_DEFINE_HANDLE(ac_rg_builder_resource);
AC_DEFINE_HANDLE(ac_rg_resource);

struct ac_rg_resource_in;
struct ac_rg_resource_out;
struct ac_rg_stage;
struct ac_rg_validation_message;

AC_DEFINE_HANDLE(ac_rg_graph);
AC_DEFINE_HANDLE(ac_rg);

typedef ac_result (*ac_rg_cb_stage)(struct ac_rg_stage*, void*);
typedef ac_result (*ac_rg_cb_build)(ac_rg_builder, void*);

typedef enum ac_rg_frame_state {
  ac_rg_frame_state_idle = 0,
  ac_rg_frame_state_pending = 1,
  ac_rg_frame_state_running = 2,
} ac_frame_state;

typedef enum ac_rg_attachment_access_bit {
  ac_rg_attachment_access_write_bit = AC_BIT(0),
  ac_rg_attachment_access_read_bit = AC_BIT(1),
} ac_rg_attachment_access_bit;
typedef uint32_t ac_rg_attachment_access_bits;

typedef enum ac_rg_import_bit {
  ac_rg_import_allow_write_bit = AC_BIT(0),
} ac_rg_import_bit;
typedef uint32_t ac_rg_import_bits;

typedef struct ac_rg_builder_group_info {
  char const*    name;
  ac_rg_cb_stage cb_prepare;
  void*          user_data;
} ac_rg_builder_group_info;

typedef struct ac_rg_builder_stage_info {
  char const*         name;
  ac_queue_type       queue;
  ac_queue_type       commands;
  ac_rg_cb_stage      cb_prepare;
  ac_rg_cb_stage      cb_cmd;
  ac_rg_cb_stage      cb_submit;
  void*               user_data;
  uint64_t            metadata;
  ac_rg_builder_group group;
} ac_rg_builder_stage_info;

typedef struct ac_rg_resource_access {
  ac_pipeline_stage_bits stages;
  ac_access_bits         access;
} ac_rg_resource_access;

typedef struct ac_rg_resource_connection {
  ac_buffer             buffer;
  ac_image              image;
  ac_image_layout       image_layout;
  ac_fence_submit_info  wait;
  ac_fence_submit_info  signal;
  ac_fence_submit_info* release;
} ac_rg_resource_connection;

typedef struct ac_rg_builder_export_resource_info {
  ac_rg_builder_resource           resource;
  ac_rg_resource                   rg_resource;
  const ac_rg_resource_connection* connection;
} ac_rg_builder_export_resource_info;

typedef struct ac_rg_builder_create_resource_info {
  ac_image_info const*             image_info;
  ac_buffer_info const*            buffer_info;
  const ac_rg_resource_connection* import_connection;
  ac_rg_resource                   import_rg_resource;
  ac_rg_import_bits                import_bits;
  bool                             do_clear;
} ac_rg_builder_create_resource_info;

typedef struct ac_rg_builder_stage_use_resource_info {
  ac_rg_builder_resource       resource;
  ac_rg_attachment_access_bits access_attachment;
  uint32_t                     usage_bits;
  ac_rg_resource_access        access_read;
  ac_rg_resource_access        access_write;
  uint64_t                     token;
  ac_image_subresource_range*  image_range;
} ac_rg_builder_stage_use_resource_info;

typedef struct ac_rg_pipeline_info {
  ac_pipeline_type type;
  union {
    struct {
      ac_shader vertex_shader;
      ac_shader pixel_shader;
    };
    ac_shader compute_shader;
  };
  ac_vertex_layout         vertex_layout;
  ac_rasterizer_state_info rasterizer_info;
  ac_primitive_topology    topology;
  ac_depth_state_info      depth_state_info;
  ac_dsl                   dsl;
  ac_channel_bits     color_attachment_discard_bits[AC_MAX_ATTACHMENT_COUNT];
  ac_blend_state_info blend_state_info;
  char const*         name;
} ac_rg_pipeline_info;

typedef struct ac_rg_stage {
  ac_device      device;
  ac_cmd         cmd;
  uint8_t        frame;
  ac_frame_state frame_states[AC_MAX_FRAME_IN_FLIGHT];
  uint64_t       metadata;
} ac_rg_stage;

typedef struct ac_rg_graph_info {
  char const*    name;
  ac_rg_cb_build cb_build;
  void*          user_data;
} ac_rg_graph_info;

typedef enum ac_rg_validation_object_type {
  ac_rg_object_type_none = 0,
  ac_rg_object_type_image = 1,
  ac_rg_object_type_buffer = 2,
  ac_rg_object_type_stage = 3,
} ac_rg_validation_object_type;

typedef enum ac_rg_validation_severity_bit {
  ac_rg_validation_severity_debug_bit = AC_BIT(0),
  ac_rg_validation_severity_info_bit = AC_BIT(1),
  ac_rg_validation_severity_warning_bit = AC_BIT(2),
  ac_rg_validation_severity_error_bit = AC_BIT(3),
} ac_rg_validation_severity_bit;
typedef uint32_t ac_rg_validation_severity_bits;

typedef enum ac_rg_validation_message_bit {
  ac_rg_validation_message_general_bit = AC_BIT(0),
  ac_rg_validation_message_validation_bit = AC_BIT(1),
  ac_rg_validation_message_performance_bit = AC_BIT(2),
} ac_rg_validation_message_bit;
typedef uint32_t ac_rg_validation_message_bits;

typedef void (*ac_rg_debug_message_function)(
  ac_rg_validation_severity_bits         severity_bits,
  ac_rg_validation_message_bits          type_bits,
  struct ac_rg_validation_message const* message,
  void*                                  user_data);

typedef struct ac_rg_validation_object_info {
  ac_rg_validation_object_type type;
  void const*                  handle;
  char const*                  name;
} ac_rg_validation_object_info;

typedef struct ac_rg_validation_message {
  char const*                         message;
  size_t                              object_count;
  ac_rg_validation_object_info const* objects;
} ac_rg_validation_message;

typedef struct ac_rg_validation_callback {
  ac_rg_validation_severity_bits severity_filter_mask;
  ac_rg_validation_message_bits  type_filter_mask;
  ac_rg_debug_message_function   function;
  void*                          user_data;
} ac_rg_validation_callback;

AC_API ac_result
ac_create_rg(ac_device device, ac_rg* rg);

AC_API void
ac_destroy_rg(ac_rg rg);

AC_API ac_result
ac_rg_create_graph(
  ac_rg                   rg,
  const ac_rg_graph_info* info,
  ac_rg_graph*            out_graph);

AC_API void
ac_rg_destroy_graph(ac_rg_graph graph);

AC_API ac_result
ac_rg_graph_execute(ac_rg_graph graph);

AC_API ac_result
ac_rg_graph_wait_idle(ac_rg_graph graph);

AC_API ac_result
ac_rg_graph_write_digraph(ac_rg_graph graph, ac_file file);

AC_API void
ac_rg_set_validation_callback(
  ac_rg                            rg,
  const ac_rg_validation_callback* callback);

AC_API ac_rg_builder_group
ac_rg_builder_create_group(
  ac_rg_builder                   builder,
  ac_rg_builder_group_info const* info);

AC_API ac_rg_builder_stage
ac_rg_builder_create_stage(
  ac_rg_builder                   builder,
  ac_rg_builder_stage_info const* info);

AC_API ac_rg_builder_resource
ac_rg_builder_create_resource(
  ac_rg_builder                             builder,
  ac_rg_builder_create_resource_info const* info);

AC_API ac_rg_builder_resource
ac_rg_builder_stage_use_resource(
  ac_rg_builder                                builder,
  ac_rg_builder_stage                          stage,
  ac_rg_builder_stage_use_resource_info const* info);

AC_API void
ac_rg_builder_export_resource(
  ac_rg_builder                             builder,
  ac_rg_builder_export_resource_info const* info);

AC_API ac_image_info
ac_rg_builder_get_image_info(ac_rg_builder_resource image);

AC_API ac_buffer_info
ac_rg_builder_get_buffer_info(ac_rg_builder_resource image);

AC_API ac_rg_builder_resource
ac_rg_builder_blit_image(
  ac_rg_builder          builder,
  ac_rg_builder_group    group,
  const char*            name,
  ac_rg_builder_resource image_src,
  ac_rg_builder_resource image_dst,
  ac_filter              filter);

AC_API ac_rg_builder_resource
ac_rg_builder_resolve_image(
  ac_rg_builder          builder,
  ac_rg_builder_resource image);

AC_API ac_image
ac_rg_stage_get_image(ac_rg_stage* stage, uint64_t token);

AC_API ac_image
ac_rg_stage_get_image_subresource(
  ac_rg_stage*                stage,
  uint64_t                    token,
  ac_image_subresource_range* out_range);

AC_API ac_buffer
ac_rg_stage_get_buffer(ac_rg_stage* stage, uint64_t token);

AC_API ac_result
ac_rg_stage_get_pipeline(
  ac_rg_stage*               stage,
  ac_rg_pipeline_info const* info,
  ac_pipeline*               out_pipeline);

AC_API ac_result
ac_rg_create_resource(
  ac_rg                 rg,
  ac_image_info const*  image_info,
  ac_buffer_info const* buffer_info,
  ac_rg_resource*       resource);

AC_API ac_image_info
ac_rg_get_image_info(ac_rg_resource resource);

AC_API ac_buffer_info
ac_rg_get_buffer_info(ac_rg_resource resource);

AC_API void*
ac_rg_get_buffer_memory(ac_rg_resource resource);

AC_API void
ac_rg_destroy_resource(ac_rg rg, ac_rg_resource resource);

#if defined(__cplusplus)
}
#endif

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif
