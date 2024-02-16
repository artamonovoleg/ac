#include "ac_private.h"

#if (AC_PLATFORM_WINDOWS)

#if defined(_MSC_VER)
#pragma warning(push, 0)
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#endif

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pramga GCC diagnostic ignored "-Wall"
#pragma GCC diagnostic ignored "-Wextra"
#endif

#include <dxgi1_6.h>
#include <d3d12/d3d12.h>
#define D3D12MA_D3D12_HEADERS_ALREADY_INCLUDED

#define D3D12MA_ASSERT AC_ASSERT
#define D3D12MA_DEBUG_LOG AC_ERROR

#include "d3d12_mem_alloc_impl.hpp"

#endif
