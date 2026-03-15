// Tests navigator_gpu_request_adapter_async() with HIGH_PERFORMANCE preference,
// exercising the [,'low-power','high-performance'][HEAPU32[options+1]] == 'high-performance'
// code path (index 2) in navigator_gpu_request_adapter_async.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  // adapter may be null if no high-performance GPU is available; only assert if returned
  if (adapter)
    assert(wgpu_is_adapter(adapter));
  EM_ASM(window.close());
}

int main()
{
  WGpuRequestAdapterOptions opts = WGPU_REQUEST_ADAPTER_OPTIONS_DEFAULT_INITIALIZER;
  opts.powerPreference = WGPU_POWER_PREFERENCE_HIGH_PERFORMANCE;
  navigator_gpu_request_adapter_async(&opts, ObtainedWebGpuAdapter, 0);
}
