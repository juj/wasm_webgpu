// Verifies that calling wgpu_offscreen_canvas_get_webgpu_context() on the same offscreen canvas
// multiple times returns the same WGpuCanvasContext handle (exercises the ctx.wid early-return
// path in the JS implementation), and that only one live object is registered.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  offscreen_canvas_create(1, 128, 128);

  WGpuCanvasContext ctx = wgpu_offscreen_canvas_get_webgpu_context(1);
  assert(ctx);
  assert(wgpu_is_canvas_context(ctx));

  int numLiveObjects = wgpu_get_num_live_objects();

  // Calling again on the same canvas ID must return the identical handle.
  WGpuCanvasContext ctx2 = wgpu_offscreen_canvas_get_webgpu_context(1);
  assert(ctx2 == ctx);

  // Number of live WebGPU objects should not have grown (no duplicate registration).
  assert(wgpu_get_num_live_objects() == numLiveObjects);

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
