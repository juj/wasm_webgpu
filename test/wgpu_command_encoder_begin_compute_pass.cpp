// Verifies that wgpu_command_encoder_begin_compute_pass() with a null descriptor returns a valid compute pass encoder that can be ended and submitted.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  assert(encoder);

  // Begin a compute pass with no descriptor
  WGpuComputePassEncoder pass = wgpu_command_encoder_begin_compute_pass(encoder, 0);
  assert(pass);
  assert(wgpu_is_compute_pass_encoder(pass));

  wgpu_compute_pass_encoder_end(pass);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(encoder));

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
