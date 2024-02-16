#include "ac_private.h"

#if (AC_INCLUDE_VULKAN)

#include "renderer_vulkan.h"
#include "renderer_vulkan_helpers.h"

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

static void VKAPI_PTR
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

static void VKAPI_PTR
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

static VKAPI_ATTR VkBool32 VKAPI_CALL
ac_vk_debug_callback(
  VkDebugUtilsMessageSeverityFlagBitsEXT      messageSeverity,
  VkDebugUtilsMessageTypeFlagsEXT             flags,
  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
  void*                                       user_data)
{
  AC_UNUSED(flags);
  AC_UNUSED(pCallbackData);
  AC_UNUSED(user_data);

  if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
  {
    AC_DEBUG("[ vulkan ] : %s", pCallbackData->pMessage);
  }

  else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
  {
    AC_INFO(" [ vulkan ] : %s", pCallbackData->pMessage);
  }
  else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
  {
    AC_WARN(" [ vulkan ] : %s", pCallbackData->pMessage);
  }
  else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
  {
    switch (pCallbackData->messageIdNumber)
    {
    case 1444176934:
      return VK_FALSE;
    }
    AC_ERROR("[ vulkan ] : %s", pCallbackData->pMessage);
    AC_DEBUGBREAK();
  }

  return VK_FALSE;
}

static inline void
ac_vk_load_global_functions(ac_vk_device* device)
{
#define LOAD(NAME)                                                             \
  do                                                                           \
  {                                                                            \
    union {                                                                    \
      PFN_vkVoidFunction vfun;                                                 \
      PFN_##NAME         nfun;                                                 \
    } u;                                                                       \
    u.vfun = device->vkGetInstanceProcAddr(NULL, #NAME);                       \
    device->NAME = u.nfun;                                                     \
  }                                                                            \
  while (false)

  LOAD(vkCreateInstance);
  LOAD(vkEnumerateInstanceExtensionProperties);
  LOAD(vkEnumerateInstanceLayerProperties);

#undef LOAD
}

static inline void
ac_vk_load_instance_functions(ac_vk_device* device)
{
#define LOAD(NAME)                                                             \
  do                                                                           \
  {                                                                            \
    union {                                                                    \
      PFN_vkVoidFunction vfun;                                                 \
      PFN_##NAME         nfun;                                                 \
    } u;                                                                       \
    u.vfun = device->vkGetInstanceProcAddr(device->instance, #NAME);           \
    device->NAME = u.nfun;                                                     \
  }                                                                            \
  while (false)

  LOAD(vkCreateDevice);
  LOAD(vkDestroyInstance);
  LOAD(vkEnumerateDeviceExtensionProperties);
  LOAD(vkEnumeratePhysicalDevices);
  LOAD(vkGetDeviceProcAddr);
  LOAD(vkGetPhysicalDeviceMemoryProperties);
  LOAD(vkGetPhysicalDeviceProperties);
  LOAD(vkGetPhysicalDeviceQueueFamilyProperties);
  LOAD(vkGetPhysicalDeviceFeatures2);
  LOAD(vkGetPhysicalDeviceMemoryProperties2);
  LOAD(vkGetPhysicalDeviceProperties2);
  LOAD(vkCmdBeginDebugUtilsLabelEXT);
  LOAD(vkCmdEndDebugUtilsLabelEXT);
  LOAD(vkCmdInsertDebugUtilsLabelEXT);
  LOAD(vkCreateDebugUtilsMessengerEXT);
  LOAD(vkDestroyDebugUtilsMessengerEXT);
  LOAD(vkSetDebugUtilsObjectNameEXT);
  LOAD(vkDestroySurfaceKHR);
  LOAD(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
  LOAD(vkGetPhysicalDeviceSurfaceFormatsKHR);
  LOAD(vkGetPhysicalDeviceSurfacePresentModesKHR);
  LOAD(vkGetPhysicalDeviceSurfaceSupportKHR);
#undef LOAD
}

static inline void
ac_vk_load_device_functions(ac_vk_device* device)
{
#define LOAD(NAME)                                                             \
  do                                                                           \
  {                                                                            \
    union {                                                                    \
      PFN_vkVoidFunction vfun;                                                 \
      PFN_##NAME         nfun;                                                 \
    } u;                                                                       \
    u.vfun = device->vkGetDeviceProcAddr(device->device, #NAME);               \
    device->NAME = u.nfun;                                                     \
  }                                                                            \
  while (false)

  LOAD(vkAllocateCommandBuffers);
  LOAD(vkAllocateDescriptorSets);
  LOAD(vkAllocateMemory);
  LOAD(vkBeginCommandBuffer);
  LOAD(vkBindBufferMemory);
  LOAD(vkBindImageMemory);
  LOAD(vkCmdBindDescriptorSets);
  LOAD(vkCmdBindIndexBuffer);
  LOAD(vkCmdBindPipeline);
  LOAD(vkCmdBindVertexBuffers);
  LOAD(vkCmdCopyBuffer);
  LOAD(vkCmdCopyBufferToImage);
  LOAD(vkCmdCopyImage);
  LOAD(vkCmdCopyImageToBuffer);
  LOAD(vkCmdDispatch);
  LOAD(vkCmdDraw);
  LOAD(vkCmdDrawIndexed);
  LOAD(vkCmdPushConstants);
  LOAD(vkCmdSetScissor);
  LOAD(vkCmdSetViewport);
  LOAD(vkCreateBuffer);
  LOAD(vkCreateCommandPool);
  LOAD(vkCreateComputePipelines);
  LOAD(vkCreateDescriptorPool);
  LOAD(vkCreateDescriptorSetLayout);
  LOAD(vkCreateGraphicsPipelines);
  LOAD(vkCreateImage);
  LOAD(vkCreateImageView);
  LOAD(vkCreatePipelineLayout);
  LOAD(vkCreateSampler);
  LOAD(vkCreateSemaphore);
  LOAD(vkCreateShaderModule);
  LOAD(vkDestroyBuffer);
  LOAD(vkDestroyCommandPool);
  LOAD(vkDestroyDescriptorPool);
  LOAD(vkDestroyDescriptorSetLayout);
  LOAD(vkDestroyDevice);
  LOAD(vkDestroyImage);
  LOAD(vkDestroyImageView);
  LOAD(vkDestroyPipeline);
  LOAD(vkDestroyPipelineLayout);
  LOAD(vkDestroySampler);
  LOAD(vkDestroySemaphore);
  LOAD(vkDestroyShaderModule);
  LOAD(vkDeviceWaitIdle);
  LOAD(vkEndCommandBuffer);
  LOAD(vkFlushMappedMemoryRanges);
  LOAD(vkFreeMemory);
  LOAD(vkGetBufferMemoryRequirements);
  LOAD(vkGetDeviceQueue);
  LOAD(vkGetImageMemoryRequirements);
  LOAD(vkInvalidateMappedMemoryRanges);
  LOAD(vkMapMemory);
  LOAD(vkQueueWaitIdle);
  LOAD(vkResetCommandPool);
  LOAD(vkUnmapMemory);
  LOAD(vkUpdateDescriptorSets);
  LOAD(vkBindBufferMemory2);
  LOAD(vkBindImageMemory2);
  LOAD(vkGetBufferMemoryRequirements2);
  LOAD(vkGetImageMemoryRequirements2);
  LOAD(vkGetBufferDeviceAddress);
  LOAD(vkGetSemaphoreCounterValue);
  LOAD(vkSignalSemaphore);
  LOAD(vkWaitSemaphores);
  LOAD(vkCmdBeginRendering);
  LOAD(vkCmdEndRendering);
  LOAD(vkCmdPipelineBarrier2);
  LOAD(vkGetDeviceBufferMemoryRequirements);
  LOAD(vkGetDeviceImageMemoryRequirements);
  LOAD(vkQueueSubmit2);
  LOAD(vkCmdDrawMeshTasksEXT);
  LOAD(vkCmdBuildAccelerationStructuresKHR);
  LOAD(vkCreateAccelerationStructureKHR);
  LOAD(vkDestroyAccelerationStructureKHR);
  LOAD(vkGetAccelerationStructureBuildSizesKHR);
  LOAD(vkGetAccelerationStructureDeviceAddressKHR);
  LOAD(vkCmdTraceRaysKHR);
  LOAD(vkCreateRayTracingPipelinesKHR);
  LOAD(vkGetRayTracingShaderGroupHandlesKHR);
  LOAD(vkAcquireNextImageKHR);
  LOAD(vkCreateSwapchainKHR);
  LOAD(vkDestroySwapchainKHR);
  LOAD(vkGetSwapchainImagesKHR);
  LOAD(vkQueuePresentKHR);

#undef LOAD
}

static inline VkImageAspectFlags
ac_format_to_vk_aspect_flags(ac_format format)
{
  VkImageAspectFlags aspect = VK_IMAGE_ASPECT_COLOR_BIT;

  if (ac_format_has_depth_aspect(format))
  {
    aspect &= ~((uint32_t)VK_IMAGE_ASPECT_COLOR_BIT);
    aspect |= VK_IMAGE_ASPECT_DEPTH_BIT;
  }
  else if (ac_format_has_stencil_aspect(format))
  {
    aspect &= ~((uint32_t)VK_IMAGE_ASPECT_COLOR_BIT);
    aspect |= VK_IMAGE_ASPECT_STENCIL_BIT;
  }

  return aspect;
}

static inline VkDeviceAddress
ac_vk_get_buffer_address(ac_vk_device* device, VkBuffer buffer)
{
  VkBufferDeviceAddressInfo info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
    .buffer = buffer,
  };
  return device->vkGetBufferDeviceAddress(device->device, &info);
}

static inline VkDeviceAddress
ac_vk_get_as_address(ac_vk_device* device, VkAccelerationStructureKHR as)
{
  VkAccelerationStructureDeviceAddressInfoKHR info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR,
    .accelerationStructure = as,
  };

  return device->vkGetAccelerationStructureDeviceAddressKHR(
    device->device,
    &info);
}

#if (AC_INCLUDE_DEBUG)
static inline void
ac_vk_set_object_name(
  const ac_vk_device* device,
  const char*         name,
  VkObjectType        type,
  uint64_t            object)
{
  if (!device->common.debug_bits)
  {
    return;
  }

  const PFN_vkSetDebugUtilsObjectNameEXT fun =
    device->vkSetDebugUtilsObjectNameEXT;
  if (!fun)
  {
    return;
  }

  if (!name)
  {
    return;
  }

  VkDebugUtilsObjectNameInfoEXT info = {
    .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
    .objectType = type,
    .objectHandle = object,
    .pObjectName = name,
  };
  VkResult res = fun(device->device, &info);
  AC_MAYBE_UNUSED(res);
  AC_ASSERT(res == VK_SUCCESS);
}
#endif

static inline ac_result
ac_vk_create_pipeline_layout(
  ac_vk_device*         device,
  uint32_t              shader_count,
  const ac_shader*      shaders,
  const ac_vk_dsl*      dsl,
  VkPipelineStageFlags* push_constant_stage_flags,
  VkPipelineLayout*     layout)
{
  VkPushConstantRange push_constant_range = {
    .size = AC_MAX_PUSH_CONSTANT_RANGE,
  };

  uint32_t              set_layout_count = 0;
  VkDescriptorSetLayout set_layouts[ac_space_count];
  for (uint32_t i = 0; i < ac_space_count; ++i)
  {
    if (dsl->dsls[i] != VK_NULL_HANDLE)
    {
      set_layouts[set_layout_count++] = dsl->dsls[i];
    }
  }

  VkPipelineLayoutCreateInfo info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    .setLayoutCount = set_layout_count,
    .pSetLayouts = set_layouts,
    .pPushConstantRanges = &push_constant_range,
  };

  for (uint32_t i = 0; i < shader_count; ++i)
  {
    if (!shaders[i])
    {
      continue;
    }

    if (ac_shader_compiler_has_push_constant(shaders[i]->reflection))
    {
      push_constant_range.stageFlags |=
        (VkShaderStageFlags)(ac_shader_stage_to_vk(shaders[i]->stage));
    }
  }

  if (push_constant_range.stageFlags)
  {
    info.pushConstantRangeCount = 1;
    *push_constant_stage_flags = push_constant_range.stageFlags;
  }

  AC_VK_RIF(device->vkCreatePipelineLayout(
    device->device,
    &info,
    &device->cpu_allocator,
    layout));

  return ac_result_success;
}

static inline bool
ac_vk_is_extensions_supported(
  const VkExtensionProperties* extensions,
  uint32_t                     count,
  const char**                 check_extensions,
  uint32_t                     check_count)
{
  uint32_t founded_count = 0;

  for (uint32_t i = 0; i < check_count; ++i)
  {
    for (uint32_t j = 0; j < count; ++j)
    {
      if (strcmp(check_extensions[i], extensions[j].extensionName) == 0)
      {
        founded_count++;
        break;
      }
    }
  }

  return founded_count == check_count;
}

static inline ac_result
ac_vk_select_queue_slots(
  ac_vk_device*     device,
  VkSurfaceKHR      surface,
  ac_vk_queue_slot* out_slots)
{
  uint32_t family_count = 0;
  device->vkGetPhysicalDeviceQueueFamilyProperties(
    device->gpu,
    &family_count,
    NULL);
  VkQueueFamilyProperties* queue_family_properties =
    ac_alloc(family_count * sizeof(VkQueueFamilyProperties));
  device->vkGetPhysicalDeviceQueueFamilyProperties(
    device->gpu,
    &family_count,
    queue_family_properties);

  for (uint32_t i = 0; i < ac_queue_type_count; ++i)
  {
    AC_ZERO(out_slots[i]);
  }

  ac_vk_queue_slot* slot_graphics = out_slots + ac_queue_type_graphics;
  ac_vk_queue_slot* slot_compute = out_slots + ac_queue_type_transfer;
  ac_vk_queue_slot* slot_transfer = out_slots + ac_queue_type_compute;

  for (uint32_t i = 0; i < family_count; ++i)
  {
    VkBool32 is_present = VK_TRUE;

    if (surface) // TODO
    {
      AC_VK_RIF(device->vkGetPhysicalDeviceSurfaceSupportKHR(
        device->gpu,
        i,
        surface,
        &is_present));
    }

    VkQueueFamilyProperties qf = queue_family_properties[i];

    uint32_t capability = 0;

    if (qf.queueFlags & VK_QUEUE_GRAPHICS_BIT)
    {
      capability |= AC_BIT(ac_queue_type_graphics);
    }

    if (qf.queueFlags & VK_QUEUE_COMPUTE_BIT)
    {
      capability |= AC_BIT(ac_queue_type_compute);
    }

    if (qf.queueFlags & VK_QUEUE_TRANSFER_BIT)
    {
      capability |= AC_BIT(ac_queue_type_transfer);
    }

    switch (capability)
    {
    // general graphics
    case AC_BIT(ac_queue_type_graphics):
    case AC_BIT(ac_queue_type_graphics) | AC_BIT(ac_queue_type_compute):
    case AC_BIT(ac_queue_type_graphics) | AC_BIT(ac_queue_type_compute) |
      AC_BIT(ac_queue_type_transfer):
    {
      if (slot_graphics->quality < 3 && is_present == VK_TRUE)
      {
        slot_graphics->family = i;
        slot_graphics->quality = 3;
      }

      if (slot_compute->quality < 1)
      {
        slot_compute->family = i;
        slot_compute->quality = 1;
      }

      if (slot_transfer->quality < 1)
      {
        slot_transfer->family = i;
        slot_transfer->quality = 1;
      }
      break;
    }
    // compute-only (?)
    case AC_BIT(ac_queue_type_compute):
    {
      if (slot_compute->quality < 2)
      {
        slot_compute->family = i;
        slot_compute->quality = 2;
      }

      if (slot_transfer->quality < 2)
      {
        slot_transfer->family = i;
        slot_transfer->quality = 2;
      }
      break;
    }
    // async compute queue
    case AC_BIT(ac_queue_type_compute) | AC_BIT(ac_queue_type_transfer):
    {
      if (slot_compute->quality < 3)
      {
        slot_compute->family = i;
        slot_compute->quality = 3;
      }

      if (slot_transfer->quality < 3)
      {
        slot_transfer->family = i;
        slot_transfer->quality = 3;
      }
      break;
    }

    // transfer-only
    case AC_BIT(ac_queue_type_transfer):
    {
      if (slot_transfer->quality < 1)
      {
        slot_transfer->family = i;
        slot_transfer->quality = 1;
      }
      break;
    }
    }
  }


  if (
    slot_graphics->quality == 0 || slot_compute->quality == 0 ||
    slot_transfer->quality == 0)
  {
    ac_free(queue_family_properties);
    return ac_result_unknown_error;
  }

  uint8_t* family_used_queue_counters =
    ac_calloc(family_count * sizeof(uint8_t));

  for (uint32_t i = 0; i < ac_queue_type_count; ++i)
  {
    ac_vk_queue_slot* slot = out_slots + i;

    slot->index = family_used_queue_counters[slot->family]++;

    uint32_t max_index = queue_family_properties[slot->family].queueCount - 1;
    if (slot->index > max_index)
    {
      slot->index = max_index;
    }
  }

  ac_free(family_used_queue_counters);
  ac_free(queue_family_properties);

  return ac_result_success;
}

static inline ac_result
ac_vk_recreate_binary_fence(ac_device device_handle, ac_vk_fence* fence)
{
  AC_FROM_HANDLE(device, ac_vk_device);

  VkSemaphore new_semaphore;
  AC_VK_RIF(device->vkCreateSemaphore(
    device->device,
    &(VkSemaphoreCreateInfo) {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    },
    &device->cpu_allocator,
    &new_semaphore));

  device->vkDeviceWaitIdle(device->device);
  device->vkDestroySemaphore(
    device->device,
    fence->semaphore,
    &device->cpu_allocator);
  fence->semaphore = new_semaphore;
  return ac_result_success;
}

static inline ac_result
ac_vk_create_buffer2(
  ac_vk_device*         device,
  const ac_buffer_info* info,
  VkBufferUsageFlags    usage,
  VkBuffer*             buffer,
  VmaAllocation*        allocation)
{
  VmaAllocationCreateInfo allocation_create_info = {
    .usage = ac_determine_vma_memory_usage(info->memory_usage),
    .pUserData = ac_const_cast(info->name),
    .flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT |
             VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
  };

  VkBufferCreateInfo buffer_create_info = {
    .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .size = info->size,
    .usage = ac_buffer_usage_bits_to_vk(info->usage) | usage,
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL,
  };

  if (info->usage & ac_buffer_usage_raytracing_bit)
  {
    // TODO: get alignment from props
    uint64_t alignment = 256;

    AC_VK_RIF(vmaCreateBufferWithAlignment(
      device->gpu_allocator,
      &buffer_create_info,
      &allocation_create_info,
      alignment,
      buffer,
      allocation,
      NULL));
  }
  else
  {
    AC_VK_RIF(vmaCreateBuffer(
      device->gpu_allocator,
      &buffer_create_info,
      &allocation_create_info,
      buffer,
      allocation,
      NULL));
  }

  AC_VK_SET_OBJECT_NAME(
    device,
    info->name,
    VK_OBJECT_TYPE_BUFFER,
    (uint64_t)*buffer);

  return ac_result_success;
}

static void
ac_vk_destroy_device(ac_device device_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);

  for (uint32_t i = 0; i < device->common.queue_count; ++i)
  {
    if (!device->common.queues[i])
    {
      continue;
    }

    ac_queue queue_handle = device->common.queues[i];
    AC_FROM_HANDLE(queue, ac_vk_queue);
    ac_free(queue);
  }

  if (device->gpu_allocator)
  {
    vmaDestroyAllocator(device->gpu_allocator);
  }

  if (device->device)
  {
    device->vkDestroyDevice(device->device, &device->cpu_allocator);
  }

  if (device->debug_messenger)
  {
    device->vkDestroyDebugUtilsMessengerEXT(
      device->instance,
      device->debug_messenger,
      &device->cpu_allocator);
  }

  if (device->instance)
  {
    device->vkDestroyInstance(device->instance, &device->cpu_allocator);
  }

  ac_unload_library(device->vk);
}

