#include "ac_private.h"

#if (AC_PLATFORM_LINUX || AC_PLATFORM_APPLE)

#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <stdnoreturn.h>
#if (AC_PLATFORM_APPLE)
#include <sys/sysctl.h>
#endif

AC_API ac_result
ac_load_library(const char* name, void** library, bool noload)
{
  AC_ASSERT(name);
  AC_ASSERT(library);

  int32_t flags = RTLD_NOW;

  if (noload)
  {
    flags |= RTLD_NOLOAD;
  }

  *library = dlopen(name, flags);

  if ((*library) == NULL)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

AC_API void
ac_unload_library(void* library)
{
  if (!library)
  {
    return;
  }

  dlclose(library);
}

AC_API ac_result
ac_load_function(void* library, const char* name, void** function)
{
  AC_ASSERT(library);
  AC_ASSERT(name);

  void* sym = dlsym(library, name);

  if (!sym)
  {
    return ac_result_unknown_error;
  }

  memcpy(function, &sym, sizeof(sym));

  return ac_result_success;
}

AC_API bool
ac_is_debugger_present(void)
{
#if (AC_PLATFORM_APPLE)
  int32_t mib[] = {
    CTL_KERN,
    KERN_PROC,
    KERN_PROC_PID,
    getpid(),
  };

  struct kinfo_proc info;
  size_t            size = sizeof(info);

  if (sysctl(mib, AC_COUNTOF(mib), &info, &size, NULL, 0))
  {
    return false;
  }

  return (info.kp_proc.p_flag & P_TRACED);
#else
  static bool is_debugger_present = false;
  static bool checked_already = false;

  if (checked_already)
  {
    return is_debugger_present;
  }

  char buf[4096];

  const int status_fd = open("/proc/self/status", O_RDONLY);
  if (status_fd == -1)
  {
    return is_debugger_present;
  }

  const ssize_t num_read = read(status_fd, buf, sizeof(buf) - 1);
  close(status_fd);

  if (num_read <= 0)
  {
    return is_debugger_present;
  }

  checked_already = true;

  buf[num_read] = '\0';

  const char  tracer_pid_string[] = "TracerPid:";
  const char* tracer_pid_ptr = strstr(buf, tracer_pid_string);

  if (!tracer_pid_ptr)
  {
    return is_debugger_present;
  }

  for (const char* character_ptr =
         tracer_pid_ptr + sizeof(tracer_pid_string) - 1;
       character_ptr <= buf + num_read;
       ++character_ptr)
  {
    if (isspace(*character_ptr))
    {
      continue;
    }
    else
    {
      is_debugger_present =
        isdigit(*character_ptr) != 0 && *character_ptr != '0';
      break;
    }
  }

  return is_debugger_present;
#endif
}

AC_API void
ac_debugbreak(void)
{
#if __has_builtin(__builtin_debugtrap)
  __builtin_debugtrap();
#else
  __asm__ volatile("int $0x03");
#endif
}

#endif
