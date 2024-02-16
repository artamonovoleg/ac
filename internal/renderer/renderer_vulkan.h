#pragma once

#include "ac_private.h"

#if (AC_INCLUDE_VULKAN)

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>
#include <vk_mem_alloc/vk_mem_alloc.h>
#include <renderdoc/renderdoc_app.h>
#include "renderer.h"

#if (AC_INCLUDE_DEBUG)
#define AC_VK_SET_OBJECT_NAME(device, name, type, object)                      \
  (ac_vk_set_object_name(device, name, type, (uint64_t)(object)))
#else
#define AC_VK_SET_OBJECT_NAME(device, name, type, object) ((void)0)
#endif

typedef struct ac_vk_device {
  ac_device_internal       common;
  VkAllocationCallbacks    cpu_allocator;
  VkInstance               instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkPhysicalDevice         gpu;
  VkDevice                 device;
  VmaAllocator             gpu_allocator;
  bool                     enabled_color_spaces;
  uint32_t                 shader_group_base_alignment;
  uint32_t                 shader_group_handle_alignment;
  uint32_t                 shader_group_handle_size;
  RENDERDOC_API_1_6_0*     rdoc;

  void*                                    vk;
  PFN_vkGetInstanceProcAddr                vkGetInstanceProcAddr;
  PFN_vkGetDeviceProcAddr                  vkGetDeviceProcAddr;
  PFN_vkGetPhysicalDeviceSurfaceFormatsKHR vkGetPhysicalDeviceSurfaceFormatsKHR;
  PFN_vkGetPhysicalDeviceProperties2       vkGetPhysicalDeviceProperties2;
  PFN_vkGetPhysicalDeviceFeatures2         vkGetPhysicalDeviceFeatures2;
  PFN_vkEnumerateDeviceExtensionProperties vkEnumerateDeviceExtensionProperties;
  PFN_vkCreateInstance                     vkCreateInstance;
  PFN_vkCreateDebugUtilsMessengerEXT       vkCreateDebugUtilsMessengerEXT;
  PFN_vkCreateDevice                       vkCreateDevice;
  PFN_vkCreateSemaphore                    vkCreateSemaphore;
  PFN_vkCreatePipelineLayout               vkCreatePipelineLayout;
  PFN_vkCreateSwapchainKHR                 vkCreateSwapchainKHR;
  PFN_vkCreateImageView                    vkCreateImageView;
  PFN_vkCreateCommandPool                  vkCreateCommandPool;
  PFN_vkCreateShaderModule                 vkCreateShaderModule;
  PFN_vkCreateDescriptorSetLayout          vkCreateDescriptorSetLayout;
  PFN_vkCreateDescriptorPool               vkCreateDescriptorPool;
  PFN_vkCreateComputePipelines             vkCreateComputePipelines;
  PFN_vkCreateRayTracingPipelinesKHR       vkCreateRayTracingPipelinesKHR;
  PFN_vkCreateGraphicsPipelines            vkCreateGraphicsPipelines;
  PFN_vkCreateSampler                      vkCreateSampler;
  PFN_vkCreateAccelerationStructureKHR     vkCreateAccelerationStructureKHR;
  PFN_vkGetDeviceQueue                     vkGetDeviceQueue;
  PFN_vkAllocateDescriptorSets             vkAllocateDescriptorSets;
  PFN_vkGetSwapchainImagesKHR              vkGetSwapchainImagesKHR;
  PFN_vkGetSemaphoreCounterValue           vkGetSemaphoreCounterValue;
  PFN_vkSignalSemaphore                    vkSignalSemaphore;
  PFN_vkWaitSemaphores                     vkWaitSemaphores;
  PFN_vkQueueSubmit2                       vkQueueSubmit2;
  PFN_vkQueuePresentKHR                    vkQueuePresentKHR;
  PFN_vkQueueWaitIdle                      vkQueueWaitIdle;
  PFN_vkAllocateCommandBuffers             vkAllocateCommandBuffers;
  PFN_vkBeginCommandBuffer                 vkBeginCommandBuffer;
  PFN_vkEndCommandBuffer                   vkEndCommandBuffer;
  PFN_vkAcquireNextImageKHR                vkAcquireNextImageKHR;
  PFN_vkResetCommandPool                   vkResetCommandPool;
  PFN_vkUpdateDescriptorSets               vkUpdateDescriptorSets;
  PFN_vkDeviceWaitIdle                     vkDeviceWaitIdle;
  PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR;
  PFN_vkCmdBeginRendering                  vkCmdBeginRendering;
  PFN_vkCmdEndRendering                    vkCmdEndRendering;
  PFN_vkCmdPipelineBarrier2                vkCmdPipelineBarrier2;
  PFN_vkCmdSetScissor                      vkCmdSetScissor;
  PFN_vkCmdSetViewport                     vkCmdSetViewport;
  PFN_vkCmdBindPipeline                    vkCmdBindPipeline;
  PFN_vkCmdDrawMeshTasksEXT                vkCmdDrawMeshTasksEXT;
  PFN_vkCmdDraw                            vkCmdDraw;
  PFN_vkCmdDrawIndexed                     vkCmdDrawIndexed;
  PFN_vkCmdBindVertexBuffers               vkCmdBindVertexBuffers;
  PFN_vkCmdBindIndexBuffer                 vkCmdBindIndexBuffer;
  PFN_vkCmdCopyBuffer                      vkCmdCopyBuffer;
  PFN_vkCmdCopyBufferToImage               vkCmdCopyBufferToImage;
  PFN_vkCmdCopyImage                       vkCmdCopyImage;
  PFN_vkCmdCopyImageToBuffer               vkCmdCopyImageToBuffer;
  PFN_vkCmdDispatch                        vkCmdDispatch;
  PFN_vkCmdTraceRaysKHR                    vkCmdTraceRaysKHR;
  PFN_vkCmdBuildAccelerationStructuresKHR  vkCmdBuildAccelerationStructuresKHR;
  PFN_vkCmdPushConstants                   vkCmdPushConstants;
  PFN_vkCmdBindDescriptorSets              vkCmdBindDescriptorSets;
  PFN_vkDestroyAccelerationStructureKHR    vkDestroyAccelerationStructureKHR;
  PFN_vkDestroySampler                     vkDestroySampler;
  PFN_vkDestroyPipeline                    vkDestroyPipeline;
  PFN_vkDestroyPipelineLayout              vkDestroyPipelineLayout;
  PFN_vkDestroyDescriptorPool              vkDestroyDescriptorPool;
  PFN_vkDestroyDescriptorSetLayout         vkDestroyDescriptorSetLayout;
  PFN_vkDestroyShaderModule                vkDestroyShaderModule;
  PFN_vkDestroyCommandPool                 vkDestroyCommandPool;
  PFN_vkDestroyImageView                   vkDestroyImageView;
  PFN_vkDestroySwapchainKHR                vkDestroySwapchainKHR;
  PFN_vkDestroySurfaceKHR                  vkDestroySurfaceKHR;
  PFN_vkDestroySemaphore                   vkDestroySemaphore;
  PFN_vkDestroyDevice                      vkDestroyDevice;
  PFN_vkDestroyInstance                    vkDestroyInstance;
  PFN_vkDestroyDebugUtilsMessengerEXT      vkDestroyDebugUtilsMessengerEXT;
  PFN_vkSetDebugUtilsObjectNameEXT         vkSetDebugUtilsObjectNameEXT;
  PFN_vkCmdBeginDebugUtilsLabelEXT         vkCmdBeginDebugUtilsLabelEXT;
  PFN_vkCmdInsertDebugUtilsLabelEXT        vkCmdInsertDebugUtilsLabelEXT;
  PFN_vkCmdEndDebugUtilsLabelEXT           vkCmdEndDebugUtilsLabelEXT;
  PFN_vkAllocateMemory                     vkAllocateMemory;
  PFN_vkBindBufferMemory                   vkBindBufferMemory;
  PFN_vkBindImageMemory                    vkBindImageMemory;
  PFN_vkCreateBuffer                       vkCreateBuffer;
  PFN_vkCreateImage                        vkCreateImage;
  PFN_vkDestroyBuffer                      vkDestroyBuffer;
  PFN_vkDestroyImage                       vkDestroyImage;
  PFN_vkFreeMemory                         vkFreeMemory;
  PFN_vkGetBufferMemoryRequirements        vkGetBufferMemoryRequirements;
  PFN_vkGetBufferMemoryRequirements2       vkGetBufferMemoryRequirements2;
  PFN_vkGetImageMemoryRequirements         vkGetImageMemoryRequirements;
  PFN_vkGetImageMemoryRequirements2        vkGetImageMemoryRequirements2;
  PFN_vkGetPhysicalDeviceMemoryProperties  vkGetPhysicalDeviceMemoryProperties;
  PFN_vkGetPhysicalDeviceProperties        vkGetPhysicalDeviceProperties;
  PFN_vkMapMemory                          vkMapMemory;
  PFN_vkUnmapMemory                        vkUnmapMemory;
  PFN_vkFlushMappedMemoryRanges            vkFlushMappedMemoryRanges;
  PFN_vkInvalidateMappedMemoryRanges       vkInvalidateMappedMemoryRanges;
  PFN_vkBindBufferMemory2                  vkBindBufferMemory2;
  PFN_vkBindImageMemory2                   vkBindImageMemory2;
  PFN_vkGetPhysicalDeviceMemoryProperties2 vkGetPhysicalDeviceMemoryProperties2;
  PFN_vkGetDeviceBufferMemoryRequirements  vkGetDeviceBufferMemoryRequirements;
  PFN_vkGetDeviceImageMemoryRequirements   vkGetDeviceImageMemoryRequirements;
  PFN_vkGetBufferDeviceAddress             vkGetBufferDeviceAddress;
  PFN_vkEnumeratePhysicalDevices           vkEnumeratePhysicalDevices;
  PFN_vkGetAccelerationStructureBuildSizesKHR
    vkGetAccelerationStructureBuildSizesKHR;
  PFN_vkGetAccelerationStructureDeviceAddressKHR
    vkGetAccelerationStructureDeviceAddressKHR;
  PFN_vkEnumerateInstanceExtensionProperties
                                         vkEnumerateInstanceExtensionProperties;
  PFN_vkEnumerateInstanceLayerProperties vkEnumerateInstanceLayerProperties;
  PFN_vkGetPhysicalDeviceQueueFamilyProperties
    vkGetPhysicalDeviceQueueFamilyProperties;
  PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR;
  PFN_vkGetPhysicalDeviceSurfacePresentModesKHR
    vkGetPhysicalDeviceSurfacePresentModesKHR;
  PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR;
} ac_vk_device;

