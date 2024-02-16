#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS)

#include "fs_windows.h"

static inline ac_result
ac_windows_fs_from_wide(const wchar_t* wstr, int max_characters, char* str)
{
  if (wcstombs(str, wstr, (size_t)max_characters) == (size_t)(-1))
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

static inline ac_result
ac_windows_fs_to_wide(const char* str, int max_characters, wchar_t* wstr)
{
  size_t length = mbstowcs(wstr, str, (size_t)max_characters);

  if (length == (size_t)(-1))
  {
    return ac_result_unknown_error;
  }

  for (size_t i = 0; i < length; ++i)
  {
    if (wstr[i] == '/')
    {
      wstr[i] = '\\';
    }
  }

  return ac_result_success;
}

static void
ac_windows_fs_shutdown(ac_fs fs_handle)
{
  AC_UNUSED(fs_handle);
}

static const char*
ac_windows_fs_get_mount_prefix(ac_fs fs_handle, ac_mount mount)
{
  AC_FROM_HANDLE(fs, ac_windows_fs);

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
ac_windows_fs_path_exists(ac_fs fs_handle, ac_mount mount, const char* path)
{
  AC_UNUSED(fs_handle);
  AC_UNUSED(mount);

  wchar_t buf[AC_MAX_PATH] = {'\0'};
  AC_RIF(ac_windows_fs_to_wide(path, AC_MAX_PATH, buf));

  DWORD dw_attrib = GetFileAttributesW(buf);
  return (dw_attrib != INVALID_FILE_ATTRIBUTES);
}

static ac_result
ac_windows_fs_mkdir(ac_fs fs_handle, ac_mount mount, const char* path)
{
  AC_UNUSED(fs_handle);
  AC_UNUSED(mount);

  wchar_t buf[AC_MAX_PATH] = {'\0'};
  AC_RIF(ac_windows_fs_to_wide(path, AC_MAX_PATH, buf));

  DWORD attrib = GetFileAttributesW(buf);
  if (
    (attrib != INVALID_FILE_ATTRIBUTES) && (attrib & FILE_ATTRIBUTE_DIRECTORY))
  {
    return ac_result_success;
  }

  if (!CreateDirectoryW(buf, NULL))
  {
    AC_DEBUG("[ fs ] [ windows ] : %s", GetLastError());
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

static bool
ac_windows_fs_is_dir(ac_fs fs_handle, ac_mount mount, const char* path)
{
  AC_UNUSED(fs_handle);
  AC_UNUSED(mount);

  wchar_t buf[AC_MAX_PATH] = {'\0'};
  if (ac_windows_fs_to_wide(path, AC_MAX_PATH, buf) != ac_result_success)
  {
    return false;
  }

  DWORD dw_attrib = GetFileAttributesW(buf);
  return (
    (dw_attrib != INVALID_FILE_ATTRIBUTES) &&
    (dw_attrib & FILE_ATTRIBUTE_DIRECTORY));
}

static ac_result
ac_windows_fs_create_file(
  ac_fs             fs_handle,
  ac_mount          mount,
  const char*       path,
  ac_file_mode_bits mode,
  ac_file*          file_handle)
{
  AC_UNUSED(fs_handle);
  AC_UNUSED(mount);

  AC_INIT_INTERNAL(file, ac_windows_file);

  wchar_t buf[AC_MAX_PATH];
  AC_RIF(ac_windows_fs_to_wide(path, AC_MAX_PATH, buf));

  DWORD access = 0;

  if (mode & ac_file_mode_read_bit)
  {
    access |= GENERIC_READ;
  }
  if (mode & ac_file_mode_write_bit)
  {
    access |= GENERIC_WRITE;
  }

  DWORD share_mode = 0;
  if (mode & ac_file_mode_read_bit)
  {
    share_mode |= FILE_SHARE_READ;
  }
  if (mode & ac_file_mode_write_bit)
  {
    share_mode |= FILE_SHARE_WRITE;
  }

  DWORD disposition = 0;

  if ((mode & ac_file_mode_read_bit) == ac_file_mode_read_bit)
  {
    disposition = OPEN_EXISTING;
  }
  else if (mode & ac_file_mode_append_bit)
  {
    disposition = OPEN_ALWAYS;
  }
  else if (mode & ac_file_mode_write_bit)
  {
    disposition = CREATE_ALWAYS;
  }

  file->file = CreateFileW(
    buf,
    access,
    share_mode,
    NULL,
    disposition,
    FILE_ATTRIBUTE_NORMAL,
    NULL);

  if (file->file == INVALID_HANDLE_VALUE)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

static ac_result
ac_windows_fs_destroy_file(ac_file file_handle)
{
  AC_FROM_HANDLE(file, ac_windows_file);

  if (file->file != INVALID_HANDLE_VALUE)
  {
    if (!CloseHandle(file->file))
    {
      return ac_result_unknown_error;
    }
  }

  return ac_result_success;
}

static ac_result
ac_windows_fs_file_seek(ac_file file_handle, int64_t offset, ac_seek seek)
{
  AC_FROM_HANDLE(file, ac_windows_file);

  LARGE_INTEGER off = {
    .QuadPart = offset,
  };

  DWORD move_method;

  switch (seek)
  {
  case ac_seek_begin:
    move_method = FILE_BEGIN;
    break;
  case ac_seek_current:
    move_method = FILE_CURRENT;
    break;
  case ac_seek_end:
    move_method = FILE_END;
    break;
  default:
    AC_ASSERT(false);
    return ac_result_unknown_error;
  }

  if (!SetFilePointerEx(file->file, off, NULL, move_method))
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

static ac_result
ac_windows_fs_file_read(ac_file file_handle, size_t size, void* buffer)
{
  AC_FROM_HANDLE(file, ac_windows_file);

  DWORD bytes;
  if (!ReadFile(file->file, buffer, (DWORD)size, &bytes, NULL))
  {
    AC_DEBUG("[ fs ] [ windows ] : %s", GetLastError());
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

static ac_result
ac_windows_fs_file_write(ac_file file_handle, size_t size, const void* data)
{
  AC_FROM_HANDLE(file, ac_windows_file);

  DWORD bytes;
  if (!WriteFile(file->file, data, (DWORD)size, &bytes, NULL))
  {
    AC_DEBUG("[ fs ] [ windows ] : %s", GetLastError());
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

static void
ac_windows_fs_file_print(ac_file file_handle, const char* fmt, va_list args)
{
  AC_FROM_HANDLE(file, ac_windows_file);

  int size = _vscprintf(fmt, args);
  if (size <= 0)
  {
    return;
  }

  size += 1;

  char* buffer = ac_alloc((size_t)size * sizeof(char));
  if (!buffer)
  {
    return;
  }

  int num_written = vsnprintf(buffer, (size_t)size, fmt, args);

  if (num_written == size - 1)
  {
    DWORD writen_size;
    (void)(WriteFile(
      file->file,
      buffer,
      (DWORD)num_written,
      &writen_size,
      NULL));
  }

  ac_free(buffer);
}

static size_t
ac_windows_file_get_size(ac_file file_handle)
{
  AC_FROM_HANDLE(file, ac_windows_file);

  LARGE_INTEGER size;
  if (!GetFileSizeEx(file->file, &size))
  {
    AC_DEBUG("[ fs ] [ windows ] : %s", GetLastError());
    return 0;
  }
  return (size_t)size.QuadPart;
}

ac_result
ac_windows_fs_init(const ac_init_info* info, ac_fs* fs_handle)
{
  AC_INIT_INTERNAL(fs, ac_windows_fs);

  fs->common.shutdown = ac_windows_fs_shutdown;
  fs->common.get_mount_prefix = ac_windows_fs_get_mount_prefix;
  fs->common.path_exists = ac_windows_fs_path_exists;
  fs->common.mkdir = ac_windows_fs_mkdir;
  fs->common.is_dir = ac_windows_fs_is_dir;
  fs->common.create_file = ac_windows_fs_create_file;
  fs->common.destroy_file = ac_windows_fs_destroy_file;
  fs->common.seek = ac_windows_fs_file_seek;
  fs->common.read = ac_windows_fs_file_read;
  fs->common.write = ac_windows_fs_file_write;
  fs->common.print = ac_windows_fs_file_print;
  fs->common.get_size = ac_windows_file_get_size;

  PWSTR saved_games = NULL;

  if (FAILED(SHGetKnownFolderPath(
        &FOLDERID_SavedGames,
        KF_FLAG_CREATE,
        NULL,
        &saved_games)))
  {
    AC_ERROR("[ windows ] [ fs ] failed to get saved games dir");
    return ac_result_unknown_error;
  }

  AC_RIF(ac_windows_fs_from_wide(saved_games, AC_MAX_PATH, fs->save_mount));

  CoTaskMemFree(saved_games);

  strcat(fs->save_mount, "\\ac-software\\");
  strcat(fs->save_mount, info->app_name);
  strcat(fs->save_mount, "\\");

  {
    wchar_t save[AC_MAX_PATH];
    AC_RIF(ac_windows_fs_to_wide(fs->save_mount, AC_MAX_PATH, save));
    int res = SHCreateDirectoryExW(NULL, save, NULL);
    if (res != ERROR_SUCCESS && res != ERROR_ALREADY_EXISTS)
    {
      return ac_result_unknown_error;
    }
  }

  {
    wchar_t rom[AC_MAX_PATH] = {'\0'};

    if (!GetModuleFileNameW(NULL, rom, AC_MAX_PATH))
    {
      return ac_result_unknown_error;
    }

    int32_t len = (int)(wcsnlen_s(rom, AC_MAX_PATH));
    while (rom[len - 1] != '\\' && len >= 0)
    {
      len--;
    }

    rom[len] = '\0';

    AC_RIF(ac_windows_fs_from_wide(rom, AC_MAX_PATH, fs->rom_mount));
  }

  if (AC_INCLUDE_DEBUG)
  {
    if (info->debug_rom)
    {
      fs->rom_mount[0] = '\0';
      strcpy(fs->rom_mount, info->debug_rom);
    }
  }

  return ac_result_success;
}

#endif
