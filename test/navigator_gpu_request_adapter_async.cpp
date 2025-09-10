// flags: -sEXIT_RUNTIME=0
// expected-output: navigator.gpu.requestAdapter(options={"forceFallbackAdapter":false,"powerPreference":"low-power"})

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  assert(wgpu_is_adapter(adapter));
  assert(userData == (void*)42);

  EM_ASM(window.close());
}

int main()
{
  WGpuRequestAdapterOptions options = {
    .powerPreference = WGPU_POWER_PREFERENCE_LOW_POWER,
    .forceFallbackAdapter = WGPU_FALSE
  };
  navigator_gpu_request_adapter_async(&options, ObtainedWebGpuAdapter, (void*)42);
}
