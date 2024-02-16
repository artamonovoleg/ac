#include "ac_private.h"

#if (AC_INCLUDE_RENDERER) && (AC_PLATFORM_WINDOWS)

#include "renderer_d3d12.h"
#include "renderer_d3d12_helpers.h"

#include <ShlObj.h>

extern "C"
{
static inline ac_result
ac_get_latest_win_pix_gpu_capturer_path(wchar_t* out)
{
  LPWSTR program_files_path = NULL;
  SHGetKnownFolderPath(
    FOLDERID_ProgramFiles,
    KF_FLAG_DEFAULT,
    NULL,
    &program_files_path);

  wchar_t pix_search_path[1024] = {0};
  wcscat_s(pix_search_path, 1024, program_files_path);
  wcscat_s(pix_search_path, 1024, L"\\Microsoft PIX\\");
  wchar_t pix_search_path_exp[1024] = {0};
  wcscat_s(pix_search_path_exp, pix_search_path);
  wcscat_s(pix_search_path_exp, L"\\*");

  WIN32_FIND_DATA find_data;
  bool            found_pix_installation = false;
  wchar_t         newest_version_found[1024] = {0};

  HANDLE hfind = FindFirstFileW(pix_search_path_exp, &find_data);
  if (hfind == INVALID_HANDLE_VALUE)
  {
    return ac_result_unknown_error;
  }

  do
  {
    if (
      ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ==
       FILE_ATTRIBUTE_DIRECTORY) &&
      (find_data.cFileName[0] != '.'))
    {
      if (
        !found_pix_installation ||
        wcscmp(newest_version_found, find_data.cFileName) <= 0)
      {
        found_pix_installation = true;
        wcscat_s(newest_version_found, 1024, find_data.cFileName);
      }
    }
  }
  while (FindNextFileW(hfind, &find_data) != 0);

  FindClose(hfind);

  if (!found_pix_installation)
  {
    return ac_result_unknown_error;
  }

  wcscat_s(out, 1024, pix_search_path);
  wcscat_s(out, 1024, newest_version_found);
  wcscat_s(out, 1024, L"\\WinPixGpuCapturer.dll");

  return ac_result_success;
}

static void
ac_d3d12_windows_debug_message_callback(
  D3D12_MESSAGE_CATEGORY category,
  D3D12_MESSAGE_SEVERITY severity,
  D3D12_MESSAGE_ID       id,
  LPCSTR                 description,
  void*                  context)
{
  AC_UNUSED(category);
  AC_UNUSED(id);
  AC_UNUSED(context);

  switch (severity)
  {
  case D3D12_MESSAGE_SEVERITY_MESSAGE:
    AC_INFO("[ renderer ] [ d3d12 ] %s", description);
    break;
  case D3D12_MESSAGE_SEVERITY_INFO:
    AC_DEBUG("[ renderer ] [ d3d12 ] %s", description);
    break;
  case D3D12_MESSAGE_SEVERITY_WARNING:
    AC_WARN("[ renderer ] [ d3d12 ] %s", description);
    break;
  case D3D12_MESSAGE_SEVERITY_ERROR:
  case D3D12_MESSAGE_SEVERITY_CORRUPTION:
    AC_ERROR("[ renderer ] [ d3d12 ] %s", description);
    break;
  default:
    break;
  }
}

static inline int
ac_d3d12_windows_compute_intersection_area(
  int ax1,
  int ay1,
  int ax2,
  int ay2,
  int bx1,
  int by1,
  int bx2,
  int by2)
{
  return AC_MAX(0, AC_MIN(ax2, bx2) - AC_MAX(ax1, bx1)) *
         AC_MAX(0, AC_MIN(ay2, by2) - AC_MAX(ay1, by1));
}

