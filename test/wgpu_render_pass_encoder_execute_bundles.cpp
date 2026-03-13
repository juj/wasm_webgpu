// Verifies that wgpu_render_pass_encoder_execute_bundles() can execute a render bundle (with no draw calls) inside a render pass without crashing.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT format = navigator_gpu_get_preferred_canvas_format();

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = format;
  wgpu_canvas_context_configure(wgpu_canvas_get_webgpu_context("canvas"), &config);

  // Create a render bundle with no draw calls
  WGpuRenderBundleEncoderDescriptor bundleDesc = {};
  bundleDesc.colorFormats = &format;
  bundleDesc.numColorFormats = 1;
  WGpuRenderBundleEncoder bundleEncoder = wgpu_device_create_render_bundle_encoder(device, &bundleDesc);
  WGpuRenderBundle bundle = wgpu_render_bundle_encoder_finish(bundleEncoder);
  assert(bundle);

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  colorAttachment.view = wgpu_canvas_context_get_current_texture(wgpu_canvas_get_webgpu_context("canvas"));

  WGpuRenderPassDescriptor passDesc = {
    .colorAttachments = &colorAttachment,
    .numColorAttachments = 1,
  };
  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(encoder, &passDesc);
  assert(pass);

  // Execute the render bundle - should not crash
  wgpu_render_pass_encoder_execute_bundles(pass, &bundle, 1);

  wgpu_render_pass_encoder_end(pass);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(encoder));

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
