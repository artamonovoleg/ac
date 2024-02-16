#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS) || (AC_PLATFORM_XBOX)

#include <Windows.h>

AC_API ac_result
ac_load_library(const char* name, void** _library, bool noload)
{
  AC_ASSERT(name);
  AC_ASSERT(_library);

  if (noload)
  {
    *_library = GetModuleHandleA(name);
  }
  else
  {
    *_library = LoadLibraryA(name);
  }

  if ((*_library) == NULL)
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

  FreeLibrary(library);
}

AC_API ac_result
ac_load_function(void* library, const char* name, void** function)
{
  AC_ASSERT(library);
  AC_ASSERT(name);

  FARPROC proc = GetProcAddress(library, name);
  if (!proc)
  {
    return ac_result_unknown_error;
  }

  AC_STATIC_ASSERT(sizeof(FARPROC) == sizeof(void*));

  memcpy(function, &proc, sizeof(proc));

  return ac_result_success;
}

AC_API bool
ac_is_debugger_present(void)
{
  return IsDebuggerPresent();
}

AC_API void
ac_debugbreak(void)
{
  __debugbreak();
}

#endif
