#include "ac_private.h"

#if (AC_INCLUDE_RG)

#include "render_graph.h"

ac_result
ac_rg_create_storage(ac_rg_storage* s, ac_device d)
{
  AC_UNUSED(d);
  AC_ZEROP(s);
  return ac_result_success;
}

void
ac_rg_destroy_storage(ac_rg rg)
{
  for (size_t i = 0; i < array_size(rg->storage.resources); ++i)
  {
    ac_rg_destroy_resource(rg, rg->storage.resources[i]);
  }

  array_free(rg->storage.resources);

  for (size_t i = 0; i < array_size(rg->storage.common_pass_instances); ++i)
  {
    ac_rg_destroy_common_pass_instance(
      rg->storage.common_pass_instances[i].instance);
  }
  array_free(rg->storage.common_pass_instances);

  AC_ZERO(rg->storage);
}

void
ac_rg_storage_cleanup(
  ac_rg                 rg,
  const ac_rg_timeline* signaled_timeline,
  const ac_rg_timeline* signaling_timeline)
{
  uint64_t count = array_size(rg->storage.resources);
  uint64_t new_count = 0;
  for (uint64_t i = 0; i < count; ++i)
  {
    ac_rg_resource resource = rg->storage.resources[i];

    uint64_t max_use_time =
      signaling_timeline->queue_times[resource->release_time.queue_index];
    if (resource->release_time.timeline > max_use_time)
    {
      resource->release_time.timeline = max_use_time;
    }

    if (
      resource->reference_count == 1 &&
      resource->release_time.timeline <=
        signaled_timeline->queue_times[resource->release_time.queue_index])
    {
      ac_rg_destroy_resource(rg, resource);
      continue;
    }

    if (i != new_count++)
    {
      rg->storage.resources[new_count - 1] = resource;
    }
  }

  if (new_count != count)
  {
    array_resize(rg->storage.resources, new_count);
  }
}

void
ac_rg_storage_on_rebuild(ac_rg_storage* storage)
{
  // TODO timeline tracking

  for (size_t i = 0; i < array_size(storage->common_pass_instances); ++i)
  {
    ac_rg_storage_common_pass_instance* instance =
      &storage->common_pass_instances[i];

    if (instance->use_count > 0)
    {
      instance->unused_times = 0;
    }
    else
    {
      ++instance->unused_times;
    }

    if (instance->unused_times > 4)
    {
      ac_rg_destroy_common_pass_instance(instance->instance);
      array_remove(storage->common_pass_instances, i);
      --i;
    }
    else
    {
      instance->use_count = 0;
    }
  }
}

