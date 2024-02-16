#include "ac_private.h"

#if (AC_INCLUDE_DEBUG)

#include "memory_manager.h"

static ac_memory_manager manager;

static int32_t
ac_memory_manager_allocation_compare(const void* a, const void* b, void* udata)
{
  AC_UNUSED(udata);
  const ac_memory_manager_allocation* a0 = a;
  const ac_memory_manager_allocation* a1 = b;

  return (a0->ptr != a1->ptr);
}

static uint64_t
ac_memory_manager_allocation_hash(
  const void* item,
  uint64_t    seed0,
  uint64_t    seed1)
{
  AC_UNUSED(seed0);
  AC_UNUSED(seed1);
  const ac_memory_manager_allocation* b = item;
  return (uintptr_t)b->ptr;
}

void
ac_memory_manager_init(void)
{
  AC_ZERO(manager);

  manager.allocation_map = hashmap_new_with_allocator(
    malloc,
    realloc,
    free,
    sizeof(ac_memory_manager_allocation),
    0,
    0,
    0,
    ac_memory_manager_allocation_hash,
    ac_memory_manager_allocation_compare,
    NULL,
    NULL);

  ac_create_mutex(&manager.mutex);

  manager.inited = true;

  AC_INFO("[ memory manager ] memory manager initilized");
}

void
ac_memory_manager_shutdown(void)
{
  if (!manager.inited)
  {
    return;
  }

  manager.inited = false;

  ac_destroy_mutex(manager.mutex);

  size_t                        iter = 0;
  ac_memory_manager_allocation* allocation;

  uint32_t leak_count = 0;
  size_t   total_leaked = 0;

  while (hashmap_iter(manager.allocation_map, &iter, (void**)&allocation))
  {
    if (allocation->freed)
    {
      continue;
    }
    AC_ERROR(
      "[ memory manager ] unfreed allocation %p %zu (bytes)",
      allocation->ptr,
      allocation->size);

    ac_memory_manager_print_stack_trace(allocation->stack_trace);
    ac_memory_manager_destroy_stack_trace(allocation->stack_trace);

    AC_DEBUGBREAK();

    total_leaked += allocation->size;
    leak_count++;
  }

  if (leak_count)
  {
    AC_ERROR(
      "[ memory manager ] leak count: %ul total leaked: %zu (bytes)",
      (unsigned long)leak_count,
      total_leaked);
  }
  else
  {
    AC_INFO("[ memory manager ] no leaks detected");
  }
  hashmap_free(manager.allocation_map);
}

void
ac_memory_manager_register_allocation(void* p, size_t size)
{
  if (!manager.inited)
  {
    return;
  }

  if (!p)
  {
    return;
  }

  ac_mutex_lock(manager.mutex);

  ac_stack_trace trace;
  if (ac_memory_manager_create_stack_trace(&trace) != ac_result_success)
  {
    trace = NULL;
  }

  ac_memory_manager_allocation allocation = {
    .ptr = p,
    .size = size,
    .freed = false,
    .stack_trace = trace,
  };

  (void)hashmap_set(manager.allocation_map, &allocation);

  ac_mutex_unlock(manager.mutex);
}

void
ac_memory_manager_register_deallocation(void* p)
{
  if (!manager.inited)
  {
    return;
  }

  if (!p)
  {
    return;
  }

  ac_mutex_lock(manager.mutex);

  ac_memory_manager_allocation* allocation = hashmap_get(
    manager.allocation_map,
    &(ac_memory_manager_allocation) {
      .ptr = p,
    });

  if (!allocation)
  {
    AC_ERROR("[ memory manager ] freed memory wasn't allocated");
    AC_DEBUGBREAK();
  }
  else
  {
    if (allocation->freed)
    {
      AC_ERROR("[ memory manager ] double free detected");
      ac_stack_trace trace;
      if (ac_memory_manager_create_stack_trace(&trace) == ac_result_success)
      {
        ac_memory_manager_print_stack_trace(trace);
        ac_memory_manager_destroy_stack_trace(trace);
      }
      AC_DEBUGBREAK();
    }

    allocation->freed = true;
  }

  ac_mutex_unlock(manager.mutex);
}

#endif
