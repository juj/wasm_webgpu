// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

WGpuAdapter adapter;

void ObtainedWebGpuDevice(WGpuDevice result, void *userData)
{
  assert(wgpu_get_num_live_objects() == 3); // Adapter, Device and Queue
  wgpu_object_destroy(adapter);
  assert(wgpu_get_num_live_objects() == 0);

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData)
{
  assert(wgpu_get_num_live_objects() == 1); // Adapter
  adapter = result;
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main() {
  assert(wgpu_get_num_live_objects() == 0);
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
