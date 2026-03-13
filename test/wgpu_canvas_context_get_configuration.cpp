// Verifies that wgpu_canvas_context_get_configuration() returns a configuration matching the device and format set by wgpu_canvas_context_configure().
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdlib.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(ctx, &config);

  WGpuCanvasConfiguration *retrieved = wgpu_canvas_context_get_configuration(ctx);
  assert(retrieved);
  assert(retrieved->device == device);
  assert(retrieved->format == config.format);
  free(retrieved);

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
