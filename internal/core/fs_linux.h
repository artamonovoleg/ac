#pragma once

#include "ac_private.h"

#if (AC_PLATFORM_LINUX)

#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <libgen.h>
#include <pwd.h>
#include <sys/stat.h>
#include "fs.h"

typedef struct ac_linux_fs {
  ac_fs_internal common;
  char           rom_mount[AC_MAX_PATH];
  char           save_mount[AC_MAX_PATH];
} ac_linux_fs;

typedef struct ac_linux_file {
  ac_file_internal common;
  int              fd;
} ac_linux_file;

#endif
