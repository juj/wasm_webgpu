// Verifies that wgpu_queue_on_submitted_work_done_sync() suspends until the work submitted to the queue has finished, and that no sync operation is left pending after it returns.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

int numWorkDoneCallbacks = 0;

void OnWorkDone(WGpuQueue queue, void *userData)
{
  ++numWorkDoneCallbacks;
}

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);
  WGpuQueue queue = wgpu_device_get_queue(device);
  assert(queue);

  // Waiting on a queue that has not had any work submitted to it should return.
  wgpu_queue_on_submitted_work_done_sync(queue);
  assert(wgpu_sync_operations_pending() == 0);

  // Submit a command buffer, and register an asynchronous work done callback for it.
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  wgpu_queue_submit_one_and_destroy(queue, wgpu_command_encoder_finish(encoder));
  wgpu_queue_set_on_submitted_work_done_callback(queue, OnWorkDone, 0);

  // The callback can only fire if execution yields back to the browser event loop.
  assert(numWorkDoneCallbacks == 0);

  // The asynchronous callback was registered first, so if the synchronous wait really did
  // wait for the submitted work to finish, then that callback must have fired by now.
  wgpu_queue_on_submitted_work_done_sync(queue);
  assert(numWorkDoneCallbacks == 1);
  assert(wgpu_sync_operations_pending() == 0);

  printf("Test OK\n");
  EM_ASM(window.close());
}