static ac_result
ac_vk_map_memory(ac_device device_handle, ac_buffer buffer_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(buffer, ac_vk_buffer);

  AC_VK_RIF(vmaMapMemory(
    device->gpu_allocator,
    buffer->allocation,
    &buffer->common.mapped_memory));
  return ac_result_success;
}

static void
ac_vk_unmap_memory(ac_device device_handle, ac_buffer buffer_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(buffer, ac_vk_buffer);

  vmaUnmapMemory(device->gpu_allocator, buffer->allocation);
}

static ac_result
ac_vk_queue_wait_idle(ac_queue queue_handle)
{
  AC_FROM_HANDLE2(device, queue_handle->device, ac_vk_device);
  AC_FROM_HANDLE(queue, ac_vk_queue);
  AC_VK_RIF(device->vkQueueWaitIdle(queue->queue));
  return ac_result_success;
}

static ac_result
ac_vk_queue_submit(ac_queue queue_handle, const ac_queue_submit_info* info)
{
  AC_FROM_HANDLE2(device, queue_handle->device, ac_vk_device);
  AC_FROM_HANDLE(queue, ac_vk_queue);

  VkSemaphoreSubmitInfo*     wait_semaphores = NULL;
  VkSemaphoreSubmitInfo*     signal_semaphores = NULL;
  VkCommandBufferSubmitInfo* cmds = NULL;

  if (info->wait_fence_count)
  {
    wait_semaphores =
      ac_alloc(info->wait_fence_count * sizeof(VkSemaphoreSubmitInfo));
  }

  if (info->signal_fence_count)
  {
    signal_semaphores =
      ac_alloc(info->signal_fence_count * sizeof(VkSemaphoreSubmitInfo));
  }

  cmds = ac_alloc(info->cmd_count * sizeof(VkCommandBufferSubmitInfo));


  uint32_t wait_count = 0;
  for (uint32_t i = 0; i < info->wait_fence_count; ++i)
  {
    ac_fence_submit_info* fi = &info->wait_fences[i];

    ac_fence fence_handle = fi->fence;

    AC_FROM_HANDLE(fence, ac_vk_fence);

    if ((fence->common.bits & ac_fence_present_bit) && !fence->signaled)
    {
      continue;
    }

    wait_semaphores[wait_count++] = (VkSemaphoreSubmitInfo) {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = fence->semaphore,
      .stageMask = fi->stages,
      .value = fi->value,
    };
  }

  for (uint32_t i = 0; i < info->signal_fence_count; ++i)
  {
    ac_fence_submit_info* fi = &info->signal_fences[i];

    ac_fence fence_handle = fi->fence;
    AC_FROM_HANDLE(fence, ac_vk_fence);

    if ((fence->common.bits & ac_fence_present_bit) && fence->signaled)
    {
      AC_RIF(ac_vk_recreate_binary_fence(queue->common.device, fence));
    }

    signal_semaphores[i] = (VkSemaphoreSubmitInfo) {
      .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO,
      .semaphore = fence->semaphore,
      .stageMask = fi->stages,
      .value = fi->value,
    };
  }

  for (uint32_t i = 0; i < info->cmd_count; ++i)
  {
    ac_cmd cmd_handle = info->cmds[i];
    AC_FROM_HANDLE(cmd, ac_vk_cmd);

    cmds[i] = (VkCommandBufferSubmitInfo) {
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO,
      .commandBuffer = cmd->cmd,
    };
  }

  VkSubmitInfo2 submit_info = {
    .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2,
    .waitSemaphoreInfoCount = wait_count,
    .pWaitSemaphoreInfos = wait_semaphores,
    .signalSemaphoreInfoCount = info->signal_fence_count,
    .pSignalSemaphoreInfos = signal_semaphores,
    .commandBufferInfoCount = info->cmd_count,
    .pCommandBufferInfos = cmds,
  };

  ac_result res = ac_result_unknown_error;

  switch (device->vkQueueSubmit2(queue->queue, 1, &submit_info, VK_NULL_HANDLE))
  {
  case VK_SUCCESS:
  {
    for (uint32_t i = 0; i < info->wait_fence_count; ++i)
    {
      ac_fence fence_handle = info->wait_fences[i].fence;
      AC_FROM_HANDLE(fence, ac_vk_fence);
      if (fence->common.bits & ac_fence_present_bit)
      {
        fence->signaled = false;
      }
    }
    for (uint32_t i = 0; i < info->signal_fence_count; ++i)
    {
      ac_fence fence_handle = info->signal_fences[i].fence;
      AC_FROM_HANDLE(fence, ac_vk_fence);
      if (fence->common.bits & ac_fence_present_bit)
      {
        fence->signaled = true;
      }
    }
    res = ac_result_success;
    break;
  }
  case VK_ERROR_OUT_OF_HOST_MEMORY:
    res = ac_result_out_of_host_memory;
    break;
  case VK_ERROR_OUT_OF_DEVICE_MEMORY:
    res = ac_result_out_of_device_memory;
    break;
  case VK_ERROR_DEVICE_LOST:
    res = ac_result_device_lost;
    break;
  default:
    break;
  }

  ac_free(wait_semaphores);
  ac_free(signal_semaphores);
  ac_free(cmds);

  return res;
}

static ac_result
ac_vk_queue_present(ac_queue queue_handle, const ac_queue_present_info* info)
{
  AC_FROM_HANDLE2(device, queue_handle->device, ac_vk_device);
  AC_FROM_HANDLE2(swapchain, info->swapchain, ac_vk_swapchain);
  AC_FROM_HANDLE(queue, ac_vk_queue);

  VkSemaphore* wait_semaphores = NULL;

  if (info->wait_fence_count)
  {
    wait_semaphores = ac_alloc(info->wait_fence_count * sizeof(VkSemaphore));
  }

  for (uint32_t i = 0; i < info->wait_fence_count; ++i)
  {
    ac_fence fence_handle = info->wait_fences[i];
    AC_FROM_HANDLE(fence, ac_vk_fence);
    AC_ASSERT(
      !(fence->common.bits & ac_fence_present_bit) || fence->signaled ||
      ("present fence should be signaled before present" && false));
    wait_semaphores[i] = fence->semaphore;
  }

  VkPresentInfoKHR present_info = {
    .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
    .pNext = NULL,
    .waitSemaphoreCount = info->wait_fence_count,
    .pWaitSemaphores = wait_semaphores,
    .swapchainCount = 1,
    .pSwapchains = &swapchain->swapchain,
    .pImageIndices = &swapchain->common.image_index,
    .pResults = NULL,
  };

  ac_result res = ac_result_unknown_error;

  VkResult present_result =
    device->vkQueuePresentKHR(queue->queue, &present_info);

  switch (present_result)
  {
  case VK_SUCCESS:
  case VK_SUBOPTIMAL_KHR:
  case VK_ERROR_OUT_OF_DATE_KHR:
  {
    for (uint32_t i = 0; i < info->wait_fence_count; ++i)
    {
      ac_fence fence_handle = info->wait_fences[i];
      AC_FROM_HANDLE(fence, ac_vk_fence);
      if (fence->common.bits & ac_fence_present_bit)
      {
        fence->signaled = false;
      }
    }
    if (present_result == VK_ERROR_OUT_OF_DATE_KHR)
    {
      res = ac_result_out_of_date;
    }
    else
    {
      res = ac_result_success;
    }
    break;
  }
  default:
  {
    break;
  }
  }

  ac_free(wait_semaphores);

  return res;
}

static ac_result
ac_vk_create_fence(
  ac_device            device_handle,
  const ac_fence_info* info,
  ac_fence*            fence_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);

  AC_INIT_INTERNAL(fence, ac_vk_fence);

  VkSemaphoreTypeCreateInfo type_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_TYPE_CREATE_INFO,
    .semaphoreType = VK_SEMAPHORE_TYPE_TIMELINE,
    .initialValue = 0,
  };

  if (info->bits & ac_fence_present_bit)
  {
    type_info.semaphoreType = VK_SEMAPHORE_TYPE_BINARY;
    fence->signaled = false;
  }

  VkSemaphoreCreateInfo semaphore_create_info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
    .pNext = &type_info,
  };

  AC_VK_RIF(device->vkCreateSemaphore(
    device->device,
    &semaphore_create_info,
    &device->cpu_allocator,
    &fence->semaphore));

  return ac_result_success;
}

static void
ac_vk_destroy_fence(ac_device device_handle, ac_fence fence_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(fence, ac_vk_fence);

  device->vkDestroySemaphore(
    device->device,
    fence->semaphore,
    &device->cpu_allocator);
}

static ac_result
ac_vk_get_fence_value(
  ac_device device_handle,
  ac_fence  fence_handle,
  uint64_t* value)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(fence, ac_vk_fence);

  AC_VK_RIF(device->vkGetSemaphoreCounterValue(
    device->device,
    fence->semaphore,
    value));

  return ac_result_success;
}

static ac_result
ac_vk_signal_fence(
  ac_device device_handle,
  ac_fence  fence_handle,
  uint64_t  value)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(fence, ac_vk_fence);

  VkSemaphoreSignalInfo info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_SIGNAL_INFO,
    .semaphore = fence->semaphore,
    .value = value,
  };

  AC_VK_RIF(device->vkSignalSemaphore(device->device, &info));

  return ac_result_success;
}

static ac_result
ac_vk_wait_fence(ac_device device_handle, ac_fence fence_handle, uint64_t value)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(fence, ac_vk_fence);

  VkSemaphoreWaitInfo info = {
    .sType = VK_STRUCTURE_TYPE_SEMAPHORE_WAIT_INFO,
    .semaphoreCount = 1,
    .pSemaphores = &fence->semaphore,
    .pValues = &value,
  };

  AC_VK_RIF(device->vkWaitSemaphores(device->device, &info, UINT64_MAX));

  return ac_result_success;
}

static ac_result
ac_vk_select_swapchain_format(
  ac_vk_device*          device,
  const ac_vk_swapchain* swapchain,
  ac_swapchain_bits      bits,
  VkSurfaceFormatKHR*    out)
{
  uint32_t surface_format_count = 0;
  AC_VK_RIF(device->vkGetPhysicalDeviceSurfaceFormatsKHR(
    device->gpu,
    swapchain->surface,
    &surface_format_count,
    NULL));
  if (surface_format_count <= 0)
  {
    return ac_result_unknown_error;
  }

  VkSurfaceFormatKHR* surface_formats =
    ac_alloc(surface_format_count * sizeof(VkSurfaceFormatKHR));
  if (
    device->vkGetPhysicalDeviceSurfaceFormatsKHR(
      device->gpu,
      swapchain->surface,
      &surface_format_count,
      surface_formats) != VK_SUCCESS)
  {
    ac_free(surface_formats);
    return ac_result_unknown_error;
  }

  bool wants_hdr =
    device->enabled_color_spaces && (bits & ac_swapchain_wants_hdr_bit);

  bool wants_unorm = (bits & ac_swapchain_wants_unorm_format_bit);

  VkSurfaceFormatKHR selected_format = {
    .format = VK_FORMAT_UNDEFINED,
  };

  bool continue_searching = true;
  for (uint32_t i = 0; i < surface_format_count && continue_searching; ++i)
  {
    switch (surface_formats[i].colorSpace)
    {
    case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
    {
      selected_format = surface_formats[i];

      bool is_unorm =
        !ac_format_is_srgb(ac_format_from_vk(surface_formats[i].format));

      if (!wants_hdr && (is_unorm && wants_unorm))
      {
        continue_searching = false;
      }
      break;
    }
    case VK_COLOR_SPACE_HDR10_ST2084_EXT:
    {
      if (!wants_hdr)
      {
        continue;
      }

      selected_format = surface_formats[i];
      continue_searching = false;
      break;
    }
    default:
    {
      continue;
    }
    }
  }

  ac_free(surface_formats);

  if (selected_format.format == VK_FORMAT_UNDEFINED)
  {
    return ac_result_unknown_error;
  }

  *out = selected_format;

  return ac_result_success;
}