static void
ac_d3d12_windows_destroy_device(ac_device device_handle)
{
  AC_FROM_HANDLE(device, ac_d3d12_device);

  for (uint32_t i = 0; i < device->common.queue_count; ++i)
  {
    if (!device->common.queues[i])
    {
      continue;
    }

    AC_FROM_HANDLE2(queue, device->common.queues[i], ac_d3d12_queue);

    ac_d3d12_destroy_fence2(&queue->fence);

    AC_D3D12_SAFE_RELEASE(queue->queue);
    ac_free(queue);
  }

  ac_d3d12_destroy_cpu_heap(&device->rtv_heap);
  ac_d3d12_destroy_cpu_heap(&device->dsv_heap);

  AC_D3D12_SAFE_RELEASE(device->allocator);

  if (device->info_queue)
  {
    ID3D12InfoQueue1* info_queue1 = NULL;
    if (SUCCEEDED(
          device->info_queue->QueryInterface(AC_IID_PPV_ARGS(&info_queue1))))
    {
      info_queue1->UnregisterMessageCallback(device->callback_cookie);
      info_queue1->Release();
    }
    device->info_queue->Release();
  }

  AC_D3D12_SAFE_RELEASE(device->device);
  AC_D3D12_SAFE_RELEASE(device->debug);
}

static ac_result
ac_d3d12_windows_queue_present(
  ac_queue                     queue_handle,
  const ac_queue_present_info* info)
{
  AC_UNUSED(queue_handle);
  AC_FROM_HANDLE2(swapchain, info->swapchain, ac_d3d12_swapchain);

  UINT interval = 1;
  UINT flags = 0;

  if (!swapchain->common.vsync)
  {
    interval = 0;
    flags = DXGI_PRESENT_ALLOW_TEARING;
  }

  AC_D3D12_RIF(swapchain->swapchain->Present(interval, flags));

  swapchain->common.image_index =
    (swapchain->common.image_index + 1) % swapchain->common.image_count;

  return ac_result_success;
}

static inline ac_result
ac_d3d12_windows_get_output_desc(
  HWND               hwnd,
  IDXGIAdapter1*     dxgi_adapter,
  DXGI_OUTPUT_DESC1* out)
{
  RECT rc;
  if (!GetClientRect(hwnd, &rc))
  {
    return ac_result_unknown_error;
  }

  UINT         i = 0;
  IDXGIOutput* current_output;
  IDXGIOutput* best_output = NULL;
  float        best_intersect_area = -1;

  while (dxgi_adapter->EnumOutputs(i, &current_output) != DXGI_ERROR_NOT_FOUND)
  {
    int ax1 = rc.left;
    int ay1 = rc.top;
    int ax2 = rc.right;
    int ay2 = rc.bottom;

    DXGI_OUTPUT_DESC desc;
    AC_D3D12_RIF(current_output->GetDesc(&desc));
    RECT r = desc.DesktopCoordinates;
    int  bx1 = r.left;
    int  by1 = r.top;
    int  bx2 = r.right;
    int  by2 = r.bottom;

    float intersect_area =
      static_cast<float>(ac_d3d12_windows_compute_intersection_area(
        ax1,
        ay1,
        ax2,
        ay2,
        bx1,
        by1,
        bx2,
        by2));

    if (intersect_area > best_intersect_area)
    {
      best_output = current_output;
      best_intersect_area = intersect_area;
    }

    i++;
  }

  if (!best_output)
  {
    return ac_result_unknown_error;
  }

  IDXGIOutput6* output = NULL;
  AC_D3D12_RIF(best_output->QueryInterface(AC_IID_PPV_ARGS(&output)));
  best_output->Release();

  if (FAILED(output->GetDesc1(out)))
  {
    AC_D3D12_SAFE_RELEASE(output);
    return ac_result_unknown_error;
  }

  output->Release();

  return ac_result_success;
}

