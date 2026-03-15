// Tests wgpu_command_encoder_begin_render_pass() with storeOp='discard' on the
// color attachment, exercising the WGPU_STORE_OP_DISCARD code path.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  // Render to an off-screen texture with storeOp=discard
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = colorFormat;
  tdesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  tdesc.width = 64;
  tdesc.height = 64;
  WGpuTexture tex = wgpu_device_create_texture(device, &tdesc);
  assert(tex);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);
  WGpuRenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  ca.view = wgpu_texture_create_view_simple(tex);
  ca.loadOp = WGPU_LOAD_OP_CLEAR;
  ca.clearValue = (WGpuColor){ .r = 0, .g = 0, .b = 1, .a = 1 };
  ca.storeOp = WGPU_STORE_OP_DISCARD; // exercises the 'discard' store op path
  WGpuRenderPassDescriptor passDesc = { .colorAttachments = &ca, .numColorAttachments = 1 };
  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(enc, &passDesc);
  wgpu_render_pass_encoder_end(pass);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));

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
