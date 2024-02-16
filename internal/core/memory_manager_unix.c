#include "ac_private.h"

#if (AC_PLATFORM_LINUX || AC_PLATFORM_APPLE) && (AC_INCLUDE_DEBUG)

#include <execinfo.h>
#include "memory_manager.h"

typedef struct ac_unix_stack_trace {
  uint32_t frame_count;
  char**   frames;
} ac_unix_stack_trace;

ac_result
ac_memory_manager_create_stack_trace(ac_stack_trace* trace)
{
  *trace = NULL;

  ac_unix_stack_trace* p = calloc(1, sizeof(ac_unix_stack_trace));
  if (!p)
  {
    return ac_result_out_of_host_memory;
  }
  *trace = p;

  void* bt[1024];
  p->frame_count = (uint32_t)backtrace(bt, 1024);
  p->frames = backtrace_symbols(bt, (int32_t)p->frame_count);

  return ac_result_success;
}

void
ac_memory_manager_print_stack_trace(ac_stack_trace trace)
{
  if (!trace)
  {
    return;
  }

  ac_unix_stack_trace* p = (ac_unix_stack_trace*)trace;

  for (uint32_t i = 2; i < p->frame_count; i++)
  {
    ac_print(NULL, "\t %d : %s\n", i - 2, &p->frames[i][1]);
  }
}

void
ac_memory_manager_destroy_stack_trace(ac_stack_trace trace)
{
  if (!trace)
  {
    return;
  }

  ac_unix_stack_trace* p = (ac_unix_stack_trace*)trace;
  free(p->frames);
  free(p);
}

#endif