static ac_result
ac_vk_create_swapchain(
  ac_device                device_handle,
  const ac_swapchain_info* info,
  ac_swapchain*            swapchain_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_INIT_INTERNAL(swapchain, ac_vk_swapchain);

  ac_wsi wsi = *info->wsi;

  if (!wsi.create_vk_surface)
  {
    return ac_result_invalid_argument;
  }

  AC_RIF(wsi.create_vk_surface(
    wsi.user_data,
    (void*)device->instance,
    (void**)&swapchain->surface));

  AC_ASSERT(swapchain->surface);

  ac_queue queue_handle = info->queue;
  AC_FROM_HANDLE(queue, ac_vk_queue);

  VkBool32 support_surface = 0;
  AC_VK_RIF(device->vkGetPhysicalDeviceSurfaceSupportKHR(
    device->gpu,
    queue->family,
    swapchain->surface,
    &support_surface));

  AC_ASSERT(support_surface);

  uint32_t present_mode_count = 0;
  AC_VK_RIF(device->vkGetPhysicalDeviceSurfacePresentModesKHR(
    device->gpu,
    swapchain->surface,
    &present_mode_count,
    NULL));
  if (!present_mode_count)
  {
    return ac_result_unknown_error;
  }
  VkPresentModeKHR* present_modes =
    ac_alloc(present_mode_count * sizeof(VkPresentModeKHR));
  if (
    device->vkGetPhysicalDeviceSurfacePresentModesKHR(
      device->gpu,
      swapchain->surface,
      &present_mode_count,
      present_modes) != VK_SUCCESS)
  {
    ac_free(present_modes);
    return ac_result_unknown_error;
  }

  VkPresentModeKHR present_mode = present_modes[0];

  VkPresentModeKHR preffered_mode =
    info->vsync ? VK_PRESENT_MODE_FIFO_KHR : VK_PRESENT_MODE_MAILBOX_KHR;

  for (uint32_t i = 0; i < present_mode_count; ++i)
  {
    if (present_modes[i] == preffered_mode)
    {
      present_mode = preffered_mode;
      break;
    }
  }

  ac_free(present_modes);

  // determine present image count
  VkSurfaceCapabilitiesKHR surface_capabilities;
  AC_ZERO(surface_capabilities);
  AC_VK_RIF(device->vkGetPhysicalDeviceSurfaceCapabilitiesKHR(
    device->gpu,
    swapchain->surface,
    &surface_capabilities));

  uint32_t min_image_count = AC_CLAMP(
    info->min_image_count,
    surface_capabilities.minImageCount,
    surface_capabilities.maxImageCount);

  VkSurfaceFormatKHR surface_format;
  AC_RIF(ac_vk_select_swapchain_format(
    device,
    swapchain,
    info->bits,
    &surface_format));
  swapchain->common.color_space =
    ac_color_space_from_vk(surface_format.colorSpace);

  swapchain->common.queue = info->queue;

  VkSurfaceTransformFlagBitsKHR pre_transform =
    surface_capabilities.currentTransform;

  swapchain->common.vsync = info->vsync;

  uint32_t width = AC_CLAMP(
    info->width,
    surface_capabilities.minImageExtent.width,
    surface_capabilities.maxImageExtent.width);

  uint32_t height = AC_CLAMP(
    info->height,
    surface_capabilities.minImageExtent.height,
    surface_capabilities.maxImageExtent.height);

  if (width == 0 || height == 0)
  {
    return ac_result_not_ready;
  }

  VkSwapchainCreateInfoKHR swapchain_create_info = {
    .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
    .pNext = NULL,
    .flags = 0,
    .surface = swapchain->surface,
    .minImageCount = min_image_count,
    .imageFormat = surface_format.format,
    .imageColorSpace = surface_format.colorSpace,
    .imageExtent.width = width,
    .imageExtent.height = height,
    .imageArrayLayers = 1,
    .imageUsage =
      VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
    .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 1,
    .pQueueFamilyIndices = &queue->family,
    .preTransform = pre_transform,
    // TODO: choose composite alpha according to caps
    .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
    .presentMode = present_mode,
    .clipped = 1,
    .oldSwapchain = VK_NULL_HANDLE,
  };

  AC_VK_RIF(device->vkCreateSwapchainKHR(
    device->device,
    &swapchain_create_info,
    &device->cpu_allocator,
    &swapchain->swapchain));

  AC_VK_RIF(device->vkGetSwapchainImagesKHR(
    device->device,
    swapchain->swapchain,
    &swapchain->common.image_count,
    NULL));
  if (!swapchain->common.image_count)
  {
    return ac_result_unknown_error;
  }
  VkImage* swapchain_images =
    ac_alloc(swapchain->common.image_count * sizeof(VkImage));
  if (
    device->vkGetSwapchainImagesKHR(
      device->device,
      swapchain->swapchain,
      &swapchain->common.image_count,
      swapchain_images) != VK_SUCCESS)
  {
    ac_free(swapchain_images);
    return ac_result_unknown_error;
  }

  swapchain->common.images =
    ac_calloc(swapchain->common.image_count * sizeof(ac_image));

  VkImageViewCreateInfo image_view_create_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .viewType = VK_IMAGE_VIEW_TYPE_2D,
    .format = surface_format.format,
    .components =
      {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
      },
    .subresourceRange =
      {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1,
      },
  };

  for (uint32_t i = 0; i < swapchain->common.image_count; ++i)
  {
    ac_image* image_handle = &swapchain->common.images[i];
    AC_INIT_INTERNAL(image, ac_vk_image);

    image_view_create_info.image = swapchain_images[i];

    image->image = swapchain_images[i];
    image->common.width = width;
    image->common.height = height;
    image->common.format = ac_format_from_vk(surface_format.format);
    image->common.samples = 1;
    image->common.levels = 1;
    image->common.layers = 1;
    image->common.type = ac_image_type_2d;
    image->common.usage =
      ac_image_usage_transfer_dst_bit | ac_image_usage_attachment_bit;

    if (
      device->vkCreateImageView(
        device->device,
        &image_view_create_info,
        &device->cpu_allocator,
        &image->srv_view) != VK_SUCCESS)
    {
      ac_free(swapchain_images);
      return ac_result_unknown_error;
    }

    AC_VK_SET_OBJECT_NAME(
      device,
      "ac swapchain",
      VK_OBJECT_TYPE_IMAGE_VIEW,
      (uint64_t)image->srv_view);
  }

  ac_free(swapchain_images);

  return ac_result_success;
}

static void
ac_vk_destroy_swapchain(ac_device device_handle, ac_swapchain swapchain_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(swapchain, ac_vk_swapchain);

  if (swapchain->common.images)
  {
    for (uint32_t i = 0; i < swapchain->common.image_count; ++i)
    {
      if (!swapchain->common.images[i])
      {
        continue;
      }

      ac_image image_handle = swapchain->common.images[i];
      AC_FROM_HANDLE(image, ac_vk_image);

      device->vkDestroyImageView(
        device->device,
        image->srv_view,
        &device->cpu_allocator);
    }
  }

  device->vkDestroySwapchainKHR(
    device->device,
    swapchain->swapchain,
    &device->cpu_allocator);

  device->vkDestroySurfaceKHR(device->instance, swapchain->surface, NULL);
}

static ac_result
ac_vk_create_cmd_pool(
  ac_device               device_handle,
  const ac_cmd_pool_info* info,
  ac_cmd_pool*            cmd_pool_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);

  AC_INIT_INTERNAL(cmd_pool, ac_vk_cmd_pool);

  AC_FROM_HANDLE2(queue, info->queue, ac_vk_queue);

  VkCommandPoolCreateInfo cmd_pool_create_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
    .pNext = NULL,
    .queueFamilyIndex = queue->family,
  };

  AC_VK_RIF(device->vkCreateCommandPool(
    device->device,
    &cmd_pool_create_info,
    &device->cpu_allocator,
    &cmd_pool->cmd_pool));

  return ac_result_success;
}

static void
ac_vk_destroy_cmd_pool(ac_device device_handle, ac_cmd_pool pool_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(pool, ac_vk_cmd_pool);

  device->vkDestroyCommandPool(
    device->device,
    pool->cmd_pool,
    &device->cpu_allocator);
}

static ac_result
ac_vk_reset_cmd_pool(ac_device device_handle, ac_cmd_pool pool_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(pool, ac_vk_cmd_pool);

  AC_VK_RIF(device->vkResetCommandPool(device->device, pool->cmd_pool, 0));

  return ac_result_success;
}

static ac_result
ac_vk_create_cmd(
  ac_device   device_handle,
  ac_cmd_pool cmd_pool_handle,
  ac_cmd*     cmd_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(cmd_pool, ac_vk_cmd_pool);

  AC_INIT_INTERNAL(cmd, ac_vk_cmd);

  VkCommandBufferAllocateInfo cmd_buffer_allocate_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
    .pNext = NULL,
    .commandPool = cmd_pool->cmd_pool,
    .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
    .commandBufferCount = 1,
  };

  AC_VK_RIF(device->vkAllocateCommandBuffers(
    device->device,
    &cmd_buffer_allocate_info,
    &cmd->cmd));

  return ac_result_success;
}

static void
ac_vk_destroy_cmd(
  ac_device   device_handle,
  ac_cmd_pool _cmd_pool,
  ac_cmd      cmd_handle)
{
  AC_UNUSED(device_handle);
  AC_UNUSED(_cmd_pool);
  AC_UNUSED(cmd_handle);
}

static ac_result
ac_vk_begin_cmd(ac_cmd cmd_handle)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);

  VkCommandBufferBeginInfo cmd_buffer_begin_info = {
    .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
    .pNext = NULL,
    .pInheritanceInfo = NULL,
  };

  AC_VK_RIF(device->vkBeginCommandBuffer(cmd->cmd, &cmd_buffer_begin_info));
  return ac_result_success;
}

static ac_result
ac_vk_end_cmd(ac_cmd cmd_handle)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  AC_VK_RIF(device->vkEndCommandBuffer(cmd->cmd));
  return ac_result_success;
}

static ac_result
ac_vk_acquire_next_image(
  ac_device    device_handle,
  ac_swapchain swapchain_handle,
  ac_fence     fence_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(swapchain, ac_vk_swapchain);
  AC_FROM_HANDLE(fence, ac_vk_fence);

  if (fence->signaled)
  {
    AC_RIF(ac_vk_recreate_binary_fence(device_handle, fence));
  }

  VkResult result = device->vkAcquireNextImageKHR(
    device->device,
    swapchain->swapchain,
    UINT64_MAX,
    fence->semaphore,
    VK_NULL_HANDLE,
    &swapchain->common.image_index);

  switch (result)
  {
  case VK_SUCCESS:
  case VK_SUBOPTIMAL_KHR:
    if (fence)
    {
      fence->signaled = true;
    }
    return ac_result_success;
  case VK_NOT_READY:
    return ac_result_not_ready;
  default:
    break;
  }
  return ac_result_unknown_error;
}

static ac_result
ac_vk_create_shader(
  ac_device             device_handle,
  const ac_shader_info* info,
  ac_shader*            shader_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);

  AC_INIT_INTERNAL(shader, ac_vk_shader);

  uint32_t    size = 0;
  const void* code = ac_shader_compiler_get_bytecode(
    info->code,
    &size,
    ac_shader_bytecode_spirv);

  if (code == NULL)
  {
    return ac_result_unknown_error;
  }

  VkShaderModuleCreateInfo shader_create_info = {
    .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .codeSize = size,
    .pCode = (const uint32_t*)(code),
  };
  AC_VK_RIF(device->vkCreateShaderModule(
    device->device,
    &shader_create_info,
    &device->cpu_allocator,
    &shader->shader));

  AC_VK_SET_OBJECT_NAME(
    device,
    info->name,
    VK_OBJECT_TYPE_SHADER_MODULE,
    (uint64_t)shader->shader);

  return ac_result_success;
}

static void
ac_vk_destroy_shader(ac_device device_handle, ac_shader shader_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(shader, ac_vk_shader);

  device->vkDestroyShaderModule(
    device->device,
    shader->shader,
    &device->cpu_allocator);
}

static ac_result
ac_vk_create_dsl(
  ac_device                   device_handle,
  const ac_dsl_info_internal* info,
  ac_dsl*                     dsl_handle)
{
  AC_INIT_INTERNAL(dsl, ac_vk_dsl);

  AC_FROM_HANDLE(device, ac_vk_device);

  array_t(VkDescriptorSetLayoutBinding) bindings[ac_space_count];
  // array_t(VkDescriptorBindingFlags) binding_flags[ac_space_count];
  AC_ZERO(bindings);
  // AC_ZERO(binding_flags);

  for (uint32_t b = 0; b < info->binding_count; ++b)
  {
    const ac_shader_binding* in = &info->bindings[b];

    VkDescriptorSetLayoutBinding out = {
      .binding = ac_shader_compiler_get_shader_binding_index(in->type, in->reg),
      .descriptorCount = in->descriptor_count,
      .descriptorType = ac_descriptor_type_to_vk((ac_descriptor_type)in->type),
      .stageFlags = VK_SHADER_STAGE_ALL,
    };

    array_append(bindings[in->space], out);
    // array_append(
    //   binding_flags[in->space],
    //   VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT);
  }

  for (uint32_t space = 0; space < ac_space_count; ++space)
  {
    // VkDescriptorSetLayoutBindingFlagsCreateInfo flags_create_info = {
    //   .sType =
    //     VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
    //   .bindingCount = (uint32_t)(array_size(binding_flags[space])),
    //   .pBindingFlags = binding_flags[space],
    // };

    VkDescriptorSetLayoutCreateInfo dsl_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
      .bindingCount = (uint32_t)(array_size(bindings[space])),
      .pBindings = bindings[space],
      // .pNext = &flags_create_info,
    };

    AC_VK_RIF(device->vkCreateDescriptorSetLayout(
      device->device,
      &dsl_create_info,
      &device->cpu_allocator,
      &dsl->dsls[space]));

    AC_VK_SET_OBJECT_NAME(
      device,
      info->info.name,
      VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
      (uint64_t)(dsl->dsls[space]));
  }

  for (size_t i = 0; i < AC_COUNTOF(bindings); ++i)
  {
    array_free(bindings[i]);
  }

  return ac_result_success;
}

static void
ac_vk_destroy_dsl(ac_device device_handle, ac_dsl dsl_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(dsl, ac_vk_dsl);

  for (uint32_t i = 0; i < ac_space_count; ++i)
  {
    if (dsl->dsls[i])
    {
      device->vkDestroyDescriptorSetLayout(
        device->device,
        dsl->dsls[i],
        &device->cpu_allocator);
    }
  }
}

static ac_result
ac_vk_create_descriptor_buffer(
  ac_device                        device_handle,
  const ac_descriptor_buffer_info* info,
  ac_descriptor_buffer*            db_handle)
{
  AC_INIT_INTERNAL(db, ac_vk_descriptor_buffer);

  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE2(dsl, info->dsl, ac_vk_dsl);

  VkDescriptorPoolSize pool_sizes[] = {
    {VK_DESCRIPTOR_TYPE_SAMPLER, 0},
    {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 0},
    {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 0},
    {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0},
    {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0},
    {VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR, 0},
  };

  for (uint32_t b = 0; b < info->dsl->binding_count; ++b)
  {
    const ac_shader_binding* in = &info->dsl->bindings[b];

    switch (in->type)
    {
    case ac_descriptor_type_sampler:
      pool_sizes[0].descriptorCount +=
        info->max_sets[in->space] * in->descriptor_count;
      break;
    case ac_descriptor_type_srv_image:
      pool_sizes[1].descriptorCount +=
        info->max_sets[in->space] * in->descriptor_count;
      break;
    case ac_descriptor_type_uav_image:
      pool_sizes[2].descriptorCount +=
        info->max_sets[in->space] * in->descriptor_count;
      break;
    case ac_descriptor_type_cbv_buffer:
      pool_sizes[3].descriptorCount +=
        info->max_sets[in->space] * in->descriptor_count;
      break;
    case ac_descriptor_type_srv_buffer:
    case ac_descriptor_type_uav_buffer:
      pool_sizes[4].descriptorCount +=
        info->max_sets[in->space] * in->descriptor_count;
      break;
    case ac_descriptor_type_as:
      pool_sizes[5].descriptorCount +=
        info->max_sets[in->space] * in->descriptor_count;
      break;
    default:
      AC_ASSERT(false);
      break;
    }
  }

  uint32_t pool_size_count = 0;
  for (uint32_t i = 0; i < AC_COUNTOF(pool_sizes); ++i)
  {
    if (pool_sizes[i].descriptorCount)
    {
      pool_sizes[pool_size_count++] = pool_sizes[i];
    }
  }

  if (pool_size_count)
  {
    uint32_t total_sets = 0;

    for (uint32_t i = 0; i < ac_space_count; ++i)
    {
      total_sets += info->max_sets[i];
    }

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
      .pNext = NULL,
      .flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT,
      .poolSizeCount = pool_size_count,
      .pPoolSizes = pool_sizes,
      .maxSets = total_sets,
    };

    AC_VK_RIF(device->vkCreateDescriptorPool(
      device->device,
      &descriptor_pool_create_info,
      &device->cpu_allocator,
      &db->descriptor_pool));

    AC_VK_SET_OBJECT_NAME(
      device,
      info->name,
      VK_OBJECT_TYPE_DESCRIPTOR_POOL,
      (uint64_t)db->descriptor_pool);
  }

  for (uint32_t s = 0; s < ac_space_count; ++s)
  {
    uint32_t set_count = info->max_sets[s];

    if (!set_count)
    {
      continue;
    }

    if (dsl->dsls[s] == VK_NULL_HANDLE)
    {
      return ac_result_unknown_error;
    }

    db->sets[s] = ac_calloc(set_count * sizeof(VkDescriptorSet));

    VkDescriptorSetAllocateInfo set_info = {
      .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
      .pNext = NULL,
      .descriptorPool = db->descriptor_pool,
      .descriptorSetCount = 1,
      .pSetLayouts = &dsl->dsls[s],
    };

    for (uint32_t set = 0; set < set_count; ++set)
    {
      AC_VK_RIF(device->vkAllocateDescriptorSets(
        device->device,
        &set_info,
        &db->sets[s][set]));
    }
  }

  return ac_result_success;
}

static void
ac_vk_destroy_descriptor_buffer(
  ac_device            device_handle,
  ac_descriptor_buffer db_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(db, ac_vk_descriptor_buffer);

  for (uint32_t s = 0; s < ac_space_count; ++s)
  {
    ac_free(db->sets[s]);
  }

  device->vkDestroyDescriptorPool(
    device->device,
    db->descriptor_pool,
    &device->cpu_allocator);
}

static ac_result
ac_vk_create_compute_pipeline(
  ac_device               device_handle,
  const ac_pipeline_info* info,
  ac_pipeline*            pipeline_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE2(shader, info->compute.shader, ac_vk_shader);
  AC_FROM_HANDLE2(dsl, info->compute.dsl, ac_vk_dsl);

  AC_INIT_INTERNAL(pipeline, ac_vk_pipeline);

  AC_RIF(ac_vk_create_pipeline_layout(
    device,
    1,
    &info->compute.shader,
    dsl,
    &pipeline->push_constant_stage_flags,
    &pipeline->pipeline_layout));

  VkPipelineShaderStageCreateInfo shader_stage_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .stage = VK_SHADER_STAGE_COMPUTE_BIT,
    .module = shader->shader,
    .pName = ac_shader_compiler_get_entry_point_name(
      (ac_shader_compiler_stage)ac_shader_stage_compute),
    .pSpecializationInfo = NULL,
  };

  VkComputePipelineCreateInfo compute_pipeline_create_info = {
    .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
    .stage = shader_stage_create_info,
    .layout = pipeline->pipeline_layout,
  };

  AC_VK_RIF(device->vkCreateComputePipelines(
    device->device,
    VK_NULL_HANDLE,
    1,
    &compute_pipeline_create_info,
    &device->cpu_allocator,
    &pipeline->pipeline));

  AC_VK_SET_OBJECT_NAME(
    device,
    info->name,
    VK_OBJECT_TYPE_PIPELINE,
    (uint64_t)pipeline->pipeline);

  return ac_result_success;
}

