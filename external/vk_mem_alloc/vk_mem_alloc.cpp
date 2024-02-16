#include "ac_private.h"

#if (AC_INCLUDE_DEBUG)
#define VMA_STATS_STRING_ENABLED 1
#endif
#define VMA_DEBUG_ERROR AC_ERROR
#define VMA_STATIC_VULKAN_FUNCTIONS 0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 0
#define VMA_IMPLEMENTATION

#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#endif

#define VMA_ASSERT AC_ASSERT

#define VK_NO_PROTOTYPES 1
#include <vulkan/vulkan.h>
#include "vk_mem_alloc.h"
