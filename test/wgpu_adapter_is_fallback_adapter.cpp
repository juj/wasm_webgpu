// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  assert(wgpu_adapter_is_fallback_adapter(adapter));

  EM_ASM(window.close());
}

int main()
{
  WGpuRequestAdapterOptions options = {
    .powerPreference = WGPU_POWER_PREFERENCE_LOW_POWER,
    .forceFallbackAdapter = WGPU_TRUE
  };
  navigator_gpu_request_adapter_async(&options, ObtainedWebGpuAdapter, 0);
}
