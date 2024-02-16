#pragma once

#include "ac_base.h"

#if defined(__cplusplus)
extern "C"
{
#endif

AC_DEFINE_HANDLE(ac_fs);
AC_DEFINE_HANDLE(ac_file);

#define AC_SYSTEM_FS ((ac_fs)0)
#define AC_STDOUT ((ac_file)0)

typedef enum ac_mount {
  ac_mount_rom = 0,
  ac_mount_save = 1,
  ac_mount_debug = 2,
} ac_mount;

typedef enum ac_file_mode_bit {
  ac_file_mode_read_bit = AC_BIT(0),
  ac_file_mode_write_bit = AC_BIT(1),
  ac_file_mode_append_bit = AC_BIT(2),
} ac_file_mode_bit;
typedef uint32_t ac_file_mode_bits;

typedef enum ac_seek {
  ac_seek_begin = 0,
  ac_seek_current = 1,
  ac_seek_end = 2,
} ac_seek;

AC_API bool
ac_path_exists(ac_fs fs, ac_mount mount, const char* path);

AC_API ac_result
ac_mkdir(ac_fs fs, ac_mount mount, const char* path, bool parents);

AC_API bool
ac_is_dir(ac_fs fs, ac_mount mount, const char* path);

AC_API ac_result
ac_create_file(
  ac_fs             fs,
  ac_mount          mount,
  const char*       filename,
  ac_file_mode_bits mode,
  ac_file*          file);

AC_API ac_result
ac_destroy_file(ac_file file);

AC_API ac_result
ac_file_seek(ac_file file, int64_t offset, ac_seek seek);

AC_API ac_result
ac_file_read(ac_file file, size_t size, void* buffer);

AC_API ac_result
ac_file_write(ac_file file, size_t size, const void* data);

AC_API void
ac_print(ac_file file, const char* fmt, ...);

AC_API void
ac_vprint(ac_file file, const char* fmt, va_list args);

AC_API size_t
ac_file_get_size(ac_file file);

#if defined(__cplusplus)
}
#endif
