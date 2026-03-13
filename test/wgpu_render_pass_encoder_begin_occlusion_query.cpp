// Verifies that wgpu_render_pass_encoder_begin_occlusion_query() and wgpu_render_pass_encoder_end_occlusion_query() can be called within a render pass that has an occlusion query set attached.
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

  // Create an occlusion query set
  WGpuQuerySetDescriptor queryDesc = {
    .type = WGPU_QUERY_TYPE_OCCLUSION,
    .count = 2,
  };
  WGpuQuerySet querySet = wgpu_device_create_query_set(device, &queryDesc);
  assert(querySet);

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  colorAttachment.view = wgpu_canvas_context_get_current_texture(wgpu_canvas_get_webgpu_context("canvas"));

  WGpuRenderPassDescriptor passDesc = {
    .colorAttachments = &colorAttachment,
    .numColorAttachments = 1,
    .occlusionQuerySet = querySet,
  };
  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(encoder, &passDesc);
  assert(pass);

  // Begin and end an occlusion query around (empty) draw calls
  wgpu_render_pass_encoder_begin_occlusion_query(pass, 0);
  wgpu_render_pass_encoder_end_occlusion_query(pass);

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
