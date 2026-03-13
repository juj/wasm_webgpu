// Verifies that configuring a canvas context with WGPU_CANVAS_ALPHA_MODE_PREMULTIPLIED is stored and retrievable via wgpu_canvas_context_get_configuration().
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  assert(ctx);

  // Configure with PREMULTIPLIED alpha mode
  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  config.alphaMode = WGPU_CANVAS_ALPHA_MODE_PREMULTIPLIED;
  wgpu_canvas_context_configure(ctx, &config);

  // Verify the configuration was stored
  WGpuCanvasConfiguration *got = wgpu_canvas_context_get_configuration(ctx);
  assert(got);
  assert(got->alphaMode == WGPU_CANVAS_ALPHA_MODE_PREMULTIPLIED);
  free(got);

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
