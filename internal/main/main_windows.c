#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS)

#include <Windows.h>

#if (AC_INCLUDE_INPUT)
#pragma comment(lib, "xinput.lib")
#endif

#if (AC_INCLUDE_RENDERER)
__declspec(dllexport) extern const UINT D3D12SDKVersion;
__declspec(dllexport) extern const char* D3D12SDKPath;

__declspec(dllexport) const UINT D3D12SDKVersion = 610;
__declspec(dllexport) const char* D3D12SDKPath = ".\\";
#endif

#if (AC_INCLUDE_DEBUG)
#pragma comment(lib, "DbgHelp.lib")
#endif


static int
ac_win_common_entry_point(int argc, char** argv)
{
  ac_result res = ac_main((uint32_t)(argc), argv);

  return (int)(res);
}

int
main(int argc, char** argv)
{
  return ac_win_common_entry_point(argc, argv);
}

_Use_decl_annotations_ int WINAPI
WinMain(
  HINSTANCE hinstance,
  HINSTANCE prev_hinstance,
  LPSTR     cmdline,
  int       n_show_cmd)
{
  AC_UNUSED(hinstance);
  AC_UNUSED(prev_hinstance);
  AC_UNUSED(cmdline);
  AC_UNUSED(n_show_cmd);

  return ac_win_common_entry_point(__argc, __argv);
}

#endif
