// Tests wgpu_canvas_context_configure() with a non-default usage flag
// (WGPU_TEXTURE_USAGE_COPY_SRC in addition to RENDER_ATTACHMENT), exercising
// the usage field in the canvas configuration.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  assert(ctx);

  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuCanvasConfiguration cfg = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  cfg.device = device;
  cfg.format = colorFormat;
  // Add COPY_SRC in addition to the implicit RENDER_ATTACHMENT usage
  cfg.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT | WGPU_TEXTURE_USAGE_COPY_SRC;

  wgpu_canvas_context_configure(ctx, &cfg);

  // Verify the canvas texture can be obtained (proves configure succeeded)
  WGpuTextureView view = wgpu_canvas_context_get_current_texture(ctx);
  assert(view);

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
