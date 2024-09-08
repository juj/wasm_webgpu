// flags: -sJSPI=1

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();

  WGpuDeviceDescriptor desc = {};
  desc.requiredLimits.maxBufferSize = 512*1024*1024; // Request elevated limit. This test assumes that browser can handle this.
  WGpuDevice device = wgpu_adapter_request_device_sync(adapter, &desc);

  assert(wgpu_is_device(device));

  WGpuSupportedLimits limits;
  wgpu_device_get_limits(device, &limits);
  printf("maxBufferSize limit: %llu\n", limits.maxBufferSize);
  assert(limits.maxBufferSize == 512*1024*1024);

  EM_ASM(window.close());
}
