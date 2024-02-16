#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS) || (AC_PLATFORM_XBOX)
#include <Windows.h>
#else
#include <unistd.h>
#endif

#if (AC_INCLUDE_DEBUG)
#include "memory_manager.h"
#endif

AC_API void*
ac_alloc(size_t size)
{
  void* ptr = NULL;

#if (AC_PLATFORM_WINDOWS) || (AC_PLATFORM_XBOX)
  ptr = _aligned_malloc(size, 2);
#else
  ptr = malloc(size);
#endif

#if (AC_INCLUDE_DEBUG)
  ac_memory_manager_register_allocation(ptr, size);
#endif

  return ptr;
}

AC_API void*
ac_calloc(size_t size)
{
  void* ptr = NULL;
#if (AC_PLATFORM_WINDOWS) || (AC_PLATFORM_XBOX)
  ptr = _aligned_malloc(size, 2);
  if (!ptr)
  {
    return ptr;
  }

  memset(ptr, 0, size);

#else
  ptr = calloc(1, size);
#endif

#if (AC_INCLUDE_DEBUG)
  ac_memory_manager_register_allocation(ptr, size);
#endif

  return ptr;
}

AC_API void*
ac_realloc(void* p, size_t size)
{
  void* ptr = NULL;

#if (AC_INCLUDE_DEBUG)
  ac_memory_manager_register_deallocation(p);
#endif

#if (AC_PLATFORM_WINDOWS) || (AC_PLATFORM_XBOX)
  ptr = _aligned_realloc(p, size, 2);
#else
  ptr = realloc(p, size);
#endif

#if (AC_INCLUDE_DEBUG)
  ac_memory_manager_register_allocation(ptr, size);
#endif

  return ptr;
}

AC_API void
ac_free(void* p)
{
#if (AC_INCLUDE_DEBUG)
  ac_memory_manager_register_deallocation(p);
#endif

#if (AC_PLATFORM_WINDOWS) || (AC_PLATFORM_XBOX)
  _aligned_free(p);
#else
  free(p);
#endif
}

AC_API void*
ac_aligned_alloc(size_t size, size_t alignment)
{
  void* ptr = NULL;

  alignment = AC_MAX(alignment, sizeof(void*));
  size = AC_ALIGN_UP(size, alignment);

#if (AC_PLATFORM_WINDOWS) || (AC_PLATFORM_XBOX)
  ptr = _aligned_malloc(size, alignment);
#else
  ptr = aligned_alloc(alignment, size);
#endif

#if (AC_INCLUDE_DEBUG)
  ac_memory_manager_register_allocation(ptr, size);
#endif

  return ptr;
}

AC_API ac_result
ac_init(const ac_init_info* info)
{
  AC_ASSERT(info->app_name);

  AC_RIF(ac_init_time());
  AC_RIF(ac_init_fs(info));
  AC_RIF(ac_init_log());

#if (AC_INCLUDE_DEBUG)
  if (info->enable_memory_manager)
  {
    ac_memory_manager_init();
  }
#endif

  return ac_result_success;
}

AC_API void
ac_shutdown(void)
{
#if (AC_INCLUDE_DEBUG)
  ac_memory_manager_shutdown();
#endif

  ac_shutdown_log();
  ac_shutdown_fs();
  ac_shutdown_time();
}
