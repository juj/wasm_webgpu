// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  assert(userData == (void*)42);
  assert(wgpu_is_device(device));

  WGpuSupportedLimits limits;
  wgpu_device_get_limits(device, &limits);
  printf("maxStorageBufferBindingSize limit: %llu\n", limits.maxStorageBufferBindingSize);
  assert(limits.maxStorageBufferBindingSize == 256*1024*1024);

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData)
{
  WGpuDeviceDescriptor desc = {};
  desc.requiredLimits.maxStorageBufferBindingSize = 256*1024*1024; // Request elevated limit. This test assumes that browser can handle this.
  wgpu_adapter_request_device_async(result, &desc, ObtainedWebGpuDevice, (void*)42);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