static ac_result
ac_vk_create_raytracing_pipeline(
  ac_device               device_handle,
  const ac_pipeline_info* info,
  ac_pipeline*            pipeline_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_INIT_INTERNAL(pipeline, ac_vk_pipeline);

  const ac_raytracing_pipeline_info* raytracing = &info->raytracing;

  AC_FROM_HANDLE2(dsl, raytracing->dsl, ac_vk_dsl);

  AC_RIF(ac_vk_create_pipeline_layout(
    device,
    raytracing->shader_count,
    raytracing->shaders,
    dsl,
    &pipeline->push_constant_stage_flags,
    &pipeline->pipeline_layout));

  VkPipelineShaderStageCreateInfo* stages = ac_calloc(
    raytracing->shader_count * sizeof(VkPipelineShaderStageCreateInfo));

  for (uint32_t i = 0; i < raytracing->shader_count; ++i)
  {
    AC_FROM_HANDLE2(shader, raytracing->shaders[i], ac_vk_shader);

    ac_shader_compiler_stage stage =
      (ac_shader_compiler_stage)shader->common.stage;

    VkPipelineShaderStageCreateInfo* out = &stages[i];

    out->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    out->stage = ac_shader_stage_to_vk(shader->common.stage);
    out->module = shader->shader;
    out->pName = ac_shader_compiler_get_entry_point_name(stage);
  }

  VkRayTracingShaderGroupCreateInfoKHR* groups = ac_calloc(
    raytracing->group_count * sizeof(VkRayTracingShaderGroupCreateInfoKHR));

  for (uint32_t i = 0; i < raytracing->group_count; ++i)
  {
    const ac_raytracing_group_info*       in = &raytracing->groups[i];
    VkRayTracingShaderGroupCreateInfoKHR* out = &groups[i];

    out->sType = VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
    out->type = ac_raytracing_group_type_to_vk(in->type);
    out->generalShader = in->general;
    out->closestHitShader = in->closest_hit;
    out->anyHitShader = in->any_hit;
    out->intersectionShader = in->intersection;
  }

  VkRayTracingPipelineCreateInfoKHR pipeline_create_info = {
    .sType = VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR,
    .stageCount = raytracing->shader_count,
    .pStages = stages,
    .groupCount = raytracing->group_count,
    .pGroups = groups,
    .maxPipelineRayRecursionDepth = raytracing->max_ray_recursion_depth,
    .layout = pipeline->pipeline_layout,
  };

  AC_VK_RIF(device->vkCreateRayTracingPipelinesKHR(
    device->device,
    VK_NULL_HANDLE,
    VK_NULL_HANDLE,
    1,
    &pipeline_create_info,
    &device->cpu_allocator,
    &pipeline->pipeline));

  return ac_result_success;
}

static ac_result
ac_vk_create_graphics_pipeline(
  ac_device               device_handle,
  const ac_pipeline_info* info,
  ac_pipeline*            pipeline_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE2(dsl, info->graphics.dsl, ac_vk_dsl);

  AC_INIT_INTERNAL(pipeline, ac_vk_pipeline);

  const ac_graphics_pipeline_info* graphics = &info->graphics;

  VkFormat color_formats[AC_MAX_ATTACHMENT_COUNT];

  VkPipelineRenderingCreateInfoKHR rendering_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
    .colorAttachmentCount = graphics->color_attachment_count,
    .pColorAttachmentFormats = color_formats,
    .depthAttachmentFormat = ac_format_to_vk(graphics->depth_stencil_format),
  };

  for (uint32_t i = 0; i < graphics->color_attachment_count; ++i)
  {
    color_formats[i] = ac_format_to_vk(graphics->color_attachment_formats[i]);
  }

  const ac_shader shaders[] = {
    graphics->vertex_shader,
    graphics->pixel_shader,
  };

  uint32_t                        stage_count = 0;
  VkPipelineShaderStageCreateInfo stage_create_infos[AC_COUNTOF(shaders)];
  AC_ZERO(stage_create_infos);

  for (uint32_t i = 0; i < AC_COUNTOF(shaders); ++i)
  {
    if (!shaders[i])
    {
      continue;
    }

    AC_FROM_HANDLE2(shader, shaders[i], ac_vk_shader);

    ac_shader_compiler_stage stage =
      (ac_shader_compiler_stage)shaders[i]->stage;

    VkPipelineShaderStageCreateInfo* out = &stage_create_infos[stage_count++];

    out->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    out->stage = ac_shader_stage_to_vk(shaders[i]->stage);
    out->module = shader->shader;
    out->pName = ac_shader_compiler_get_entry_point_name(stage);
    out->pSpecializationInfo = NULL;
  }

  VkVertexInputBindingDescription
    binding_descriptions[AC_MAX_VERTEX_BINDING_COUNT];
  AC_ZERO(binding_descriptions);

  VkVertexInputAttributeDescription
    attribute_descriptions[AC_MAX_VERTEX_ATTRIBUTE_COUNT];
  AC_ZERO(attribute_descriptions);

  const ac_vertex_layout* vl = &graphics->vertex_layout;

  ac_shader_location semantic_location[ac_attribute_semantic_count];
  AC_RIF(ac_shader_compiler_get_locations(
    graphics->vertex_shader->reflection,
    semantic_location));

  for (uint32_t i = 0; i < vl->binding_count; ++i)
  {
    VkVertexInputBindingDescription* out = &binding_descriptions[i];
    const ac_vertex_binding_info*    in = &vl->bindings[i];

    out->binding = in->binding;
    out->inputRate = ac_input_rate_to_vk(in->input_rate);
    out->stride = in->stride;
  }

  uint32_t attribute_count = 0;
  for (uint32_t i = 0; i < vl->attribute_count; ++i)
  {
    const ac_vertex_attribute_info* in = &vl->attributes[i];
    if (semantic_location[in->semantic] == UINT8_MAX)
    {
      continue;
    }

    VkVertexInputAttributeDescription* out =
      &attribute_descriptions[attribute_count++];

    out->location = semantic_location[in->semantic];
    out->binding = in->binding;
    out->format = ac_format_to_vk(in->format);
    out->offset = in->offset;
  }

  VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    .vertexBindingDescriptionCount = vl->binding_count,
    .pVertexBindingDescriptions = binding_descriptions,
    .vertexAttributeDescriptionCount = attribute_count,
    .pVertexAttributeDescriptions = attribute_descriptions,
  };

  VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
    .topology = ac_primitive_topology_to_vk(graphics->topology),
    .primitiveRestartEnable = 0,
  };

  // Dynamic states
  VkViewport viewport;
  AC_ZERO(viewport);
  VkRect2D scissor;
  AC_ZERO(scissor);

  VkPipelineViewportStateCreateInfo viewport_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor,
  };

  VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .polygonMode =
      ac_polygon_mode_to_vk(graphics->rasterizer_info.polygon_mode),
    .cullMode = ac_cull_mode_to_vk(graphics->rasterizer_info.cull_mode),
    .frontFace = ac_front_face_to_vk(graphics->rasterizer_info.front_face),
    .lineWidth = 1.0f,
    .depthBiasEnable = graphics->rasterizer_info.depth_bias_enable,
    .depthBiasConstantFactor =
      (float)graphics->rasterizer_info.depth_bias_constant_factor,
    .depthBiasSlopeFactor = graphics->rasterizer_info.depth_bias_slope_factor,
  };

  VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = ac_samples_to_vk(graphics->samples),
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 1.0f,
  };

  VkPipelineColorBlendAttachmentState
    attachment_states[AC_MAX_ATTACHMENT_COUNT];
  AC_ZERO(attachment_states);

  const ac_blend_state_info* blend = &graphics->blend_state_info;
  for (uint32_t i = 0; i < graphics->color_attachment_count; ++i)
  {
    VkPipelineColorBlendAttachmentState* out = &attachment_states[i];
    const ac_blend_attachment_state*     in = &blend->attachment_states[i];

    out->blendEnable = (in->src_factor != ac_blend_factor_zero) ||
                       (in->src_alpha_factor != ac_blend_factor_zero) ||
                       (in->dst_factor != ac_blend_factor_zero) ||
                       (in->dst_alpha_factor != ac_blend_factor_zero);
    out->srcColorBlendFactor = ac_blend_factor_to_vk(in->src_factor);
    out->dstColorBlendFactor = ac_blend_factor_to_vk(in->dst_factor);
    out->colorBlendOp = ac_blend_op_to_vk(in->op);
    out->srcAlphaBlendFactor = ac_blend_factor_to_vk(in->src_alpha_factor);
    out->dstAlphaBlendFactor = ac_blend_factor_to_vk(in->dst_alpha_factor);
    out->alphaBlendOp = ac_blend_op_to_vk(in->alpha_op);

    uint32_t discard_bits = graphics->color_attachment_discard_bits[i];

    if ((discard_bits & ac_channel_r_bit) == 0)
    {
      out->colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
    }
    if ((discard_bits & ac_channel_g_bit) == 0)
    {
      out->colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
    }
    if ((discard_bits & ac_channel_b_bit) == 0)
    {
      out->colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
    }
    if ((discard_bits & ac_channel_a_bit) == 0)
    {
      out->colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
    }
  }

  VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = graphics->color_attachment_count,
    .pAttachments = attachment_states,
  };

  VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable =
      graphics->depth_state_info.depth_test ? VK_TRUE : VK_FALSE,
    .depthWriteEnable =
      graphics->depth_state_info.depth_write ? VK_TRUE : VK_FALSE,
    .depthCompareOp =
      graphics->depth_state_info.depth_test
        ? ac_compare_op_to_vk(graphics->depth_state_info.compare_op)
        : VK_COMPARE_OP_ALWAYS,
    .depthBoundsTestEnable = VK_FALSE,
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 1.0f,
    .stencilTestEnable = VK_FALSE,
  };

  VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_SCISSOR,
    VK_DYNAMIC_STATE_VIEWPORT,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = AC_COUNTOF(dynamic_states),
    .pDynamicStates = dynamic_states,
  };

  AC_RIF(ac_vk_create_pipeline_layout(
    device,
    AC_COUNTOF(shaders),
    shaders,
    dsl,
    &pipeline->push_constant_stage_flags,
    &pipeline->pipeline_layout));

  VkGraphicsPipelineCreateInfo pipeline_create_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = stage_count,
    .pStages = stage_create_infos,
    .pVertexInputState = &vertex_input_state_create_info,
    .pInputAssemblyState = &input_assembly_state_create_info,
    .pViewportState = &viewport_state_create_info,
    .pMultisampleState = &multisample_state_create_info,
    .pRasterizationState = &rasterization_state_create_info,
    .pColorBlendState = &color_blend_state_create_info,
    .pDepthStencilState = &depth_stencil_state_create_info,
    .pDynamicState = &dynamic_state_create_info,
    .layout = pipeline->pipeline_layout,
    .pNext = &rendering_info,
  };
  AC_VK_RIF(device->vkCreateGraphicsPipelines(
    device->device,
    VK_NULL_HANDLE,
    1,
    &pipeline_create_info,
    &device->cpu_allocator,
    &pipeline->pipeline));

  AC_VK_SET_OBJECT_NAME(
    device,
    info->name,
    VK_OBJECT_TYPE_PIPELINE,
    (uint64_t)pipeline->pipeline);

  return ac_result_success;
}

static ac_result
ac_vk_create_mesh_pipeline(
  ac_device               device_handle,
  const ac_pipeline_info* info,
  ac_pipeline*            pipeline_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE2(dsl, info->mesh.dsl, ac_vk_dsl);

  AC_INIT_INTERNAL(pipeline, ac_vk_pipeline);

  const ac_mesh_pipeline_info* mesh = &info->mesh;

  VkFormat color_formats[AC_MAX_ATTACHMENT_COUNT];

  VkPipelineRenderingCreateInfoKHR rendering_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
    .colorAttachmentCount = mesh->color_attachment_count,
    .pColorAttachmentFormats = color_formats,
    .depthAttachmentFormat = ac_format_to_vk(mesh->depth_stencil_format),
  };

  for (uint32_t i = 0; i < mesh->color_attachment_count; ++i)
  {
    color_formats[i] = ac_format_to_vk(mesh->color_attachment_formats[i]);
  }

  const ac_shader shaders[] = {
    mesh->task_shader,
    mesh->mesh_shader,
    mesh->pixel_shader,
  };

  uint32_t                        stage_count = 0;
  VkPipelineShaderStageCreateInfo stage_create_infos[AC_COUNTOF(shaders)];
  AC_ZERO(stage_create_infos);

  for (uint32_t i = 0; i < AC_COUNTOF(shaders); ++i)
  {
    if (!shaders[i])
    {
      continue;
    }

    AC_FROM_HANDLE2(shader, shaders[i], ac_vk_shader);

    VkPipelineShaderStageCreateInfo* out = &stage_create_infos[stage_count++];

    ac_shader_compiler_stage stage =
      (ac_shader_compiler_stage)shaders[i]->stage;

    out->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    out->stage = ac_shader_stage_to_vk(shaders[i]->stage);
    out->module = shader->shader;
    out->pName = ac_shader_compiler_get_entry_point_name(stage);
    out->pSpecializationInfo = NULL;
  }

  // Dynamic states
  VkViewport viewport;
  AC_ZERO(viewport);
  VkRect2D scissor;
  AC_ZERO(scissor);

  VkPipelineViewportStateCreateInfo viewport_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
    .viewportCount = 1,
    .pViewports = &viewport,
    .scissorCount = 1,
    .pScissors = &scissor,
  };

  VkPipelineRasterizationStateCreateInfo rasterization_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
    .polygonMode = ac_polygon_mode_to_vk(mesh->rasterizer_info.polygon_mode),
    .cullMode = ac_cull_mode_to_vk(mesh->rasterizer_info.cull_mode),
    .frontFace = ac_front_face_to_vk(mesh->rasterizer_info.front_face),
    .lineWidth = 1.0f,
    .depthBiasEnable = mesh->rasterizer_info.depth_bias_enable,
    .depthBiasConstantFactor =
      (float)mesh->rasterizer_info.depth_bias_constant_factor,
    .depthBiasSlopeFactor = mesh->rasterizer_info.depth_bias_slope_factor,
  };

  VkPipelineMultisampleStateCreateInfo multisample_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
    .rasterizationSamples = ac_samples_to_vk(mesh->samples),
    .sampleShadingEnable = VK_FALSE,
    .minSampleShading = 1.0f,
  };

  VkPipelineColorBlendAttachmentState
    attachment_states[AC_MAX_ATTACHMENT_COUNT];
  AC_ZERO(attachment_states);

  const ac_blend_state_info* blend = &mesh->blend_state_info;
  for (uint32_t i = 0; i < mesh->color_attachment_count; ++i)
  {
    VkPipelineColorBlendAttachmentState* out = &attachment_states[i];
    const ac_blend_attachment_state*     in = &blend->attachment_states[i];

    out->blendEnable = (in->src_factor != ac_blend_factor_zero) ||
                       (in->src_alpha_factor != ac_blend_factor_zero) ||
                       (in->dst_factor != ac_blend_factor_zero) ||
                       (in->dst_alpha_factor != ac_blend_factor_zero);
    out->srcColorBlendFactor = ac_blend_factor_to_vk(in->src_factor);
    out->dstColorBlendFactor = ac_blend_factor_to_vk(in->dst_factor);
    out->colorBlendOp = ac_blend_op_to_vk(in->op);
    out->srcAlphaBlendFactor = ac_blend_factor_to_vk(in->src_alpha_factor);
    out->dstAlphaBlendFactor = ac_blend_factor_to_vk(in->dst_alpha_factor);
    out->alphaBlendOp = ac_blend_op_to_vk(in->alpha_op);

    uint32_t discard_bits = mesh->color_attachment_discard_bits[i];

    if ((discard_bits & ac_channel_r_bit) == 0)
    {
      out->colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
    }
    if ((discard_bits & ac_channel_g_bit) == 0)
    {
      out->colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
    }
    if ((discard_bits & ac_channel_b_bit) == 0)
    {
      out->colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
    }
    if ((discard_bits & ac_channel_a_bit) == 0)
    {
      out->colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
    }
  }

  VkPipelineColorBlendStateCreateInfo color_blend_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
    .attachmentCount = mesh->color_attachment_count,
    .pAttachments = attachment_states,
  };

  VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    .depthTestEnable = mesh->depth_state_info.depth_test ? VK_TRUE : VK_FALSE,
    .depthWriteEnable = mesh->depth_state_info.depth_write ? VK_TRUE : VK_FALSE,
    .depthCompareOp = mesh->depth_state_info.depth_test
                        ? ac_compare_op_to_vk(mesh->depth_state_info.compare_op)
                        : VK_COMPARE_OP_ALWAYS,
    .depthBoundsTestEnable = VK_FALSE,
    .minDepthBounds = 0.0f,
    .maxDepthBounds = 1.0f,
    .stencilTestEnable = VK_FALSE,
  };

  VkDynamicState dynamic_states[] = {
    VK_DYNAMIC_STATE_SCISSOR,
    VK_DYNAMIC_STATE_VIEWPORT,
  };

  VkPipelineDynamicStateCreateInfo dynamic_state_create_info = {
    .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
    .dynamicStateCount = AC_COUNTOF(dynamic_states),
    .pDynamicStates = dynamic_states,
  };

  AC_RIF(ac_vk_create_pipeline_layout(
    device,
    AC_COUNTOF(shaders),
    shaders,
    dsl,
    &pipeline->push_constant_stage_flags,
    &pipeline->pipeline_layout));

  VkGraphicsPipelineCreateInfo pipeline_create_info = {
    .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
    .stageCount = stage_count,
    .pStages = stage_create_infos,
    .pVertexInputState = NULL,
    .pInputAssemblyState = NULL,
    .pViewportState = &viewport_state_create_info,
    .pMultisampleState = &multisample_state_create_info,
    .pRasterizationState = &rasterization_state_create_info,
    .pColorBlendState = &color_blend_state_create_info,
    .pDepthStencilState = &depth_stencil_state_create_info,
    .pDynamicState = &dynamic_state_create_info,
    .layout = pipeline->pipeline_layout,
    .pNext = &rendering_info,
  };
  AC_VK_RIF(device->vkCreateGraphicsPipelines(
    device->device,
    VK_NULL_HANDLE,
    1,
    &pipeline_create_info,
    &device->cpu_allocator,
    &pipeline->pipeline));

  AC_VK_SET_OBJECT_NAME(
    device,
    info->name,
    VK_OBJECT_TYPE_PIPELINE,
    (uint64_t)pipeline->pipeline);

  return ac_result_success;
}

