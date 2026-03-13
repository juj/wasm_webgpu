// Verifies that wgpu_queue_submit_multiple_and_destroy() correctly submits an array of two command buffers in a single call without crashing.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuQueue queue = wgpu_device_get_queue(device);

  // Create two command buffers
  WGpuCommandEncoder enc1 = wgpu_device_create_command_encoder(device, 0);
  WGpuCommandBuffer cmd1 = wgpu_command_encoder_finish(enc1);
  assert(cmd1);

  WGpuCommandEncoder enc2 = wgpu_device_create_command_encoder(device, 0);
  WGpuCommandBuffer cmd2 = wgpu_command_encoder_finish(enc2);
  assert(cmd2);

  // Submit both at once and destroy them
  WGpuCommandBuffer cmds[] = { cmd1, cmd2 };
  wgpu_queue_submit_multiple_and_destroy(queue, cmds, 2);

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
