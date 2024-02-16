#include "ac_private.h"
#include "fs.h"

static ac_fs g_fs;

static inline void
ac_prepare_path(ac_fs fs, ac_mount mount, const char* in, char* out)
{
  out[0] = '\0';

  const char* prefix = fs->get_mount_prefix(fs, mount);
  strcpy(out, prefix);
  strcat(out, "/");
  strcat(out, in);
}

ac_result
ac_init_fs(const ac_init_info* info)
{
  if (g_fs)
  {
    return ac_result_success;
  }

  ac_result (*init)(const ac_init_info* info, ac_fs* fs) = NULL;

#if (AC_PLATFORM_LINUX)
  init = ac_linux_fs_init;
#elif (AC_PLATFORM_APPLE)
  init = ac_apple_fs_init;
#elif (AC_PLATFORM_WINDOWS)
  init = ac_windows_fs_init;
#elif (AC_PLATFORM_XBOX)
  init = ac_xbox_fs_init;
#elif (AC_PLATFORM_NINTENDO_SWITCH)
  init = ac_nn_fs_init;
#elif (AC_PLATFORM_PS5) || (AC_PLATFORM_PS4)
  init = ac_ps_fs_init;
#endif

  if (!init)
  {
    return ac_result_unknown_error;
  }

  ac_result res = init(info, &g_fs);

  if (res != ac_result_success)
  {
    g_fs->shutdown(g_fs);
    ac_free(g_fs);
    g_fs = NULL;
  }

  return res;
}

void
ac_shutdown_fs(void)
{
  if (!g_fs)
  {
    return;
  }

  g_fs->shutdown(g_fs);
  ac_free(g_fs);
}

AC_API bool
ac_path_exists(ac_fs fs, ac_mount mount, const char* path)
{
  if (mount == ac_mount_debug && !AC_INCLUDE_DEBUG)
  {
    return false;
  }
  AC_ASSERT(path);

  if (!fs)
  {
    fs = g_fs;
  }

  char buf[AC_MAX_PATH] = {'\0'};
  ac_prepare_path(fs, mount, path, buf);
  return fs->path_exists(fs, mount, buf);
}

AC_API ac_result
ac_mkdir(ac_fs fs, ac_mount mount, const char* path, bool parents)
{
  if (mount == ac_mount_debug && !AC_INCLUDE_DEBUG)
  {
    return ac_result_invalid_argument;
  }

  if (!fs)
  {
    fs = g_fs;
  }

  char buf[AC_MAX_PATH] = {'\0'};
  ac_prepare_path(fs, mount, path, buf);

  if (!parents)
  {
    return fs->mkdir(fs, mount, buf);
  }

  size_t size = strnlen(buf, AC_MAX_PATH);
  if (buf[size - 1] == '/')
  {
    buf[size - 1] = '\0';
  }

  if (ac_path_exists(fs, mount, buf))
  {
    if (ac_is_dir(fs, mount, buf))
    {
      return ac_result_success;
    }
  }

  char* p = NULL;

  for (p = buf + 1; *p; p++)
  {
    if (*p == '/')
    {
      *p = 0;
      if (!ac_path_exists(fs, mount, p))
      {
        ac_result r = fs->mkdir(fs, mount, p);
        if (r != ac_result_success)
        {
          return r;
        }
      }
      else if (!ac_is_dir(fs, mount, p))
      {
        return ac_result_unknown_error;
      }
      *p = '/';
    }
  }

  if (!ac_path_exists(fs, mount, buf))
  {
    ac_result r = fs->mkdir(fs, mount, buf);
    if (r != ac_result_success)
    {
      return r;
    }
  }
  else if (!ac_is_dir(fs, mount, buf))
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

AC_API bool
ac_is_dir(ac_fs fs, ac_mount mount, const char* path)
{
  if (mount == ac_mount_debug && !AC_INCLUDE_DEBUG)
  {
    return false;
  }

  AC_ASSERT(path);

  if (!fs)
  {
    fs = g_fs;
  }

  char buf[AC_MAX_PATH] = {'\0'};
  ac_prepare_path(fs, mount, path, buf);
  return fs->is_dir(fs, mount, buf);
}

AC_API ac_result
ac_create_file(
  ac_fs             fs,
  ac_mount          mount,
  const char*       path,
  ac_file_mode_bits mode,
  ac_file*          file)
{
  if (mount == ac_mount_debug && !AC_INCLUDE_DEBUG)
  {
    return ac_result_invalid_argument;
  }

  AC_ASSERT(path);
  AC_ASSERT(file);
  AC_ASSERT(mode);
  AC_ASSERT((mount != ac_mount_rom) || (!(mode & ac_file_mode_write_bit)));

  if (!fs)
  {
    fs = g_fs;
  }

  AC_ASSERT(fs);

  *file = NULL;

  char buf[AC_MAX_PATH] = {'\0'};

  ac_prepare_path(fs, mount, path, buf);
  ac_result res = fs->create_file(fs, mount, buf, mode, file);

  if (res != ac_result_success)
  {
    fs->destroy_file(*file);
    *file = NULL;
    AC_DEBUG("failed to open file %s", path);
    return res;
  }

  (*file)->mount = mount;
  (*file)->fs = fs;

  return res;
}

AC_API ac_result
ac_destroy_file(ac_file file)
{
  if (!file)
  {
    return ac_result_success;
  }

  ac_result res = file->fs->destroy_file(file);

  if (res != ac_result_success)
  {
    return res;
  }

  ac_free(file);
  return ac_result_success;
}

AC_API ac_result
ac_file_seek(ac_file file, int64_t offset, ac_seek seek)
{
  AC_ASSERT(file);
  return file->fs->seek(file, offset, seek);
}

AC_API ac_result
ac_file_read(ac_file file, size_t size, void* buffer)
{
  AC_ASSERT(file);
  AC_ASSERT(buffer);
  return file->fs->read(file, size, buffer);
}

AC_API ac_result
ac_file_write(ac_file file, size_t size, const void* data)
{
  AC_ASSERT(file);
  AC_ASSERT(data);
  AC_ASSERT(file->mount != ac_mount_rom);

  return file->fs->write(file, size, data);
}

AC_API void
ac_print(ac_file file, const char* fmt, ...)
{
  AC_ASSERT(fmt);

  va_list args;
  va_start(args, fmt);
  ac_vprint(file, fmt, args);
  va_end(args);
}

AC_API void
ac_vprint(ac_file file, const char* fmt, va_list args)
{
  AC_ASSERT(fmt);

  va_list args_copy;
  va_copy(args_copy, args);

  if (file)
  {
    file->fs->print(file, fmt, args_copy);
  }
  else
  {
    vprintf(fmt, args_copy);
  }
}

AC_API size_t
ac_file_get_size(ac_file file)
{
  AC_ASSERT(file);
  return file->fs->get_size(file);
}
