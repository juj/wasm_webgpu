// Verifies that the callback registered via wgpu_queue_set_on_submitted_work_done_callback() is invoked with the correct queue and user-data after command buffer submission.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void OnWorkDone(WGpuQueue queue, void *userData)
{
  assert(queue);
  assert(wgpu_is_queue(queue));
  assert(userData == (void*)99);

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuQueue queue = wgpu_device_get_queue(device);
  assert(queue);

  // Register a callback that fires when submitted work is done
  wgpu_queue_set_on_submitted_work_done_callback(queue, OnWorkDone, (void*)99);

  // Submit an empty command buffer to trigger the callback
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  wgpu_queue_submit_one_and_destroy(queue, wgpu_command_encoder_finish(encoder));
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