static void
ac_vk_destroy_pipeline(ac_device device_handle, ac_pipeline pipeline_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(pipeline, ac_vk_pipeline);

  device->vkDestroyPipelineLayout(
    device->device,
    pipeline->pipeline_layout,
    &device->cpu_allocator);

  device->vkDestroyPipeline(
    device->device,
    pipeline->pipeline,
    &device->cpu_allocator);
}

static ac_result
ac_vk_create_buffer(
  ac_device             device_handle,
  const ac_buffer_info* info,
  ac_buffer*            buffer_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);

  AC_INIT_INTERNAL(buffer, ac_vk_buffer);

  return ac_vk_create_buffer2(
    device,
    info,
    0,
    &buffer->buffer,
    &buffer->allocation);
}

static void
ac_vk_destroy_buffer(ac_device device_handle, ac_buffer buffer_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(buffer, ac_vk_buffer);

  if (buffer->allocation && buffer->common.mapped_memory)
  {
    vmaUnmapMemory(device->gpu_allocator, buffer->allocation);
  }
  vmaDestroyBuffer(device->gpu_allocator, buffer->buffer, buffer->allocation);
}

static ac_result
ac_vk_create_sampler(
  ac_device              device_handle,
  const ac_sampler_info* info,
  ac_sampler*            sampler_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);

  AC_INIT_INTERNAL(sampler, ac_vk_sampler);

  VkSamplerCreateInfo sampler_create_info = {
    .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .magFilter = ac_filter_to_vk(info->mag_filter),
    .minFilter = ac_filter_to_vk(info->min_filter),
    .mipmapMode = ac_sampler_mipmap_mode_to_vk(info->mipmap_mode),
    .addressModeU = ac_sampler_address_mode_to_vk(info->address_mode_u),
    .addressModeV = ac_sampler_address_mode_to_vk(info->address_mode_v),
    .addressModeW = ac_sampler_address_mode_to_vk(info->address_mode_w),
    .mipLodBias = info->mip_lod_bias,
    .anisotropyEnable = info->anisotropy_enable,
    .maxAnisotropy = (float)info->max_anisotropy,
    .compareEnable = info->compare_enable,
    .compareOp = ac_compare_op_to_vk(info->compare_op),
    .minLod = info->min_lod,
    .maxLod = info->max_lod,
    .unnormalizedCoordinates = 0,
    .borderColor = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
  };

  AC_VK_RIF(device->vkCreateSampler(
    device->device,
    &sampler_create_info,
    &device->cpu_allocator,
    &sampler->sampler));

  return ac_result_success;
}

static void
ac_vk_destroy_sampler(ac_device device_handle, ac_sampler sampler_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(sampler, ac_vk_sampler);

  device->vkDestroySampler(
    device->device,
    sampler->sampler,
    &device->cpu_allocator);
}

static ac_result
ac_vk_create_image(
  ac_device            device_handle,
  const ac_image_info* info,
  ac_image*            image_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);

  AC_INIT_INTERNAL(image, ac_vk_image);

  VmaAllocationCreateInfo allocation_create_info = {
    .usage = VMA_MEMORY_USAGE_GPU_ONLY,
    .pUserData = ac_const_cast(info->name),
    .flags = VMA_ALLOCATION_CREATE_USER_DATA_COPY_STRING_BIT |
             VMA_ALLOCATION_CREATE_WITHIN_BUDGET_BIT,
  };

  VkImageCreateInfo image_create_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .format = ac_format_to_vk(info->format),
    .extent.width = info->width,
    .extent.height = info->height,
    .extent.depth = 1,
    .mipLevels = info->levels,
    .arrayLayers = info->layers,
    .samples = ac_samples_to_vk(info->samples),
    .tiling = VK_IMAGE_TILING_OPTIMAL,
    .usage = ac_image_usage_bits_to_vk(info->usage, info->format),
    .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    .queueFamilyIndexCount = 0,
    .pQueueFamilyIndices = NULL,
    .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
  };

  switch (info->type)
  {
  case ac_image_type_1d:
  case ac_image_type_1d_array:
    image_create_info.imageType = VK_IMAGE_TYPE_1D;
    break;
  case ac_image_type_cube:
    AC_ASSERT(info->layers == 6);
    image_create_info.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    break;
  case ac_image_type_2d:
  case ac_image_type_2d_array:
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    break;
  default:
    AC_ASSERT(false);
    return ac_result_invalid_argument;
  }

  AC_VK_RIF(vmaCreateImage(
    device->gpu_allocator,
    &image_create_info,
    &allocation_create_info,
    &image->image,
    &image->allocation,
    NULL));

  AC_VK_SET_OBJECT_NAME(
    device,
    info->name,
    VK_OBJECT_TYPE_IMAGE,
    (uint64_t)image->image);

  image->common.format = info->format;
  image->common.levels = info->levels;
  image->common.layers = info->layers;

  VkImageViewCreateInfo sampled_view_create_info = {
    .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
    .pNext = NULL,
    .flags = 0,
    .image = image->image,
    .format = image_create_info.format,
    .subresourceRange =
      {
        .aspectMask = ac_format_to_vk_aspect_flags(info->format),
        .baseArrayLayer = 0,
        .levelCount = info->levels,
        .layerCount = info->layers,
      },
    .components =
      {
        .r = VK_COMPONENT_SWIZZLE_IDENTITY,
        .g = VK_COMPONENT_SWIZZLE_IDENTITY,
        .b = VK_COMPONENT_SWIZZLE_IDENTITY,
        .a = VK_COMPONENT_SWIZZLE_IDENTITY,
      },
  };

  switch (info->type)
  {
  case ac_image_type_1d:
    sampled_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_1D;
    break;
  case ac_image_type_1d_array:
    sampled_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_1D_ARRAY;
    break;
  case ac_image_type_cube:
    AC_ASSERT(info->layers == 6);
    sampled_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    break;
  case ac_image_type_2d:
    sampled_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    break;
  case ac_image_type_2d_array:
    sampled_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    break;
  default:
    AC_ASSERT(false);
    return ac_result_invalid_argument;
  }

  AC_VK_RIF(device->vkCreateImageView(
    device->device,
    &sampled_view_create_info,
    &device->cpu_allocator,
    &image->srv_view));

  AC_VK_SET_OBJECT_NAME(
    device,
    info->name,
    VK_OBJECT_TYPE_IMAGE_VIEW,
    (uint64_t)image->srv_view);

  if (info->usage & ac_image_usage_uav_bit)
  {
    image->uav_views = ac_alloc(sizeof(VkImageView) * info->levels);

    VkImageViewCreateInfo storage_view_create_info = sampled_view_create_info;
    storage_view_create_info.subresourceRange.levelCount = 1;

    if (storage_view_create_info.viewType == VK_IMAGE_VIEW_TYPE_CUBE)
    {
      storage_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
    }

    for (uint32_t mip = 0; mip < info->levels; ++mip)
    {
      storage_view_create_info.subresourceRange.baseMipLevel = mip;

      AC_VK_RIF(device->vkCreateImageView(
        device->device,
        &storage_view_create_info,
        &device->cpu_allocator,
        &image->uav_views[mip]));

      AC_VK_SET_OBJECT_NAME(
        device,
        info->name,
        VK_OBJECT_TYPE_IMAGE_VIEW,
        (uint64_t)image->uav_views[mip]);
    }
  }

  return ac_result_success;
}

static void
ac_vk_destroy_image(ac_device device_handle, ac_image image_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(image, ac_vk_image);

  if (image->uav_views)
  {
    for (uint32_t mip = 0; mip < image->common.levels; ++mip)
    {
      device->vkDestroyImageView(
        device->device,
        image->uav_views[mip],
        &device->cpu_allocator);
    }
    ac_free(image->uav_views);
  }

  device->vkDestroyImageView(
    device->device,
    image->srv_view,
    &device->cpu_allocator);

  vmaDestroyImage(device->gpu_allocator, image->image, image->allocation);
}

static void
ac_vk_update_descriptor(
  ac_device                  device_handle,
  ac_descriptor_buffer       db_handle,
  ac_space                   space,
  uint32_t                   set,
  const ac_descriptor_write* info)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(db, ac_vk_descriptor_buffer);

  VkDescriptorSet descriptor_set = db->sets[space][set];

  VkWriteDescriptorSetAccelerationStructureKHR acceleration_write = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR,
    .accelerationStructureCount = info->count,
  };

  ac_shader_descriptor_type type = (ac_shader_descriptor_type)info->type;
  uint32_t                  binding =
    ac_shader_compiler_get_shader_binding_index(type, info->reg);

  VkWriteDescriptorSet write = {
    .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    .dstBinding = binding,
    .descriptorCount = info->count,
    .dstSet = descriptor_set,
    .descriptorType = ac_descriptor_type_to_vk(info->type),
  };

  VkDescriptorBufferInfo*     buffer_descriptors = NULL;
  VkDescriptorImageInfo*      image_descriptors = NULL;
  VkAccelerationStructureKHR* acceleration_structures = NULL;

  switch (info->type)
  {
  case ac_descriptor_type_cbv_buffer:
  case ac_descriptor_type_srv_buffer:
  case ac_descriptor_type_uav_buffer:
  {
    buffer_descriptors =
      ac_calloc(sizeof(VkDescriptorBufferInfo) * info->count);

    for (uint32_t j = 0; j < info->count; ++j)
    {
      const ac_descriptor* descriptor = &info->descriptors[j];

      if (!descriptor->buffer)
      {
        continue;
      }

      AC_FROM_HANDLE2(buffer, descriptor->buffer, ac_vk_buffer);
      buffer_descriptors[j].buffer = buffer->buffer;
      buffer_descriptors[j].offset = descriptor->offset;
      buffer_descriptors[j].range =
        descriptor->range == AC_WHOLE_SIZE ? VK_WHOLE_SIZE : descriptor->range;
    }

    write.pBufferInfo = buffer_descriptors;
    break;
  }
  case ac_descriptor_type_srv_image:
  case ac_descriptor_type_uav_image:
  {
    image_descriptors = ac_calloc(sizeof(VkDescriptorImageInfo) * info->count);

    for (uint32_t j = 0; j < info->count; ++j)
    {
      const ac_descriptor* descriptor = &info->descriptors[j];

      if (!descriptor->image)
      {
        continue;
      }

      AC_FROM_HANDLE2(image, descriptor->image, ac_vk_image);

      AC_ASSERT(descriptor->level < descriptor->image->levels);

      if (info->type == ac_descriptor_type_srv_image)
      {
        image_descriptors[j].imageLayout =
          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image_descriptors[j].imageView = image->srv_view;
      }
      else
      {
        AC_ASSERT(image->common.usage & ac_image_usage_uav_bit);
        AC_ASSERT(image->uav_views);
        image_descriptors[j].imageView = image->uav_views[descriptor->level];
        image_descriptors[j].imageLayout = VK_IMAGE_LAYOUT_GENERAL;
      }
    }

    write.pImageInfo = image_descriptors;
    break;
  }
  case ac_descriptor_type_sampler:
  {
    image_descriptors = ac_calloc(sizeof(VkDescriptorImageInfo) * info->count);

    for (uint32_t j = 0; j < info->count; ++j)
    {
      const ac_descriptor* descriptor = &info->descriptors[j];

      if (!descriptor->sampler)
      {
        continue;
      }

      AC_FROM_HANDLE2(sampler, descriptor->sampler, ac_vk_sampler);
      image_descriptors[j].sampler = sampler->sampler;
    }

    write.pImageInfo = image_descriptors;
    break;
  }
  case ac_descriptor_type_as:
  {
    acceleration_structures =
      ac_calloc(sizeof(VkAccelerationStructureKHR) * info->count);

    acceleration_write.pAccelerationStructures = acceleration_structures;

    for (uint32_t j = 0; j < info->count; ++j)
    {
      const ac_descriptor* descriptor = &info->descriptors[j];

      if (!descriptor->as)
      {
        continue;
      }

      AC_FROM_HANDLE2(as, descriptor->as, ac_vk_as);

      acceleration_structures[j] = as->as;
    }

    write.pNext = &acceleration_write;
    break;
  }
  default:
  {
    AC_ASSERT(false);
    break;
  }
  }

  device->vkUpdateDescriptorSets(device->device, 1, &write, 0, NULL);

  ac_free(buffer_descriptors);
  ac_free(image_descriptors);
  ac_free(acceleration_structures);
}

static ac_result
ac_vk_create_sbt(
  ac_device          device_handle,
  const ac_sbt_info* info,
  ac_sbt*            sbt_handle)
{
  // TODO: IMPLEMENT
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE2(pipeline, info->pipeline, ac_vk_pipeline);

  AC_INIT_INTERNAL(sbt, ac_vk_sbt);

  uint64_t handle_size = device->shader_group_handle_size;
  uint64_t handle_aligned_size =
    AC_ALIGN_UP(handle_size, device->shader_group_handle_alignment);
  uint32_t group_count = info->group_count;
  uint64_t sbt_size =
    (uint64_t)group_count *
    AC_ALIGN_UP(handle_aligned_size, device->shader_group_base_alignment);

  ac_buffer_info buffer_info = {
    .memory_usage = ac_memory_usage_cpu_to_gpu,
    .usage = 0,
    .size = sbt_size,
    .name = info->name,
  };

  AC_RIF(ac_vk_create_buffer2(
    device,
    &buffer_info,
    VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    &sbt->buffer,
    &sbt->allocation));

  uint8_t* handles = (uint8_t*)ac_calloc(sbt_size);
  AC_VK_RIF(device->vkGetRayTracingShaderGroupHandlesKHR(
    device->device,
    pipeline->pipeline,
    0,
    group_count,
    sbt_size,
    handles));

  uint8_t* mem = NULL;
  AC_VK_RIF(vmaMapMemory(device->gpu_allocator, sbt->allocation, (void**)&mem));

  memcpy(mem, handles, handle_aligned_size);
  mem += AC_ALIGN_UP(handle_aligned_size, device->shader_group_base_alignment);
  handles += handle_aligned_size;
  memcpy(mem, handles, handle_aligned_size);
  mem += AC_ALIGN_UP(handle_aligned_size, device->shader_group_base_alignment);
  handles += handle_aligned_size;
  memcpy(mem, handles, handle_aligned_size);

  vmaUnmapMemory(device->gpu_allocator, sbt->allocation);

  VkDeviceAddress base_address = ac_vk_get_buffer_address(device, sbt->buffer);

  sbt->raygen.deviceAddress = base_address;
  sbt->raygen.size = handle_aligned_size;
  sbt->raygen.stride = handle_aligned_size;

  sbt->miss.deviceAddress =
    sbt->raygen.deviceAddress +
    AC_ALIGN_UP(handle_aligned_size, device->shader_group_base_alignment);
  sbt->miss.size = handle_aligned_size;
  sbt->miss.stride = handle_aligned_size;

  sbt->hit.deviceAddress =
    sbt->miss.deviceAddress +
    AC_ALIGN_UP(handle_aligned_size, device->shader_group_base_alignment);
  sbt->hit.size = handle_aligned_size;
  sbt->hit.stride = handle_aligned_size;

  return ac_result_success;
}

static void
ac_vk_destroy_sbt(ac_device device_handle, ac_sbt sbt_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(sbt, ac_vk_sbt);

  vmaDestroyBuffer(device->gpu_allocator, sbt->buffer, sbt->allocation);
}

