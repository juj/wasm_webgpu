// Tests wgpu_command_encoder_copy_texture_to_texture() with non-zero mipLevel
// and non-zero origin coordinates, exercising those fields in
// $wgpuReadGpuTexelCopyTextureInfo: mipLevel, origin[x,y,z].
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Source texture: 16x16 with 4 mip levels
  WGpuTextureDescriptor srcDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  srcDesc.format           = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  srcDesc.usage            = WGPU_TEXTURE_USAGE_COPY_SRC | WGPU_TEXTURE_USAGE_COPY_DST;
  srcDesc.width            = 16;
  srcDesc.height           = 16;
  srcDesc.mipLevelCount    = 4; // mip levels 0..3
  WGpuTexture srcTex = wgpu_device_create_texture(device, &srcDesc);
  assert(srcTex);

  // Destination texture: 8x8 (same size as mip level 1 of source)
  WGpuTextureDescriptor dstDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  dstDesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  dstDesc.usage  = WGPU_TEXTURE_USAGE_COPY_DST;
  dstDesc.width  = 8;
  dstDesc.height = 8;
  WGpuTexture dstTex = wgpu_device_create_texture(device, &dstDesc);
  assert(dstTex);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  WGpuTexelCopyTextureInfo src = WGPU_TEXEL_COPY_TEXTURE_INFO_DEFAULT_INITIALIZER;
  src.texture  = srcTex;
  src.mipLevel = 1;              // non-zero mipLevel — exercises HEAP32[ptr+1]
  src.origin   = { .x = 2, .y = 2, .z = 0 }; // non-zero origin x,y — exercises [ptr+2],[ptr+3]
  src.aspect   = WGPU_TEXTURE_ASPECT_ALL;

  WGpuTexelCopyTextureInfo dst = WGPU_TEXEL_COPY_TEXTURE_INFO_DEFAULT_INITIALIZER;
  dst.texture  = dstTex;
  dst.mipLevel = 0;
  dst.origin   = { .x = 0, .y = 0, .z = 0 };
  dst.aspect   = WGPU_TEXTURE_ASPECT_ALL;

  // Copy 4x4 region from mip 1 (which is 8x8) starting at (2,2) → dst (0,0)
  wgpu_command_encoder_copy_texture_to_texture(enc, &src, &dst, 4, 4, 1);
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