typedef struct ac_vk_cmd_pool {
  ac_cmd_pool_internal common;
  VkCommandPool        cmd_pool;
} ac_vk_cmd_pool;

typedef struct ac_vk_cmd {
  ac_cmd_internal common;
  VkCommandBuffer cmd;
} ac_vk_cmd;

typedef struct ac_vk_queue {
  ac_queue_internal common;
  uint32_t          family;
  VkQueue           queue;
} ac_vk_queue;

typedef struct ac_vk_fence {
  ac_fence_internal common;
  VkSemaphore       semaphore;
  bool              signaled;
} ac_vk_fence;

typedef struct ac_vk_sampler {
  ac_sampler_internal common;
  VkSampler           sampler;
} ac_vk_sampler;

typedef struct ac_vk_image {
  ac_image_internal common;
  VkImage           image;
  VkImageView       srv_view;
  VkImageView*      uav_views;
  VmaAllocation     allocation;
} ac_vk_image;

typedef struct ac_vk_buffer {
  ac_buffer_internal common;
  VkBuffer           buffer;
  VmaAllocation      allocation;
} ac_vk_buffer;

typedef struct ac_vk_swapchain {
  ac_swapchain_internal common;
  VkSurfaceKHR          surface;
  VkSwapchainKHR        swapchain;
} ac_vk_swapchain;

