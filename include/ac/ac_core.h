#pragma once

#include "ac_base.h"

#if defined(__cplusplus)
extern "C"
{
#endif

typedef enum ac_log_level {
  ac_log_level_debug = 0,
  ac_log_level_info = 2,
  ac_log_level_warn = 3,
  ac_log_level_error = 4,
} ac_log_level;

typedef enum ac_time_units {
  ac_time_unit_seconds = 0,
  ac_time_unit_milliseconds = 1,
  ac_time_unit_microseconds = 2,
  ac_time_unit_nanoseconds = 3,
} ac_time_units;

typedef struct ac_init_info {
  const char* app_name;
  bool        enable_memory_manager;
  const char* debug_rom;
} ac_init_info;

AC_API ac_result
ac_init(const ac_init_info* info);

AC_API void
ac_shutdown(void);

AC_API ac_result
ac_load_library(const char* name, void** library, bool noload);

AC_API void
ac_unload_library(void* library);

AC_API ac_result
ac_load_function(void* library, const char* name, void** function);

AC_API uint64_t
ac_get_time(ac_time_units units);

AC_API void
ac_sleep(ac_time_units units, int64_t value);

AC_API void*
ac_alloc(size_t size);

AC_API void*
ac_calloc(size_t size);

AC_API void*
ac_realloc(void* p, size_t size);

AC_API void*
ac_aligned_alloc(size_t size, size_t alignment);

AC_API void
ac_free(void* p);

AC_API void
ac_log(ac_log_level level, const char* fmt, ...);

#define AC_DEBUG(...) ac_log(ac_log_level_debug, __VA_ARGS__)
#define AC_INFO(...) ac_log(ac_log_level_info, __VA_ARGS__)
#define AC_WARN(...) ac_log(ac_log_level_warn, __VA_ARGS__)
#define AC_ERROR(...) ac_log(ac_log_level_error, __VA_ARGS__)

AC_API bool
ac_is_debugger_present(void);

AC_API void
ac_debugbreak(void);

#if defined(__cplusplus)
}
#endif
