#include "ac_private.h"

#if (AC_PLATFORM_LINUX) || (AC_PLATFORM_APPLE)

#include <pthread.h>
#include <limits.h>
#include <unistd.h>

AC_STATIC_ASSERT(AC_MAX_THREAD_NAME <= 16);

typedef struct ac_thread_internal {
  pthread_t          thread;
  ac_thread_function function;
  void*              function_data;
#if (AC_INCLUDE_DEBUG)
  char name[AC_MAX_THREAD_NAME];
#endif
} ac_thread_internal;

typedef struct ac_mutex_internal {
  pthread_mutex_t mutex;
} ac_mutex_internal;

typedef struct ac_shared_mutex_internal {
  pthread_rwlock_t lock;
} ac_shared_mutex_internal;

typedef struct ac_cond_internal {
  pthread_cond_t cond;
} ac_cond_internal;

static void*
ac_unix_start_routine(void* thread_handle)
{
  ac_thread thread = thread_handle;

#if (AC_INCLUDE_DEBUG)
  if (thread->name[0])
  {
#if (AC_PLATFORM_APPLE)
    (void)(pthread_setname_np(thread->name));
#else
    (void)(pthread_setname_np(thread->thread, thread->name));
#endif
  }
#endif

  ac_result result = thread->function(thread->function_data);

  void* value = (void*)((uintptr_t)(result));

  return value;
}

AC_API ac_result
ac_create_thread(const ac_thread_info* info, ac_thread* p)
{
  AC_ASSERT(p);

  ac_thread thread = ac_calloc(sizeof(ac_thread_internal));
  *p = thread;

  if (!thread)
  {
    return ac_result_out_of_host_memory;
  }

  thread->function = info->function;
  thread->function_data = info->function_data;

  size_t page_size = sysconf(_SC_PAGESIZE);
  size_t stack_size_aligned =
    AC_ALIGN_UP(AC_MAX(info->stack_size, AC_MIN_THREAD_STACK_SIZE), page_size);

  pthread_attr_t attr;
  if (pthread_attr_init(&attr) != 0)
  {
    return ac_result_unknown_error;
  }

  int                policy = SCHED_FIFO;
  int                inherit;
  struct sched_param param;

  switch (info->priority)
  {
  case ac_thread_priority_lowest:
    inherit = PTHREAD_EXPLICIT_SCHED;
    policy = SCHED_FIFO;
    param.sched_priority = sched_get_priority_min(policy);
    break;
  case ac_thread_priority_default:
    inherit = PTHREAD_INHERIT_SCHED;
    // can be any, they are ignored
    policy = SCHED_FIFO;
    param.sched_priority = sched_get_priority_max(policy);
    break;
  case ac_thread_priority_highest:
    inherit = PTHREAD_EXPLICIT_SCHED;
    policy = SCHED_FIFO;
    param.sched_priority = sched_get_priority_max(policy);
    break;
  default:
    return ac_result_invalid_argument;
  }

  if (pthread_attr_setstacksize(&attr, stack_size_aligned) != 0)
  {
    return ac_result_invalid_argument;
  }

  if (pthread_attr_setinheritsched(&attr, inherit) != 0)
  {
    return ac_result_invalid_argument;
  }

  if (pthread_attr_setschedpolicy(&attr, policy) != 0)
  {
    return ac_result_invalid_argument;
  }

  if (pthread_attr_setschedparam(&attr, &param) != 0)
  {
    return ac_result_invalid_argument;
  }

  if (
    pthread_create(&thread->thread, &attr, ac_unix_start_routine, thread) != 0)
  {
    return ac_result_unknown_error;
  }

#if (AC_INCLUDE_DEBUG)
  if (info->name)
  {
    strncpy(thread->name, info->name, AC_MAX_THREAD_NAME - 1);
  }
#endif

  if (pthread_attr_destroy(&attr) != 0)
  {
    return ac_result_unknown_error;
  }

  return ac_result_success;
}

