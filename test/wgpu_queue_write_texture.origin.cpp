// Tests wgpu_queue_write_texture() with non-zero origin coordinates,
// exercising the origin[x,y,z] array construction in $wgpuReadGpuTexelCopyTextureInfo
// when called from wgpu_queue_write_texture.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  const uint32_t kWidth = 8, kHeight = 8;

  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  tdesc.usage  = WGPU_TEXTURE_USAGE_COPY_DST | WGPU_TEXTURE_USAGE_COPY_SRC;
  tdesc.width  = kWidth;
  tdesc.height = kHeight;
  WGpuTexture texture = wgpu_device_create_texture(device, &tdesc);
  assert(texture);

  // Fill a 4x4 block of RGBA pixels (each 4 bytes)
  uint8_t data[4 * 4 * 4];
  memset(data, 0xFF, sizeof(data)); // solid white

  // Write to a sub-region starting at origin (2, 2, 0) — non-zero x and y
  WGpuTexelCopyTextureInfo dst = WGPU_TEXEL_COPY_TEXTURE_INFO_DEFAULT_INITIALIZER;
  dst.texture  = texture;
  dst.mipLevel = 0;
  dst.origin   = { .x = 2, .y = 2, .z = 0 }; // non-zero origin — exercises origin array path
  dst.aspect   = WGPU_TEXTURE_ASPECT_ALL;

  // bytesPerBlockRow = 4 pixels × 4 bytes/pixel = 16
  wgpu_queue_write_texture(wgpu_device_get_queue(device), &dst, data, 4 * 4, 4, 4, 4, 1);

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