#define dcmp(x, y)                                                             \
  cmp = (int32_t)(x##1->y) - (int32_t)(x##2->y);                               \
  if (cmp != 0)                                                                \
    return cmp;                                                                \
  (void)1

static inline int32_t
ac_rg_storage_compare_buffer_info(
  const ac_buffer_info* p1,
  const ac_buffer_info* p2)
{
  int32_t cmp;
  dcmp(p, size);
  dcmp(p, usage);
  dcmp(p, memory_usage);
  return cmp;
}

static inline int32_t
ac_rg_storage_compare_image_info(
  const ac_image_info* p1,
  const ac_image_info* p2)
{
  int32_t cmp;
  dcmp(p, width);
  dcmp(p, height);
  dcmp(p, format);
  dcmp(p, samples);
  dcmp(p, layers);
  dcmp(p, levels);
  dcmp(p, usage);
  dcmp(p, type);
  return cmp;
}

static int
ac_rg_storage_compare_instance_info(
  const ac_rg_common_pass_instance_info* p1,
  const ac_rg_common_pass_instance_info* p2)
{
  int cmp;
  dcmp(p, pipeline);
  dcmp(p, commands_type);
  return cmp;
}

#undef dcmp

ac_result
ac_rg_storage_get_resource(
  ac_rg                 rg,
  ac_rg_resource_type   resource_type,
  const void*           resource_info,
  const ac_rg_timeline* acquire_time,
  ac_rg_time            release_time,
  ac_rg_resource*       out_resource)
{
  AC_ASSERT(
    acquire_time->queue_times[release_time.queue_index] <=
    release_time.timeline);

  ac_image_info const*  image_info = NULL;
  ac_buffer_info const* buffer_info = NULL;

  if (resource_type == ac_rg_resource_type_buffer)
  {
    buffer_info = resource_info;
  }
  else
  {
    image_info = resource_info;
  }

  uint64_t count = array_size(rg->storage.resources);
  for (uint64_t i = 0; i < count; ++i)
  {
    ac_rg_resource resource = rg->storage.resources[i];

    if (resource->type != resource_type)
    {
      continue;
    }

    uint64_t time =
      acquire_time->queue_times[resource->release_time.queue_index];

    if (resource->release_time.timeline > time)
    {
      continue;
    }

    switch (resource_type)
    {
    case ac_rg_resource_type_buffer:
    {
      if (ac_rg_storage_compare_buffer_info(
            &resource->buffer_info,
            buffer_info))
      {
        continue;
      }

      break;
    }
    case ac_rg_resource_type_image:
    {
      if (ac_rg_storage_compare_image_info(&resource->image_info, image_info))
      {
        continue;
      }

      break;
    }
    default:
    {
      AC_ASSERT(false);
      break;
    }
    }

    *out_resource = resource;
    resource->release_time = release_time;
    return ac_result_success;
  }

  ac_rg_resource resource = NULL;
  ac_result res = ac_rg_create_resource(rg, image_info, buffer_info, &resource);
  if (res != ac_result_success)
  {
    return res;
  }

  --resource->reference_count;

  resource->release_time = release_time;
  *out_resource = resource;
  return ac_result_success;
}

ac_result
ac_rg_storage_get_instance(
  ac_rg_storage*                         storage,
  ac_rg_common_passes*                   passes,
  ac_device                              device,
  const ac_rg_common_pass_instance_info* info,
  ac_rg_common_pass_instance**           out)
{
  uint64_t count = array_size(storage->common_pass_instances);
  for (uint64_t i = 0; i < count; ++i)
  {
    ac_rg_storage_common_pass_instance* instance =
      storage->common_pass_instances + i;

    if (instance->use_count > 0)
    {
      continue;
    }

    if (ac_rg_storage_compare_instance_info(&instance->instance->info, info))
    {
      continue;
    }

    ++instance->use_count;
    *out = instance->instance;

    return ac_result_success;
  }

  ac_rg_storage_common_pass_instance instance;
  AC_ZERO(instance);

  ac_result res =
    ac_rg_create_common_pass_instance(device, passes, info, &instance.instance);
  if (res != ac_result_success)
  {
    return res;
  }

  instance.use_count = 1;
  array_append(storage->common_pass_instances, instance);
  *out = instance.instance;

  return ac_result_success;
}

AC_API ac_result
ac_rg_create_resource(
  ac_rg                 rg,
  ac_image_info const*  image_info,
  ac_buffer_info const* buffer_info,
  ac_rg_resource*       out_resource)
{
  *out_resource = NULL;
  if ((image_info != NULL) + (buffer_info != NULL) != 1)
  {
    return ac_result_invalid_argument;
  }

  ac_rg_resource resource = ac_calloc(sizeof(*resource));
  if (!resource)
  {
    return ac_result_out_of_host_memory;
  }

  if (buffer_info)
  {
    resource->type = ac_rg_resource_type_buffer;
  }
  else
  {
    resource->type = ac_rg_resource_type_image;
  }

  resource->reference_count = 2;

  ac_result res = ac_result_success;

  switch (resource->type)
  {
  case ac_rg_resource_type_buffer:
  {
    memcpy(&resource->buffer_info, buffer_info, sizeof resource->buffer_info);

    res =
      ac_create_buffer(rg->device, &resource->buffer_info, &resource->buffer);
    break;
  }
  case ac_rg_resource_type_image:
  {
    memcpy(&resource->image_info, image_info, sizeof resource->image_info);

    res = ac_create_image(rg->device, &resource->image_info, &resource->image);
    break;
  }
  default:
  {
    AC_ASSERT(false);
    break;
  }
  }

  if (res == ac_result_success)
  {
    if (buffer_info && buffer_info->memory_usage == ac_memory_usage_cpu_to_gpu)
    {
      res = ac_buffer_map_memory(resource->buffer);
    }
  }

  if (res != ac_result_success)
  {
    ac_rg_destroy_resource(rg, resource);
    return res;
  }

  array_append(rg->storage.resources, resource);
  *out_resource = resource;
  return ac_result_success;
}

AC_API ac_image_info
ac_rg_get_image_info(ac_rg_resource resource)
{
  AC_ASSERT(!resource || resource->type == ac_rg_resource_type_image);

  if (!resource || resource->type != ac_rg_resource_type_image)
  {
    return (ac_image_info) {0};
  }

  return resource->image_info;
}

AC_API ac_buffer_info
ac_rg_get_buffer_info(ac_rg_resource resource)
{
  AC_ASSERT(!resource || resource->type == ac_rg_resource_type_buffer);

  if (!resource || resource->type != ac_rg_resource_type_buffer)
  {
    return (ac_buffer_info) {0};
  }

  return resource->buffer_info;
}

AC_API void*
ac_rg_get_buffer_memory(ac_rg_resource resource)
{
  AC_ASSERT(!resource || resource->type == ac_rg_resource_type_buffer);

  if (!resource || resource->type != ac_rg_resource_type_buffer)
  {
    return NULL;
  }

  return ac_buffer_get_mapped_memory(resource->buffer);
}

AC_API void
ac_rg_destroy_resource(ac_rg rg, ac_rg_resource resource)
{
  AC_UNUSED(rg);
  uint32_t rc = --resource->reference_count;
  if (rc > 0)
  {
    return;
  }

  switch (resource->type)
  {
  case ac_rg_resource_type_buffer:
  {
    ac_destroy_buffer(resource->buffer);
    break;
  }
  case ac_rg_resource_type_image:
  {
    ac_destroy_image(resource->image);
    break;
  }
  default:
  {
    AC_ASSERT(false);
    break;
  }
  }

  ac_free(resource);
}

#endif
