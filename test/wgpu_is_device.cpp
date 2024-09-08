// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  assert(!wgpu_is_device(0));
  assert(wgpu_is_device(device));

  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  assert(!wgpu_is_device(ctx));

  wgpu_object_destroy(device);
  assert(!wgpu_is_device(device));

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  assert(!wgpu_is_device(adapter));
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
