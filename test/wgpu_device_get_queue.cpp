// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuQueue queue = wgpu_device_get_queue(device);
  assert(queue);

  WGpuQueue queue2 = wgpu_device_get_queue(device);

  assert(queue == queue2); // Asking for the same queue multiple times should return the same object.

  char label[32];
  wgpu_object_get_label(queue, label, sizeof(label));
  assert(!strcmp(label, "ThisIsNameForMyQueue"));

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGpuDeviceDescriptor desc = {};
  desc.defaultQueue.label = "ThisIsNameForMyQueue";
  wgpu_adapter_request_device_async(adapter, &desc, ObtainedWebGpuDevice, 0);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
