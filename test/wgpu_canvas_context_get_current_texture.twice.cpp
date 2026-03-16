// Verifies that calling wgpu_canvas_context_get_current_texture() twice in the same
// animation frame exercises the `if (canvasTexture != wgpu[1])` FALSE branch in the
// JS implementation. On the second call within the same frame, getCurrentTexture()
// returns the same JS object that is already stored at wgpu[1], so the condition is
// false and the destroy/store body is skipped. Both calls must return the same handle.
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

  // First call: canvasTexture != wgpu[1] (wgpu[1] is undefined), so the if-body runs.
  WGpuTexture tex1 = wgpu_canvas_context_get_current_texture(ctx);
  assert(tex1);
  assert(wgpu_is_texture(tex1));

  // Second call within the same frame: getCurrentTexture() returns the same JS object,
  // so canvasTexture == wgpu[1] and the destroy/store body is skipped (false branch).
  WGpuTexture tex2 = wgpu_canvas_context_get_current_texture(ctx);
  assert(tex2);
  assert(wgpu_is_texture(tex2));

  // Both calls return the hardcoded canvas texture ID 1.
  assert(tex1 == tex2);

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
