#include "ac_private.h"

#if (AC_PLATFORM_LINUX) || (AC_PLATFORM_APPLE)

#include <time.h>
#include "timer.h"

static struct {
  struct timespec start;
} unix_time;

ac_result
ac_init_time(void)
{
  clock_gettime(CLOCK_MONOTONIC_RAW, &unix_time.start);
  return ac_result_success;
}

void
ac_shutdown_time(void)
{}

AC_API uint64_t
ac_get_time(ac_time_units units)
{
  struct timespec now;
  clock_gettime(CLOCK_MONOTONIC_RAW, &now);
  int64_t sec = (int64_t)(now.tv_sec - unix_time.start.tv_sec);
  int64_t ns = (int64_t)(now.tv_nsec - unix_time.start.tv_nsec);

  switch (units)
  {
  case ac_time_unit_seconds:
    return (uint64_t)sec + (uint64_t)AC_NS_TO_SEC(ns);
  case ac_time_unit_milliseconds:
    return (uint64_t)AC_SEC_TO_MS(sec) + (uint64_t)AC_NS_TO_MS(ns);
  case ac_time_unit_microseconds:
    return (uint64_t)AC_SEC_TO_US(sec) + (uint64_t)AC_NS_TO_US(ns);
  case ac_time_unit_nanoseconds:
    return (uint64_t)AC_SEC_TO_NS(sec) + (uint64_t)ns;
  default:
    break;
  }
  return 0;
}

AC_API void
ac_sleep(ac_time_units units, int64_t value)
{
  int64_t sec = 0;
  int64_t us = 0;

  switch (units)
  {
  case ac_time_unit_seconds:
    sec = value;
    break;
  case ac_time_unit_milliseconds:
    us = AC_MS_TO_NS(value);
    break;
  case ac_time_unit_microseconds:
    us = AC_US_TO_NS(value);
    break;
  case ac_time_unit_nanoseconds:
    break;
  default:
    break;
  }

  nanosleep((const struct timespec[]) {{sec, us}}, NULL);
}

#endif
