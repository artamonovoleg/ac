#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS) && (AC_INCLUDE_DEBUG)

#include <Windows.h>
#include <DbgHelp.h>
#include <stdio.h>
#include "memory_manager.h"

#define MAX_CALLERS 62
typedef struct ac_windows_stack_trace {
  uint16_t frames;
  void*    callers_stack[MAX_CALLERS];
} ac_windows_stack_trace;

ac_result
ac_memory_manager_create_stack_trace(ac_stack_trace* trace)
{
  *trace = NULL;

  ac_windows_stack_trace* p = calloc(1, sizeof(ac_windows_stack_trace));
  if (!p)
  {
    return ac_result_out_of_host_memory;
  }
  *trace = p;

  p->frames = CaptureStackBackTrace(0, MAX_CALLERS, p->callers_stack, NULL);

  return ac_result_success;
}

void
ac_memory_manager_print_stack_trace(ac_stack_trace trace)
{
  if (!trace)
  {
    return;
  }

  ac_windows_stack_trace* p = (ac_windows_stack_trace*)trace;

  HANDLE process = GetCurrentProcess();
  SymInitialize(process, NULL, TRUE);

  SYMBOL_INFO* symbol =
    (SYMBOL_INFO*)calloc(1, sizeof(SYMBOL_INFO) + 256 * sizeof(char));
  symbol->MaxNameLen = 255;
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);

  for (uint32_t i = 2; i < p->frames; i++)
  {
    SymFromAddr(process, (DWORD64)(uintptr_t)(p->callers_stack[i]), 0, symbol);
    ac_print(
      NULL,
      "\t %d : %p %s 0x%ull\n",
      i - 2,
      p->callers_stack[i],
      symbol->Name,
      symbol->Address);
    fflush(stdout);
  }

  free(symbol);
}

void
ac_memory_manager_destroy_stack_trace(ac_stack_trace trace)
{
  if (!trace)
  {
    return;
  }

  ac_windows_stack_trace* p = (ac_windows_stack_trace*)trace;
  free(p);
}

#endif
