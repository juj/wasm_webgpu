// Verifies that wgpu_device_create_render_bundle_encoder() returns a valid encoder and that calling wgpu_render_bundle_encoder_finish() produces a valid WGpuRenderBundle.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuRenderBundleEncoderDescriptor desc = {};
  desc.colorFormats = &colorFormat;
  desc.numColorFormats = 1;
  desc.sampleCount = 1;

  WGpuRenderBundleEncoder encoder = wgpu_device_create_render_bundle_encoder(device, &desc);
  assert(encoder);
  assert(wgpu_is_render_bundle_encoder(encoder));

  // Finish the bundle encoder to get a render bundle
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
