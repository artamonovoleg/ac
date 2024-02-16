#pragma once

#define AC_SEC_TO_MS(sec) ((sec) * (1000))
#define AC_SEC_TO_US(sec) ((sec) * (1000000))
#define AC_SEC_TO_NS(sec) ((sec) * (1000000000))
#define AC_NS_TO_SEC(ns) ((ns) / (1000000000))
#define AC_NS_TO_MS(ns) ((ns) / (1000000))
#define AC_NS_TO_US(ns) ((ns) / (1000))

#define AC_US_TO_SEC(us) ((us) / (1000000))

#define AC_MS_TO_SEC(ms) ((ms) / (1000))

#define AC_MS_TO_NS(ms) ((ms) * (1000000))
#define AC_US_TO_NS(ms) ((ms) * (1000))
