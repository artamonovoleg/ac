#include "ac_private.h"

#if (AC_PLATFORM_APPLE)

#if !__has_feature(objc_arc)
#error AC must be built with Objective-C ARC (automatic reference counting) enabled
#endif

#include "renderer_metal.h"

static inline void
ac_mtl_cmd_debug_callback(id<MTLCommandBuffer> c)
{
  AC_OBJC_BEGIN_ARP();

  if (!c)
  {
    return;
  }

  if (c.status == MTLCommandBufferStatusCompleted)
  {
    return;
  }

  if (c.error)
  {
    if (c.error.localizedDescription)
    {
      AC_ERROR(
        "[ renderer ] [ metal ] : %s",
        [c.error.localizedDescription UTF8String]);
    }

    AC_ERROR(
      "[ renderer ] [ metal ] : domain: %s code: %s",
      c.error.domain.length ? [c.error.domain UTF8String] : "<unknown>",
      [ac_mtl_cmd_error_to_string((MTLCommandBufferError)c.error.code)
        UTF8String]);
  }

  NSArray<id<MTLCommandBufferEncoderInfo>>* infos =
    c.error.userInfo[MTLCommandBufferEncoderInfoErrorKey];

  for (uint32_t i = 0; i < infos.count; ++i)
  {
    id<MTLCommandBufferEncoderInfo> info = infos[i];
    AC_ERROR(
      "[ renderer ] [ metal ] : %s : %s",
      (info.label.length > 0u ? info.label.UTF8String
                              : "<unlabelled render pass>"),
      [ac_mtl_cmd_error_state_to_string(info.errorState) UTF8String]);

    NSString* signposts = [info.debugSignposts componentsJoinedByString:@", "];
    if (signposts.length > 0u)
    {
      AC_ERROR("[ renderer ] [ metal ] : %s", [signposts UTF8String]);
    }
  }

  AC_DEBUGBREAK();

  AC_OBJC_END_ARP();
}