AC_API ac_result
ac_destroy_thread(ac_thread thread)
{
  if (!thread)
  {
    return ac_result_success;
  }

  if (pthread_join(thread->thread, NULL))
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

  ac_mutex m = ac_calloc(sizeof(ac_mutex_internal));
  pthread_mutex_init(&m->mutex, NULL);
  *p = m;
}

AC_API void
ac_destroy_mutex(ac_mutex mtx)
{
  if (!mtx)
  {
    return;
  }

  pthread_mutex_destroy(&mtx->mutex);
  ac_free(mtx);
}

AC_API ac_result
ac_mutex_try_lock(ac_mutex mtx)
{
  AC_ASSERT(mtx);

  if (pthread_mutex_trylock(&mtx->mutex) == 0)
  {
    return ac_result_success;
  }

  return ac_result_not_ready;
}

AC_API ac_result
ac_mutex_lock(ac_mutex mtx)
{
  AC_ASSERT(mtx);

  if (pthread_mutex_lock(&mtx->mutex) == 0)
  {
    return ac_result_success;
  }
  else
  {
    return ac_result_not_ready;
  }
}

AC_API void
ac_mutex_unlock(ac_mutex mtx)
{
  AC_ASSERT(mtx);

  pthread_mutex_unlock(&mtx->mutex);
}

AC_API void
ac_create_shared_mutex(ac_shared_mutex* p)
{
  AC_ASSERT(p);

  ac_shared_mutex m = ac_calloc(sizeof(ac_shared_mutex_internal));
  pthread_rwlock_init(&m->lock, NULL);
  *p = m;
}

AC_API void
ac_destroy_shared_mutex(ac_shared_mutex mtx)
{
  if (!mtx)
  {
    return;
  }

  pthread_rwlock_destroy(&mtx->lock);
  ac_free(mtx);
}

AC_API void
ac_shared_mutex_lock_read(ac_shared_mutex mtx)
{
  AC_ASSERT(mtx);

  pthread_rwlock_rdlock(&mtx->lock);
}

AC_API ac_result
ac_shared_mutex_try_lock_read(ac_shared_mutex mtx)
{
  AC_ASSERT(mtx);

  if (pthread_rwlock_tryrdlock(&mtx->lock) == 0)
  {
    return ac_result_success;
  }

  return ac_result_not_ready;
}

AC_API void
ac_shared_mutex_unlock_read(ac_shared_mutex mtx)
{
  AC_ASSERT(mtx);

  pthread_rwlock_unlock(&mtx->lock);
}

AC_API void
ac_shared_mutex_lock_write(ac_shared_mutex mtx)
{
  AC_ASSERT(mtx);

  pthread_rwlock_wrlock(&mtx->lock);
}

AC_API ac_result
ac_shared_mutex_try_lock_write(ac_shared_mutex mtx)
{
  AC_ASSERT(mtx);

  if (pthread_rwlock_trywrlock(&mtx->lock) == 0)
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

  pthread_rwlock_unlock(&mtx->lock);
}

AC_API void
ac_create_cond(ac_cond* p)
{
  AC_ASSERT(p);

  ac_cond c = ac_calloc(sizeof(ac_cond_internal));
  pthread_cond_init(&c->cond, NULL);
  *p = c;
}

AC_API void
ac_destroy_cond(ac_cond c)
{
  if (!c)
  {
    return;
  }

  pthread_cond_destroy(&c->cond);
  ac_free(c);
}

AC_API void
ac_cond_signal(ac_cond c)
{
  AC_ASSERT(c);

  pthread_cond_signal(&c->cond);
}

AC_API void
ac_cond_broadcast(ac_cond c)
{
  AC_ASSERT(c);

  pthread_cond_broadcast(&c->cond);
}

AC_API ac_result
ac_cond_wait(ac_cond c, ac_mutex m)
{
  AC_ASSERT(m);
  AC_ASSERT(c);

  if (pthread_cond_wait(&c->cond, &m->mutex) == 0)
  {
    return ac_result_success;
  }
  else
  {
    return ac_result_not_ready;
  }
}

#endif
