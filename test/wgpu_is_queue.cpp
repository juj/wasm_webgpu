// Verifies that wgpu_is_queue() returns true for a WGpuQueue obtained from the device and false for a WGpuDevice.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuQueue queue = wgpu_device_get_queue(device);
  assert(queue);
  assert(wgpu_is_queue(queue));
  assert(!wgpu_is_queue(device)); // a device is not a queue

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