static ac_result
ac_d3d12_windows_create_swapchain(
  ac_device                device_handle,
  const ac_swapchain_info* info,
  ac_swapchain*            swapchain_handle)
{
  AC_FROM_HANDLE(device, ac_d3d12_device);
  AC_FROM_HANDLE2(queue, info->queue, ac_d3d12_queue);

  AC_INIT_INTERNAL(swapchain, ac_d3d12_swapchain);

  ac_result rslt = ac_result_unknown_error;

  HWND                  hwnd = reinterpret_cast<HWND>(info->wsi->native_window);
  BOOL                  allow_tearing = FALSE;
  IDXGIFactory6*        factory = NULL;
  IDXGIAdapter1*        dxgi_adapter = NULL;
  DXGI_SWAP_CHAIN_DESC1 swap_desc = {};
  ac_format             format = ac_format_r8g8b8a8_srgb;

  if (!hwnd)
  {
    goto end;
  }

  AC_D3D12_EIF(
    device->create_factory(device->factory_flags, AC_IID_PPV_ARGS(&factory)));

  (void)(factory->CheckFeatureSupport(
    DXGI_FEATURE_PRESENT_ALLOW_TEARING,
    &allow_tearing,
    sizeof(allow_tearing)));

  AC_D3D12_EIF(factory->EnumAdapterByLuid(
    device->adapter_luid,
    IID_PPV_ARGS(&dxgi_adapter)));

  DXGI_OUTPUT_DESC1 output_desc;
  if (
    ac_d3d12_windows_get_output_desc(hwnd, dxgi_adapter, &output_desc) !=
    ac_result_success)
  {
    goto end;
  }

  swapchain->common.color_space = ac_color_space_srgb;

  if (
    (info->bits & ac_swapchain_wants_hdr_bit) &&
    (output_desc.ColorSpace == DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020))
  {
    format = ac_format_r10g10b10a2_unorm;
  }
  else if (info->bits & ac_swapchain_wants_unorm_format_bit)
  {
    format = ac_format_r8g8b8a8_unorm;
  }

  swap_desc.BufferCount = info->min_image_count;
  swap_desc.Width = info->width;
  swap_desc.Height = info->height;
  swap_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
  swap_desc.SampleDesc.Count = 1;
  swap_desc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
  swap_desc.Scaling = DXGI_SCALING_STRETCH;
  swap_desc.Format = ac_format_to_d3d12(ac_format_to_unorm(format));

  if (!info->vsync && allow_tearing)
  {
    swap_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING;
  }
  else
  {
    swapchain->common.vsync = true;
  }

  AC_D3D12_EIF(factory->CreateSwapChainForHwnd(
    queue->queue,
    hwnd,
    &swap_desc,
    nullptr,
    nullptr,
    &swapchain->swapchain));

  AC_D3D12_EIF(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

  if (format == ac_format_r10g10b10a2_unorm)
  {
    IDXGISwapChain3* s = NULL;
    do
    {
      if (FAILED(swapchain->swapchain->QueryInterface(AC_IID_PPV_ARGS(&s))))
      {
        break;
      }

      UINT color_space_support = 0;
      if (FAILED(s->CheckColorSpaceSupport(
            DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020,
            &color_space_support)))
      {
        break;
      }

      if (!((color_space_support &
             DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT) ==
            DXGI_SWAP_CHAIN_COLOR_SPACE_SUPPORT_FLAG_PRESENT))
      {
        break;
      }

      if (FAILED(s->SetColorSpace1(DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020)))
      {
        break;
      }

      swapchain->common.color_space = ac_color_space_hdr;
    }
    while (false);

    AC_D3D12_SAFE_RELEASE(s);
  }

  swapchain->common.images =
    static_cast<ac_image*>(ac_calloc(info->min_image_count * sizeof(ac_image)));

  swapchain->common.image_count = info->min_image_count;

  for (uint32_t i = 0; i < info->min_image_count; ++i)
  {
    ac_image* image_handle = &swapchain->common.images[i];
    AC_INIT_INTERNAL(image, ac_d3d12_image);

    image->common.width = swap_desc.Width;
    image->common.height = swap_desc.Height;
    image->common.format = format;
    image->common.levels = 1;
    image->common.layers = 1;
    image->common.samples = 1;
    image->common.usage =
      ac_image_usage_transfer_dst_bit | ac_image_usage_attachment_bit;
    image->common.type = ac_image_type_2d;

    AC_D3D12_EIF(
      swapchain->swapchain->GetBuffer(i, AC_IID_PPV_ARGS(&image->resource)));

    image->handle = ac_d3d12_cpu_heap_alloc_handle(&device->rtv_heap);

    D3D12_RENDER_TARGET_VIEW_DESC view_desc = {};
    view_desc.Format = ac_format_to_d3d12(format);
    view_desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;

    device->device->CreateRenderTargetView(
      image->resource,
      &view_desc,
      ac_d3d12_get_cpu_handle(
        device->rtv_heap.heap,
        device->rtv_descriptor_size,
        image->handle));
  }

  swapchain->common.vsync = info->vsync;

  rslt = ac_result_success;

end:
  AC_D3D12_SAFE_RELEASE(factory);
  AC_D3D12_SAFE_RELEASE(dxgi_adapter);
  return rslt;
}

static void
ac_d3d12_windows_destroy_swapchain(
  ac_device    device_handle,
  ac_swapchain swapchain_handle)
{
  AC_FROM_HANDLE(device, ac_d3d12_device);
  AC_FROM_HANDLE(swapchain, ac_d3d12_swapchain);

  for (uint32_t i = 0; i < swapchain->common.image_count; ++i)
  {
    if (!swapchain->common.images[i])
    {
      continue;
    }

    AC_FROM_HANDLE2(image, swapchain->common.images[i], ac_d3d12_image);
    ac_d3d12_cpu_heap_free_handle(&device->rtv_heap, image->handle);

    AC_D3D12_SAFE_RELEASE(image->resource);
  }

  AC_D3D12_SAFE_RELEASE(swapchain->swapchain);
}

static ac_result
ac_d3d12_windows_acquire_next_image(
  ac_device    device_handle,
  ac_swapchain swapchain_handle,
  ac_fence     fence_handle)
{
  AC_UNUSED(device_handle);
  AC_UNUSED(fence_handle);
  AC_UNUSED(swapchain_handle);

  return ac_result_success;
}

ac_result
ac_d3d12_create_device(const ac_device_info* info, ac_device* device_handle)
{
  AC_UNUSED(info);
  AC_INIT_INTERNAL(device, ac_d3d12_device);

  ac_d3d12_get_device_common_functions(device);
  device->common.destroy_device = ac_d3d12_windows_destroy_device;
  device->common.queue_present = ac_d3d12_windows_queue_present;
  device->common.create_swapchain = ac_d3d12_windows_create_swapchain;
  device->common.destroy_swapchain = ac_d3d12_windows_destroy_swapchain;
  device->common.acquire_next_image = ac_d3d12_windows_acquire_next_image;

  HMODULE handle = NULL;
  handle = GetModuleHandleW(L"dxgi.dll");

  if (!handle)
  {
    handle = LoadLibraryW(L"dxgi.dll");
  }

  if (!handle)
  {
    AC_ERROR("[ renderer ] [ d3d12 ] failed to load dxgi.dll");
    return ac_result_unknown_error;
  }

  device->create_factory = reinterpret_cast<ac_pfn_create_dxgi_factory2>(
    reinterpret_cast<void*>(GetProcAddress(handle, "CreateDXGIFactory2")));

  handle = GetModuleHandleW(L"D3D12.dll");

  if (!handle)
  {
    handle = LoadLibraryW(L"D3D12.dll");
  }

  if (!handle)
  {
    AC_ERROR("[ renderer ] [ d3d12 ] failed to load D3D12Core.dll");
    return ac_result_unknown_error;
  }

  PFN_D3D12_GET_DEBUG_INTERFACE d3d12_get_debug_interface =
    reinterpret_cast<PFN_D3D12_GET_DEBUG_INTERFACE>(reinterpret_cast<void*>(
      GetProcAddress(handle, "D3D12GetDebugInterface")));
  PFN_D3D12_CREATE_DEVICE d3d12_create_device =
    reinterpret_cast<PFN_D3D12_CREATE_DEVICE>(
      reinterpret_cast<void*>(GetProcAddress(handle, "D3D12CreateDevice")));

  device->factory_flags = 0;

  if (AC_INCLUDE_DEBUG)
  {
    if (info->debug_bits & ac_device_debug_validation_bit)
    {
      if (SUCCEEDED(d3d12_get_debug_interface(AC_IID_PPV_ARGS(&device->debug))))
      {
        device->factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
        device->debug->EnableDebugLayer();
        if (info->debug_bits & ac_device_debug_validation_gpu_based_bit)
        {
          device->debug->SetEnableGPUBasedValidation(true);
        }
        if (info->debug_bits & ac_device_debug_validation_synchronization_bit)
        {
          device->debug->SetEnableSynchronizedCommandQueueValidation(true);
        }
      }
    }

    handle = GetModuleHandleW(L"renderdoc.dll");
    pRENDERDOC_GetAPI renderdoc_get_api = NULL;

    if (handle)
    {
      renderdoc_get_api = reinterpret_cast<pRENDERDOC_GetAPI>(
        reinterpret_cast<void*>((GetProcAddress(handle, "RENDERDOC_GetAPI"))));
    }

    if (renderdoc_get_api)
    {
      if (!renderdoc_get_api(
            eRENDERDOC_API_Version_1_6_0,
            reinterpret_cast<void**>(&device->rdoc)))
      {
        device->rdoc = NULL;
      }
    }
  }

  if (
    AC_INCLUDE_DEBUG &&
    (info->debug_bits & ac_device_debug_enable_debugger_attaching_bit))
  {
    wchar_t   gpu_capturer[1024] = {0};
    ac_result res = ac_get_latest_win_pix_gpu_capturer_path(gpu_capturer);

    if (res == ac_result_success)
    {
      handle = GetModuleHandleW(gpu_capturer);

      if (!handle)
      {
        handle = LoadLibraryW(gpu_capturer);
      }

      if (handle)
      {
        AC_INFO("[ renderer ] [ d3d12 ] : WinPixGpuCapturer.dll loaded");
      }
      else
      {
        AC_WARN("[ renderer ] [ d3d12 ] : WinPixGpuCapturer.dll wasn't loaded "
                "attaching not supported");
      }
    }
    else
    {
      AC_WARN("[ renderer ] [ d3d12 ] : WinPixGpuCapturer.dll wasn't found "
              "attaching not supported");
    }
  }

  IDXGIFactory6* factory;
  AC_D3D12_RIF(
    device->create_factory(device->factory_flags, AC_IID_PPV_ARGS(&factory)));

  D3D_FEATURE_LEVEL feature_levels[] = {
    D3D_FEATURE_LEVEL_12_2,
    D3D_FEATURE_LEVEL_12_1,
    D3D_FEATURE_LEVEL_12_0,
  };

  IDXGIAdapter1* adapter = NULL;

  for (uint32_t i = 0; i < AC_COUNTOF(feature_levels); ++i)
  {
    ID3D12Device5* d = NULL;
    bool           rt = false;
    bool           mesh_shaders = false;
    bool           enhanced_barriers = false;

    D3D_FEATURE_LEVEL feature_level = feature_levels[i];

    for (uint32_t adapter_index = 0;
         SUCCEEDED(factory->EnumAdapterByGpuPreference(
           adapter_index,
           DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
           AC_IID_PPV_ARGS(&adapter)));
         ++adapter_index)
    {
      DXGI_ADAPTER_DESC1 desc;
      adapter->GetDesc1(&desc);

      if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
      {
        continue;
      }

      if (SUCCEEDED(d3d12_create_device(
            adapter,
            feature_level,
            _uuidof(ID3D12Device),
            nullptr)))
      {
        break;
      }
    }

    if (!adapter)
    {
      for (uint32_t adapter_index = 0;
           SUCCEEDED(factory->EnumAdapters1(adapter_index, &adapter));
           ++adapter_index)
      {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
          continue;
        }

        if (SUCCEEDED(d3d12_create_device(
              adapter,
              feature_level,
              _uuidof(ID3D12Device),
              nullptr)))
        {
          break;
        }
      }
    }

    if (!adapter)
    {
      continue;
    }

    HRESULT res =
      d3d12_create_device(adapter, feature_level, AC_IID_PPV_ARGS(&d));

    if (FAILED(res))
    {
      continue;
    }

    D3D12_FEATURE_DATA_ROOT_SIGNATURE feature_data = {};
    feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;

    res = d->CheckFeatureSupport(
      D3D12_FEATURE_ROOT_SIGNATURE,
      &feature_data,
      sizeof(feature_data));

    if (
      FAILED(res) ||
      feature_data.HighestVersion < D3D_ROOT_SIGNATURE_VERSION_1_1)
    {
      AC_DEBUG(
        "[ renderer ] [ d3d12 ] root signature versions other than 1.1 isn't "
        "supported now");
      continue;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS5 options5 = {};

    res = d->CheckFeatureSupport(
      D3D12_FEATURE_D3D12_OPTIONS5,
      &options5,
      sizeof(options5));

    if (FAILED(res))
    {
      rt = false;
    }
    else
    {
      // TODO: inline raytracing needs tier 1 1, maybe later it will be optional
      // in ac, for now we want all set of features
      rt = options5.RaytracingTier >= D3D12_RAYTRACING_TIER_1_1;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS7 options7 = {};

    res = d->CheckFeatureSupport(
      D3D12_FEATURE_D3D12_OPTIONS7,
      &options7,
      sizeof(options7));

    if (FAILED(res))
    {
      mesh_shaders = false;
    }
    else
    {
      mesh_shaders = options7.MeshShaderTier >= D3D12_MESH_SHADER_TIER_1;
    }

    D3D12_FEATURE_DATA_D3D12_OPTIONS12 options12 = {};

    res = d->CheckFeatureSupport(
      D3D12_FEATURE_D3D12_OPTIONS12,
      &options12,
      sizeof(options12));

    if (FAILED(res))
    {
      enhanced_barriers = false;
    }
    else
    {
      enhanced_barriers = options12.EnhancedBarriersSupported;
    }

    device->common.support_raytracing = rt;
    device->common.support_mesh_shaders = mesh_shaders;
    device->enhanced_barriers = enhanced_barriers;
    device->device = d;

    if (enhanced_barriers)
    {
      break;
    }
  }

  AC_D3D12_SAFE_RELEASE(factory);

  if (!device->device || !adapter)
  {
    return ac_result_unknown_error;
  }

  if (info->debug_bits)
  {
    HRESULT res =
      device->device->QueryInterface(AC_IID_PPV_ARGS(&device->info_queue));

    if (SUCCEEDED(res))
    {
      D3D12_MESSAGE_ID ids[] = {
        D3D12_MESSAGE_ID_CREATERESOURCE_STATE_IGNORED,
        D3D12_MESSAGE_ID_RESOURCE_BARRIER_BEFORE_AFTER_MISMATCH,
      };

      D3D12_INFO_QUEUE_FILTER filter = {};
      filter.DenyList.NumIDs = AC_COUNTOF(ids);
      filter.DenyList.pIDList = ids;
      device->info_queue->AddStorageFilterEntries(&filter);

      ID3D12InfoQueue1* info_queue1 = NULL;

      if (SUCCEEDED(
            device->info_queue->QueryInterface(AC_IID_PPV_ARGS(&info_queue1))))
      {
        info_queue1->RegisterMessageCallback(
          ac_d3d12_windows_debug_message_callback,
          D3D12_MESSAGE_CALLBACK_FLAG_NONE,
          NULL,
          &device->callback_cookie);

        info_queue1->Release();
      }
      else
      {
        AC_DEBUG("[ renderer ] [ d3d12 ] : failed to query ID3D12InfoQueue1 "
                 "interface, "
                 "output of d3d12 won't be redirected");
      }
    }
  }

  D3D12MA::ALLOCATION_CALLBACKS allocation_callbacks = {};
  allocation_callbacks.pAllocate = [](size_t size, size_t alignment, void*)
  {
    return ac_aligned_alloc(size, alignment);
  };
  allocation_callbacks.pFree = [](void* ptr, void*)
  {
    ac_free(ptr);
  };

  D3D12MA::ALLOCATOR_DESC allocator_desc = {};
  allocator_desc.Flags = D3D12MA::ALLOCATOR_FLAG_NONE;
  allocator_desc.pAdapter = adapter;
  allocator_desc.pDevice = device->device;
  allocator_desc.pAllocationCallbacks = &allocation_callbacks;
  if (FAILED(D3D12MA::CreateAllocator(&allocator_desc, &device->allocator)))
  {
    AC_D3D12_SAFE_RELEASE(adapter);
    return ac_result_unknown_error;
  }

  AC_D3D12_SAFE_RELEASE(adapter);

  AC_D3D12_RIF(ac_d3d12_create_cpu_heap(
    device,
    D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
    &device->rtv_heap));
  AC_D3D12_RIF(ac_d3d12_create_cpu_heap(
    device,
    D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
    &device->dsv_heap));

  device->rtv_descriptor_size =
    device->device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
  device->dsv_descriptor_size =
    device->device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
  device->resource_descriptor_size =
    device->device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
  device->sampler_descriptor_size =
    device->device->GetDescriptorHandleIncrementSize(
      D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

  device->common.props.api = "DirectX12";
  device->common.props.cbv_buffer_alignment = 256;
  device->common.props.image_row_alignment = D3D12_TEXTURE_DATA_PITCH_ALIGNMENT;
  device->common.props.image_alignment = D3D12_TEXTURE_DATA_PLACEMENT_ALIGNMENT;
  // FIXME: this is guaranteed value for all rt formats but not possible highest
  device->common.props.max_sample_count = 4;
  device->common.props.as_instance_size =
    sizeof(D3D12_RAYTRACING_INSTANCE_DESC);

  device->common.queue_count = 0;

  AC_ZERO(device->common.queue_map);

  for (int32_t i = 0; i < ac_queue_type_count; ++i)
  {
    for (int32_t j = i; j >= 0; --j)
    {
      D3D12_COMMAND_QUEUE_DESC queue_desc = {};
      queue_desc.Type = ac_queue_type_to_d3d12(static_cast<ac_queue_type>(j));
      queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

      ID3D12CommandQueue* cmd_queue = NULL;

      HRESULT res = device->device->CreateCommandQueue(
        &queue_desc,
        AC_IID_PPV_ARGS(&cmd_queue));

      if (FAILED(res))
      {
        continue;
      }

      ac_queue* queue_handle =
        &device->common.queues[device->common.queue_count];

      AC_INIT_INTERNAL(queue, ac_d3d12_queue);

      device->common.queue_map[i] = device->common.queue_count;

      ac_fence_info fence_info = {};
      fence_info.bits = ac_fence_present_bit;
      ac_d3d12_create_fence2(device, &fence_info, &queue->fence);

      queue->queue = cmd_queue;
      queue->common.type = static_cast<ac_queue_type>(j);

      ++device->common.queue_count;
      break;
    }
  }

  device->adapter_luid = device->device->GetAdapterLuid();

  return ac_result_success;
}
}

#endif
