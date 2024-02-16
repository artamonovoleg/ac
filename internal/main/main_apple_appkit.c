#include "ac_private.h"

#if (AC_PLATFORM_APPLE_MACOS)

int
main(int argc, char** argv)
{
  ac_result res = ac_main((uint32_t)argc, argv);
  return (int)res;
}

#endif
