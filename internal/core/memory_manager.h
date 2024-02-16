#include "ac_private.h"

#if (AC_INCLUDE_DEBUG)

typedef void* ac_stack_trace;

typedef struct ac_memory_manager_allocation {
  void*          ptr;
  size_t         size;
  bool           freed;
  ac_stack_trace stack_trace;
} ac_memory_manager_allocation;

typedef struct ac_memory_manager {
  struct hashmap* allocation_map;
  bool            inited;
  ac_mutex        mutex;
} ac_memory_manager;

#if defined(__cplusplus)
extern "C"
{
#endif

void
ac_memory_manager_init(void);

void
ac_memory_manager_shutdown(void);

void
ac_memory_manager_register_allocation(void* p, size_t size);

void
ac_memory_manager_register_deallocation(void* p);

ac_result
ac_memory_manager_create_stack_trace(ac_stack_trace* trace);

void
ac_memory_manager_print_stack_trace(ac_stack_trace trace);

void
ac_memory_manager_destroy_stack_trace(ac_stack_trace trace);

#if defined(__cplusplus)
}
#endif

#endif
