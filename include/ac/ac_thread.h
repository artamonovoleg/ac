#pragma once

#include "ac_base.h"

#if defined(__cplusplus)
extern "C"
{
#endif

#define AC_MAX_THREAD_NAME (16)

AC_DEFINE_HANDLE(ac_thread);
AC_DEFINE_HANDLE(ac_mutex);
AC_DEFINE_HANDLE(ac_shared_mutex);
AC_DEFINE_HANDLE(ac_cond);

typedef ac_result (*ac_thread_function)(void*);

typedef enum ac_thread_priority {
  ac_thread_priority_default = 0,
  ac_thread_priority_lowest = 1,
  ac_thread_priority_highest = 2,
} ac_thread_priority;

typedef struct ac_thread_info {
  uint32_t           stack_size;
  ac_thread_priority priority;
  void*              function_data;
  ac_thread_function function;
  const char*        name;
} ac_thread_info;

AC_API ac_result
ac_create_thread(const ac_thread_info* info, ac_thread* thread);

AC_API ac_result
ac_destroy_thread(ac_thread thread);

AC_API void
ac_create_mutex(ac_mutex* mtx);

AC_API void
ac_destroy_mutex(ac_mutex mtx);

AC_API ac_result
ac_mutex_try_lock(ac_mutex mtx);

AC_API ac_result
ac_mutex_lock(ac_mutex mtx);

AC_API void
ac_mutex_unlock(ac_mutex mtx);

AC_API void
ac_create_shared_mutex(ac_shared_mutex* mtx);

AC_API void
ac_destroy_shared_mutex(ac_shared_mutex mtx);

AC_API void
ac_shared_mutex_lock_read(ac_shared_mutex mtx);

AC_API ac_result
ac_shared_mutex_try_lock_read(ac_shared_mutex mtx);

AC_API void
ac_shared_mutex_unlock_read(ac_shared_mutex mtx);

AC_API void
ac_shared_mutex_lock_write(ac_shared_mutex mtx);

AC_API ac_result
ac_shared_mutex_try_lock_write(ac_shared_mutex mtx);

AC_API void
ac_shared_mutex_unlock_write(ac_shared_mutex mtx);

AC_API void
ac_create_cond(ac_cond* cv);

AC_API void
ac_destroy_cond(ac_cond cv);

AC_API void
ac_cond_signal(ac_cond cv);

AC_API void
ac_cond_broadcast(ac_cond cv);

AC_API ac_result
ac_cond_wait(ac_cond cv, ac_mutex mtx);

#if defined(__cplusplus)
}
#endif
