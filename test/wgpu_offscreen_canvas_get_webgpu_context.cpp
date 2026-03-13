// Verifies that wgpu_offscreen_canvas_get_webgpu_context() returns a valid, configurable WGpuCanvasContext for an offscreen canvas created with offscreen_canvas_create().
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create an offscreen canvas and get its WebGPU context
  offscreen_canvas_create(1, 128, 128);

  WGpuCanvasContext ctx = wgpu_offscreen_canvas_get_webgpu_context(1);
  assert(ctx);
  assert(wgpu_is_canvas_context(ctx));

  // Configure and use the offscreen canvas context
  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(ctx, &config);

  offscreen_canvas_destroy(1);

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
