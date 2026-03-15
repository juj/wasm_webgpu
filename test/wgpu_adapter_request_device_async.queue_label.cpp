// Tests wgpu_adapter_request_device_async() with a non-null defaultQueue.label,
// exercising the $wgpuReadQueueDescriptor code path where HEAPU32[heap32Idx]
// is nonzero: returns { 'label': utf8(...) } instead of void 0.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  assert(wgpu_is_device(device));
  printf("Device obtained with named queue label: OK\n");
  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  // Set a non-null queue label to exercise the $wgpuReadQueueDescriptor
  // truthy-label branch: HEAPU32[heap32Idx] != 0 → { 'label': 'my-queue' }
  WGpuDeviceDescriptor desc = {};
  desc.defaultQueue.label = "my-queue";
  wgpu_adapter_request_device_async(adapter, &desc, ObtainedWebGpuDevice, 0);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
