// Verifies that wgpu_is_command_encoder() returns true for a WGpuCommandEncoder and false for a WGpuDevice.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  assert(encoder);
  assert(wgpu_is_command_encoder(encoder));
  assert(!wgpu_is_command_encoder(device)); // a device is not a command encoder

  wgpu_object_destroy(encoder);

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
