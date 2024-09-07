// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice result, void *userData)
{
  assert(wgpu_get_num_live_objects() == 3); // Adapter, Device and Queue
  assert(wgpu_is_valid_object(result));

  wgpu_destroy_all_objects(); // Should clear all objects

  assert(wgpu_get_num_live_objects() == 0);
  assert(!wgpu_is_valid_object(result));

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData)
{
  wgpu_adapter_request_device_async_simple(result, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
