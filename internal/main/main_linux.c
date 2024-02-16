#include "ac_private.h"

#if (AC_PLATFORM_LINUX)

int
main(int argc, char** argv)
{
  ac_result res = ac_main((uint32_t)argc, argv);
  return (int)res;
}

#endif
