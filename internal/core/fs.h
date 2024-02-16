#pragma once

#include "ac_private.h"

#include <stdio.h>
#include <stdarg.h>

#if defined(__cplusplus)
extern "C"
{
#endif

typedef struct ac_fs_internal {
  ac_result (*init)(const ac_init_info* info);
  void (*shutdown)(ac_fs);
  const char* (*get_mount_prefix)(ac_fs, ac_mount);
  bool (*path_exists)(ac_fs, ac_mount, const char*);
  ac_result (*mkdir)(ac_fs, ac_mount, const char*);
  bool (*is_dir)(ac_fs, ac_mount, const char*);
  ac_result (
    *create_file)(ac_fs, ac_mount, const char*, ac_file_mode_bits, ac_file*);
  ac_result (*destroy_file)(ac_file);
  ac_result (*seek)(ac_file, int64_t, ac_seek);
  ac_result (*read)(ac_file, size_t, void*);
  ac_result (*write)(ac_file, size_t, const void*);
  void (*print)(ac_file, const char*, va_list);
  size_t (*get_size)(ac_file);
} ac_fs_internal;

typedef struct ac_file_internal {
  ac_fs    fs;
  ac_mount mount;
} ac_file_internal;

#if (AC_PLATFORM_LINUX)
ac_result
ac_linux_fs_init(const ac_init_info* info, ac_fs* fs);
#endif

#if (AC_PLATFORM_WINDOWS)
ac_result
ac_windows_fs_init(const ac_init_info* info, ac_fs* fs);
#endif

#if (AC_PLATFORM_XBOX)
ac_result
ac_xbox_fs_init(const ac_init_info* info, ac_fs* fs);
#endif

#if (AC_PLATFORM_APPLE)
ac_result
ac_apple_fs_init(const ac_init_info* info, ac_fs* fs);
#endif

#if (AC_PLATFORM_NINTENDO_SWITCH)
ac_result
ac_nn_fs_init(const ac_init_info* info, ac_fs* fs);
#endif

#if (AC_PLATFORM_PS)
ac_result
ac_ps_fs_init(const ac_init_info* info, ac_fs* fs);
#endif

#if defined(__cplusplus)
}
#endif
