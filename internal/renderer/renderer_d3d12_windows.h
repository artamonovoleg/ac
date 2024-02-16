#pragma once

#include "ac_private.h"

#if (AC_INCLUDE_RENDERER) && (AC_PLATFORM_WINDOWS)

#include <dxgi1_6.h>
#include <d3d12/d3d12.h>
#if (AC_INCLUDE_DEBUG)
#define USE_PIX
#endif
#include <pix/pix3.h>

#define AC_IID_PPV_ARGS IID_PPV_ARGS

#define AC_D3D12_USE_ENHANCED_BARRIERS 1
#define AC_D3D12_USE_RAYTRACING 1
#define AC_D3D12_USE_MESH_SHADERS 1

#endif
