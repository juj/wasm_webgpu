// Tests navigator_gpu_request_adapter_async() with forceFallbackAdapter=true,
// exercising that code path in the JS adapter request options.
// The adapter may be null if the platform does not support fallback adapters;
// the test accepts either outcome gracefully.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  // adapter may be 0 if the platform does not support a software fallback adapter
  if (adapter)
  {
    assert(wgpu_is_adapter(adapter));
    WGpuAdapterInfo info = {};
    wgpu_adapter_get_info(adapter, &info);
    // A fallback adapter should be reported as fallback
    printf("isFallbackAdapter = %d\n", info.isFallbackAdapter);
  }
  else
  {
    printf("No fallback adapter available on this platform, skipping.\n");
  }

  EM_ASM(window.close());
}

int main()
{
  WGpuRequestAdapterOptions opts = WGPU_REQUEST_ADAPTER_OPTIONS_DEFAULT_INITIALIZER;
  opts.forceFallbackAdapter = WGPU_TRUE; // exercises the 'forceFallbackAdapter' field

  navigator_gpu_request_adapter_async(&opts, ObtainedWebGpuAdapter, 0);
}
