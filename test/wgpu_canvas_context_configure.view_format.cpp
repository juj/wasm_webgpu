// Test wgpu_canvas_context_configure() with a .viewFormats member passed in.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT format = WGPU_TEXTURE_FORMAT_BGRA8UNORM;
  WGpuCanvasConfiguration config = {
    .device = device,
    .format = format,
    .numViewFormats = 1,
    .viewFormats = &format
  };
  wgpu_canvas_context_configure(wgpu_canvas_get_webgpu_context("canvas"), &config);

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main(int argc, char **argv)
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
