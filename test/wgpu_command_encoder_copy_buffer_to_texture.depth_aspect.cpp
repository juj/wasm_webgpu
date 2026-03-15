// Tests copying data to a depth texture with aspect=WGPU_TEXTURE_ASPECT_DEPTH_ONLY,
// exercising the depth-only aspect code path in wgpuReadGpuTexelCopyTextureInfo.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  const uint32_t kWidth = 4, kHeight = 4;

  // Create a depth32float texture (depth-only format, supports COPY_DST)
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = WGPU_TEXTURE_FORMAT_DEPTH32FLOAT;
  tdesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT | WGPU_TEXTURE_USAGE_COPY_DST;
  tdesc.width = kWidth;
  tdesc.height = kHeight;
  WGpuTexture depthTex = wgpu_device_create_texture(device, &tdesc);
  assert(depthTex);

  // Fill a staging buffer with depth values (4 bytes per pixel for DEPTH32FLOAT)
  float depthData[kWidth * kHeight];
  for (uint32_t i = 0; i < kWidth * kHeight; i++) depthData[i] = 1.0f;
  WGpuBufferDescriptor bdesc = {
    .size = sizeof(depthData),
    .usage = WGPU_BUFFER_USAGE_COPY_SRC | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer buf = wgpu_device_create_buffer(device, &bdesc);
  assert(buf);
  wgpu_queue_write_buffer(wgpu_device_get_queue(device), buf, 0, depthData, sizeof(depthData));

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  WGpuTexelCopyBufferInfo src = WGPU_TEXEL_COPY_BUFFER_INFO_DEFAULT_INITIALIZER;
  src.buffer = buf;
  src.bytesPerRow = kWidth * sizeof(float); // 16 bytes per row (must be multiple of 256 for other formats; depth32float allows 16)
  src.rowsPerImage = kHeight;

  WGpuTexelCopyTextureInfo dst = WGPU_TEXEL_COPY_TEXTURE_INFO_DEFAULT_INITIALIZER;
  dst.texture = depthTex;
  dst.aspect = WGPU_TEXTURE_ASPECT_DEPTH_ONLY; // exercises the depth-only aspect path

  wgpu_command_encoder_copy_buffer_to_texture(enc, &src, &dst, kWidth, kHeight, 1);

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
