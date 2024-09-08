// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuSupportedLimits limits;
  wgpu_device_get_limits(device, &limits);

  printf("maxTextureDimension2D: %u\n", limits.maxTextureDimension2D);
  assert(limits.maxTextureDimension2D == 16384);

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGpuDeviceDescriptor desc = {};
  desc.requiredLimits.maxTextureDimension2D = 16384; // Request 2x elevated limit. This test assumes that the test device can handle this.
  wgpu_adapter_request_device_async(adapter, &desc, ObtainedWebGpuDevice, 0);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