typedef struct ac_vk_shader {
  ac_shader_internal common;
  VkShaderModule     shader;
} ac_vk_shader;

typedef struct ac_vk_dsl {
  ac_dsl_internal       common;
  VkDescriptorSetLayout dsls[ac_space_count];
} ac_vk_dsl;

typedef struct ac_vk_descriptor_buffer {
  ac_descriptor_buffer_internal common;
  VkDescriptorPool              descriptor_pool;
  VkDescriptorSet*              sets[ac_space_count];
} ac_vk_descriptor_buffer;

typedef struct ac_vk_pipeline {
  ac_pipeline_internal common;
  VkPipeline           pipeline;
  VkPipelineLayout     pipeline_layout;
  VkPipelineStageFlags push_constant_stage_flags;
} ac_vk_pipeline;

typedef struct ac_vk_sbt {
  ac_sbt_internal                 common;
  VkBuffer                        buffer;
  VmaAllocation                   allocation;
  VkStridedDeviceAddressRegionKHR raygen;
  VkStridedDeviceAddressRegionKHR miss;
  VkStridedDeviceAddressRegionKHR hit;
  VkStridedDeviceAddressRegionKHR callable;
} ac_vk_sbt;

typedef struct ac_vk_as {
  ac_as_internal                      common;
  VkAccelerationStructureKHR          as;
  VkBuffer                            buffer;
  VmaAllocation                       allocation;
  uint32_t                            geometry_count;
  VkAccelerationStructureGeometryKHR* geometries;
} ac_vk_as;

typedef struct ac_vk_queue_slot {
  uint32_t family;
  uint32_t index;
  uint32_t quality;
} ac_vk_queue_slot;

#endif
