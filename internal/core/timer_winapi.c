#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS || AC_PLATFORM_XBOX)

#include <Windows.h>
#include "timer.h"

static struct {
  LARGE_INTEGER start;
  LARGE_INTEGER freq;
} windows_time;

ac_result
ac_init_time(void)
{
  if (!QueryPerformanceFrequency(&windows_time.freq))
  {
    return ac_result_unknown_error;
  }

  if (!QueryPerformanceCounter(&windows_time.start))
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

void
ac_shutdown_time(void)
{}

AC_API uint64_t
ac_get_time(ac_time_units units)
{
  LARGE_INTEGER now;
  (void)QueryPerformanceCounter(&now);

  uint64_t diff = (uint64_t)(now.QuadPart - windows_time.start.QuadPart);
  uint64_t freq = (uint64_t)AC_MAX(windows_time.freq.QuadPart, 1);

  switch (units)
  {
  case ac_time_unit_nanoseconds:
    return AC_SEC_TO_NS(diff) / freq;
  case ac_time_unit_microseconds:
    return AC_SEC_TO_US(diff) / freq;
  case ac_time_unit_milliseconds:
    return AC_SEC_TO_MS(diff) / freq;
  case ac_time_unit_seconds:
    return diff / freq;
  default:
    AC_ASSERT(false);
    break;
  }

  return 0;
}

AC_API void
ac_sleep(ac_time_units units, int64_t value)
{
  switch (units)
  {
  case ac_time_unit_nanoseconds:
    value = AC_NS_TO_SEC(value);
    break;
  case ac_time_unit_microseconds:
    value = AC_US_TO_SEC(value);
    break;
  case ac_time_unit_milliseconds:
    value = AC_MS_TO_SEC(value);
    break;
  case ac_time_unit_seconds:
    break;
  default:
    AC_ASSERT(false);
    break;
  }

  HANDLE timer = CreateWaitableTimer(NULL, TRUE, NULL);

  if (!timer)
  {
    return;
  }

  LARGE_INTEGER li;
  li.QuadPart = -value;
  if (!SetWaitableTimer(timer, &li, 0, NULL, NULL, FALSE))
  {
    CloseHandle(timer);
    return;
  }

  WaitForSingleObject(timer, INFINITE);
  CloseHandle(timer);
}

#endif
