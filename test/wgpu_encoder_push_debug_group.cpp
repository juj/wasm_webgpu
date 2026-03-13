// Verifies that wgpu_encoder_push_debug_group(), wgpu_encoder_insert_debug_marker(), and wgpu_encoder_pop_debug_group() can be called on both a command encoder and a compute pass encoder without errors.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  assert(encoder);

  // Test debug group push/pop on a command encoder
  wgpu_encoder_push_debug_group(encoder, "MyGroup");
  wgpu_encoder_insert_debug_marker(encoder, "MyMarker");
  wgpu_encoder_pop_debug_group(encoder);

  // Also test on a compute pass encoder
  WGpuComputePassEncoder pass = wgpu_command_encoder_begin_compute_pass(encoder, 0);
  wgpu_encoder_push_debug_group(pass, "ComputeGroup");
  wgpu_encoder_insert_debug_marker(pass, "ComputeMarker");
  wgpu_encoder_pop_debug_group(pass);
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
