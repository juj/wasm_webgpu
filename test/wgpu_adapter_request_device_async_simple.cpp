// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  assert(device);
  assert(wgpu_is_device(device));

  assert(userData == 0);

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
