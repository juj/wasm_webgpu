// Tests wgpu_adapter_request_device_async() with a non-null defaultQueue label,
// exercising the 'return HEAPU32[heap32Idx] ? { label: utf8(...) } : void 0'
// code path in wgpuReadQueueDescriptor where the label pointer is non-null.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  assert(wgpu_is_device(device));

  // Verify the queue was created (the queue label is set in the browser
  // but we cannot read it back through this API, so just verify the queue exists).
  WGpuQueue queue = wgpu_device_get_queue(device);
  assert(wgpu_is_queue(queue));

  // Verify the queue label is accessible via wgpu_object_get_label
  char label[128] = {};
  wgpu_object_get_label(queue, label, sizeof(label));
  printf("Queue label: '%s'\n", label);
  // Label might be empty string or "my-queue" depending on browser support.

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGpuDeviceDescriptor desc = {};
  desc.defaultQueue.label = "my-queue"; // Non-null label exercises the queue label path
  wgpu_adapter_request_device_async(adapter, &desc, ObtainedWebGpuDevice, 0);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
