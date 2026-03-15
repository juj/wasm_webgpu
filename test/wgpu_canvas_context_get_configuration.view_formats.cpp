// Tests wgpu_canvas_context_get_configuration() when viewFormats is non-empty,
// exercising the numViewFormats > 0 path where formats are written into the
// allocated config struct.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdlib.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");

  WGPU_TEXTURE_FORMAT fmt = WGPU_TEXTURE_FORMAT_BGRA8UNORM;
  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = fmt;
  config.numViewFormats = 1;
  config.viewFormats = &fmt;
  wgpu_canvas_context_configure(ctx, &config);

  // Now read it back — this exercises the numViewFormats > 0 branch
  WGpuCanvasConfiguration *cfg = wgpu_canvas_context_get_configuration(ctx);
  assert(cfg);
  assert(cfg->format == fmt);
  assert(cfg->numViewFormats == 1);
  assert(cfg->viewFormats);
  assert(cfg->viewFormats[0] == fmt);
  free(cfg);

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
