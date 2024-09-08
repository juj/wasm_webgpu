// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  assert(wgpu_device_supports_feature(device, WGPU_FEATURE_SHADER_F16));
  assert(wgpu_device_supports_feature(device, WGPU_FEATURE_DEPTH_CLIP_CONTROL));
  assert(!wgpu_device_supports_feature(device, WGPU_FEATURE_DEPTH32FLOAT_STENCIL8)); // We did not ask for this feature.

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
