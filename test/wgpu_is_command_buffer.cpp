// Verifies that wgpu_is_command_buffer() returns true for a finished WGpuCommandBuffer and false for a WGpuDevice.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  WGpuCommandBuffer cmdBuf = wgpu_command_encoder_finish(encoder);
  assert(cmdBuf);
  assert(wgpu_is_command_buffer(cmdBuf));
  assert(!wgpu_is_command_buffer(device)); // a device is not a command buffer

  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), cmdBuf);

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
