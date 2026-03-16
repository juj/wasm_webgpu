// Verifies that wgpu_device_create_render_bundle_encoder() works with sampleCount=4 (MSAA),
// exercising the `'sampleCount': HEAPU32[descriptor+4]` field path for values other than 1.
// All other render bundle encoder tests use sampleCount=1.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuRenderBundleEncoderDescriptor desc = {};
  desc.colorFormats = &colorFormat;
  desc.numColorFormats = 1;
  desc.sampleCount = 4; // MSAA — exercises the non-default sampleCount field path

  WGpuRenderBundleEncoder encoder = wgpu_device_create_render_bundle_encoder(device, &desc);
  assert(encoder);
  assert(wgpu_is_render_bundle_encoder(encoder));

  WGpuRenderBundle bundle = wgpu_render_bundle_encoder_finish(encoder);
  assert(bundle);
  assert(wgpu_is_render_bundle(bundle));

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
