#pragma once

#include <stdbool.h>

#if !defined(AC_NO_STDINT)
#if defined(_MSC_VER) && (_MSC_VER < 1600)
typedef signed __int8    int8_t;
typedef unsigned __int8  uint8_t;
typedef signed __int16   int16_t;
typedef unsigned __int16 uint16_t;
typedef signed __int32   int32_t;
typedef unsigned __int32 uint32_t;
typedef signed __int64   int64_t;
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif
#endif

#if !defined(AC_NO_STDDEF)
#include <stddef.h>
#endif

#if !defined(AC_NO_STDARG)
#include <stdarg.h>
#endif


#if (defined(_WIN32) || defined(_WIN64))
#if !defined(_GAMING_XBOX)
#define AC_PLATFORM_WINDOWS 1
#else
#define AC_PLATFORM_XBOX 1
#if defined(_GAMING_XBOX_SCARLETT)
#define AC_PLATFORM_XBOX_XS 1
#elif defined(_GAMING_XBOX_XBOXONE)
#define AC_PLATFORM_XBOX_ONE 1
#endif
#endif
#elif defined(__linux) || defined(__linux__)
#define AC_PLATFORM_LINUX 1
#elif defined(__APPLE__) && defined(__MACH__)
#define AC_PLATFORM_APPLE 1
#include <TargetConditionals.h>
#define AC_PLATFORM_APPLE_MACOS TARGET_OS_OSX
#define AC_PLATFORM_APPLE_NOT_MACOS (!(TARGET_OS_OSX))
#define AC_PLATFORM_APPLE_IOS TARGET_OS_IOS
#define AC_PLATFORM_APPLE_TVOS TARGET_OS_TV
#define AC_PLATFORM_APPLE_VISIONOS TARGET_OS_VISION
#define AC_PLATFORM_APPLE_SIMULATOR TARGET_OS_SIMULATOR
#elif defined(__NINTENDO__)
#define AC_PLATFORM_NINTENDO_SWITCH 1
#elif defined(__ORBIS__) || defined(__PROSPERO__)
#define AC_PLATFORM_PS 1
#if defined(__ORBIS__)
#define AC_PLATFORM_PS4 1
#elif defined(__PROSPERO__)
#define AC_PLATFORM_PS5 1
#endif
#else
#error "platform is unsupported"
#endif

#if !defined(AC_PLATFORM_WINDOWS)
#define AC_PLATFORM_WINDOWS 0
#endif
#if !defined(AC_PLATFORM_XBOX)
#define AC_PLATFORM_XBOX 0
#endif
#if !defined(AC_PLATFORM_XBOX_XS)
#define AC_PLATFORM_XBOX_XS 0
#endif
#if !defined(AC_PLATFORM_XBOX_ONE)
#define AC_PLATFORM_XBOX_ONE 0
#endif
#if !defined(AC_PLATFORM_LINUX)
#define AC_PLATFORM_LINUX 0
#endif
#if !defined(AC_PLATFORM_APPLE)
#define AC_PLATFORM_APPLE 0
#endif
#if !defined(AC_PLATFORM_APPLE_MACOS)
#define AC_PLATFORM_APPLE_MACOS 0
#endif
#if !defined(AC_PLATFORM_APPLE_IOS)
#define AC_PLATFORM_APPLE_IOS 0
#endif
#if !defined(AC_PLATFORM_NINTENDO_SWITCH)
#define AC_PLATFORM_NINTENDO_SWITCH 0
#endif
#if !defined(AC_PLATFORM_PS)
#define AC_PLATFORM_PS 0
#endif
#if !defined(AC_PLATFORM_PS4)
#define AC_PLATFORM_PS4 0
#endif
#if !defined(AC_PLATFORM_PS5)
#define AC_PLATFORM_PS5 0
#endif

#if !defined(AC_INCLUDE_DEBUG)
#define AC_INCLUDE_DEBUG 1
#endif

#if !defined(AC_INCLUDE_WINDOW)
#define AC_INCLUDE_WINDOW 1
#endif