static ac_result
ac_vk_create_blas(
  ac_device         device_handle,
  const ac_as_info* info,
  ac_as*            as_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_INIT_INTERNAL(as, ac_vk_as);

  as->geometry_count = info->count;
  as->geometries =
    ac_calloc(info->count * sizeof(VkAccelerationStructureGeometryKHR));

  for (uint32_t i = 0; i < info->count; ++i)
  {
    const ac_as_geometry*               src = &info->geometries[i];
    VkAccelerationStructureGeometryKHR* dst = &as->geometries[i];

    dst->sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
    dst->flags = ac_geometry_bits_to_vk(src->bits);
    dst->geometryType = ac_geometry_type_to_vk(src->type);

    switch (src->type)
    {
    case ac_geometry_type_triangles:
    {
      const ac_as_geometry_triangles* src_triangles = &src->triangles;
      VkAccelerationStructureGeometryTrianglesDataKHR* dst_triangles =
        &dst->geometry.triangles;

      AC_FROM_HANDLE2(
        vertex_buffer,
        src_triangles->vertex.buffer,
        ac_vk_buffer);
      AC_FROM_HANDLE2(index_buffer, src_triangles->index.buffer, ac_vk_buffer);
      AC_FROM_HANDLE2(
        transform_buffer,
        src_triangles->transform.buffer,
        ac_vk_buffer);

      dst_triangles->sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
      dst_triangles->vertexFormat =
        ac_format_to_vk(src_triangles->vertex_format);
      dst_triangles->vertexData.deviceAddress =
        ac_vk_get_buffer_address(device, vertex_buffer->buffer) +
        src_triangles->vertex.offset;
      dst_triangles->vertexStride = src_triangles->vertex_stride;
      dst_triangles->maxVertex = src_triangles->vertex_count - 1;
      dst_triangles->indexType = ac_index_type_to_vk(src_triangles->index_type);
      dst_triangles->indexData.deviceAddress =
        ac_vk_get_buffer_address(device, index_buffer->buffer) +
        src_triangles->index.offset;

      if (transform_buffer)
      {
        dst_triangles->transformData.deviceAddress =
          ac_vk_get_buffer_address(device, transform_buffer->buffer) +
          src_triangles->transform.offset;
      }

      break;
    }
    case ac_geometry_type_aabbs:
    {
      const ac_as_geometry_aabs*                   src_aabs = &src->aabs;
      VkAccelerationStructureGeometryAabbsDataKHR* dst_aabs =
        &dst->geometry.aabbs;

      AC_FROM_HANDLE2(buffer, src_aabs->buffer, ac_vk_buffer);

      dst_aabs->sType =
        VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_AABBS_DATA_KHR;
      dst_aabs->stride = src_aabs->stride;
      dst_aabs->data.deviceAddress =
        ac_vk_get_buffer_address(device, buffer->buffer) + src_aabs->offset;

      break;
    }
    default:
    {
      return ac_result_invalid_argument;
    }
    }
  }

  uint64_t size = 0;

  {
    VkAccelerationStructureBuildGeometryInfoKHR build_info = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
      .type = ac_as_type_to_vk(info->type),
      .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
      .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
      .geometryCount = as->geometry_count,
      .pGeometries = as->geometries,
    };

    VkAccelerationStructureBuildSizesInfoKHR build_sizes = {
      .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
    };

    device->vkGetAccelerationStructureBuildSizesKHR(
      device->device,
      VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
      &build_info,
      &build_info.geometryCount,
      &build_sizes);

    size = build_sizes.accelerationStructureSize;

    as->common.scratch_size = build_sizes.buildScratchSize;
  }

  ac_buffer_info buffer_info = {
    .memory_usage = ac_memory_usage_gpu_only,
    .size = size,
    .usage = 0,
    .name = info->name,
  };

  AC_RIF(ac_vk_create_buffer2(
    device,
    &buffer_info,
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    &as->buffer,
    &as->allocation));

  VkAccelerationStructureCreateInfoKHR as_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
    .buffer = as->buffer,
    .size = size,
    .type = ac_as_type_to_vk(info->type),
  };

  AC_VK_RIF(device->vkCreateAccelerationStructureKHR(
    device->device,
    &as_info,
    &device->cpu_allocator,
    &as->as));

  return ac_result_success;
}

static ac_result
ac_vk_create_tlas(
  ac_device         device_handle,
  const ac_as_info* info,
  ac_as*            as_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_INIT_INTERNAL(as, ac_vk_as);

  AC_FROM_HANDLE2(buffer, info->instances_buffer, ac_vk_buffer);

  VkAccelerationStructureGeometryKHR geometry = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR,
    .flags = 0,
    .geometryType = VK_GEOMETRY_TYPE_INSTANCES_KHR,
    .geometry.instances =
      {
        .sType =
          VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR,
        .arrayOfPointers = VK_FALSE,
        .data.deviceAddress = ac_vk_get_buffer_address(device, buffer->buffer) +
                              info->instances_buffer_offset,
      },
  };

  as->geometries = ac_calloc(sizeof(VkAccelerationStructureGeometryKHR));
  as->geometries[0] = geometry;
  as->geometry_count = 1;

  uint64_t size = 0;

  VkAccelerationStructureBuildGeometryInfoKHR build_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
    .type = ac_as_type_to_vk(info->type),
    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
    .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
    .geometryCount = as->geometry_count,
    .pGeometries = as->geometries,
  };

  VkAccelerationStructureBuildSizesInfoKHR build_sizes = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR,
  };

  device->vkGetAccelerationStructureBuildSizesKHR(
    device->device,
    VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR,
    &build_info,
    &build_info.geometryCount,
    &build_sizes);

  size = build_sizes.accelerationStructureSize;

  as->common.scratch_size = build_sizes.buildScratchSize;

  ac_buffer_info buffer_info = {
    .memory_usage = ac_memory_usage_gpu_only,
    .usage = 0,
    .size = size,
    .name = info->name,
  };

  AC_RIF(ac_vk_create_buffer2(
    device,
    &buffer_info,
    VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR |
      VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
    &as->buffer,
    &as->allocation));

  VkAccelerationStructureCreateInfoKHR as_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR,
    .buffer = as->buffer,
    .size = size,
    .type = ac_as_type_to_vk(info->type),
  };

  AC_VK_RIF(device->vkCreateAccelerationStructureKHR(
    device->device,
    &as_info,
    &device->cpu_allocator,
    &as->as));

  return ac_result_success;
}

static void
ac_vk_destroy_as(ac_device device_handle, ac_as as_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);
  AC_FROM_HANDLE(as, ac_vk_as);

  vmaDestroyBuffer(device->gpu_allocator, as->buffer, as->allocation);
  device->vkDestroyAccelerationStructureKHR(
    device->device,
    as->as,
    &device->cpu_allocator);

  ac_free(as->geometries);
}

static void
ac_vk_write_as_instances(
  ac_device             device_handle,
  uint32_t              count,
  const ac_as_instance* instances,
  void*                 mem)
{
  AC_FROM_HANDLE(device, ac_vk_device);

  VkAccelerationStructureInstanceKHR* dst_instances = mem;

  for (uint32_t i = 0; i < count; ++i)
  {
    const ac_as_instance*               src = &instances[i];
    VkAccelerationStructureInstanceKHR* dst = &dst_instances[i];

    AC_FROM_HANDLE2(as, src->as, ac_vk_as);

    memcpy(
      dst->transform.matrix,
      src->transform.matrix,
      sizeof(dst->transform.matrix));

    dst->instanceCustomIndex = src->instance_index;
    dst->mask = src->mask;
    dst->instanceShaderBindingTableRecordOffset = src->instance_sbt_offset;
    dst->flags = ac_as_instance_bits_to_vk(src->bits);
    dst->accelerationStructureReference = ac_vk_get_as_address(device, as->as);
  }
}

static void
ac_vk_cmd_begin_rendering(ac_cmd cmd_handle, const ac_rendering_info* info)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);

  VkRect2D rect;
  AC_ZERO(rect);

  VkRenderingAttachmentInfo color_attachments[AC_MAX_ATTACHMENT_COUNT];
  AC_ZERO(color_attachments);

  for (uint32_t i = 0; i < info->color_attachment_count; ++i)
  {
    const ac_attachment_info*  in = &info->color_attachments[i];
    VkRenderingAttachmentInfo* out = &color_attachments[i];
    AC_FROM_HANDLE2(image, in->image, ac_vk_image);

    rect.extent.width = in->image->width;
    rect.extent.height = in->image->height;

    *out = (VkRenderingAttachmentInfo) {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = image->srv_view,
      .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .loadOp = ac_attachment_load_op_to_vk(in->load_op),
      .storeOp = ac_attachment_store_op_to_vk(in->store_op),
    };

    if (ac_format_is_floating_point(image->common.format))
    {
      out->clearValue.color.float32[0] = in->clear_value.color[0];
      out->clearValue.color.float32[1] = in->clear_value.color[1];
      out->clearValue.color.float32[2] = in->clear_value.color[2];
      out->clearValue.color.float32[3] = in->clear_value.color[3];
    }
    else
    {
      out->clearValue.color.int32[0] = (int32_t)in->clear_value.color[0];
      out->clearValue.color.int32[1] = (int32_t)in->clear_value.color[1];
      out->clearValue.color.int32[2] = (int32_t)in->clear_value.color[2];
      out->clearValue.color.int32[3] = (int32_t)in->clear_value.color[3];
    }

    if (in->resolve_image)
    {
      AC_FROM_HANDLE2(resolve_image, in->resolve_image, ac_vk_image);
      out->resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
      out->resolveImageView = resolve_image->srv_view;

      if (ac_format_is_floating_point(resolve_image->common.format))
      {
        out->resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
      }
      else
      {
        out->resolveMode = VK_RESOLVE_MODE_SAMPLE_ZERO_BIT;
      }
    }
  }

  VkRenderingAttachmentInfo  depth_attachment;
  VkRenderingAttachmentInfo* p_depth_attachment = NULL;

  if (info->depth_attachment.image)
  {
    const ac_attachment_info* in = &info->depth_attachment;
    AC_FROM_HANDLE2(image, in->image, ac_vk_image);
    p_depth_attachment = &depth_attachment;

    rect.extent.width = in->image->width;
    rect.extent.height = in->image->height;

    depth_attachment = (VkRenderingAttachmentInfo) {
      .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
      .imageView = image->srv_view,
      .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
      .loadOp = ac_attachment_load_op_to_vk(in->load_op),
      .storeOp = ac_attachment_store_op_to_vk(in->store_op),
      .clearValue =
        {
          .depthStencil =
            {
              .depth = in->clear_value.depth,
              .stencil = in->clear_value.stencil,
            },
        },
    };

    if (in->resolve_image)
    {
      AC_FROM_HANDLE2(resolve_image, in->resolve_image, ac_vk_image);
      depth_attachment.resolveMode = VK_RESOLVE_MODE_MIN_BIT;
      depth_attachment.resolveImageLayout =
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
      depth_attachment.resolveImageView = resolve_image->srv_view;
    }
  }

  VkRenderingInfo rendering_info = {
    .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
    .layerCount = 1,
    .colorAttachmentCount = info->color_attachment_count,
    .pColorAttachments = color_attachments,
    .pDepthAttachment = p_depth_attachment,
    .renderArea = rect,
  };

  device->vkCmdBeginRendering(cmd->cmd, &rendering_info);
}

static void
ac_vk_cmd_end_rendering(ac_cmd cmd_handle)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  device->vkCmdEndRendering(cmd->cmd);
}

static void
ac_vk_cmd_barrier(
  ac_cmd                   cmd_handle,
  uint32_t                 buffer_barrier_count,
  const ac_buffer_barrier* buffer_barriers,
  uint32_t                 image_barrier_count,
  const ac_image_barrier*  image_barriers)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);

  VkBufferMemoryBarrier2* buffer_memory_barriers = NULL;
  VkImageMemoryBarrier2*  image_memory_barriers = NULL;

  if (buffer_barrier_count)
  {
    buffer_memory_barriers =
      ac_alloc(buffer_barrier_count * sizeof(VkBufferMemoryBarrier2));
  }

  if (image_barrier_count)
  {
    image_memory_barriers =
      ac_alloc(image_barrier_count * sizeof(VkImageMemoryBarrier2));
  }

  for (uint32_t i = 0; i < buffer_barrier_count; ++i)
  {
    const ac_buffer_barrier* b = &buffer_barriers[i];

    AC_ASSERT(b->buffer);

    AC_FROM_HANDLE2(buffer, b->buffer, ac_vk_buffer);

    VkBufferMemoryBarrier2* out = &buffer_memory_barriers[i];
    out->sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
    out->pNext = NULL;
    out->srcAccessMask = ac_access_bits_to_vk(b->src_access);
    out->dstAccessMask = ac_access_bits_to_vk(b->dst_access);
    out->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    out->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    if (b->src_queue)
    {
      AC_FROM_HANDLE2(q, b->src_queue, ac_vk_queue);
      out->srcQueueFamilyIndex = q->family;
    }

    if (b->dst_queue)
    {
      AC_FROM_HANDLE2(q, b->dst_queue, ac_vk_queue);
      out->dstQueueFamilyIndex = q->family;
    }

    out->buffer = buffer->buffer;
    out->offset = b->offset;
    out->size = b->size;
    out->srcStageMask = ac_pipeline_stage_bits_to_vk(b->src_stage);
    out->dstStageMask = ac_pipeline_stage_bits_to_vk(b->dst_stage);
  }

  for (uint32_t i = 0; i < image_barrier_count; ++i)
  {
    const ac_image_barrier* in = &image_barriers[i];
    AC_ASSERT(in->image);

    AC_FROM_HANDLE2(image, in->image, ac_vk_image);

    VkImageSubresourceRange range = {
      .aspectMask = ac_format_to_vk_aspect_flags(image->common.format),
      .baseArrayLayer = in->range.base_layer,
      .layerCount = in->range.layers,
      .baseMipLevel = in->range.base_level,
      .levelCount = in->range.levels,
    };

    VkImageMemoryBarrier2* out = &image_memory_barriers[i];

    out->sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
    out->pNext = NULL;
    out->srcAccessMask = ac_access_bits_to_vk(in->src_access);
    out->dstAccessMask = ac_access_bits_to_vk(in->dst_access);
    out->oldLayout =
      ac_image_layout_to_vk(in->old_layout, image->common.format);
    out->newLayout =
      ac_image_layout_to_vk(in->new_layout, image->common.format);
    out->srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    out->dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    out->subresourceRange = range;
    out->image = image->image;

    if (in->src_queue)
    {
      AC_FROM_HANDLE2(q, in->src_queue, ac_vk_queue);
      out->srcQueueFamilyIndex = q->family;
    }

    if (in->dst_queue)
    {
      AC_FROM_HANDLE2(q, in->dst_queue, ac_vk_queue);
      out->dstQueueFamilyIndex = q->family;
    }

    if (in->range.layers == AC_WHOLE_LAYERS)
    {
      out->subresourceRange.layerCount = image->common.layers;
      out->subresourceRange.baseArrayLayer = 0;
    }

    if (in->range.levels == AC_WHOLE_LEVELS)
    {
      out->subresourceRange.levelCount = image->common.levels;
      out->subresourceRange.baseMipLevel = 0;
    }

    out->srcStageMask = ac_pipeline_stage_bits_to_vk(in->src_stage);
    out->dstStageMask = ac_pipeline_stage_bits_to_vk(in->dst_stage);
  }

  VkDependencyInfo dep_info = {
    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
    .dependencyFlags = 0,
    .memoryBarrierCount = 0,
    .pMemoryBarriers = NULL,
    .bufferMemoryBarrierCount = buffer_barrier_count,
    .pBufferMemoryBarriers = buffer_memory_barriers,
    .imageMemoryBarrierCount = image_barrier_count,
    .pImageMemoryBarriers = image_memory_barriers,
  };

  device->vkCmdPipelineBarrier2(cmd->cmd, &dep_info);

  ac_free(buffer_memory_barriers);
  ac_free(image_memory_barriers);
}

static void
ac_vk_cmd_set_scissor(
  ac_cmd   cmd_handle,
  uint32_t x,
  uint32_t y,
  uint32_t width,
  uint32_t height)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);

  VkRect2D scissor = {
    .offset.x = (int32_t)x,
    .offset.y = (int32_t)y,
    .extent.width = width,
    .extent.height = height,
  };
  device->vkCmdSetScissor(cmd->cmd, 0, 1, &scissor);
}

static void
ac_vk_cmd_set_viewport(
  ac_cmd cmd_handle,
  float  x,
  float  y,
  float  width,
  float  height,
  float  min_depth,
  float  max_depth)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);

  VkViewport viewport = {
    .x = x,
    .y = y + height,
    .width = width,
    .height = -height,
    .minDepth = min_depth,
    .maxDepth = max_depth,
  };

  device->vkCmdSetViewport(cmd->cmd, 0, 1, &viewport);
}

static void
ac_vk_cmd_bind_pipeline(ac_cmd cmd_handle, ac_pipeline pipeline_handle)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  AC_FROM_HANDLE(pipeline, ac_vk_pipeline);

  device->vkCmdBindPipeline(
    cmd->cmd,
    ac_pipeline_type_to_vk_bind_point(pipeline->common.type),
    pipeline->pipeline);
}

static void
ac_vk_cmd_draw_mesh_tasks(
  ac_cmd   cmd_handle,
  uint32_t group_count_x,
  uint32_t group_count_y,
  uint32_t group_count_z)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  device->vkCmdDrawMeshTasksEXT(
    cmd->cmd,
    group_count_x,
    group_count_y,
    group_count_z);
}

static void
ac_vk_cmd_draw(
  ac_cmd   cmd_handle,
  uint32_t vertex_count,
  uint32_t instance_count,
  uint32_t first_vertex,
  uint32_t first_instance)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);

  device->vkCmdDraw(
    cmd->cmd,
    vertex_count,
    instance_count,
    first_vertex,
    first_instance);
}

static void
ac_vk_cmd_draw_indexed(
  ac_cmd   cmd_handle,
  uint32_t index_count,
  uint32_t instance_count,
  uint32_t first_index,
  int32_t  vertex_offset,
  uint32_t first_instance)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);

  device->vkCmdDrawIndexed(
    cmd->cmd,
    index_count,
    instance_count,
    first_index,
    vertex_offset,
    first_instance);
}

static void
ac_vk_cmd_bind_vertex_buffer(
  ac_cmd         cmd_handle,
  uint32_t       binding,
  ac_buffer      buffer_handle,
  const uint64_t offset)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  AC_FROM_HANDLE(buffer, ac_vk_buffer);

  device
    ->vkCmdBindVertexBuffers(cmd->cmd, binding, 1, &buffer->buffer, &offset);
}

static void
ac_vk_cmd_bind_index_buffer(
  ac_cmd        cmd_handle,
  ac_buffer     buffer_handle,
  uint64_t      offset,
  ac_index_type index_type)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  AC_FROM_HANDLE(buffer, ac_vk_buffer);

  device->vkCmdBindIndexBuffer(
    cmd->cmd,
    buffer->buffer,
    offset,
    ac_index_type_to_vk(index_type));
}

