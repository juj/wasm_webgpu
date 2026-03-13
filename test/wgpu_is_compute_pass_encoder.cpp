// Verifies that wgpu_is_compute_pass_encoder() returns true for a WGpuComputePassEncoder and false for a WGpuCommandEncoder.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  WGpuComputePassEncoder pass = wgpu_command_encoder_begin_compute_pass(encoder, 0);
  assert(pass);
  assert(wgpu_is_compute_pass_encoder(pass));
  assert(!wgpu_is_compute_pass_encoder(encoder)); // a command encoder is not a compute pass encoder

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
