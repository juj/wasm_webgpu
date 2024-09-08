// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  // Get the bitfield of all supported features
  WGPU_FEATURES_BITFIELD features = wgpu_adapter_get_features(adapter);

  // And then for each individual feature, test that adapter supports it.
  for(int i = 1; i < WGPU_FEATURE_FIRST_UNUSED_BIT; i <<= 1)
    if ((features & i) != 0)
      assert(wgpu_adapter_supports_feature(adapter, i));

  // Value 0 does not correspond to any feature so should never return true.
  assert(!wgpu_adapter_supports_feature(adapter, 0));

  // Test a statically written out feature cap. (this assumes the adapter does support this feature)
  assert(wgpu_adapter_supports_feature(adapter, WGPU_FEATURE_SHADER_F16));

  EM_ASM(window.close());
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
