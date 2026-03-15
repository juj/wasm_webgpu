// Tests wgpu_device_create_render_bundle_encoder() with a depthStencilFormat
// and depthReadOnly/stencilReadOnly flags, exercising those fields in
// wgpu_device_create_render_bundle_encoder's descriptor read path.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  // Create a render bundle encoder with depth-stencil format and read-only flags
  WGPU_TEXTURE_FORMAT colorFormats[1] = { colorFormat };
  WGpuRenderBundleEncoderDescriptor desc = {};
  desc.colorFormats = colorFormats;
  desc.numColorFormats = 1;
  desc.depthStencilFormat = WGPU_TEXTURE_FORMAT_DEPTH24PLUS_STENCIL8;
  desc.sampleCount = 1;
  desc.depthReadOnly = WGPU_TRUE;
  desc.stencilReadOnly = WGPU_TRUE;

  WGpuRenderBundleEncoder enc = wgpu_device_create_render_bundle_encoder(device, &desc);
  assert(enc);
  assert(wgpu_is_render_bundle_encoder(enc));

  // Finish the (empty) bundle
  WGpuRenderBundle bundle = wgpu_render_bundle_encoder_finish(enc);
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
