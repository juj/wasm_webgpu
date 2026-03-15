// Tests wgpu_canvas_context_configure() with WGPU_CANVAS_TONE_MAPPING_MODE_STANDARD
// (index 1), exercising GPUCanvasToneMappingModes[1] → 'standard' in lib_webgpu.js.
// This is distinct from the existing .tone_mapping.cpp which tests EXTENDED (index 2).
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdlib.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  assert(ctx);

  WGpuCanvasConfiguration cfg = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  cfg.device = device;
  cfg.format = navigator_gpu_get_preferred_canvas_format();
  cfg.toneMapping.mode = WGPU_CANVAS_TONE_MAPPING_MODE_STANDARD; // index 1 → 'standard'

  wgpu_canvas_context_configure(ctx, &cfg);

  // Read back and verify round-trip (standard tone mapping is universally supported)
  WGpuCanvasConfiguration *got = wgpu_canvas_context_get_configuration(ctx);
  assert(got);
  // toneMapping field is not yet supported in Firefox. TODO: Remove this when Firefox impl. is ready.
  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    assert(got->toneMapping.mode == WGPU_CANVAS_TONE_MAPPING_MODE_STANDARD);
  }
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
