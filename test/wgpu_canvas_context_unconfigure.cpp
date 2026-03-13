// Verifies that wgpu_canvas_context_unconfigure() can be called repeatedly after wgpu_canvas_context_configure() without crashing.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(ctx, &config);

  // Unconfigure - should not crash
  wgpu_canvas_context_unconfigure(ctx);

  // Re-configure and unconfigure again to confirm repeated calls work
  wgpu_canvas_context_configure(ctx, &config);
  wgpu_canvas_context_unconfigure(ctx);

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
