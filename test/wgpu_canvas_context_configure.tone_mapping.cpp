// Tests wgpu_canvas_context_configure() with WGPU_CANVAS_TONE_MAPPING_MODE_EXTENDED.
// This exercises the GPUCanvasToneMappingModes[2] code path in lib_webgpu.js.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  config.toneMapping.mode = WGPU_CANVAS_TONE_MAPPING_MODE_EXTENDED;

  // Push a validation error scope so that if EXTENDED tone mapping is not
  // supported by this browser, the error is captured rather than propagating.
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  wgpu_canvas_context_configure(ctx, &config);
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *msg, void *) {
    // Either succeeds (NO_ERROR) or produces a validation error, both are acceptable.
    (void)type; (void)msg;
    EM_ASM(window.close());
  }, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