#if !defined(AC_INCLUDE_INPUT)
#define AC_INCLUDE_INPUT 1
#endif

#if !defined(AC_INCLUDE_RENDERER)
#define AC_INCLUDE_RENDERER 1
#endif

#if !(AC_INCLUDE_RENDERER)
#undef AC_INCLUDE_RG
#define AC_INCLUDE_RG 0
#endif

#if !defined(AC_INCLUDE_RG)
#define AC_INCLUDE_RG 1
#endif

#if !defined(AC_API)
#define AC_API
#endif

#if defined(__cplusplus)
#define AC_CAST(T, V) (static_cast<T>((V)))
#define AC_REINTERPET_CAST(T, V) (reinterpret_cast<T>((V)))
#else
#define AC_CAST(T, V) ((T)(V))
#define AC_REINTERPET_CAST(T, V) ((T)(V))
#endif

#if (AC_INCLUDE_DEBUG)
#define AC_DEBUGBREAK()                                                        \
  do                                                                           \
  {                                                                            \
    if (ac_is_debugger_present())                                              \
    {                                                                          \
      ac_debugbreak();                                                         \
    }                                                                          \
  }                                                                            \
  while (false)
#define AC_ASSERT(expr)                                                        \
  do                                                                           \
  {                                                                            \
    if (!(expr))                                                               \
    {                                                                          \
      AC_ERROR(                                                                \
        "assertion failed. expr: %s file: %s func: %s line: %d",               \
        #expr,                                                                 \
        (__FILE__),                                                            \
        (__func__),                                                            \
        AC_CAST(uint32_t, __LINE__));                                          \
      AC_DEBUGBREAK();                                                         \
    }                                                                          \
  }                                                                            \
  while (false)
#else
#define AC_DEBUGBREAK() ((void)0)
#define AC_ASSERT(expr) ((void)0)
#endif

#define AC_STATIC_ASSERT(X) static_assert(X, "")

#define AC_OFFSETOF(s, m) ((size_t) & (((s*)0)->m))

#define AC_UNUSED(x) (void)(x)
#define AC_MAYBE_UNUSED(x) (void)(x)

#define AC_COUNTOF(x) ((sizeof((x))) / sizeof((x[0])))
#define AC_ZERO(x) memset(&(x), 0, sizeof((x)))
#define AC_ZEROP(x) memset((x), 0, sizeof((*x)))

#define AC_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define AC_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define AC_CLAMP(x, min, max)                                                  \
  (((x) < (min)) ? (min) : ((max) < (x)) ? (max) : (x))
#define AC_ALIGN_UP(value, multiple)                                           \
  ((((value) + (multiple)-1) / (multiple)) * (multiple))

#define AC_DEFINE_HANDLE(s)                                                    \
  struct s##_internal;                                                         \
  typedef struct s##_internal* s

#define AC_BIT(x) (1 << x)

#if (AC_INCLUDE_DEBUG)
#define AC_DEBUG_NAME(x) (x)
#else
#define AC_DEBUG_NAME(x) (NULL)
#endif

#define AC_MAX_PATH 2048

typedef enum ac_result {
  ac_result_success = 0,
  ac_result_not_ready = 1,
  ac_result_canceled = 2,
  ac_result_unknown_error = -1,
  ac_result_out_of_host_memory = -2,
  ac_result_out_of_device_memory = -3,
  ac_result_invalid_argument = -4,
  ac_result_device_lost = -5,
  ac_result_out_of_date = -6,
  ac_result_bad_usage = -7,
  ac_result_format_not_supported = -8,
  ac_result_exported_image_not_supported = -9,
} ac_result;

#define AC_RIF(x)                                                              \
  do                                                                           \
  {                                                                            \
    ac_result __ac_rif_res = (x);                                              \
    if (__ac_rif_res != ac_result_success)                                     \
    {                                                                          \
      AC_DEBUGBREAK();                                                         \
      return __ac_rif_res;                                                     \
    }                                                                          \
  }                                                                            \
  while (0)
