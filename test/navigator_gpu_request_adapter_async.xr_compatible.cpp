// Tests navigator_gpu_request_adapter_async() with xrCompatible=true,
// exercising the xrCompatible field in the adapter options struct.
// The adapter may be null if XR is not supported on the platform;
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
    printf("Got XR-compatible adapter.\n");
  }
  else
  {
    printf("No XR-compatible adapter available on this platform, skipping.\n");
  }

  EM_ASM(window.close());
}

int main()
{
  WGpuRequestAdapterOptions opts = WGPU_REQUEST_ADAPTER_OPTIONS_DEFAULT_INITIALIZER;
  opts.xrCompatible = WGPU_TRUE; // exercises the 'xrCompatible' field

  navigator_gpu_request_adapter_async(&opts, ObtainedWebGpuAdapter, 0);
}
