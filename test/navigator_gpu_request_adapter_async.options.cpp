// Verifies that navigator_gpu_request_adapter_async() correctly accepts a WGpuRequestAdapterOptions struct (power preference, forceFallbackAdapter) and delivers a valid adapter to the callback.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  assert(userData == (void*)42);
  // adapter may be 0 if the requested configuration is not supported;
  // only assert success if an adapter was returned.
  if (adapter)
  {
    assert(wgpu_is_adapter(adapter));
    // Verify adapter info is accessible
    WGpuAdapterInfo info;
    wgpu_adapter_get_info(adapter, &info);
    // vendor/architecture may be empty strings but must be null-terminated
    (void)info.vendor[0];
  }

  EM_ASM(window.close());
}

int main()
{
  // Request an adapter with explicit power preference options
  WGpuRequestAdapterOptions opts = WGPU_REQUEST_ADAPTER_OPTIONS_DEFAULT_INITIALIZER;
  opts.powerPreference = WGPU_POWER_PREFERENCE_HIGH_PERFORMANCE;
  opts.forceFallbackAdapter = WGPU_FALSE;

  navigator_gpu_request_adapter_async(&opts, ObtainedWebGpuAdapter, (void*)42);
}
