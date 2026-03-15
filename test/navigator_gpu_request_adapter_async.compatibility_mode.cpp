// Tests navigator_gpu_request_adapter_async() with featureLevel=COMPATIBILITY,
// exercising the 'compatibility' feature level code path in adapter options.
// The adapter may be null if compatibility mode is not supported on the platform;
// the test accepts either outcome gracefully.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  if (adapter)
  {
    assert(wgpu_is_adapter(adapter));
    printf("Got compatibility-mode adapter.\n");
  }
  else
  {
    printf("No compatibility-mode adapter available on this platform, skipping.\n");
  }

  EM_ASM(window.close());
}

int main()
{
  WGpuRequestAdapterOptions opts = WGPU_REQUEST_ADAPTER_OPTIONS_DEFAULT_INITIALIZER;
  opts.featureLevel = WGPU_FEATURE_LEVEL_COMPATIBILITY; // exercises the 'compatibility' feature level

  navigator_gpu_request_adapter_async(&opts, ObtainedWebGpuAdapter, 0);
}
