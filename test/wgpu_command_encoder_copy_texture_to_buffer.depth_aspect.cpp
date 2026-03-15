// Tests wgpu_command_encoder_copy_texture_to_buffer() with
// WGPU_TEXTURE_ASPECT_DEPTH_ONLY on the source, exercising that aspect value
// in $wgpuReadGpuTexelCopyTextureInfo for the copy-to-buffer direction.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  const uint32_t kWidth = 4, kHeight = 4;

  // Create a DEPTH32FLOAT texture that supports COPY_SRC
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = WGPU_TEXTURE_FORMAT_DEPTH32FLOAT;
  tdesc.usage  = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT | WGPU_TEXTURE_USAGE_COPY_SRC;
  tdesc.width  = kWidth;
  tdesc.height = kHeight;
  WGpuTexture depthTex = wgpu_device_create_texture(device, &tdesc);
  assert(depthTex);

  // Destination buffer: DEPTH32FLOAT is 4 bytes/pixel, bytesPerRow must be ≥256
  const uint32_t kBytesPerRow = 256;
  WGpuBufferDescriptor bdesc = {
    .size  = kBytesPerRow * kHeight,
    .usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_MAP_READ,
  };
  WGpuBuffer dstBuf = wgpu_device_create_buffer(device, &bdesc);
  assert(dstBuf);

  // First render a cleared depth texture so it has valid content
  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassDepthStencilAttachment dsAttach = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_DEFAULT_INITIALIZER;
  dsAttach.view         = depthTex;
  dsAttach.depthLoadOp  = WGPU_LOAD_OP_CLEAR;
  dsAttach.depthClearValue = 0.5f;
  dsAttach.depthStoreOp = WGPU_STORE_OP_STORE;
  WGpuRenderPassDescriptor clearPass = {};
  clearPass.depthStencilAttachment = dsAttach;
  WGpuRenderPassEncoder rp = wgpu_command_encoder_begin_render_pass(enc, &clearPass);
  wgpu_render_pass_encoder_end(rp);

  // Now copy the depth texture to the buffer using DEPTH_ONLY aspect
  WGpuTexelCopyTextureInfo src = WGPU_TEXEL_COPY_TEXTURE_INFO_DEFAULT_INITIALIZER;
  src.texture = depthTex;
  src.aspect  = WGPU_TEXTURE_ASPECT_DEPTH_ONLY; // exercises the depth-only aspect copy path

  WGpuTexelCopyBufferInfo dst = WGPU_TEXEL_COPY_BUFFER_INFO_DEFAULT_INITIALIZER;
  dst.buffer       = dstBuf;
  dst.bytesPerRow  = kBytesPerRow;
  dst.rowsPerImage = kHeight;

  wgpu_command_encoder_copy_texture_to_buffer(enc, &src, &dst, kWidth, kHeight, 1);
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
