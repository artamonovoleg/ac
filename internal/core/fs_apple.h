#pragma once

#include "ac_private.h"

#if (AC_PLATFORM_APPLE)

#include <sys/stat.h>
#import <Foundation/Foundation.h>

#include "fs.h"

typedef struct ac_apple_fs {
  ac_fs_internal common;
  char           rom_mount[AC_MAX_PATH];
  char           save_mount[AC_MAX_PATH];
} ac_apple_fs;

typedef struct ac_apple_file {
  ac_file_internal common;
  int              fd;
} ac_apple_file;

#endif
