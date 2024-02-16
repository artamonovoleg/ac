#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS || AC_PLATFORM_XBOX)

#include <Windows.h>
#include <process.h>

typedef struct ac_thread_internal {
  HANDLE             thread;
  ac_thread_function function;
  void*              function_data;
#if (AC_INCLUDE_DEBUG)
  wchar_t name[AC_MAX_THREAD_NAME];
#endif
} ac_thread_internal;

typedef struct ac_mutex_internal {
  CRITICAL_SECTION section;
} ac_mutex_internal;

typedef struct ac_shared_mutex_internal {
  CRITICAL_SECTION section;
} ac_shared_mutex_internal;

typedef struct ac_cond_internal {
  CONDITION_VARIABLE cond;
} ac_cond_internal;

static unsigned int
ac_winapi_thread_proc(void* thread_handle)
{
  ac_thread_internal* thread = thread_handle;

  ac_result res = thread->function(thread->function_data);

  _endthreadex((unsigned int)res);

  return (unsigned int)res;
}

AC_API ac_result
ac_create_thread(const ac_thread_info* info, ac_thread* p)
{
  AC_ASSERT(p);

  ac_thread thread = ac_calloc(sizeof(ac_thread_internal));

  (*p) = thread;

  if (!thread)
  {
    return ac_result_out_of_host_memory;
  }

  thread->function = info->function;
  thread->function_data = info->function_data;

  int priority;

  switch (info->priority)
  {
  case ac_thread_priority_default:
    priority = THREAD_PRIORITY_NORMAL;
    break;
  case ac_thread_priority_lowest:
    priority = THREAD_PRIORITY_LOWEST;
    break;
  case ac_thread_priority_highest:
    priority = THREAD_PRIORITY_HIGHEST;
    break;
  default:
    return ac_result_invalid_argument;
  }

  thread->thread = (HANDLE)(_beginthreadex(
    NULL,
    AC_MAX(info->stack_size, AC_MIN_THREAD_STACK_SIZE),
    ac_winapi_thread_proc,
    thread,
    0,
    NULL));

  if (!thread->thread)
  {
    return ac_result_unknown_error;
  }

  if (!SetThreadPriority(thread->thread, priority))
  {
    return ac_result_unknown_error;
  }

#if (AC_INCLUDE_DEBUG)
  if (info->name)
  {
    mbstowcs(thread->name, info->name, AC_MAX_THREAD_NAME - 1);
    if (FAILED(SetThreadDescription(thread->thread, thread->name)))
    {
      return ac_result_invalid_argument;
    }
  }
#endif

  return ac_result_success;
}

AC_API ac_result
ac_destroy_thread(ac_thread thread)
{
  if (!thread)
  {
    return ac_result_success;
  }

  if (WaitForSingleObject(thread->thread, INFINITE) == WAIT_FAILED)
  {
    return ac_result_unknown_error;
  }

  if (!CloseHandle(thread->thread))
  {
    return ac_result_unknown_error;
  }

  ac_free(thread);
  return ac_result_success;
}

AC_API void
ac_create_mutex(ac_mutex* p)
{
  AC_ASSERT(p);

  *p = ac_calloc(sizeof(ac_mutex_internal));

  ac_mutex m = *p;
  InitializeCriticalSection(&m->section);
}

AC_API void
ac_destroy_mutex(ac_mutex mtx)
{
  if (!mtx)
  {
    return;
  }

  DeleteCriticalSection(&mtx->section);
  ac_free(mtx);
}

AC_API ac_result
ac_mutex_try_lock(ac_mutex mtx)
{
  AC_ASSERT(mtx);

  if (TryEnterCriticalSection(&mtx->section))
  {
    return ac_result_success;
  }
  else
  {
    return ac_result_not_ready;
  }
}

AC_API ac_result
ac_mutex_lock(ac_mutex mtx)
{
  AC_ASSERT(mtx);

  EnterCriticalSection(&mtx->section);

  return ac_result_success;
}

AC_API void
ac_mutex_unlock(ac_mutex mtx)
{
  AC_ASSERT(mtx);

  LeaveCriticalSection(&mtx->section);
}

AC_API void
ac_create_shared_mutex(ac_shared_mutex* p)
{
  AC_ASSERT(p);

  *p = ac_calloc(sizeof(ac_shared_mutex_internal));

  ac_shared_mutex m = *p;
  InitializeCriticalSection(&m->section);
}

AC_API void
ac_destroy_shared_mutex(ac_shared_mutex mtx)
{
  if (!mtx)
  {
    return;
  }

  DeleteCriticalSection(&mtx->section);
  ac_free(mtx);
}

AC_API void
ac_shared_mutex_lock_read(ac_shared_mutex mtx)
{
  AC_ASSERT(mtx);

  EnterCriticalSection(&mtx->section);
}

AC_API ac_result
ac_shared_mutex_try_lock_read(ac_shared_mutex mtx)
{
  AC_ASSERT(mtx);

  if (TryEnterCriticalSection(&mtx->section))
  {
    return ac_result_success;
  }
  else
  {
    return ac_result_not_ready;
  }
}

AC_API void
ac_shared_mutex_unlock_read(ac_shared_mutex mtx)
{
  AC_ASSERT(mtx);

  LeaveCriticalSection(&mtx->section);
}

AC_API void
ac_shared_mutex_lock_write(ac_shared_mutex mtx)
{
  AC_ASSERT(mtx);

  EnterCriticalSection(&mtx->section);
}

AC_API ac_result
ac_shared_mutex_try_lock_write(ac_shared_mutex mtx)
{
  AC_ASSERT(mtx);

  if (TryEnterCriticalSection(&mtx->section))
  {
    return ac_result_success;
  }
  else
  {
    return ac_result_not_ready;
  }
}

AC_API void
ac_shared_mutex_unlock_write(ac_shared_mutex mtx)
{
  AC_ASSERT(mtx);

  LeaveCriticalSection(&mtx->section);
}

AC_API void
ac_create_cond(ac_cond* p)
{
  AC_ASSERT(p);

  *p = ac_calloc(sizeof(ac_cond_internal));

  ac_cond c = *p;
  InitializeConditionVariable(&c->cond);
}

AC_API void
ac_destroy_cond(ac_cond c)
{
  if (!c)
  {
    return;
  }

  ac_free(c);
}

AC_API void
ac_cond_signal(ac_cond c)
{
  AC_ASSERT(c);

  WakeConditionVariable(&c->cond);
}

AC_API void
ac_cond_broadcast(ac_cond c)
{
  AC_ASSERT(c);

  WakeAllConditionVariable(&c->cond);
}

AC_API ac_result
ac_cond_wait(ac_cond c, ac_mutex m)
{
  AC_ASSERT(m);
  AC_ASSERT(c);

  if (SleepConditionVariableCS(&c->cond, &m->section, INFINITE))
  {
    return ac_result_success;
  }
  else
  {
    return ac_result_not_ready;
  }
}

#endif
