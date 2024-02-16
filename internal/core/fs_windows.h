#pragma once

#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS)

#include <Windows.h>
#include <ShlObj.h>
#include <stdio.h>
#include "fs.h"

typedef struct ac_windows_fs {
  ac_fs_internal common;
  char           rom_mount[AC_MAX_PATH];
  char           save_mount[AC_MAX_PATH];
} ac_windows_fs;

typedef struct ac_windows_file {
  ac_file_internal common;
  HANDLE           file;
} ac_windows_file;

#endif
