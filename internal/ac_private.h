#pragma once

#if defined(_MSC_VER)
#pragma warning(disable : 4127)
#pragma warning(disable : 4201)
#pragma warning(disable : 4820)
#pragma warning(disable : 4061)
#pragma warning(disable : 5045)
#pragma warning(disable : 4577)
#pragma warning(disable : 5204)
#pragma warning(disable : 4115)
#pragma warning(disable : 4514)
#pragma warning(disable : 4710)
#pragma warning(disable : 4711)
#endif

#if defined(__clang__)
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#pragma clang diagnostic ignored "-Wreserved-identifier"
#pragma clang diagnostic ignored "-Wswitch-enum"
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wcovered-switch-default"
#pragma clang diagnostic ignored "-Wc++98-compat"
#pragma clang diagnostic ignored "-Wc++98-compat-pedantic"
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#pragma clang diagnostic ignored "-Wnullable-to-nonnull-conversion"
#pragma clang diagnostic ignored "-Wobjc-messaging-id"
#pragma clang diagnostic ignored "-Wdirect-ivar-access"
#pragma clang diagnostic ignored "-Wobjc-interface-ivars"
#pragma clang diagnostic ignored "-Wdocumentation"
#pragma clang diagnostic ignored "-Wzero-as-null-pointer-constant"
#pragma clang diagnostic ignored "-Wcast-function-type-strict"
#pragma clang diagnostic ignored "-Wincompatible-function-pointer-types"
#pragma clang diagnostic ignored "-Wincompatible-function-pointer-types-strict"
#if defined(_WIN32)
#pragma clang diagnostic ignored "-Wlanguage-extension-token"
#endif

#endif

#if defined(_MSC_VER)
#define _UNICODE 1
#define UNICODE 1
#define _CRT_SECURE_NO_WARNINGS 1
#define NOMINMAX 1
#if !defined(AC_WIN_NO_LEAN_AND_MEAN)
#define VC_EXTRALEAN 1
#define WIN32_LEAN_AND_MEAN 1
#endif
#endif

#if defined(__linux) || defined(__linux__)
#define _GNU_SOURCE 1
#endif

#include "ac/ac.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <c_hashmap/c_hashmap.h>
#include <c_array/c_array.h>

#define AC_INIT_INTERNAL(name, type)                                           \
  type* name = AC_CAST(type*, ac_calloc(sizeof(type)));                        \
  *name##_handle = &name->common

#define AC_FROM_HANDLE(name, impl)                                             \
  impl* name = AC_CAST(impl*, AC_CAST(void*, name##_handle))

#define AC_FROM_HANDLE2(name, p, impl)                                         \
  impl* name = AC_CAST(impl*, AC_CAST(void*, p))

#define AC_MIN_THREAD_STACK_SIZE 16384

static inline void*
ac_const_cast(const void* p)
{
  union {
    const void* c;
    void*       nc;
  } u;

  u.c = p;

  return u.nc;
}

#if (AC_PLATFORM_APPLE)
#define AC_OBJC_BEGIN_ARP()                                                    \
  @autoreleasepool                                                             \
  {                                                                            \
    (void)0
#define AC_OBJC_END_ARP()                                                      \
  }                                                                            \
  (void)0
#endif

#if (AC_INCLUDE_RENDERER) &&                                                   \
  ((AC_PLATFORM_LINUX) || (AC_PLATFORM_WINDOWS) || (AC_PLATFORM_APPLE) ||      \
   (AC_PLATFORM_NINTENDO_SWITCH))
#define AC_INCLUDE_VULKAN 1
#else
#define AC_INCLUDE_VULKAN 0
#endif

#if defined(__cplusplus)
extern "C"
{
#endif

AC_API ac_result
ac_main(uint32_t argc, char** argv);

ac_result
ac_init_log(void);

void
ac_shutdown_log(void);

ac_result
ac_init_time(void);

void
ac_shutdown_time(void);

ac_result
ac_init_fs(const ac_init_info* info);

void
ac_shutdown_fs(void);

#if defined(__cplusplus)
}
#endif