static void
ac_vk_cmd_copy_buffer(
  ac_cmd    cmd_handle,
  ac_buffer src_handle,
  uint64_t  src_offset,
  ac_buffer dst_handle,
  uint64_t  dst_offset,
  uint64_t  size)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  AC_FROM_HANDLE(src, ac_vk_buffer);
  AC_FROM_HANDLE(dst, ac_vk_buffer);

  VkBufferCopy buffer_copy = {
    .srcOffset = src_offset,
    .dstOffset = dst_offset,
    .size = size,
  };

  device->vkCmdCopyBuffer(cmd->cmd, src->buffer, dst->buffer, 1, &buffer_copy);
}

static void
ac_vk_cmd_copy_buffer_to_image(
  ac_cmd                      cmd_handle,
  ac_buffer                   src_handle,
  ac_image                    dst_handle,
  const ac_buffer_image_copy* copy)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  AC_FROM_HANDLE(src, ac_vk_buffer);
  AC_FROM_HANDLE(dst, ac_vk_image);

  device->vkCmdCopyBufferToImage(
    cmd->cmd,
    src->buffer,
    dst->image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &(VkBufferImageCopy) {
      .bufferOffset = copy->buffer_offset,
      .imageSubresource =
        {
          .aspectMask = ac_format_to_vk_aspect_flags(dst->common.format),
          .mipLevel = copy->level,
          .baseArrayLayer = copy->layer,
          .layerCount = 1,
        },
      .imageOffset =
        {
          .x = (int32_t)copy->x,
          .y = (int32_t)copy->y,
          .z = 0,
        },
      .imageExtent =
        {
          .width = copy->width,
          .height = copy->height,
          .depth = 1,
        },
    });
}

static void
ac_vk_cmd_copy_image(
  ac_cmd               cmd_handle,
  ac_image             src_handle,
  ac_image             dst_handle,
  const ac_image_copy* copy)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  AC_FROM_HANDLE(src, ac_vk_image);
  AC_FROM_HANDLE(dst, ac_vk_image);

  device->vkCmdCopyImage(
    cmd->cmd,
    src->image,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    dst->image,
    VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
    1,
    &(VkImageCopy) {
      .srcSubresource =
        {
          .aspectMask = ac_format_to_vk_aspect_flags(src->common.format),
          .mipLevel = copy->src.level,
          .baseArrayLayer = copy->src.layer,
          .layerCount = 1,
        },
      .srcOffset =
        {
          .x = (int32_t)copy->src.x,
          .y = (int32_t)copy->src.y,
          .z = 0,
        },
      .dstSubresource =
        {
          .aspectMask = ac_format_to_vk_aspect_flags(dst->common.format),
          .mipLevel = copy->dst.level,
          .baseArrayLayer = copy->dst.layer,
          .layerCount = 1,
        },
      .dstOffset =
        {
          .x = (int32_t)copy->dst.x,
          .y = (int32_t)copy->dst.y,
          .z = 0,
        },
      .extent =
        {
          .width = copy->width,
          .height = copy->height,
          .depth = 1,
        },
    });
}

static void
ac_vk_cmd_copy_image_to_buffer(
  ac_cmd                      cmd_handle,
  ac_image                    src_handle,
  ac_buffer                   dst_handle,
  const ac_buffer_image_copy* copy)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  AC_FROM_HANDLE(src, ac_vk_image);
  AC_FROM_HANDLE(dst, ac_vk_buffer);

  device->vkCmdCopyImageToBuffer(
    cmd->cmd,
    src->image,
    VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
    dst->buffer,
    1,
    &(VkBufferImageCopy) {
      .bufferOffset = copy->buffer_offset,
      .imageSubresource =
        {
          .aspectMask = ac_format_to_vk_aspect_flags(src->common.format),
          .mipLevel = copy->level,
          .baseArrayLayer = copy->layer,
          .layerCount = 1,
        },
      .imageOffset =
        {
          .x = (int32_t)copy->x,
          .y = (int32_t)copy->y,
          .z = 0,
        },
      .imageExtent =
        {
          .width = copy->width,
          .height = copy->height,
          .depth = 1,
        },
    });
}

static void
ac_vk_cmd_dispatch(
  ac_cmd   cmd_handle,
  uint32_t group_count_x,
  uint32_t group_count_y,
  uint32_t group_count_z)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);

  device->vkCmdDispatch(cmd->cmd, group_count_x, group_count_y, group_count_z);
}

static void
ac_vk_cmd_trace_rays(
  ac_cmd   cmd_handle,
  ac_sbt   sbt_handle,
  uint32_t width,
  uint32_t height,
  uint32_t depth)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  AC_FROM_HANDLE(sbt, ac_vk_sbt);

  device->vkCmdTraceRaysKHR(
    cmd->cmd,
    &sbt->raygen,
    &sbt->miss,
    &sbt->hit,
    &sbt->callable,
    width,
    height,
    depth);
}

static void
ac_vk_cmd_build_as(ac_cmd cmd_handle, ac_as_build_info* info)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  AC_FROM_HANDLE2(as, info->as, ac_vk_as);

  VkAccelerationStructureBuildGeometryInfoKHR build_info = {
    .sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR,
    .type = ac_as_type_to_vk(as->common.type),
    .flags = VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR,
    .mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR,
    .geometryCount = as->geometry_count,
    .pGeometries = as->geometries,
    .dstAccelerationStructure = as->as,
  };

  {
    AC_FROM_HANDLE2(buf, info->scratch_buffer, ac_vk_buffer);
    build_info.scratchData.deviceAddress =
      ac_vk_get_buffer_address(device, buf->buffer) +
      info->scratch_buffer_offset;
  }

  VkAccelerationStructureBuildRangeInfoKHR build_range_info = {
    .primitiveCount = build_info.geometryCount,
    .primitiveOffset = 0,
    .firstVertex = 0,
    .transformOffset = 0,
  };

  const VkAccelerationStructureBuildRangeInfoKHR* build_range_infos[] = {
    &build_range_info,
  };

  device->vkCmdBuildAccelerationStructuresKHR(
    cmd->cmd,
    1,
    &build_info,
    build_range_infos);
}

static void
ac_vk_cmd_push_constants(ac_cmd cmd_handle, uint32_t size, const void* data)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  AC_FROM_HANDLE2(pipeline, cmd_handle->pipeline, ac_vk_pipeline);

  if (pipeline->push_constant_stage_flags)
  {
    device->vkCmdPushConstants(
      cmd->cmd,
      pipeline->pipeline_layout,
      pipeline->push_constant_stage_flags,
      0,
      size,
      data);
  }
}

static void
ac_vk_cmd_bind_set(
  ac_cmd               cmd_handle,
  ac_descriptor_buffer db_handle,
  ac_space             space,
  uint32_t             set)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  AC_FROM_HANDLE(cmd, ac_vk_cmd);
  AC_FROM_HANDLE2(pipeline, cmd_handle->pipeline, ac_vk_pipeline);
  AC_FROM_HANDLE(db, ac_vk_descriptor_buffer);

  device->vkCmdBindDescriptorSets(
    cmd->cmd,
    ac_pipeline_type_to_vk_bind_point(pipeline->common.type),
    pipeline->pipeline_layout,
    (uint32_t)space,
    1,
    &db->sets[space][set],
    0,
    NULL);
}

static void
ac_vk_cmd_begin_debug_label(
  ac_cmd      cmd_handle,
  const char* name,
  const float color[4])
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  if (device->vkCmdBeginDebugUtilsLabelEXT)
  {
    AC_FROM_HANDLE(cmd, ac_vk_cmd);
    VkDebugUtilsLabelEXT info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
      .pLabelName = name,
      .color =
        {
          color[0],
          color[1],
          color[2],
          color[3],
        },
    };
    device->vkCmdBeginDebugUtilsLabelEXT(cmd->cmd, &info);
  }
}

static void
ac_vk_cmd_insert_debug_label(
  ac_cmd      cmd_handle,
  const char* name,
  const float color[4])
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  if (device->vkCmdInsertDebugUtilsLabelEXT)
  {
    AC_FROM_HANDLE(cmd, ac_vk_cmd);
    VkDebugUtilsLabelEXT info = {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
      .pLabelName = name,
      .color =
        {
          color[0],
          color[1],
          color[2],
          color[3],
        },
    };
    device->vkCmdInsertDebugUtilsLabelEXT(cmd->cmd, &info);
  }
}

static void
ac_vk_cmd_end_debug_label(ac_cmd cmd_handle)
{
  AC_FROM_HANDLE2(device, cmd_handle->device, ac_vk_device);
  if (device->vkCmdEndDebugUtilsLabelEXT)
  {
    AC_FROM_HANDLE(cmd, ac_vk_cmd);
    device->vkCmdEndDebugUtilsLabelEXT(cmd->cmd);
  }
}

static void
ac_vk_begin_capture(ac_device device_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);

  if (device->rdoc)
  {
    if (!device->rdoc->IsFrameCapturing())
    {
      device->rdoc->StartFrameCapture(NULL, NULL);
    }
  }
}

static void
ac_vk_end_capture(ac_device device_handle)
{
  AC_FROM_HANDLE(device, ac_vk_device);

  if (device->rdoc)
  {
    if (device->rdoc->IsFrameCapturing())
    {
      device->rdoc->EndFrameCapture(NULL, NULL);
    }
  }
}

