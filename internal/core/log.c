#include "ac_private.h"

#include <stdio.h>
#include <stdarg.h>

static ac_file g_log_file = NULL;

ac_result
ac_init_log(void)
{
  ac_mount mount = ac_mount_debug;

  if (AC_PLATFORM_WINDOWS || AC_PLATFORM_LINUX || AC_PLATFORM_APPLE_MACOS)
  {
    mount = ac_mount_save;
  }

  if (mount == ac_mount_debug && !AC_INCLUDE_DEBUG)
  {
    return ac_result_success;
  }

  if (
    ac_create_file(
      AC_SYSTEM_FS,
      mount,
      "ac-log.txt",
      ac_file_mode_write_bit,
      &g_log_file) != ac_result_success)
  {
    g_log_file = NULL;
  }

  return ac_result_success;
}

void
ac_shutdown_log(void)
{
  ac_destroy_file(g_log_file);
  g_log_file = NULL;
}

AC_API void
ac_log(ac_log_level level, const char* fmt, ...)
{
  if (level == ac_log_level_debug && !AC_INCLUDE_DEBUG)
  {
    return;
  }

  const char* levelc = "";

  switch (level)
  {
  case ac_log_level_debug:
    levelc = "[ debug ]";
    break;
  case ac_log_level_info:
    levelc = "[ info ]";
    break;
  case ac_log_level_warn:
    levelc = "[ warn ]";
    break;
  case ac_log_level_error:
    levelc = "[ error ]";
    break;
  default:
    levelc = "";
    break;
  }

  va_list carg;
  va_start(carg, fmt);

  ac_print(NULL, "%s ", levelc);
  ac_vprint(NULL, fmt, carg);
  ac_print(NULL, "\n");

  if (g_log_file)
  {
    ac_print(g_log_file, "%s ", levelc);
    ac_vprint(g_log_file, fmt, carg);
    ac_print(g_log_file, "\n");
  }

  va_end(carg);

  fflush(stdout);
}
