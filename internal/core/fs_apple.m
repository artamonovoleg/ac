#include "ac_private.h"

#if (AC_PLATFORM_APPLE)

#include "fs_apple.h"

static void
ac_apple_fs_shutdown(ac_fs fs_handle)
{
  AC_UNUSED(fs_handle);
}

static const char*
ac_apple_fs_get_mount_prefix(ac_fs fs_handle, ac_mount mount)
{
  AC_FROM_HANDLE(fs, ac_apple_fs);

  switch (mount)
  {
  case ac_mount_rom:
    return fs->rom_mount;
  case ac_mount_save:
    return fs->save_mount;
  case ac_mount_debug:
    return ".";
  default:
    break;
  }

  AC_ASSERT(false);
  return NULL;
}

static bool
ac_apple_fs_path_exists(ac_fs fs_handle, ac_mount mount, const char* path)
{
  AC_UNUSED(fs_handle);
  AC_UNUSED(mount);

  struct stat sb;
  return stat(path, &sb) == 0;
}

static ac_result
ac_apple_fs_create_file(
  ac_fs             fs,
  ac_mount          mount,
  const char*       path,
  ac_file_mode_bits mode,
  ac_file*          file_handle)
{
  AC_UNUSED(fs);
  AC_UNUSED(mount);

  AC_INIT_INTERNAL(file, ac_apple_file);

  int flags = 0;

  if (mode & ac_file_mode_write_bit)
  {
    flags |= O_CREAT;

    if (!(mode & ac_file_mode_append_bit))
    {
      flags |= O_TRUNC;
    }

    if (mode & ac_file_mode_read_bit)
    {
      flags |= O_RDWR;
    }
    else
    {
      flags |= O_WRONLY;
    }
  }
  else
  {
    flags |= O_RDONLY;
  }

  mode_t m = S_IRUSR;
  if (mode & ac_file_mode_write_bit)
  {
    m |= S_IWUSR;
  }

  file->fd = open(path, flags, m);

  if (file->fd == -1)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

static ac_result
ac_apple_fs_destroy_file(ac_file file_handle)
{
  AC_FROM_HANDLE(file, ac_apple_file);

  if (file->fd != -1)
  {
    if (close(file->fd) != 0)
    {
      return ac_result_unknown_error;
    }
  }

  return ac_result_success;
}

static ac_result
ac_apple_fs_file_seek(ac_file file_handle, int64_t offset, ac_seek seek)
{
  AC_FROM_HANDLE(file, ac_apple_file);

  int s = 0;
  switch (seek)
  {
  case ac_seek_begin:
    s = SEEK_SET;
    break;
  case ac_seek_end:
    s = SEEK_END;
    break;
  case ac_seek_current:
    s = SEEK_CUR;
    break;
  default:
    return ac_result_invalid_argument;
  }

  int64_t off = lseek(file->fd, offset, s);

  if (off == -1)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

static ac_result
ac_apple_fs_file_read(ac_file file_handle, size_t size, void* buffer)
{
  AC_FROM_HANDLE(file, ac_apple_file);

  if ((size_t)read(file->fd, buffer, size) != size)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

static ac_result
ac_apple_fs_file_write(ac_file file_handle, size_t size, const void* data)
{
  AC_FROM_HANDLE(file, ac_apple_file);

  if ((size_t)write(file->fd, data, size) != size)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

static void
ac_apple_fs_file_print(ac_file file_handle, const char* fmt, va_list list)
{
  AC_FROM_HANDLE(file, ac_apple_file);

  (void)vdprintf(file->fd, fmt, list);
}

static size_t
ac_apple_fs_file_get_size(ac_file file_handle)
{
  AC_FROM_HANDLE(file, ac_apple_file);

  struct stat s;
  if (fstat(file->fd, &s) == -1)
  {
    return (size_t)-1;
  }
  return (size_t)s.st_size;
}

static bool
ac_apple_fs_is_dir(ac_fs fs_handle, ac_mount mount, const char* path)
{
  AC_UNUSED(fs_handle);
  AC_UNUSED(mount);

  struct stat sb;
  return stat(path, &sb) == 0 && S_ISDIR(sb.st_mode);
}

static ac_result
ac_apple_fs_mkdir(ac_fs fs_handle, ac_mount mount, const char* path)
{
  AC_UNUSED(fs_handle);
  AC_UNUSED(mount);

  struct stat sb;
  if (stat(path, &sb) != 0)
  {
    if (mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO) < 0)
    {
      return ac_result_success;
    }
  }
  else if (!S_ISDIR(sb.st_mode))
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

ac_result
ac_apple_fs_init(const ac_init_info* info, ac_fs* fs_handle)
{
  AC_OBJC_BEGIN_ARP();

  AC_INIT_INTERNAL(fs, ac_apple_fs);

  fs->common.shutdown = ac_apple_fs_shutdown;
  fs->common.get_mount_prefix = ac_apple_fs_get_mount_prefix;
  fs->common.path_exists = ac_apple_fs_path_exists;
  fs->common.mkdir = ac_apple_fs_mkdir;
  fs->common.is_dir = ac_apple_fs_is_dir;
  fs->common.create_file = ac_apple_fs_create_file;
  fs->common.destroy_file = ac_apple_fs_destroy_file;
  fs->common.seek = ac_apple_fs_file_seek;
  fs->common.read = ac_apple_fs_file_read;
  fs->common.write = ac_apple_fs_file_write;
  fs->common.print = ac_apple_fs_file_print;
  fs->common.get_size = ac_apple_fs_file_get_size;

  const char* resource_path = [[[NSBundle mainBundle] resourcePath] UTF8String];
  if (!resource_path)
  {
    AC_ERROR("failed to get resource path");
    return ac_result_unknown_error;
  }

  strncpy(fs->rom_mount, resource_path, strlen(resource_path));

  if (AC_INCLUDE_DEBUG && AC_PLATFORM_APPLE_MACOS)
  {
    if (info->debug_rom)
    {
      fs->rom_mount[0] = '\0';
      strcpy(fs->rom_mount, info->debug_rom);
    }
  }

  const char* home_path = [NSHomeDirectory() UTF8String];
  if (!home_path)
  {
    AC_ERROR("failed to get home directory");
    return ac_result_unknown_error;
  }

  strncpy(fs->save_mount, home_path, strlen(home_path));
  strncat(
    fs->save_mount,
    "/Library/Application Support/",
    strlen("/Library/Application Support/"));
  strncat(fs->save_mount, "ac-software/", strlen("ac-software/"));
  mkdir(fs->save_mount, S_IRWXU | S_IRWXG | S_IRWXO);
  strncat(fs->save_mount, info->app_name, strlen(info->app_name));
  mkdir(fs->save_mount, S_IRWXU | S_IRWXG | S_IRWXO);

  AC_OBJC_END_ARP();

  return ac_result_success;
}

#endif