static inline id<MTLCommandBuffer>
ac_mtl_create_cmd2(ac_mtl_queue* queue)
{
  AC_OBJC_BEGIN_ARP();

  MTLCommandBufferDescriptor* descriptor = [MTLCommandBufferDescriptor new];
  descriptor.retainedReferences = YES;

  if (AC_INCLUDE_DEBUG && queue->common.device->debug_bits)
  {
    descriptor.errorOptions = MTLCommandBufferErrorOptionEncoderExecutionStatus;
  }

  id<MTLCommandBuffer> cmd =
    [queue->queue commandBufferWithDescriptor:descriptor];

  if (AC_INCLUDE_DEBUG && queue->common.device->debug_bits)
  {
    [cmd addCompletedHandler:^(id<MTLCommandBuffer> c) {
      ac_mtl_cmd_debug_callback(c);
    }];
  }

  return cmd;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_get_physical_device_properties(
  VkPhysicalDevice            device_handle,
  VkPhysicalDeviceProperties* props)
{
  AC_UNUSED(device_handle);
  props->limits.bufferImageGranularity = 64;
}

static void
ac_mtl_get_physical_device_memory_properties(
  VkPhysicalDevice                  device_handle,
  VkPhysicalDeviceMemoryProperties* props)
{
  AC_FROM_HANDLE(device, ac_mtl_device);

  props->memoryHeapCount = VK_MAX_MEMORY_HEAPS;
  props->memoryTypeCount = VK_MAX_MEMORY_TYPES;

  static const uint32_t shared_heap_index = VK_MAX_MEMORY_HEAPS - 1;

  props->memoryHeaps[0].size = [device->device recommendedMaxWorkingSetSize];
  props->memoryHeaps[0].flags = VK_MEMORY_HEAP_DEVICE_LOCAL_BIT;

  if (!(TARGET_CPU_ARM64))
  {
    props->memoryHeaps[1].size = [[NSProcessInfo processInfo] physicalMemory];
  }

  props->memoryTypes[ac_memory_usage_gpu_only].heapIndex = 0;
  props->memoryTypes[ac_memory_usage_gpu_only].propertyFlags =
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

  props->memoryTypes[ac_memory_usage_gpu_to_cpu].heapIndex = shared_heap_index;
  props->memoryTypes[ac_memory_usage_gpu_to_cpu].propertyFlags =
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_CACHED_BIT;

  props->memoryTypes[ac_memory_usage_cpu_to_gpu].heapIndex = shared_heap_index;
  props->memoryTypes[ac_memory_usage_cpu_to_gpu].propertyFlags =
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
}

static VkResult
ac_mtl_allocate_memory(
  VkDevice                     device_handle,
  const VkMemoryAllocateInfo*  allocate_info,
  const VkAllocationCallbacks* allocator,
  VkDeviceMemory*              memory_handle)
{
  AC_FROM_HANDLE(device, ac_mtl_device);

  MTLResourceOptions resource_options =
    ac_memory_usage_to_mtl((ac_memory_usage)allocate_info->memoryTypeIndex);

  MTLHeapDescriptor* heap_desc = [[MTLHeapDescriptor alloc] init];
  [heap_desc setSize:allocate_info->allocationSize];
  [heap_desc setType:MTLHeapTypePlacement];
  [heap_desc setResourceOptions:resource_options];

  VkDeviceMemory_T* memory = (VkDeviceMemory_T*)allocator->pfnAllocation(
    NULL,
    sizeof(VkDeviceMemory_T),
    sizeof(void*),
    0);
  memory->heap = [device->device newHeapWithDescriptor:heap_desc];

  if (AC_INCLUDE_DEBUG)
  {
    switch (allocate_info->memoryTypeIndex)
    {
    case ac_memory_usage_gpu_only:
      [memory->heap setLabel:@("ac gpu only heap")];
      break;
    case ac_memory_usage_cpu_to_gpu:
      [memory->heap setLabel:@("ac cpu to gpu heap")];
      break;
    case ac_memory_usage_gpu_to_cpu:
      [memory->heap setLabel:@("ac gpu to cpu heap")];
      break;
    default:
      return VK_ERROR_UNKNOWN;
    }
  }

  *memory_handle = memory;
  if (!memory->heap)
  {
    AC_DEBUGBREAK();
    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
  }

  if ((device->heap_count + 1) > device->heaps_capacity)
  {
    if (!device->heaps_capacity)
    {
      device->heaps_capacity = 1;
    }
    else
    {
      device->heaps_capacity *= 2;
    }

    device->heaps =
      (__unsafe_unretained id<MTLHeap>*)allocator->pfnReallocation(
        NULL,
        device->heaps,
        device->heaps_capacity * sizeof(id<MTLHeap>),
        sizeof(void*),
        0);
  }

  device->heaps[device->heap_count++] = memory->heap;

  if (!memory)
  {
    AC_DEBUGBREAK();
    return VK_ERROR_OUT_OF_DEVICE_MEMORY;
  }

  return VK_SUCCESS;
}

static void
ac_mtl_free_memory(
  VkDevice                     device_handle,
  VkDeviceMemory               memory,
  const VkAllocationCallbacks* allocator)
{
  AC_FROM_HANDLE(device, ac_mtl_device);

  uint32_t heap_index = UINT32_MAX;
  for (uint32_t i = 0; i < device->heap_count; ++i)
  {
    if ([memory->heap isEqual:device->heaps[i]])
    {
      heap_index = i;
      break;
    }
  }

  if (heap_index == UINT32_MAX)
  {
    return;
  }

  if (heap_index != UINT32_MAX)
  {
    for (uint32_t i = heap_index + 1; i < device->heap_count; ++i)
    {
      device->heaps[i - 1] = device->heaps[i];
    }
    --device->heap_count;
  }

  [memory->heap setPurgeableState:MTLPurgeableStateEmpty];
  memory->heap = nil;
  allocator->pfnFree(NULL, memory);
}


static void* VKAPI_PTR
ac_vk_alloc_fn(
  void*                   user_data,
  size_t                  size,
  size_t                  alignment,
  VkSystemAllocationScope allocation_scope)
{
  AC_UNUSED(user_data);
  AC_UNUSED(alignment);
  AC_UNUSED(allocation_scope);
  return ac_alloc(size);
}

static void VKAPI_PTR
ac_vk_free_fn(void* user_data, void* memory)
{
  AC_UNUSED(user_data);
  ac_free(memory);
}

static void* VKAPI_PTR
ac_vk_realloc_fn(
  void*                   user_data,
  void*                   original,
  size_t                  size,
  size_t                  alignment,
  VkSystemAllocationScope allocation_scope)
{
  AC_UNUSED(user_data);
  AC_UNUSED(alignment);
  AC_UNUSED(allocation_scope);
  return ac_realloc(original, size);
}

void VKAPI_PTR
ac_vk_internal_alloc_fn(
  void*                    user_data,
  size_t                   size,
  VkInternalAllocationType allocation_type,
  VkSystemAllocationScope  allocation_scope)
{
  AC_UNUSED(user_data);
  AC_UNUSED(size);
  AC_UNUSED(allocation_type);
  AC_UNUSED(allocation_scope);
}

void VKAPI_PTR
ac_vk_internal_free_fn(
  void*                    user_data,
  size_t                   size,
  VkInternalAllocationType allocation_type,
  VkSystemAllocationScope  allocation_scope)
{
  AC_UNUSED(user_data);
  AC_UNUSED(size);
  AC_UNUSED(allocation_type);
  AC_UNUSED(allocation_scope);
}

static inline ac_result
ac_mtl_alloc(
  ac_mtl_device*  device,
  ac_memory_usage usage,
  size_t          size,
  size_t          alignment,
  VmaAllocation*  allocation)
{
  VkMemoryRequirements mem_reqs = {
    .alignment = (VkDeviceSize)alignment,
    .size = (VkDeviceSize)size,
    .memoryTypeBits = (1 << usage),
  };

  VmaAllocationCreateInfo info = {
    .flags = VMA_ALLOCATION_CREATE_STRATEGY_MIN_MEMORY_BIT |
             VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
    .usage = (VmaMemoryUsage)usage,
  };

  VkResult res = vmaAllocateMemory(
    device->gpu_allocator,
    &mem_reqs,
    &info,
    allocation,
    NULL);

  if (VK_SUCCESS != res)
  {
    AC_DEBUGBREAK();
    return ac_result_out_of_device_memory;
  }

  return ac_result_success;
}

static void
ac_mtl_destroy_device(ac_device device_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(device, ac_mtl_device);

  vmaDestroyAllocator(device->gpu_allocator);
  ac_free(device->heaps);

  for (uint32_t i = 0; i < device->common.queue_count; ++i)
  {
    if (!device->common.queues[i])
    {
      continue;
    }
    AC_FROM_HANDLE2(queue, device->common.queues[i], ac_mtl_queue);
    queue->queue = NULL;
    ac_free(queue);
  }
  device->device = NULL;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_cmd_pool(
  ac_device               device_handle,
  const ac_cmd_pool_info* info,
  ac_cmd_pool*            pool_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_INIT_INTERNAL(pool, ac_mtl_cmd_pool);
  pool->common.queue = info->queue;
  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_destroy_cmd_pool(ac_device device_handle, ac_cmd_pool pool_handle)
{
  AC_UNUSED(device_handle);
  AC_UNUSED(pool_handle);
}

static ac_result
ac_mtl_reset_cmd_pool(ac_device device_handle, ac_cmd_pool pool_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);
  AC_UNUSED(pool_handle);
  return ac_result_success;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_cmd(
  ac_device   device_handle,
  ac_cmd_pool pool_handle,
  ac_cmd*     cmd_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);
  AC_UNUSED(pool_handle);

  AC_INIT_INTERNAL(cmd, ac_mtl_cmd);

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_destroy_cmd(
  ac_device   device_handle,
  ac_cmd_pool cmd_pool_handle,
  ac_cmd      cmd_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);
  AC_UNUSED(cmd_pool_handle);

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  cmd->cmd = NULL;
  cmd->render_encoder = NULL;
  cmd->compute_encoder = NULL;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_queue_wait_idle(ac_queue queue_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(queue, ac_mtl_queue);

  id<MTLCommandBuffer> cmd = ac_mtl_create_cmd2(queue);

  [cmd commit];
  [cmd waitUntilCompleted];

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_queue_present(ac_queue queue_handle, const ac_queue_present_info* info)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(queue, ac_mtl_queue);
  AC_FROM_HANDLE2(swapchain, info->swapchain, ac_mtl_swapchain);

  id<MTLCommandBuffer> cmd = ac_mtl_create_cmd2(queue);

  [cmd presentDrawable:swapchain->drawable];
  [cmd commit];

  swapchain->drawable = NULL;

  swapchain->common.image_index =
    (swapchain->common.image_index + 1) % swapchain->common.image_count;

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_queue_submit(ac_queue queue_handle, const ac_queue_submit_info* info)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(queue_handle);

  for (uint32_t i = 0; i < info->cmd_count; ++i)
  {
    AC_FROM_HANDLE2(cmd, info->cmds[i], ac_mtl_cmd);

    for (uint32_t f = 0; f < info->wait_fence_count; ++f)
    {
      ac_fence_submit_info* fi = &info->wait_fences[f];

      AC_FROM_HANDLE2(fence, fi->fence, ac_mtl_fence);

      [cmd->cmd encodeWaitForEvent:fence->event value:fi->value];
    }

    if (i == info->cmd_count - 1)
    {
      for (uint32_t f = 0; f < info->signal_fence_count; ++f)
      {
        ac_fence_submit_info* fi = &info->signal_fences[f];

        AC_FROM_HANDLE2(fence, fi->fence, ac_mtl_fence);

        [cmd->cmd encodeSignalEvent:fence->event value:fi->value];
      }
    }

    [cmd->cmd commit];
    cmd->cmd = NULL;
  }
  return ac_result_success;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_fence(
  ac_device            device_handle,
  const ac_fence_info* info,
  ac_fence*            fence_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(info);

  AC_INIT_INTERNAL(fence, ac_mtl_fence);

  AC_FROM_HANDLE(device, ac_mtl_device);

  fence->event = [device->device newSharedEvent];
  fence->event.signaledValue = 1;

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_destroy_fence(ac_device device_handle, ac_fence fence_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_FROM_HANDLE(fence, ac_mtl_fence);

  fence->event = NULL;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_get_fence_value(
  ac_device device_handle,
  ac_fence  fence_handle,
  uint64_t* value)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_FROM_HANDLE(fence, ac_mtl_fence);

  *value = fence->event.signaledValue;

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_signal_fence(
  ac_device device_handle,
  ac_fence  fence_handle,
  uint64_t  value)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_FROM_HANDLE(fence, ac_mtl_fence);

  fence->event.signaledValue = value;

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_wait_fence(
  ac_device device_handle,
  ac_fence  fence_handle,
  uint64_t  value)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_FROM_HANDLE(fence, ac_mtl_fence);

  while (fence->event.signaledValue < value)
  {
    ac_sleep(ac_time_unit_nanoseconds, 10);
  }

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_swapchain(
  ac_device                device_handle,
  const ac_swapchain_info* info,
  ac_swapchain*            swapchain_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(device, ac_mtl_device);

  AC_INIT_INTERNAL(swapchain, ac_mtl_swapchain);

  if (!info->wsi->native_window)
  {
    return ac_result_unknown_error;
  }

#if (AC_PLATFORM_APPLE_MACOS)
  NSWindow* window = (__bridge NSWindow*)(info->wsi->native_window);
  NSView*   view = window.contentView;
  view.wantsLayer = YES;
#elif (AC_PLATFORM_APPLE_IOS)
  UIWindow* window = (__bridge UIWindow*)(info->wsi->native_window);
  UIView*   view = window.rootViewController.view;
#endif
  if (!view)
  {
    AC_ERROR("[ renderer ] [ metal ] : wsi view is NULL");
    return ac_result_unknown_error;
  }

  view.autoresizesSubviews = YES;

  swapchain->layer = (CAMetalLayer*)view.layer;
  swapchain->layer.drawableSize = CGSizeMake(info->width, info->height);

  if (!swapchain->layer)
  {
    AC_ERROR("[ renderer ] [ metal ] : CAMetalLayer is NULL");
    return ac_result_unknown_error;
  }

  ac_format   format = ac_format_b8g8r8a8_srgb;
  CFStringRef colorspace_name = kCGColorSpaceSRGB;

  swapchain->common.color_space = ac_color_space_srgb;

  if ((info->bits & ac_swapchain_wants_hdr_bit) && AC_PLATFORM_APPLE_MACOS)
  {
    swapchain->layer.wantsExtendedDynamicRangeContent = YES;
  }
  else
  {
    swapchain->layer.wantsExtendedDynamicRangeContent = NO;
  }

  CGFloat max_edr = 1.0;

#if (AC_PLATFORM_APPLE_MACOS)
  max_edr = view.window.screen.maximumExtendedDynamicRangeColorComponentValue;
#endif

  if ((info->bits & ac_swapchain_wants_hdr_bit) && max_edr > 1.0)
  {
    colorspace_name = kCGColorSpaceITUR_2100_PQ;
    swapchain->common.color_space = ac_color_space_hdr;

    format = ac_format_r10g10b10a2_unorm;
  }
  else if (info->bits & ac_swapchain_wants_unorm_format_bit)
  {
    format = ac_format_b8g8r8a8_unorm;
  }

  swapchain->layer.device = device->device;
  swapchain->layer.colorspace = CGColorSpaceCreateWithName(colorspace_name);

#if (AC_PLATFORM_APPLE_MACOS)
  swapchain->layer.displaySyncEnabled = info->vsync;
#endif
  swapchain->layer.drawsAsynchronously = YES;
  swapchain->layer.pixelFormat = ac_format_to_mtl(format);

  swapchain->common.image_count = 3;
  swapchain->common.vsync = info->vsync;
  swapchain->common.queue = info->queue;

  swapchain->common.images =
    (ac_image*)ac_calloc(swapchain->common.image_count * sizeof(ac_image));

  for (uint32_t i = 0; i < swapchain->common.image_count; ++i)
  {
    ac_image* image_handle = &swapchain->common.images[i];
    AC_INIT_INTERNAL(image, ac_mtl_image);
    image->common.width = (uint32_t)swapchain->layer.drawableSize.width;
    image->common.height = (uint32_t)swapchain->layer.drawableSize.height;
    image->common.format = format;
    image->common.samples = 1;
    image->common.levels = 1;
    image->common.layers = 1;
    image->common.usage =
      ac_image_usage_transfer_dst_bit | ac_image_usage_attachment_bit;
    image->common.type = ac_image_type_2d;
  }

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_destroy_swapchain(ac_device device_handle, ac_swapchain swapchain_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_FROM_HANDLE(swapchain, ac_mtl_swapchain);
  CGColorSpaceRelease(swapchain->layer.colorspace);
  swapchain->drawable = NULL;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_begin_cmd(ac_cmd cmd_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE2(queue, cmd->common.pool->queue, ac_mtl_queue);

  AC_ASSERT(cmd->cmd == NULL);
  AC_ASSERT(cmd->render_encoder == NULL);
  AC_ASSERT(cmd->compute_encoder == NULL);
  AC_ASSERT(cmd->index_buffer == NULL);

  cmd->cmd = ac_mtl_create_cmd2(queue);

  if (!cmd->cmd)
  {
    AC_ERROR("[ renderer ] [ metal ] : failed to allocate command buffer");
    return ac_result_out_of_host_memory;
  }

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_end_cmd(ac_cmd cmd_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);

  if (cmd->compute_encoder)
  {
    [cmd->compute_encoder endEncoding];
    cmd->compute_encoder = NULL;
  }

  if (cmd->render_encoder)
  {
    [cmd->render_encoder endEncoding];
    cmd->render_encoder = NULL;
  }
  cmd->index_buffer = NULL;

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_acquire_next_image(
  ac_device    device_handle,
  ac_swapchain swapchain_handle,
  ac_fence     fence_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);
  AC_UNUSED(fence_handle);

  AC_FROM_HANDLE(swapchain, ac_mtl_swapchain);
  AC_FROM_HANDLE2(
    image,
    swapchain_handle->images[swapchain->common.image_index],
    ac_mtl_image);

  swapchain->drawable = [swapchain->layer nextDrawable];

  if (!swapchain->drawable)
  {
    AC_ERROR("[ renderer ] [ metal ] : nextDrawable returned NULL");
    return ac_result_unknown_error;
  }
  image->texture = swapchain->drawable.texture;
  return ac_result_success;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_shader(
  ac_device             device_handle,
  const ac_shader_info* info,
  ac_shader*            shader_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_INIT_INTERNAL(shader, ac_mtl_shader);
  AC_FROM_HANDLE(device, ac_mtl_device);

  uint32_t    size = 0;
  const void* code = ac_shader_compiler_get_bytecode(
    info->code,
    &size,
    ac_shader_bytecode_metallib);

  if (code == NULL)
  {
    return ac_result_unknown_error;
  }

  NSError* __autoreleasing err = NULL;

  dispatch_data_t bytecode =
    dispatch_data_create(code, size, dispatch_get_main_queue(), NULL);

  id<MTLLibrary> lib = [device->device newLibraryWithData:bytecode error:&err];

  if (err)
  {
    AC_ERROR(
      "[ metal ]: failed to create lib: %s",
      [[err localizedDescription] UTF8String]);
    return ac_result_unknown_error;
  }

  const char* entry = ac_shader_compiler_get_entry_point_name(
    (ac_shader_compiler_stage)info->stage);
  shader->function =
    [lib newFunctionWithName:[NSString stringWithUTF8String:entry]];

  if (!shader->function)
  {
    AC_ERROR(
      "[ renderer ] [ metal ] failed to create function %s for shader: %s",
      entry,
      info->name);
    return ac_result_unknown_error;
  }

  AC_MTL_SET_OBJECT_NAME(shader->function, info->name);

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_destroy_shader(ac_device device_handle, ac_shader shader_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_FROM_HANDLE(shader, ac_mtl_shader);

  shader->function = NULL;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_dsl(
  ac_device                   device_handle,
  const ac_dsl_info_internal* info,
  ac_dsl*                     dsl_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_INIT_INTERNAL(dsl, ac_mtl_dsl);

  AC_FROM_HANDLE2(shader, info->info.shaders[0], ac_mtl_shader);
  dsl->function = shader->function;

  for (uint32_t i = 0; i < ac_space_count; ++i)
  {
    ac_mtl_dsl_space* space = &dsl->spaces[i];

    space->resource_map = hashmap_new(
      sizeof(ac_mtl_resource),
      0,
      0,
      0,
      ac_mtl_resource_hash,
      ac_mtl_resource_compare,
      NULL,
      NULL);
  }

  for (uint32_t i = 0; i < info->binding_count; ++i)
  {
    ac_shader_binding* b = &info->bindings[i];
    ac_mtl_dsl_space*  space = &dsl->spaces[b->space];

    ac_mtl_resource resource;
    AC_ZERO(resource);
    resource.reg = b->reg;
    resource.type = (ac_descriptor_type)(b->type);

    bool is_resource = b->type != ac_descriptor_type_sampler;

    if (
      b->type == ac_descriptor_type_uav_image ||
      b->type == ac_descriptor_type_uav_buffer)
    {
      resource.rw = true;
      resource.index = space->rw_resource_count;

      if (is_resource)
      {
        space->rw_resource_count += b->descriptor_count;
      }
    }
    else
    {
      resource.index = space->ro_resource_count;

      if (is_resource)
      {
        space->ro_resource_count += b->descriptor_count;
      }
    }

    space->exist = true;

    hashmap_set(space->resource_map, &resource);
  }

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_destroy_dsl(ac_device device_handle, ac_dsl dsl_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_FROM_HANDLE(dsl, ac_mtl_dsl);

  for (uint32_t i = 0; i < ac_space_count; ++i)
  {
    ac_mtl_dsl_space* space = &dsl->spaces[i];
    hashmap_free(space->resource_map);
  }

  dsl->function = NULL;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_descriptor_buffer(
  ac_device                        device_handle,
  const ac_descriptor_buffer_info* info,
  ac_descriptor_buffer*            db_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_INIT_INTERNAL(db, ac_mtl_descriptor_buffer);

  AC_FROM_HANDLE(device, ac_mtl_device);
  AC_FROM_HANDLE2(dsl, info->dsl, ac_mtl_dsl);

  uint32_t arg_buffer_size = 0;

  uint64_t alignment = device_handle->props.cbv_buffer_alignment;

  for (uint32_t i = 0; i < ac_space_count; ++i)
  {
    if (!dsl->spaces[i].exist)
    {
      continue;
    }

    id<MTLArgumentEncoder> arg_enc =
      [dsl->function newArgumentEncoderWithBufferIndex:i];

    if (!arg_enc)
    {
      return ac_result_unknown_error;
    }

    ac_mtl_db_space* space = &db->spaces[i];

    space->arg_enc = arg_enc;
    space->arg_buffer_offset = arg_buffer_size;
    arg_buffer_size +=
      AC_ALIGN_UP(arg_enc.encodedLength, alignment) * info->max_sets[i];
  }

  if (arg_buffer_size)
  {
    db->arg_buffer = [device->device newBufferWithLength:arg_buffer_size
                                                 options:0];

    if (!db->arg_buffer)
    {
      return ac_result_unknown_error;
    }

    AC_MTL_SET_OBJECT_NAME(db->arg_buffer, info->name);
  }

  for (uint32_t s = 0; s < ac_space_count; ++s)
  {
    const ac_mtl_dsl_space* in = &dsl->spaces[s];

    if (!info->max_sets[s])
    {
      continue;
    }

    ac_mtl_db_space* out = &db->spaces[s];

    if (in->ro_resource_count)
    {
      out->resources.ro_resource_count = in->ro_resource_count;
      out->resources.ro_resources =
        (__unsafe_unretained id<MTLResource>*)ac_calloc(
          sizeof(id<MTLResource>) *
          (out->resources.ro_resource_count * info->max_sets[s]));
    }

    if (in->rw_resource_count)
    {
      out->resources.rw_resource_count = in->rw_resource_count;
      out->resources.rw_resources =
        (__unsafe_unretained id<MTLResource>*)ac_calloc(
          sizeof(id<MTLResource>) *
          (out->resources.rw_resource_count * info->max_sets[s]));
    }

    out->set_count = info->max_sets[s];

    out->resources.resource_map = hashmap_new(
      sizeof(ac_mtl_resource),
      0,
      0,
      0,
      ac_mtl_resource_hash,
      ac_mtl_resource_compare,
      NULL,
      NULL);

    size_t           iter = 0;
    ac_mtl_resource* resource;
    while (hashmap_iter(in->resource_map, &iter, (void**)&resource))
    {
      hashmap_set(out->resources.resource_map, resource);
    }
  }

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_destroy_descriptor_buffer(
  ac_device            device_handle,
  ac_descriptor_buffer db_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_FROM_HANDLE(db, ac_mtl_descriptor_buffer);

  for (uint32_t s = 0; s < ac_space_count; ++s)
  {
    ac_mtl_db_space* space = &db->spaces[s];

    ac_free(space->resources.ro_resources);
    ac_free(space->resources.rw_resources);

    hashmap_free(space->resources.resource_map);
    space->arg_enc = NULL;
  }

  db->arg_buffer = NULL;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_graphics_pipeline(
  ac_device               device_handle,
  const ac_pipeline_info* info,
  ac_pipeline*            pipeline_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(device, ac_mtl_device);

  AC_INIT_INTERNAL(pipeline, ac_mtl_pipeline);

  const ac_graphics_pipeline_info* graphics = &info->graphics;

  MTLRenderPipelineDescriptor* desc = [MTLRenderPipelineDescriptor new];
  desc.rasterSampleCount = graphics->samples;
  desc.inputPrimitiveTopology = ac_mtl_determine_primitive_topology_class(
    graphics->rasterizer_info.polygon_mode,
    graphics->topology);

  {
    AC_FROM_HANDLE2(shader, graphics->vertex_shader, ac_mtl_shader);
    desc.vertexFunction = shader->function;
  }

  if (graphics->pixel_shader)
  {
    AC_FROM_HANDLE2(shader, graphics->pixel_shader, ac_mtl_shader);
    desc.fragmentFunction = shader->function;
  }

  const ac_vertex_layout* vl = &graphics->vertex_layout;

  MTLVertexBufferLayoutDescriptorArray* bindings =
    desc.vertexDescriptor.layouts;
  MTLVertexAttributeDescriptorArray* attributes =
    desc.vertexDescriptor.attributes;

  for (uint32_t i = 0; i < vl->binding_count; ++i)
  {
    const ac_vertex_binding_info* src = &vl->bindings[i];
    const uint32_t                idx = AC_MTL_MAX_BINDINGS - src->binding;
    const ac_input_rate           input_rate = src->input_rate;

    bindings[idx].stepFunction = ac_input_rate_to_mtl(input_rate);
    bindings[idx].stride = src->stride;
  }

  ac_shader_location semantic_location[ac_attribute_semantic_count];
  AC_ZERO(semantic_location);
  AC_RIF(ac_shader_compiler_get_locations(
    graphics->vertex_shader->reflection,
    semantic_location));

  for (uint32_t i = 0; i < vl->attribute_count; ++i)
  {
    const ac_vertex_attribute_info* src = &vl->attributes[i];
    const uint32_t                  loc = semantic_location[src->semantic];

    attributes[loc].format = ac_format_to_mtl_vertex_format(src->format);
    attributes[loc].offset = src->offset;
    attributes[loc].bufferIndex = AC_MTL_MAX_BINDINGS - src->binding;
  }

  const ac_blend_state_info* blend = &graphics->blend_state_info;

  for (uint32_t i = 0; i < graphics->color_attachment_count; ++i)
  {
    MTLRenderPipelineColorAttachmentDescriptor* att = desc.colorAttachments[i];

    att.pixelFormat = ac_format_to_mtl(graphics->color_attachment_formats[i]);

    const ac_blend_attachment_state* state = &blend->attachment_states[i];

    att.blendingEnabled = (state->src_factor != ac_blend_factor_zero) ||
                          (state->src_alpha_factor != ac_blend_factor_zero) ||
                          (state->dst_factor != ac_blend_factor_zero) ||
                          (state->dst_alpha_factor != ac_blend_factor_zero);
    att.sourceRGBBlendFactor = ac_blend_factor_to_mtl(state->src_factor);
    att.destinationRGBBlendFactor = ac_blend_factor_to_mtl(state->dst_factor);
    att.rgbBlendOperation = ac_blend_op_to_mtl(state->op);
    att.sourceAlphaBlendFactor =
      ac_blend_factor_to_mtl(state->src_alpha_factor);
    att.destinationAlphaBlendFactor =
      ac_blend_factor_to_mtl(state->dst_alpha_factor);
    att.alphaBlendOperation = ac_blend_op_to_mtl(state->alpha_op);

    uint32_t discard_bits = graphics->color_attachment_discard_bits[i];

    if ((discard_bits & ac_channel_r_bit) == 0)
    {
      att.writeMask |= MTLColorWriteMaskRed;
    }

    if ((discard_bits & ac_channel_g_bit) == 0)
    {
      att.writeMask |= MTLColorWriteMaskGreen;
    }

    if ((discard_bits & ac_channel_b_bit) == 0)
    {
      att.writeMask |= MTLColorWriteMaskBlue;
    }

    if ((discard_bits & ac_channel_a_bit) == 0)
    {
      att.writeMask |= MTLColorWriteMaskAlpha;
    }
  }

  desc.depthAttachmentPixelFormat =
    ac_format_to_mtl(graphics->depth_stencil_format);

  MTLDepthStencilDescriptor* depth_info = [MTLDepthStencilDescriptor new];
  if (graphics->depth_state_info.depth_test)
  {
    depth_info.depthCompareFunction =
      ac_compare_op_to_mtl(graphics->depth_state_info.compare_op);
  }
  else
  {
    depth_info.depthCompareFunction = MTLCompareFunctionAlways;
  }
  depth_info.depthWriteEnabled = graphics->depth_state_info.depth_write;

  AC_MTL_SET_OBJECT_NAME(desc, info->name);
  AC_MTL_SET_OBJECT_NAME(depth_info, info->name);

  NSError* __autoreleasing err = NULL;

  pipeline->depth_state =
    [device->device newDepthStencilStateWithDescriptor:depth_info];

  pipeline->graphics_pipeline =
    [device->device newRenderPipelineStateWithDescriptor:desc error:&err];

  if (!pipeline->depth_state)
  {
    AC_ERROR("[ metal ]: failed to create depth stencil state");
    return ac_result_unknown_error;
  }

  if (err)
  {
    AC_ERROR(
      "[ metal ]: failed to create pipeline: %s",
      [[err localizedDescription] UTF8String]);
    return ac_result_unknown_error;
  }

  if (graphics->rasterizer_info.depth_bias_enable)
  {
    pipeline->depth_bias =
      (float)graphics->rasterizer_info.depth_bias_constant_factor;
    pipeline->slope_scale = graphics->rasterizer_info.depth_bias_slope_factor;
  }

  pipeline->has_non_fragment_pc =
    ac_shader_compiler_has_push_constant(graphics->vertex_shader->reflection);

  if (graphics->pixel_shader)
  {
    pipeline->has_fragment_pc =
      ac_shader_compiler_has_push_constant(graphics->pixel_shader->reflection);
  }

  pipeline->cull_mode =
    ac_cull_mode_to_mtl(graphics->rasterizer_info.cull_mode);
  pipeline->primitive_type = ac_primitive_topology_to_mtl(graphics->topology);
  pipeline->winding =
    ac_front_face_to_mtl(graphics->rasterizer_info.front_face);

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_compute_pipeline(
  ac_device               device_handle,
  const ac_pipeline_info* info,
  ac_pipeline*            pipeline_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(device, ac_mtl_device);
  AC_INIT_INTERNAL(pipeline, ac_mtl_pipeline);

  ac_shader_workgroup wg;
  AC_RIF(
    ac_shader_compiler_get_workgroup(info->compute.shader->reflection, &wg));

  pipeline->workgroup = MTLSizeMake(wg.x, wg.y, wg.z);

  const ac_compute_pipeline_info* compute = &info->compute;

  MTLComputePipelineDescriptor* desc = [MTLComputePipelineDescriptor new];

  {
    AC_FROM_HANDLE2(shader, compute->shader, ac_mtl_shader);
    desc.computeFunction = shader->function;
  }

  AC_MTL_SET_OBJECT_NAME(desc, info->name);

  NSError* __autoreleasing err = NULL;
  pipeline->compute_pipeline =
    [device->device newComputePipelineStateWithDescriptor:desc
                                                  options:MTLPipelineOptionNone
                                               reflection:NULL
                                                    error:&err];

  if (err)
  {
    AC_ERROR(
      "[ metal ]: failed to create pipeline: %s",
      [[err localizedDescription] UTF8String]);
    return ac_result_unknown_error;
  }

  pipeline->has_non_fragment_pc =
    ac_shader_compiler_has_push_constant(compute->shader->reflection);

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_destroy_pipeline(ac_device device_handle, ac_pipeline pipeline_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_FROM_HANDLE(pipeline, ac_mtl_pipeline);
  switch (pipeline->common.type)
  {
  case ac_pipeline_type_graphics:
  case ac_pipeline_type_mesh:
    pipeline->depth_state = NULL;
    pipeline->graphics_pipeline = NULL;
    break;
  default:
    pipeline->compute_pipeline = NULL;
    break;
  }

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_buffer(
  ac_device             device_handle,
  const ac_buffer_info* info,
  ac_buffer*            buffer_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_INIT_INTERNAL(buffer, ac_mtl_buffer);
  AC_FROM_HANDLE(device, ac_mtl_device);

  MTLResourceOptions resource_options =
    ac_memory_usage_to_mtl(info->memory_usage);

  MTLSizeAndAlign size_align =
    [device->device heapBufferSizeAndAlignWithLength:info->size
                                             options:resource_options];

  AC_RIF(ac_mtl_alloc(
    device,
    info->memory_usage,
    size_align.size,
    size_align.align,
    &buffer->allocation));

  VmaAllocationInfo alloc_info;
  vmaGetAllocationInfo(device->gpu_allocator, buffer->allocation, &alloc_info);

  buffer->buffer =
    [alloc_info.deviceMemory->heap newBufferWithLength:info->size
                                               options:resource_options
                                                offset:alloc_info.offset];

  if (!buffer->buffer)
  {
    return ac_result_unknown_error;
  }

  AC_MTL_SET_OBJECT_NAME(buffer->buffer, info->name);

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_destroy_buffer(ac_device device_handle, ac_buffer buffer_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(device, ac_mtl_device);

  AC_FROM_HANDLE(buffer, ac_mtl_buffer);

  vmaFreeMemory(device->gpu_allocator, buffer->allocation);
  buffer->buffer = NULL;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_sampler(
  ac_device              device_handle,
  const ac_sampler_info* info,
  ac_sampler*            sampler_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_INIT_INTERNAL(sampler, ac_mtl_sampler);
  AC_FROM_HANDLE(device, ac_mtl_device);

  MTLSamplerDescriptor* desc = [MTLSamplerDescriptor new];
  desc.normalizedCoordinates = YES;
  desc.supportArgumentBuffers = YES;
  desc.sAddressMode = ac_sampler_address_mode_to_mtl(info->address_mode_u);
  desc.tAddressMode = ac_sampler_address_mode_to_mtl(info->address_mode_v);
  desc.rAddressMode = ac_sampler_address_mode_to_mtl(info->address_mode_w);
  desc.minFilter = ac_filter_to_mtl(info->min_filter);
  desc.magFilter = ac_filter_to_mtl(info->mag_filter);
  if (info->max_lod > 1)
  {
    desc.mipFilter = ac_sampler_mipmap_mode_to_mtl(info->mipmap_mode);
  }
  else
  {
    desc.mipFilter = MTLSamplerMipFilterNotMipmapped;
  }
  desc.lodMinClamp = info->min_lod;
  desc.lodMaxClamp = info->max_lod;
  if (info->anisotropy_enable)
  {
    desc.maxAnisotropy = AC_CLAMP(info->max_anisotropy, 1, 16);
  }
  else
  {
    desc.maxAnisotropy = 1;
  }

  if (info->compare_enable)
  {
    desc.compareFunction = ac_compare_op_to_mtl(info->compare_op);
  }
  else
  {
    desc.compareFunction = MTLCompareFunctionAlways;
  }

  sampler->sampler = [device->device newSamplerStateWithDescriptor:desc];

  if (!sampler->sampler)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_destroy_sampler(ac_device device_handle, ac_sampler sampler_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_FROM_HANDLE(sampler, ac_mtl_sampler);
  sampler->sampler = NULL;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_image(
  ac_device            device_handle,
  const ac_image_info* info,
  ac_image*            p)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(device, ac_mtl_device);

  size_t size = sizeof(ac_mtl_image);
  if (info->usage & ac_image_usage_uav_bit)
  {
    size += (info->levels * sizeof(id<MTLTexture>));
  }

  ac_mtl_image* image = ac_calloc(size);
  void*         mem = image + 1;
  image->mips = (id<MTLTexture> __strong*)(mem);
  *p = &image->common;

  MTLTextureDescriptor* desc = [MTLTextureDescriptor new];
  desc.width = info->width;
  desc.height = info->height;
  desc.depth = 1;
  desc.mipmapLevelCount = info->levels;
  desc.sampleCount = info->samples;
  desc.arrayLength = info->layers;
  desc.storageMode = MTLStorageModePrivate;
  desc.cpuCacheMode = MTLCPUCacheModeDefaultCache;
  desc.resourceOptions = MTLResourceStorageModePrivate;
  desc.pixelFormat = ac_format_to_mtl(info->format);
  desc.usage = ac_image_usage_bits_to_mtl(info->usage);

  switch (info->type)
  {
  case ac_image_type_1d:
    desc.textureType = MTLTextureType1D;
    break;
  case ac_image_type_1d_array:
    desc.textureType = MTLTextureType1DArray;
    break;
  case ac_image_type_cube:
    AC_ASSERT(info->layers == 6);
    desc.arrayLength = 1;
    desc.textureType = MTLTextureTypeCube;
    break;
  case ac_image_type_2d:
    desc.textureType = MTLTextureType2D;
    if (info->samples > 1)
    {
      desc.textureType = MTLTextureType2DMultisample;
    }
    break;
  case ac_image_type_2d_array:
    desc.textureType = MTLTextureType2DArray;
    break;
  default:
    AC_ASSERT(false);
    return ac_result_invalid_argument;
  }

  MTLSizeAndAlign size_align =
    [device->device heapTextureSizeAndAlignWithDescriptor:desc];

  AC_RIF(ac_mtl_alloc(
    device,
    ac_memory_usage_gpu_only,
    size_align.size,
    size_align.align,
    &image->allocation));

  VmaAllocationInfo alloc_info;
  vmaGetAllocationInfo(device->gpu_allocator, image->allocation, &alloc_info);

  image->texture =
    [alloc_info.deviceMemory->heap newTextureWithDescriptor:desc
                                                     offset:alloc_info.offset];

  if (!image->texture)
  {
    return ac_result_unknown_error;
  }

  if (info->usage & ac_image_usage_uav_bit)
  {
    NSRange        slices = NSMakeRange(0, info->layers);
    MTLTextureType tex_type = image->texture.textureType;
    if (tex_type == MTLTextureTypeCube)
    {
      tex_type = MTLTextureType2DArray;
    }

    for (uint32_t i = 0; i < info->levels; ++i)
    {
      NSRange levels = NSMakeRange(i, 1);
      image->mips[i] =
        [image->texture newTextureViewWithPixelFormat:image->texture.pixelFormat
                                          textureType:tex_type
                                               levels:levels
                                               slices:slices];

      if (!image->mips[i])
      {
        return ac_result_unknown_error;
      }
    }
  }

  AC_MTL_SET_OBJECT_NAME(image->texture, info->name);

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_destroy_image(ac_device device_handle, ac_image image_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(device, ac_mtl_device);
  AC_FROM_HANDLE(image, ac_mtl_image);

  vmaFreeMemory(device->gpu_allocator, image->allocation);

  if (image->common.usage & ac_image_usage_uav_bit)
  {
    for (uint32_t i = 0; i < image->common.levels; ++i)
    {
      image->mips[i] = NULL;
    }
  }
  image->texture = NULL;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_update_descriptor(
  ac_device                  device_handle,
  ac_descriptor_buffer       db_handle,
  ac_space                   space_index,
  uint32_t                   set,
  const ac_descriptor_write* write)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(db, ac_mtl_descriptor_buffer);

  ac_mtl_db_space* space = &db->spaces[space_index];

  uint64_t alignment = device_handle->props.cbv_buffer_alignment;

  if (db->arg_buffer)
  {
    [space->arg_enc
      setArgumentBuffer:db->arg_buffer
                 offset:space->arg_buffer_offset +
                        (AC_ALIGN_UP(space->arg_enc.encodedLength, alignment) *
                         set)];
  }

  ac_mtl_resource* resource =
    ac_mtl_update_resources(&space->resources, set, write);

  if (!resource)
  {
    AC_ERROR("TODO: RETURN AC_RESULT FROM HERE!!!!");
    return;
  }

  for (uint32_t j = 0; j < write->count; ++j)
  {
    ac_descriptor* descriptor = &write->descriptors[j];

    uint32_t index = ac_shader_compiler_get_shader_binding_index(
                       (ac_shader_descriptor_type)write->type,
                       write->reg) +
                     j;

    switch (write->type)
    {
    case ac_descriptor_type_uav_buffer:
    case ac_descriptor_type_cbv_buffer:
    case ac_descriptor_type_srv_buffer:
    {
      AC_FROM_HANDLE2(buffer, descriptor->buffer, ac_mtl_buffer);
      [space->arg_enc setBuffer:buffer->buffer
                         offset:descriptor->offset
                        atIndex:index];
      break;
    }
    case ac_descriptor_type_sampler:
    {
      AC_FROM_HANDLE2(sampler, descriptor->sampler, ac_mtl_sampler);
      [space->arg_enc setSamplerState:sampler->sampler atIndex:index];
      break;
    }
    case ac_descriptor_type_uav_image:
    {
      AC_FROM_HANDLE2(image, descriptor->image, ac_mtl_image);
      [space->arg_enc setTexture:image->mips[descriptor->level] atIndex:index];
      break;
    }
    case ac_descriptor_type_srv_image:
    {
      AC_FROM_HANDLE2(image, descriptor->image, ac_mtl_image);
      [space->arg_enc setTexture:image->texture atIndex:index];
      break;
    }
    case ac_descriptor_type_as:
    {
      AC_FROM_HANDLE2(as, descriptor->as, ac_mtl_as);
      [space->arg_enc setAccelerationStructure:as->as atIndex:index];
      break;
    }
    default:
    {
      AC_ASSERT(false);
      break;
    }
    }
  }

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_blas(
  ac_device         device_handle,
  const ac_as_info* info,
  ac_as*            as_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_INIT_INTERNAL(as, ac_mtl_as);

  AC_FROM_HANDLE(device, ac_mtl_device);

  NSMutableArray<MTLAccelerationStructureGeometryDescriptor*>* geometries =
    [NSMutableArray array];

  for (uint32_t i = 0; i < info->count; ++i)
  {
    switch (info->geometries[i].type)
    {
    case ac_geometry_type_triangles:
    {
      const ac_as_geometry_triangles* src = &info->geometries[i].triangles;

      MTLAccelerationStructureTriangleGeometryDescriptor* dst =
        [MTLAccelerationStructureTriangleGeometryDescriptor descriptor];

      AC_FROM_HANDLE2(vertex_buffer, src->vertex.buffer, ac_mtl_buffer);
      AC_FROM_HANDLE2(index_buffer, src->index.buffer, ac_mtl_buffer);

      dst.vertexBuffer = vertex_buffer->buffer;
      dst.vertexBufferOffset = src->vertex.offset;
      dst.vertexFormat = ac_format_to_mtl_attribute_format(src->vertex_format);
      dst.vertexStride = src->vertex_stride;
      dst.indexBuffer = index_buffer->buffer;
      dst.indexBufferOffset = src->index.offset;
      dst.indexType = src->index_type == ac_index_type_u32 ? MTLIndexTypeUInt32
                                                           : MTLIndexTypeUInt16;
      dst.triangleCount = src->index_count / 3;
      if (src->transform.buffer)
      {
        AC_FROM_HANDLE2(transform_buffer, src->transform.buffer, ac_mtl_buffer);
        dst.transformationMatrixBuffer = transform_buffer->buffer;
        dst.transformationMatrixBufferOffset = src->transform.offset;
      }

      [geometries addObject:dst];
      break;
    }
    case ac_geometry_type_aabbs:
    {
      const ac_as_geometry_aabs* src = &info->geometries[i].aabs;

      MTLAccelerationStructureBoundingBoxGeometryDescriptor* dst =
        [MTLAccelerationStructureBoundingBoxGeometryDescriptor descriptor];

      AC_FROM_HANDLE2(aabs_buffer, src->buffer, ac_mtl_buffer);
      dst.boundingBoxBuffer = aabs_buffer->buffer;
      dst.boundingBoxBufferOffset = src->offset;
      dst.boundingBoxStride = src->stride;
      dst.boundingBoxCount =
        (aabs_buffer->common.size - src->offset) / src->stride;

      [geometries addObject:dst];

      break;
    }
    default:
    {
      return ac_result_invalid_argument;
    }
    }
  }

  MTLPrimitiveAccelerationStructureDescriptor* descriptor =
    [MTLPrimitiveAccelerationStructureDescriptor new];
  descriptor.geometryDescriptors = geometries;

  MTLAccelerationStructureSizes sizes =
    [device->device accelerationStructureSizesWithDescriptor:descriptor];

  MTLSizeAndAlign size_align = [device->device
    heapAccelerationStructureSizeAndAlignWithSize:sizes
                                                    .accelerationStructureSize];

  AC_RIF(ac_mtl_alloc(
    device,
    ac_memory_usage_gpu_only,
    size_align.size,
    size_align.align,
    &as->allocation));

  VmaAllocationInfo alloc_info;
  vmaGetAllocationInfo(device->gpu_allocator, as->allocation, &alloc_info);

  as->descriptor = descriptor;
  as->as = [alloc_info.deviceMemory->heap
    newAccelerationStructureWithSize:sizes.accelerationStructureSize
                              offset:alloc_info.offset];

  if (!as->as)
  {
    return ac_result_out_of_device_memory;
  }

  as->common.scratch_size = sizes.buildScratchBufferSize;

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_create_tlas(
  ac_device         device_handle,
  const ac_as_info* info,
  ac_as*            as_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_INIT_INTERNAL(as, ac_mtl_as);

  AC_FROM_HANDLE(device, ac_mtl_device);

  AC_FROM_HANDLE2(buffer, info->instances_buffer, ac_mtl_buffer);

  NSMutableArray<id<MTLAccelerationStructure>>* acceleration_structures =
    [NSMutableArray array];

  for (uint32_t i = 0; i < info->count; ++i)
  {
    const ac_as_instance* src = &info->instances[i];

    AC_FROM_HANDLE2(blas, src->as, ac_mtl_as);
    [acceleration_structures addObject:blas->as];
  }

  MTLInstanceAccelerationStructureDescriptor* descriptor =
    [MTLInstanceAccelerationStructureDescriptor new];
  descriptor.instanceDescriptorBuffer = buffer->buffer;
  descriptor.instanceDescriptorBufferOffset = info->instances_buffer_offset;
  descriptor.instanceCount = info->count;
  descriptor.instanceDescriptorType =
    MTLAccelerationStructureInstanceDescriptorTypeDefault;
  descriptor.instanceDescriptorStride =
    sizeof(MTLAccelerationStructureUserIDInstanceDescriptor);
  descriptor.instancedAccelerationStructures = acceleration_structures;

  as->descriptor = descriptor;

  MTLAccelerationStructureSizes sizes =
    [device->device accelerationStructureSizesWithDescriptor:descriptor];

  MTLSizeAndAlign size_align = [device->device
    heapAccelerationStructureSizeAndAlignWithSize:sizes
                                                    .accelerationStructureSize];

  AC_RIF(ac_mtl_alloc(
    device,
    ac_memory_usage_gpu_only,
    size_align.size,
    size_align.align,
    &as->allocation));

  VmaAllocationInfo alloc_info;
  vmaGetAllocationInfo(device->gpu_allocator, as->allocation, &alloc_info);

  as->as = [alloc_info.deviceMemory->heap
    newAccelerationStructureWithSize:sizes.accelerationStructureSize
                              offset:alloc_info.offset];

  if (!as->as)
  {
    return ac_result_out_of_device_memory;
  }

  as->common.scratch_size = sizes.buildScratchBufferSize;

  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_destroy_as(ac_device device_handle, ac_as as_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(device, ac_mtl_device);
  AC_FROM_HANDLE(as, ac_mtl_as);

  vmaFreeMemory(device->gpu_allocator, as->allocation);

  as->as = NULL;
  as->descriptor = NULL;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_write_as_instances(
  ac_device             device_handle,
  uint32_t              count,
  const ac_as_instance* instances,
  void*                 mem)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  MTLAccelerationStructureUserIDInstanceDescriptor* dst_instances = mem;

  for (uint32_t i = 0; i < count; ++i)
  {
    const ac_as_instance*                             src = &instances[i];
    MTLAccelerationStructureUserIDInstanceDescriptor* dst = &dst_instances[i];

    for (uint32_t r = 0; r < 3; ++r)
    {
      for (uint32_t c = 0; c < 4; ++c)
      {
        dst->transformationMatrix.columns[c].elements[r] =
          src->transform.matrix[r][c];
      }
    }

    dst->userID = src->instance_index;
    dst->mask = src->mask;
    dst->intersectionFunctionTableOffset = src->instance_sbt_offset;
    dst->options = ac_as_instance_bits_to_mtl(src->bits);
    dst->accelerationStructureIndex = i;
  }

  AC_OBJC_END_ARP();
}

static ac_result
ac_mtl_map_memory(ac_device device_handle, ac_buffer buffer_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_FROM_HANDLE(buffer, ac_mtl_buffer);
  buffer->common.mapped_memory = buffer->buffer.contents;
  if (!buffer->common.mapped_memory)
  {
    return ac_result_unknown_error;
  }
  return ac_result_success;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_unmap_memory(ac_device device_handle, ac_buffer buffer_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  AC_FROM_HANDLE(buffer, ac_mtl_buffer);
  buffer->common.mapped_memory = NULL;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_begin_rendering(ac_cmd cmd_handle, const ac_rendering_info* info)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);

  if (cmd->compute_encoder)
  {
    [cmd->compute_encoder endEncoding];
    cmd->compute_encoder = NULL;
  }

  MTLRenderPassDescriptor* pass_info = [MTLRenderPassDescriptor new];
  for (uint32_t i = 0; i < info->color_attachment_count; ++i)
  {
    const ac_attachment_info* att = &info->color_attachments[i];
    const float*              c = att->clear_value.color;
    AC_FROM_HANDLE2(image, att->image, ac_mtl_image);
    pass_info.colorAttachments[i].texture = image->texture;
    pass_info.colorAttachments[i].loadAction =
      ac_attachment_load_op_to_mtl(att->load_op);
    pass_info.colorAttachments[i].clearColor =
      MTLClearColorMake((double)c[0], (double)c[1], (double)c[2], (double)c[3]);
    pass_info.colorAttachments[i].storeAction =
      ac_attachment_store_op_to_mtl(att->store_op);

    if (att->resolve_image)
    {
      AC_FROM_HANDLE2(resolve_image, att->resolve_image, ac_mtl_image);
      pass_info.colorAttachments[i].storeAction =
        MTLStoreActionMultisampleResolve;
      pass_info.colorAttachments[i].resolveTexture = resolve_image->texture;
    }
  }

  if (info->depth_attachment.image)
  {
    const ac_attachment_info* att = &info->depth_attachment;
    const ac_clear_value*     v = &info->depth_attachment.clear_value;
    AC_FROM_HANDLE2(image, att->image, ac_mtl_image);
    pass_info.depthAttachment.texture = image->texture;
    pass_info.depthAttachment.clearDepth = (double)v->depth;
    pass_info.depthAttachment.loadAction =
      ac_attachment_load_op_to_mtl(att->load_op);
    pass_info.depthAttachment.storeAction =
      ac_attachment_store_op_to_mtl(att->store_op);

    if (att->resolve_image)
    {
      AC_FROM_HANDLE2(resolve_image, att->resolve_image, ac_mtl_image);
      pass_info.depthAttachment.storeAction = MTLStoreActionMultisampleResolve;
      pass_info.depthAttachment.resolveTexture = resolve_image->texture;
      pass_info.depthAttachment.depthResolveFilter =
        MTLMultisampleDepthResolveFilterMin;
    }
  }

  AC_ASSERT(cmd->render_encoder == NULL);
  cmd->render_encoder = [cmd->cmd renderCommandEncoderWithDescriptor:pass_info];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_end_rendering(ac_cmd cmd_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  [cmd->render_encoder endEncoding];
  cmd->render_encoder = NULL;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_barrier(
  ac_cmd                   cmd_handle,
  uint32_t                 buffer_barrier_count,
  const ac_buffer_barrier* buffer_barriers,
  uint32_t                 image_barrier_count,
  const ac_image_barrier*  image_barriers)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(cmd_handle);
  AC_UNUSED(buffer_barrier_count);
  AC_UNUSED(buffer_barriers);
  AC_UNUSED(image_barrier_count);
  AC_UNUSED(image_barriers);

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_set_scissor(
  ac_cmd   cmd_handle,
  uint32_t x,
  uint32_t y,
  uint32_t width,
  uint32_t height)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);

  // TODO: change interface to use uint32_t for x, y
  MTLScissorRect scissor;
  scissor.x = (NSUInteger)x;
  scissor.y = (NSUInteger)y;
  scissor.width = (NSUInteger)width;
  scissor.height = (NSUInteger)height;
  [cmd->render_encoder setScissorRect:scissor];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_set_viewport(
  ac_cmd cmd_handle,
  float  x,
  float  y,
  float  width,
  float  height,
  float  min_depth,
  float  max_depth)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);

  MTLViewport viewport;
  viewport.originX = (double)x;
  viewport.originY = (double)y;
  viewport.width = (double)width;
  viewport.height = (double)height;
  viewport.znear = (double)min_depth;
  viewport.zfar = (double)max_depth;
  [cmd->render_encoder setViewport:viewport];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_bind_pipeline(ac_cmd cmd_handle, ac_pipeline pipeline_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE(pipeline, ac_mtl_pipeline);

  switch (pipeline->common.type)
  {
  case ac_pipeline_type_graphics:
  case ac_pipeline_type_mesh:
  {
    [cmd->render_encoder setRenderPipelineState:pipeline->graphics_pipeline];
    [cmd->render_encoder setDepthStencilState:pipeline->depth_state];
    [cmd->render_encoder setDepthBias:pipeline->depth_bias
                           slopeScale:pipeline->slope_scale
                                clamp:1.0f];
    [cmd->render_encoder setCullMode:pipeline->cull_mode];
    [cmd->render_encoder setFrontFacingWinding:pipeline->winding];
    break;
  }
  case ac_pipeline_type_compute:
  {
    AC_ASSERT(cmd->render_encoder == NULL);

    if (cmd->compute_encoder == NULL)
    {
      cmd->compute_encoder = [cmd->cmd
        computeCommandEncoderWithDispatchType:MTLDispatchTypeConcurrent];
    }

    [cmd->compute_encoder setComputePipelineState:pipeline->compute_pipeline];
    break;
  }
  default:
  {
    break;
  }
  }

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_draw_mesh_tasks(
  ac_cmd   cmd_handle,
  uint32_t group_count_x,
  uint32_t group_count_y,
  uint32_t group_count_z)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE2(pipeline, cmd->common.pipeline, ac_mtl_pipeline);

  [cmd->render_encoder drawMeshThreadgroups:MTLSizeMake(
                                              group_count_x,
                                              group_count_y,
                                              group_count_z)
                threadsPerObjectThreadgroup:pipeline->object_workgroup
                  threadsPerMeshThreadgroup:pipeline->mesh_workgroup];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_draw(
  ac_cmd   cmd_handle,
  uint32_t vertex_count,
  uint32_t instance_count,
  uint32_t first_vertex,
  uint32_t first_instance)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE2(pipeline, cmd->common.pipeline, ac_mtl_pipeline);

  [cmd->render_encoder drawPrimitives:pipeline->primitive_type
                          vertexStart:first_vertex
                          vertexCount:vertex_count
                        instanceCount:instance_count
                         baseInstance:first_instance];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_draw_indexed(
  ac_cmd   cmd_handle,
  uint32_t index_count,
  uint32_t instance_count,
  uint32_t first_index,
  int32_t  first_vertex,
  uint32_t first_instance)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE2(pipeline, cmd->common.pipeline, ac_mtl_pipeline);

  const size_t index_size = cmd->index_type == MTLIndexTypeUInt32 ? 4 : 2;
  const size_t index_offset = (first_index * index_size) + cmd->index_offset;

  [cmd->render_encoder drawIndexedPrimitives:pipeline->primitive_type
                                  indexCount:index_count
                                   indexType:cmd->index_type
                                 indexBuffer:cmd->index_buffer
                           indexBufferOffset:index_offset
                               instanceCount:instance_count
                                  baseVertex:first_vertex
                                baseInstance:first_instance];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_bind_vertex_buffer(
  ac_cmd    cmd_handle,
  uint32_t  binding,
  ac_buffer buffer_handle,
  uint64_t  offset)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE(buffer, ac_mtl_buffer);

  [cmd->render_encoder setVertexBuffer:buffer->buffer
                                offset:offset
                               atIndex:AC_MTL_MAX_BINDINGS - binding];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_bind_index_buffer(
  ac_cmd        cmd_handle,
  ac_buffer     buffer_handle,
  uint64_t      offset,
  ac_index_type index_type)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE(buffer, ac_mtl_buffer);

  if (index_type == ac_index_type_u32)
  {
    cmd->index_type = MTLIndexTypeUInt32;
  }
  else
  {
    cmd->index_type = MTLIndexTypeUInt16;
  }
  cmd->index_offset = offset;
  cmd->index_buffer = buffer->buffer;

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_copy_buffer(
  ac_cmd    cmd_handle,
  ac_buffer src_handle,
  uint64_t  src_offset,
  ac_buffer dst_handle,
  uint64_t  dst_offset,
  uint64_t  size)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE(src, ac_mtl_buffer);
  AC_FROM_HANDLE(dst, ac_mtl_buffer);

  id<MTLBlitCommandEncoder> encoder = [cmd->cmd blitCommandEncoder];

  [encoder copyFromBuffer:src->buffer
             sourceOffset:src_offset
                 toBuffer:dst->buffer
        destinationOffset:dst_offset
                     size:size];

  [encoder endEncoding];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_copy_buffer_to_image(
  ac_cmd                      cmd_handle,
  ac_buffer                   src_handle,
  ac_image                    dst_handle,
  const ac_buffer_image_copy* copy)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE(src, ac_mtl_buffer);
  AC_FROM_HANDLE(dst, ac_mtl_image);

  MTLSize  source_size = MTLSizeMake(copy->width, copy->height, 1);
  uint32_t bytes = ac_format_size_bytes(dst->common.format);

  id<MTLBlitCommandEncoder> encoder = [cmd->cmd blitCommandEncoder];

  [encoder copyFromBuffer:src->buffer
             sourceOffset:copy->buffer_offset
        sourceBytesPerRow:copy->width * bytes
      sourceBytesPerImage:copy->width * copy->height * bytes
               sourceSize:source_size
                toTexture:dst->texture
         destinationSlice:copy->layer
         destinationLevel:copy->level
        destinationOrigin:MTLOriginMake(copy->x, copy->y, 0)
                  options:MTLBlitOptionNone];

  [encoder endEncoding];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_copy_image(
  ac_cmd               cmd_handle,
  ac_image             src_handle,
  ac_image             dst_handle,
  const ac_image_copy* copy)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE(src, ac_mtl_image);
  AC_FROM_HANDLE(dst, ac_mtl_image);

  id<MTLBlitCommandEncoder> encoder = [cmd->cmd blitCommandEncoder];

  [encoder copyFromTexture:src->texture
               sourceSlice:copy->src.layer
               sourceLevel:copy->src.level
              sourceOrigin:MTLOriginMake(copy->src.x, copy->dst.y, 0)
                sourceSize:MTLSizeMake(copy->width, copy->height, 1)
                 toTexture:dst->texture
          destinationSlice:copy->dst.layer
          destinationLevel:copy->dst.level
         destinationOrigin:MTLOriginMake(copy->dst.x, copy->dst.y, 0)];

  [encoder endEncoding];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_copy_image_to_buffer(
  ac_cmd                      cmd_handle,
  ac_image                    src_handle,
  ac_buffer                   dst_handle,
  const ac_buffer_image_copy* copy)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE(src, ac_mtl_image);
  AC_FROM_HANDLE(dst, ac_mtl_buffer);

  MTLSize  source_size = MTLSizeMake(src->common.width, src->common.height, 1);
  uint32_t bytes = ac_format_size_bytes(src->common.format);

  id<MTLBlitCommandEncoder> encoder = [cmd->cmd blitCommandEncoder];

  [encoder copyFromTexture:src->texture
                 sourceSlice:copy->layer
                 sourceLevel:copy->level
                sourceOrigin:MTLOriginMake(copy->x, copy->y, 0)
                  sourceSize:source_size
                    toBuffer:dst->buffer
           destinationOffset:copy->buffer_offset
      destinationBytesPerRow:copy->width * bytes
    destinationBytesPerImage:copy->width * copy->height * bytes
                     options:MTLBlitOptionNone];

  [encoder endEncoding];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_dispatch(
  ac_cmd   cmd_handle,
  uint32_t group_count_x,
  uint32_t group_count_y,
  uint32_t group_count_z)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE2(pipeline, cmd->common.pipeline, ac_mtl_pipeline);

  [cmd->compute_encoder dispatchThreadgroups:MTLSizeMake(
                                               group_count_x,
                                               group_count_y,
                                               group_count_z)
                       threadsPerThreadgroup:pipeline->workgroup];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_trace_rays(
  ac_cmd   cmd_handle,
  ac_sbt   sbt_handle,
  uint32_t width,
  uint32_t height,
  uint32_t depth)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(cmd_handle);
  AC_UNUSED(sbt_handle);
  AC_UNUSED(width);
  AC_UNUSED(height);
  AC_UNUSED(depth);

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_build_as(ac_cmd cmd_handle, ac_as_build_info* info)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE2(as, info->as, ac_mtl_as);
  AC_FROM_HANDLE2(scratch, info->scratch_buffer, ac_mtl_buffer);

  id<MTLAccelerationStructureCommandEncoder> command_encoder =
    [cmd->cmd accelerationStructureCommandEncoder];

  [command_encoder buildAccelerationStructure:as->as
                                   descriptor:as->descriptor
                                scratchBuffer:scratch->buffer
                          scratchBufferOffset:info->scratch_buffer_offset];

  [command_encoder endEncoding];

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_push_constants(ac_cmd cmd_handle, uint32_t size, const void* data)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE2(pipeline, cmd->common.pipeline, ac_mtl_pipeline);

  uint8_t buf[AC_ALIGN_UP(
    AC_MAX_PUSH_CONSTANT_RANGE,
    AC_MTL_PUSH_CONSTANT_ALIGNMENT)];

#if (AC_INCLUDE_DEBUG)
  AC_ZERO(buf);
#endif
  memcpy(buf, data, size);

  uint32_t aligned_size = AC_ALIGN_UP(size, AC_MTL_PUSH_CONSTANT_ALIGNMENT);

  switch (cmd_handle->pipeline->type)
  {
  case ac_pipeline_type_graphics:
  case ac_pipeline_type_mesh:
    if (pipeline->has_non_fragment_pc)
    {
      [cmd->render_encoder setVertexBytes:buf
                                   length:aligned_size
                                  atIndex:AC_MTL_PUSH_CONSTANT_INDEX];
    }

    if (pipeline->has_fragment_pc)
    {
      [cmd->render_encoder setFragmentBytes:buf
                                     length:aligned_size
                                    atIndex:AC_MTL_PUSH_CONSTANT_INDEX];
    }
    break;
  default:
    if (pipeline->has_non_fragment_pc)
    {
      [cmd->compute_encoder setBytes:buf
                              length:aligned_size
                             atIndex:AC_MTL_PUSH_CONSTANT_INDEX];
    }
    break;
  }

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_bind_set(
  ac_cmd               cmd_handle,
  ac_descriptor_buffer db_handle,
  ac_space             space_index,
  uint32_t             set)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);
  AC_FROM_HANDLE2(pipeline, cmd->common.pipeline, ac_mtl_pipeline);
  AC_FROM_HANDLE(db, ac_mtl_descriptor_buffer);

  if (!db->arg_buffer)
  {
    return;
  }

  ac_mtl_db_space* space = &db->spaces[space_index];

  __unsafe_unretained id<MTLResource>* ro_resources = NULL;
  if (space->resources.ro_resource_count)
  {
    ro_resources = space->resources.ro_resources +
                   (space->resources.ro_resource_count * set);
  }

  __unsafe_unretained id<MTLResource>* rw_resources = NULL;
  if (space->resources.rw_resource_count)
  {
    rw_resources = space->resources.rw_resources +
                   (space->resources.rw_resource_count * set);
  }

  uint64_t alignment = cmd->common.device->props.cbv_buffer_alignment;

  switch (pipeline->common.type)
  {
  case ac_pipeline_type_graphics:
  case ac_pipeline_type_mesh:
  {
    [cmd->render_encoder
      setVertexBuffer:db->arg_buffer
               offset:space->arg_buffer_offset +
                      (AC_ALIGN_UP(space->arg_enc.encodedLength, alignment) *
                       set)
              atIndex:space_index];

    [cmd->render_encoder
      setFragmentBuffer:db->arg_buffer
                 offset:space->arg_buffer_offset +
                        (AC_ALIGN_UP(space->arg_enc.encodedLength, alignment) *
                         set)
                atIndex:space_index];

    if (space->resources.ro_resource_count)
    {
      [cmd->render_encoder
        useResources:ro_resources
               count:space->resources.ro_resource_count
               usage:MTLResourceUsageRead
              stages:MTLRenderStageVertex | MTLRenderStageFragment];
    }

    if (space->resources.rw_resource_count)
    {
      [cmd->render_encoder
        useResources:rw_resources
               count:space->resources.rw_resource_count
               usage:MTLResourceUsageRead | MTLResourceUsageWrite
              stages:MTLRenderStageVertex | MTLRenderStageFragment];
    }

    break;
  }
  case ac_pipeline_type_compute:
  {
    [cmd->compute_encoder
      setBuffer:db->arg_buffer
         offset:space->arg_buffer_offset +
                (AC_ALIGN_UP(space->arg_enc.encodedLength, alignment) * set)
        atIndex:space_index];

    if (space->resources.ro_resource_count)
    {
      [cmd->compute_encoder useResources:ro_resources
                                   count:space->resources.ro_resource_count
                                   usage:MTLResourceUsageRead];
    }

    if (space->resources.rw_resource_count)
    {
      [cmd->compute_encoder
        useResources:rw_resources
               count:space->resources.rw_resource_count
               usage:MTLResourceUsageRead | MTLResourceUsageWrite];
    }

    break;
  }
  default:
  {
    AC_ASSERT(false);
    break;
  }
  }

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_begin_debug_label(
  ac_cmd      cmd_handle,
  const char* name,
  const float color[4])
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(color);

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);

  if (cmd->render_encoder)
  {
    [cmd->render_encoder pushDebugGroup:[NSString stringWithUTF8String:name]];
  }
  else if (cmd->compute_encoder)
  {
    [cmd->compute_encoder pushDebugGroup:[NSString stringWithUTF8String:name]];
  }
  else
  {
    [cmd->cmd pushDebugGroup:[NSString stringWithUTF8String:name]];
  }

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_insert_debug_label(
  ac_cmd      cmd_handle,
  const char* name,
  const float color[4])
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(color);

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);

  if (cmd->render_encoder)
  {
    [cmd->render_encoder
      insertDebugSignpost:[NSString stringWithUTF8String:name]];
  }
  else if (cmd->compute_encoder)
  {
    [cmd->compute_encoder
      insertDebugSignpost:[NSString stringWithUTF8String:name]];
  }

  AC_OBJC_END_ARP();
}

static void
ac_mtl_cmd_end_debug_label(ac_cmd cmd_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(cmd, ac_mtl_cmd);

  if (cmd->render_encoder)
  {
    [cmd->render_encoder popDebugGroup];
  }
  else if (cmd->compute_encoder)
  {
    [cmd->compute_encoder popDebugGroup];
  }
  else
  {
    [cmd->cmd popDebugGroup];
  }

  AC_OBJC_END_ARP();
}

static void
ac_mtl_begin_capture(ac_device device_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_FROM_HANDLE(device, ac_mtl_device);

  MTLCaptureManager* mgr = [MTLCaptureManager sharedCaptureManager];

  if (![mgr isCapturing])
  {
    MTLCaptureDescriptor* descriptor = [MTLCaptureDescriptor new];
    descriptor.destination = MTLCaptureDestinationDeveloperTools;
    descriptor.captureObject = device->device;

    NSError* __autoreleasing err = NULL;
    [mgr startCaptureWithDescriptor:descriptor error:&err];

    if (err)
    {
      AC_ERROR(
        "[ metal ]: failed to begin capture %s",
        [[err localizedDescription] UTF8String]);
    }
  }

  AC_OBJC_END_ARP();
}

static void
ac_mtl_end_capture(ac_device device_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(device_handle);

  MTLCaptureManager* mgr = [MTLCaptureManager sharedCaptureManager];

  if ([mgr isCapturing])
  {
    [mgr stopCapture];
  }

  AC_OBJC_END_ARP();
}

ac_result
ac_mtl_create_device(const ac_device_info* info, ac_device* device_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_UNUSED(info);

  AC_INIT_INTERNAL(device, ac_mtl_device);

  device->common.destroy_device = ac_mtl_destroy_device;
  device->common.queue_wait_idle = ac_mtl_queue_wait_idle;
  device->common.queue_submit = ac_mtl_queue_submit;
  device->common.queue_present = ac_mtl_queue_present;
  device->common.create_fence = ac_mtl_create_fence;
  device->common.destroy_fence = ac_mtl_destroy_fence;
  device->common.get_fence_value = ac_mtl_get_fence_value;
  device->common.signal_fence = ac_mtl_signal_fence;
  device->common.wait_fence = ac_mtl_wait_fence;
  device->common.create_swapchain = ac_mtl_create_swapchain;
  device->common.destroy_swapchain = ac_mtl_destroy_swapchain;
  device->common.create_cmd_pool = ac_mtl_create_cmd_pool;
  device->common.destroy_cmd_pool = ac_mtl_destroy_cmd_pool;
  device->common.reset_cmd_pool = ac_mtl_reset_cmd_pool;
  device->common.create_cmd = ac_mtl_create_cmd;
  device->common.destroy_cmd = ac_mtl_destroy_cmd;
  device->common.begin_cmd = ac_mtl_begin_cmd;
  device->common.end_cmd = ac_mtl_end_cmd;
  device->common.acquire_next_image = ac_mtl_acquire_next_image;
  device->common.create_shader = ac_mtl_create_shader;
  device->common.destroy_shader = ac_mtl_destroy_shader;
  device->common.create_dsl = ac_mtl_create_dsl;
  device->common.destroy_dsl = ac_mtl_destroy_dsl;
  device->common.create_descriptor_buffer = ac_mtl_create_descriptor_buffer;
  device->common.destroy_descriptor_buffer = ac_mtl_destroy_descriptor_buffer;
  device->common.create_graphics_pipeline = ac_mtl_create_graphics_pipeline;
  device->common.create_compute_pipeline = ac_mtl_create_compute_pipeline;
  device->common.destroy_pipeline = ac_mtl_destroy_pipeline;
  device->common.create_buffer = ac_mtl_create_buffer;
  device->common.destroy_buffer = ac_mtl_destroy_buffer;
  device->common.map_memory = ac_mtl_map_memory;
  device->common.unmap_memory = ac_mtl_unmap_memory;
  device->common.create_sampler = ac_mtl_create_sampler;
  device->common.destroy_sampler = ac_mtl_destroy_sampler;
  device->common.create_image = ac_mtl_create_image;
  device->common.destroy_image = ac_mtl_destroy_image;
  device->common.update_descriptor = ac_mtl_update_descriptor;
  device->common.create_blas = ac_mtl_create_blas;
  device->common.create_tlas = ac_mtl_create_tlas;
  device->common.destroy_as = ac_mtl_destroy_as;
  device->common.write_as_instances = ac_mtl_write_as_instances;
  device->common.cmd_begin_rendering = ac_mtl_cmd_begin_rendering;
  device->common.cmd_end_rendering = ac_mtl_cmd_end_rendering;
  device->common.cmd_barrier = ac_mtl_cmd_barrier;
  device->common.cmd_set_scissor = ac_mtl_cmd_set_scissor;
  device->common.cmd_set_viewport = ac_mtl_cmd_set_viewport;
  device->common.cmd_bind_pipeline = ac_mtl_cmd_bind_pipeline;
  device->common.cmd_draw_mesh_tasks = ac_mtl_cmd_draw_mesh_tasks;
  device->common.cmd_draw = ac_mtl_cmd_draw;
  device->common.cmd_draw_indexed = ac_mtl_cmd_draw_indexed;
  device->common.cmd_bind_vertex_buffer = ac_mtl_cmd_bind_vertex_buffer;
  device->common.cmd_bind_index_buffer = ac_mtl_cmd_bind_index_buffer;
  device->common.cmd_copy_buffer = ac_mtl_cmd_copy_buffer;
  device->common.cmd_copy_buffer_to_image = ac_mtl_cmd_copy_buffer_to_image;
  device->common.cmd_copy_image = ac_mtl_cmd_copy_image;
  device->common.cmd_copy_image_to_buffer = ac_mtl_cmd_copy_image_to_buffer;
  device->common.cmd_bind_set = ac_mtl_cmd_bind_set;
  device->common.cmd_dispatch = ac_mtl_cmd_dispatch;
  device->common.cmd_build_as = ac_mtl_cmd_build_as;
  device->common.cmd_trace_rays = ac_mtl_cmd_trace_rays;
  device->common.cmd_push_constants = ac_mtl_cmd_push_constants;
  device->common.cmd_begin_debug_label = ac_mtl_cmd_begin_debug_label;
  device->common.cmd_end_debug_label = ac_mtl_cmd_end_debug_label;
  device->common.cmd_insert_debug_label = ac_mtl_cmd_insert_debug_label;
  device->common.begin_capture = ac_mtl_begin_capture;
  device->common.end_capture = ac_mtl_end_capture;

  if (info->debug_bits && AC_INCLUDE_DEBUG)
  {
    AC_INFO("[ renderer ] [ metal ] : set breakpoint in MTLReportFailure to "
            "stop on errors");

    setenv("MTL_DEBUG_LAYER", "1", true);
    setenv("MTL_DEBUG_LAYER_VALIDATE_UNRETAINED_RESOURCES", "5", true);
    setenv("MTL_DEBUG_LAYER_ERROR_MODE", "nslog", true);
    // setenv( "MTL_DEBUG_LAYER_WARNING_MODE", "nslog", true );
  }

  device->device = MTLCreateSystemDefaultDevice();

  if (device->device == NULL)
  {
    AC_ERROR("[ renderer ] [ metal ] : failed to create system default device");
    return ac_result_unknown_error;
  }

  if (!device->device.argumentBuffersSupport)
  {
    AC_ERROR("[ renderer ] [ metal ] : ac not supported on devices without "
             "argument buffers");
    return ac_result_unknown_error;
  }

  ac_device_properties* props = &device->common.props;
  props->api = "Metal";
  props->cbv_buffer_alignment = 256;

  if (AC_PLATFORM_APPLE_TVOS)
  {
    props->cbv_buffer_alignment = 64;

    if ([device->device supportsFamily:MTLGPUFamilyApple2])
    {
      props->cbv_buffer_alignment = 16;
    }
  }
  else if (AC_PLATFORM_APPLE_IOS)
  {
    props->cbv_buffer_alignment = 64;

    if ([device->device supportsFamily:MTLGPUFamilyApple3])
    {
      props->cbv_buffer_alignment = 64;
    }
  }
  else if (AC_PLATFORM_APPLE_MACOS)
  {
    props->cbv_buffer_alignment = 256;

    if (
      [device->device respondsToSelector:@selector(supportsFamily:)] &&
      [device->device supportsFamily:MTLGPUFamilyApple1])
    {
      props->cbv_buffer_alignment = 16;
    }
  }

  if (AC_PLATFORM_APPLE_SIMULATOR)
  {
    props->cbv_buffer_alignment = 256;
  }

  props->image_alignment = 1;
  props->image_row_alignment = 1;
  // TODO:
  props->max_sample_count = 4;
  props->as_instance_size =
    sizeof(MTLAccelerationStructureUserIDInstanceDescriptor);

  device->common.queue_count = ac_queue_type_count;

  for (uint32_t i = 0; i < device->common.queue_count; ++i)
  {
    device->common.queue_map[i] = i;

    ac_queue* queue_handle = &device->common.queues[i];
    AC_INIT_INTERNAL(queue, ac_mtl_queue);
    queue->queue = [device->device newCommandQueue];

    if (!queue->queue)
    {
      AC_ERROR("[ renderer ] [ metal ] : failed to create new command queue");
      return ac_result_unknown_error;
    }

    queue->common.type = (ac_queue_type)i;
  }

  device->common.support_raytracing = true;

  static VkAllocationCallbacks cpu_fns = {
    .pfnAllocation = &ac_vk_alloc_fn,
    .pfnReallocation = &ac_vk_realloc_fn,
    .pfnFree = &ac_vk_free_fn,
    .pfnInternalAllocation = &ac_vk_internal_alloc_fn,
    .pfnInternalFree = &ac_vk_internal_free_fn,
  };

  VmaVulkanFunctions gpu_fns = {
    .vkAllocateMemory = ac_mtl_allocate_memory,
    .vkFreeMemory = ac_mtl_free_memory,
    .vkGetPhysicalDeviceMemoryProperties =
      ac_mtl_get_physical_device_memory_properties,
    .vkGetPhysicalDeviceProperties = ac_mtl_get_physical_device_properties,
  };

  VmaAllocatorCreateInfo vma_info = {
    .pAllocationCallbacks = &cpu_fns,
    .pVulkanFunctions = &gpu_fns,
    .instance = (VkInstance)(*device_handle),
    .physicalDevice = (VkPhysicalDevice)(*device_handle),
    .device = (VkDevice)(*device_handle),
  };

  vmaCreateAllocator(&vma_info, &device->gpu_allocator);

  return ac_result_success;

  AC_OBJC_END_ARP();
}

#endif
