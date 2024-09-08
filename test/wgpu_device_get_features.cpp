// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_FEATURES_BITFIELD features = wgpu_device_get_features(device);

  assert(features == (WGPU_FEATURE_SHADER_F16 | WGPU_FEATURE_DEPTH_CLIP_CONTROL)); // Should have exactly only the requested features.

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGpuDeviceDescriptor desc = {};
  desc.requiredFeatures = WGPU_FEATURE_SHADER_F16 | WGPU_FEATURE_DEPTH_CLIP_CONTROL; // Ask for two features. This test assumes that the test device can handle this.
  wgpu_adapter_request_device_async(adapter, &desc, ObtainedWebGpuDevice, 0);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
