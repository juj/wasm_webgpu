// Tests wgpu_canvas_context_configure() with WGPU_CANVAS_ALPHA_MODE_OPAQUE (index 1),
// exercising GPUCanvasAlphaModes[1] → 'opaque' in lib_webgpu.js.
// Reads back the configuration to verify the alpha mode was correctly marshalled.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdlib.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  assert(ctx);

  WGpuCanvasConfiguration cfg = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  cfg.device    = device;
  cfg.format    = navigator_gpu_get_preferred_canvas_format();
  cfg.alphaMode = WGPU_CANVAS_ALPHA_MODE_OPAQUE; // index 1 → 'opaque'

  wgpu_canvas_context_configure(ctx, &cfg);

  // Read back the configuration and verify the alpha mode round-trips correctly
  WGpuCanvasConfiguration *got = wgpu_canvas_context_get_configuration(ctx);
  assert(got);
  assert(got->alphaMode == WGPU_CANVAS_ALPHA_MODE_OPAQUE);
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
}