ac_result
ac_vk_create_device(const ac_device_info* info, ac_device* device_handle)
{
  AC_INIT_INTERNAL(device, ac_vk_device);

  device->common.destroy_device = ac_vk_destroy_device;
  device->common.queue_wait_idle = ac_vk_queue_wait_idle;
  device->common.queue_submit = ac_vk_queue_submit;
  device->common.queue_present = ac_vk_queue_present;
  device->common.create_fence = ac_vk_create_fence;
  device->common.destroy_fence = ac_vk_destroy_fence;
  device->common.get_fence_value = ac_vk_get_fence_value;
  device->common.signal_fence = ac_vk_signal_fence;
  device->common.wait_fence = ac_vk_wait_fence;
  device->common.create_swapchain = ac_vk_create_swapchain;
  device->common.destroy_swapchain = ac_vk_destroy_swapchain;
  device->common.create_cmd_pool = ac_vk_create_cmd_pool;
  device->common.destroy_cmd_pool = ac_vk_destroy_cmd_pool;
  device->common.reset_cmd_pool = ac_vk_reset_cmd_pool;
  device->common.create_cmd = ac_vk_create_cmd;
  device->common.destroy_cmd = ac_vk_destroy_cmd;
  device->common.begin_cmd = ac_vk_begin_cmd;
  device->common.end_cmd = ac_vk_end_cmd;
  device->common.acquire_next_image = ac_vk_acquire_next_image;
  device->common.create_shader = ac_vk_create_shader;
  device->common.destroy_shader = ac_vk_destroy_shader;
  device->common.create_dsl = ac_vk_create_dsl;
  device->common.destroy_dsl = ac_vk_destroy_dsl;
  device->common.create_descriptor_buffer = ac_vk_create_descriptor_buffer;
  device->common.destroy_descriptor_buffer = ac_vk_destroy_descriptor_buffer;
  device->common.create_compute_pipeline = ac_vk_create_compute_pipeline;
  device->common.create_raytracing_pipeline = ac_vk_create_raytracing_pipeline;
  device->common.create_graphics_pipeline = ac_vk_create_graphics_pipeline;
  device->common.create_mesh_pipeline = ac_vk_create_mesh_pipeline;
  device->common.destroy_pipeline = ac_vk_destroy_pipeline;
  device->common.create_buffer = ac_vk_create_buffer;
  device->common.destroy_buffer = ac_vk_destroy_buffer;
  device->common.map_memory = ac_vk_map_memory;
  device->common.unmap_memory = ac_vk_unmap_memory;
  device->common.create_sampler = ac_vk_create_sampler;
  device->common.destroy_sampler = ac_vk_destroy_sampler;
  device->common.create_image = ac_vk_create_image;
  device->common.destroy_image = ac_vk_destroy_image;
  device->common.update_descriptor = ac_vk_update_descriptor;
  device->common.create_sbt = ac_vk_create_sbt;
  device->common.destroy_sbt = ac_vk_destroy_sbt;
  device->common.create_blas = ac_vk_create_blas;
  device->common.create_tlas = ac_vk_create_tlas;
  device->common.destroy_as = ac_vk_destroy_as;
  device->common.write_as_instances = ac_vk_write_as_instances;
  device->common.cmd_begin_rendering = ac_vk_cmd_begin_rendering;
  device->common.cmd_end_rendering = ac_vk_cmd_end_rendering;
  device->common.cmd_barrier = ac_vk_cmd_barrier;
  device->common.cmd_set_scissor = ac_vk_cmd_set_scissor;
  device->common.cmd_set_viewport = ac_vk_cmd_set_viewport;
  device->common.cmd_bind_pipeline = ac_vk_cmd_bind_pipeline;
  device->common.cmd_draw_mesh_tasks = ac_vk_cmd_draw_mesh_tasks;
  device->common.cmd_draw = ac_vk_cmd_draw;
  device->common.cmd_draw_indexed = ac_vk_cmd_draw_indexed;
  device->common.cmd_bind_vertex_buffer = ac_vk_cmd_bind_vertex_buffer;
  device->common.cmd_bind_index_buffer = ac_vk_cmd_bind_index_buffer;
  device->common.cmd_copy_buffer = ac_vk_cmd_copy_buffer;
  device->common.cmd_copy_buffer_to_image = ac_vk_cmd_copy_buffer_to_image;
  device->common.cmd_copy_image = ac_vk_cmd_copy_image;
  device->common.cmd_copy_image_to_buffer = ac_vk_cmd_copy_image_to_buffer;
  device->common.cmd_bind_set = ac_vk_cmd_bind_set;
  device->common.cmd_dispatch = ac_vk_cmd_dispatch;
  device->common.cmd_build_as = ac_vk_cmd_build_as;
  device->common.cmd_trace_rays = ac_vk_cmd_trace_rays;
  device->common.cmd_push_constants = ac_vk_cmd_push_constants;
  device->common.cmd_begin_debug_label = ac_vk_cmd_begin_debug_label;
  device->common.cmd_end_debug_label = ac_vk_cmd_end_debug_label;
  device->common.cmd_insert_debug_label = ac_vk_cmd_insert_debug_label;
  device->common.begin_capture = ac_vk_begin_capture;
  device->common.end_capture = ac_vk_end_capture;

#if (AC_PLATFORM_WINDOWS)
  AC_RIF(ac_load_library("vulkan-1.dll", &device->vk, false));
  AC_RIF(ac_load_function(
    device->vk,
    "vkGetInstanceProcAddr",
    (void**)&device->vkGetInstanceProcAddr));
#elif (AC_PLATFORM_APPLE)
  const char* vk_lib_names[] = {
    "libvulkan.dylib",
    "libvulkan.1.dylib",
    "libMoltenVK.dylib",
  };

  for (uint32_t i = 0; i < AC_COUNTOF(vk_lib_names); ++i)
  {
    ac_result res = ac_load_library(vk_lib_names[i], &device->vk, false);

    if (res != ac_result_success)
    {
      continue;
    }

    AC_RIF(ac_load_function(
      device->vk,
      "vkGetInstanceProcAddr",
      (void**)&device->vkGetInstanceProcAddr));
  }
#elif (AC_PLATFORM_LINUX)
  const char* vk_lib_names[] = {
    "libvulkan.so.1",
    "libvulkan.so",
  };

  for (uint32_t i = 0; i < AC_COUNTOF(vk_lib_names); ++i)
  {
    void* vk_lib = NULL;

    if (ac_load_library(vk_lib_names[i], &vk_lib, false) == ac_result_success)
    {
      AC_RIF(ac_load_function(
        vk_lib,
        "vkGetInstanceProcAddr",
        (void**)&device->vkGetInstanceProcAddr));
      break;
    }
  }
#else
  VKAPI_ATTR PFN_vkVoidFunction VKAPI_CALL vkGetInstanceProcAddr(
    VkInstance  instance,
    const char* pName);

  device->vkGetInstanceProcAddr = vkGetInstanceProcAddr;
#endif

  ac_vk_load_global_functions(device);

  device->cpu_allocator = (VkAllocationCallbacks) {
    .pfnAllocation = &ac_vk_alloc_fn,
    .pfnReallocation = &ac_vk_realloc_fn,
    .pfnFree = &ac_vk_free_fn,
    .pfnInternalAllocation = &ac_vk_internal_alloc_fn,
    .pfnInternalFree = &ac_vk_internal_free_fn,
  };

  ac_wsi wsi = *info->wsi;

  VkApplicationInfo app_info = {
    .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
    .pNext = NULL,
    .pApplicationName = "ac-application",
    .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
    .pEngineName = "ac-engine",
    .engineVersion = VK_MAKE_VERSION(0, 0, 1),
    .apiVersion = VK_API_VERSION_1_3,
  };

  if (!wsi.get_vk_instance_extensions)
  {
    return ac_result_invalid_argument;
  }

  uint32_t extension_count = 0;

  AC_RIF(wsi.get_vk_instance_extensions(wsi.user_data, &extension_count, NULL));
  array_t(const char*) instance_extensions = NULL;
  array_resize(instance_extensions, extension_count);
  if (
    wsi.get_vk_instance_extensions(
      wsi.user_data,
      &extension_count,
      instance_extensions) != ac_result_success)
  {
    array_free(instance_extensions);
    return ac_result_unknown_error;
  }

  bool debug_utils = false;

  uint32_t    layer_count = 0;
  const char* layers[3];
  AC_ZERO(layers);

  if (
    device->vkEnumerateInstanceExtensionProperties(
      NULL,
      &extension_count,
      NULL) != VK_SUCCESS)
  {
    array_free(instance_extensions);
    return ac_result_unknown_error;
  }

  VkExtensionProperties* extension_props =
    ac_alloc(extension_count * sizeof(VkExtensionProperties));

  if (
    device->vkEnumerateInstanceExtensionProperties(
      NULL,
      &extension_count,
      extension_props) != VK_SUCCESS)
  {
    array_free(instance_extensions);
    ac_free(extension_props);
    return ac_result_unknown_error;
  }

  for (uint32_t i = 0; i < extension_count; ++i)
  {
    if (AC_INCLUDE_DEBUG && info->debug_bits)
    {
      if (
        strcmp(
          VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
          extension_props[i].extensionName) == 0)
      {
        debug_utils = true;
        array_append(instance_extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
      }
    }

    if (
      strcmp(
        VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME,
        extension_props[i].extensionName) == 0)
    {
      device->enabled_color_spaces = true;
      array_append(
        instance_extensions,
        VK_EXT_SWAPCHAIN_COLOR_SPACE_EXTENSION_NAME);
    }
  }

  ac_free(extension_props);

  if (AC_INCLUDE_DEBUG && info->debug_bits)
  {
    void* name = NULL;
    if (AC_PLATFORM_WINDOWS)
    {
      name = "renderdoc.dll";
    }
    else if (AC_PLATFORM_LINUX)
    {
      name = "librenderdoc.so";
    }

    if (name)
    {
      void*             rdoc = NULL;
      pRENDERDOC_GetAPI renderdoc_get_api = NULL;

      if (ac_load_library(name, &rdoc, true) == ac_result_success)
      {
        AC_RIF(ac_load_function(
          rdoc,
          "RENDERDOC_GetAPI",
          (void**)&renderdoc_get_api));
      }

      if (renderdoc_get_api)
      {
        if (!renderdoc_get_api(
              eRENDERDOC_API_Version_1_6_0,
              (void**)&device->rdoc))
        {
          device->rdoc = NULL;
        }
      }
    }
  }

  uint32_t           available_layer_count = 0;
  VkLayerProperties* available_layers = NULL;

  AC_VK_RIF(
    device->vkEnumerateInstanceLayerProperties(&available_layer_count, NULL));

  if (available_layer_count)
  {
    available_layers =
      ac_alloc(available_layer_count * sizeof(VkLayerProperties));

    if (
      device->vkEnumerateInstanceLayerProperties(
        &available_layer_count,
        available_layers) != VK_SUCCESS)
    {
      ac_free(available_layers);
      return ac_result_unknown_error;
    }
  }

  for (uint32_t i = 0; i < available_layer_count; ++i)
  {
    if (
      (info->debug_bits & ac_device_debug_validation_bit) &&
      strcmp("VK_LAYER_KHRONOS_validation", available_layers[i].layerName) == 0)
    {
      layers[layer_count++] = "VK_LAYER_KHRONOS_validation";
    }

    if (
      (info->debug_bits & ac_device_debug_enable_debugger_attaching_bit) &&
      !device->rdoc &&
      (strcmp("VK_LAYER_RENDERDOC_Capture", available_layers[i].layerName) ==
       0))
    {
      layers[layer_count++] = "VK_LAYER_RENDERDOC_Capture";
    }
  }

  ac_free(available_layers);

  VkValidationFeatureEnableEXT validation_features_enable[2];
  AC_ZERO(validation_features_enable);

  VkValidationFeaturesEXT validation_features = {
    .sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT,
    .pEnabledValidationFeatures = validation_features_enable,
  };

  if (info->debug_bits & ac_device_debug_validation_bit)
  {
    if (info->debug_bits & ac_device_debug_validation_synchronization_bit)
    {
      validation_features_enable[validation_features
                                   .enabledValidationFeatureCount++] =
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT;
    }

    if (info->debug_bits & ac_device_debug_validation_gpu_based_bit)
    {
      validation_features_enable[validation_features
                                   .enabledValidationFeatureCount++] =
        VK_VALIDATION_FEATURE_ENABLE_GPU_ASSISTED_EXT;
    }
  }

  VkInstanceCreateInfo instance_create_info = {
    .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
    .pApplicationInfo = &app_info,
    .enabledLayerCount = layer_count,
    .ppEnabledLayerNames = layers,
    .enabledExtensionCount = (uint32_t)array_size(instance_extensions),
    .ppEnabledExtensionNames = instance_extensions,
    .pNext = validation_features.enabledValidationFeatureCount
               ? &validation_features
               : NULL,
  };

  if (
    device->vkCreateInstance(
      &instance_create_info,
      &device->cpu_allocator,
      &device->instance) != VK_SUCCESS)
  {
    array_free(instance_extensions);
    return ac_result_unknown_error;
  }

  array_free(instance_extensions);

  ac_vk_load_instance_functions(device);

  if (debug_utils)
  {
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info;
    AC_ZERO(debug_messenger_create_info);
    debug_messenger_create_info.sType =
      VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_messenger_create_info.messageSeverity =
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_messenger_create_info.messageType =
      VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_messenger_create_info.pfnUserCallback = ac_vk_debug_callback;
    debug_messenger_create_info.pUserData = NULL;

    AC_VK_RIF(device->vkCreateDebugUtilsMessengerEXT(
      device->instance,
      &debug_messenger_create_info,
      &device->cpu_allocator,
      &device->debug_messenger));
  }

  {
    uint32_t gpu_count = 0;
    AC_VK_RIF(
      device->vkEnumeratePhysicalDevices(device->instance, &gpu_count, NULL));
    if (!gpu_count)
    {
      return ac_result_unknown_error;
    }
    VkPhysicalDevice* physical_devices =
      ac_alloc(gpu_count * sizeof(VkPhysicalDevice));
    if (
      device->vkEnumeratePhysicalDevices(
        device->instance,
        &gpu_count,
        physical_devices) != VK_SUCCESS)
    {
      ac_free(physical_devices);
      return ac_result_unknown_error;
    }

    device->gpu = physical_devices[0];

    for (uint32_t i = 0; i < gpu_count; ++i)
    {
      VkPhysicalDeviceProperties2 props = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      };

      device->vkGetPhysicalDeviceProperties2(physical_devices[i], &props);

      if (props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
      {
        device->gpu = physical_devices[i];
        break;
      }
    }
    ac_free(physical_devices);
  }

  VkPhysicalDeviceMeshShaderFeaturesEXT mesh_shader = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MESH_SHADER_FEATURES_EXT,
  };

  VkPhysicalDeviceAccelerationStructureFeaturesKHR acceleration = {
    .sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR,
    .pNext = &mesh_shader,
  };

  VkPhysicalDeviceRayTracingPipelineFeaturesKHR ray_tracing = {
    .sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR,
    .pNext = &acceleration,
  };

  VkPhysicalDeviceRayQueryFeaturesKHR ray_query = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_QUERY_FEATURES_KHR,
    .pNext = &ray_tracing,
  };

  VkPhysicalDeviceVulkan11Features vk_11 = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES,
    .pNext = &ray_query,
  };

  VkPhysicalDeviceVulkan12Features vk_12 = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
    .pNext = &vk_11,
  };

  VkPhysicalDeviceVulkan13Features vk_13 = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    .pNext = &vk_12,
  };

  VkPhysicalDeviceFeatures2 features = {
    .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
    .pNext = &vk_13,
  };

  {
    VkPhysicalDeviceRayTracingPipelinePropertiesKHR rt_props = {
      .sType =
        VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR,
    };

    VkPhysicalDeviceProperties2 props = {
      .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
      .pNext = &rt_props,
    };

    device->vkGetPhysicalDeviceProperties2(device->gpu, &props);
    device->vkGetPhysicalDeviceFeatures2(device->gpu, &features);

    device->common.props.api = "Vulkan";
    device->common.props.max_sample_count = 4;
    device->common.props.cbv_buffer_alignment =
      props.properties.limits.minUniformBufferOffsetAlignment;
    // TODO: use optimal alignment for vulkan
    device->common.props.image_row_alignment = 1;
    device->common.props.image_alignment = 1;

    device->common.support_raytracing =
      (acceleration.accelerationStructure && ray_tracing.rayTracingPipeline &&
       ray_query.rayQuery);
    device->common.support_mesh_shaders =
      (mesh_shader.meshShader && mesh_shader.taskShader);
    device->common.props.as_instance_size =
      sizeof(VkAccelerationStructureInstanceKHR);

    device->shader_group_base_alignment = rt_props.shaderGroupBaseAlignment;
    device->shader_group_handle_alignment = rt_props.shaderGroupHandleAlignment;
    device->shader_group_handle_size = rt_props.shaderGroupHandleSize;
  }

  // TODO: check features

  uint32_t supported_extension_count = 0;
  AC_VK_RIF(device->vkEnumerateDeviceExtensionProperties(
    device->gpu,
    NULL,
    &supported_extension_count,
    NULL));

  if (!supported_extension_count)
  {
    return ac_result_unknown_error;
  }

  VkExtensionProperties* supported_extensions =
    ac_alloc(supported_extension_count * sizeof(VkExtensionProperties));

  if (
    device->vkEnumerateDeviceExtensionProperties(
      device->gpu,
      NULL,
      &supported_extension_count,
      supported_extensions) != VK_SUCCESS)
  {
    ac_free(supported_extensions);
    return ac_result_unknown_error;
  }

  array_t(const char*) device_extensions = NULL;

  const char* common_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME,
  };

  const char* mesh_extensions[] = {
    VK_EXT_MESH_SHADER_EXTENSION_NAME,
  };

  const char* raytracing_extensions[] = {
    VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME,
    VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME,
    VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME,
    VK_KHR_SPIRV_1_4_EXTENSION_NAME,
    VK_KHR_SHADER_FLOAT_CONTROLS_EXTENSION_NAME,
    VK_KHR_RAY_QUERY_EXTENSION_NAME,
  };

  if (ac_vk_is_extensions_supported(
        supported_extensions,
        supported_extension_count,
        common_extensions,
        AC_COUNTOF(common_extensions) - 1))
  {
    for (uint32_t i = 0; i < AC_COUNTOF(common_extensions); ++i)
    {
      array_append(device_extensions, common_extensions[i]);
    }
  }
  else
  {
    ac_free(supported_extensions);
    array_free(device_extensions);
    return ac_result_unknown_error;
  }

  if (device->common.support_mesh_shaders)
  {
    if (ac_vk_is_extensions_supported(
          supported_extensions,
          supported_extension_count,
          mesh_extensions,
          AC_COUNTOF(mesh_extensions)))
    {
      for (uint32_t i = 0; i < AC_COUNTOF(mesh_extensions); ++i)
      {
        array_append(device_extensions, mesh_extensions[i]);
      }
    }
    else
    {
      device->common.support_mesh_shaders = false;
    }
  }

  if (device->common.support_raytracing)
  {
    if (ac_vk_is_extensions_supported(
          supported_extensions,
          supported_extension_count,
          raytracing_extensions,
          AC_COUNTOF(raytracing_extensions)))
    {
      for (uint32_t i = 0; i < AC_COUNTOF(raytracing_extensions); ++i)
      {
        array_append(device_extensions, raytracing_extensions[i]);
      }
    }
    else
    {
      device->common.support_raytracing = false;
    }
  }

  ac_free(supported_extensions);

  uint32_t                queue_family_create_count = 0;
  VkDeviceQueueCreateInfo queue_family_create_infos[ac_queue_type_count];

  float queue_priorities[ac_queue_type_count];
  for (size_t i = 0; i < ac_queue_type_count; ++i)
  {
    queue_priorities[i] = 1.f;
  }

  {
    ac_vk_queue_slot queue_slots[ac_queue_type_count];
    AC_RIF(ac_vk_select_queue_slots(device, NULL, queue_slots));

    device->common.queue_count = 0;

    ac_vk_queue_slot unique_slots[ac_queue_type_count];

    for (uint32_t i = 0; i < ac_queue_type_count; ++i)
    {
      ac_vk_queue_slot* slot = queue_slots + i;

      bool is_unique = true;
      for (uint32_t j = 0; j < device->common.queue_count; ++j)
      {
        is_unique = slot->family != unique_slots[j].family ||
                    slot->index != unique_slots[j].index;
        if (!is_unique)
        {
          device->common.queue_map[i] = j;
          break;
        }
      }

      if (!is_unique)
      {
        continue;
      }

      unique_slots[device->common.queue_count] = *slot;

      device->common.queue_map[i] = device->common.queue_count;

      ac_queue* queue_handle =
        &device->common.queues[device->common.queue_count];

      AC_INIT_INTERNAL(queue, ac_vk_queue);

      queue->family = slot->family;
      queue->common.type = ac_queue_type_count;

      ++device->common.queue_count;
    }

    for (uint32_t i = 0; i < ac_queue_type_count; ++i)
    {
      uint32_t index = device->common.queue_map[i];

      ac_queue queue_handle = device->common.queues[index];

      AC_FROM_HANDLE(queue, ac_vk_queue);
      if (queue->common.type != ac_queue_type_count)
      {
        continue;
      }

      queue->common.type = (ac_queue_type)i;

      while (index--)
      {
        ac_queue queue2_handle = device->common.queues[index];
        AC_FROM_HANDLE(queue2, ac_vk_queue);

        if (queue2->family == queue->family)
        {
          queue->common.type = queue2->common.type;
        }
      }
    }

    bool is_unique = true;
    for (size_t i = 0; i < device->common.queue_count; ++i)
    {
      ac_vk_queue_slot* slot = unique_slots + i;

      for (size_t j = 0; j < queue_family_create_count; ++j)
      {
        if (queue_family_create_infos[j].queueFamilyIndex == slot->family)
        {
          ++queue_family_create_infos[j].queueCount;
          is_unique = false;
          break;
        }
      }

      if (!is_unique)
      {
        continue;
      }

      queue_family_create_infos[queue_family_create_count++] =
        (VkDeviceQueueCreateInfo) {
          .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
          .pNext = NULL,
          .flags = 0,
          .queueFamilyIndex = slot->family,
          .queueCount = 1,
          .pQueuePriorities = queue_priorities,
        };
    }
  }

  VkDeviceCreateInfo device_create_info = {
    .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
    .pNext = &features,
    .flags = 0,
    .queueCreateInfoCount = queue_family_create_count,
    .pQueueCreateInfos = queue_family_create_infos,
    .enabledLayerCount = 0,
    .ppEnabledLayerNames = NULL,
    .enabledExtensionCount = (uint32_t)array_size(device_extensions),
    .ppEnabledExtensionNames = device_extensions,
  };

  if (
    device->vkCreateDevice(
      device->gpu,
      &device_create_info,
      &device->cpu_allocator,
      &device->device) != VK_SUCCESS)
  {
    array_free(device_extensions);
    return ac_result_unknown_error;
  }

  array_free(device_extensions);

  ac_vk_load_device_functions(device);

  VmaVulkanFunctions vulkan_functions = {
    .vkAllocateMemory = device->vkAllocateMemory,
    .vkBindBufferMemory = device->vkBindBufferMemory,
    .vkBindImageMemory = device->vkBindImageMemory,
    .vkCreateBuffer = device->vkCreateBuffer,
    .vkCreateImage = device->vkCreateImage,
    .vkDestroyBuffer = device->vkDestroyBuffer,
    .vkDestroyImage = device->vkDestroyImage,
    .vkFreeMemory = device->vkFreeMemory,
    .vkGetBufferMemoryRequirements = device->vkGetBufferMemoryRequirements,
    .vkGetBufferMemoryRequirements2KHR = device->vkGetBufferMemoryRequirements2,
    .vkGetImageMemoryRequirements = device->vkGetImageMemoryRequirements,
    .vkGetImageMemoryRequirements2KHR = device->vkGetImageMemoryRequirements2,
    .vkGetPhysicalDeviceMemoryProperties =
      device->vkGetPhysicalDeviceMemoryProperties,
    .vkGetPhysicalDeviceProperties = device->vkGetPhysicalDeviceProperties,
    .vkMapMemory = device->vkMapMemory,
    .vkUnmapMemory = device->vkUnmapMemory,
    .vkFlushMappedMemoryRanges = device->vkFlushMappedMemoryRanges,
    .vkInvalidateMappedMemoryRanges = device->vkInvalidateMappedMemoryRanges,
    .vkCmdCopyBuffer = device->vkCmdCopyBuffer,
    .vkBindBufferMemory2KHR = device->vkBindBufferMemory2,
    .vkBindImageMemory2KHR = device->vkBindImageMemory2,
    .vkGetPhysicalDeviceMemoryProperties2KHR =
      device->vkGetPhysicalDeviceMemoryProperties2,
    .vkGetDeviceBufferMemoryRequirements =
      device->vkGetDeviceBufferMemoryRequirements,
    .vkGetDeviceImageMemoryRequirements =
      device->vkGetDeviceImageMemoryRequirements,
  };

  VmaAllocatorCreateInfo vma_allocator_create_info = {
    .instance = device->instance,
    .physicalDevice = device->gpu,
    .device = device->device,
    .flags = 0,
    .pAllocationCallbacks = &device->cpu_allocator,
    .pVulkanFunctions = &vulkan_functions,
    .vulkanApiVersion = app_info.apiVersion,
  };

  if (device->common.support_raytracing)
  {
    vma_allocator_create_info.flags |=
      VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
  }

  AC_VK_RIF(
    vmaCreateAllocator(&vma_allocator_create_info, &device->gpu_allocator));

  for (uint32_t i = 0; i < device->common.queue_count; ++i)
  {
    ac_queue queue_handle = device->common.queues[i];
    AC_FROM_HANDLE(queue, ac_vk_queue);
    device->vkGetDeviceQueue(device->device, queue->family, 0, &queue->queue);
  }

  return ac_result_success;
}

#endif
