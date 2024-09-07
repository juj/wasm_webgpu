// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice result, void *userData)
{
  assert(wgpu_get_num_live_objects() == 3); // Adapter, Device and Queue

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData)
{
  assert(wgpu_get_num_live_objects() == 1); // Adapter
  wgpu_adapter_request_device_async_simple(result, ObtainedWebGpuDevice);
}

int main()
{
  assert(wgpu_get_num_live_objects() == 0); // No live objects
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
