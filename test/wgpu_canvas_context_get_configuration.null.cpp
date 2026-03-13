// Verifies that wgpu_canvas_context_get_configuration() returns null both before any configuration is set and after wgpu_canvas_context_unconfigure() is called.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdlib.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  assert(ctx);

  // Without calling configure(), getConfiguration() should return null
  WGpuCanvasConfiguration *got = wgpu_canvas_context_get_configuration(ctx);
  assert(got == 0); // null pointer = unconfigured

  // Now configure and unconfigure
  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(ctx, &config);
  wgpu_canvas_context_unconfigure(ctx);

  // After unconfigure(), getConfiguration() should again return null
  got = wgpu_canvas_context_get_configuration(ctx);
  assert(got == 0);

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
  emscripten_exit_with_live_runtime();
}
