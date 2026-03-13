// Verifies that wgpu_canvas_context_get_current_texture_view() returns a valid texture view object after the context has been configured.
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

  WGpuTextureView view = wgpu_canvas_context_get_current_texture_view(ctx);
  assert(view);
  assert(wgpu_is_texture_view(view));
  assert(!wgpu_is_texture(view)); // a texture view is not a texture

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
