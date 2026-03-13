// Verifies that wgpu_texture_format() returns the format (RGBA8UNORM or R8UNORM) that was specified in the WGpuTextureDescriptor at creation time.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  WGpuTexture texture = wgpu_device_create_texture(device, &desc);
  assert(texture);
  assert(wgpu_texture_format(texture) == WGPU_TEXTURE_FORMAT_RGBA8UNORM);

  WGpuTextureDescriptor desc2 = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc2.format = WGPU_TEXTURE_FORMAT_R8UNORM;
  desc2.usage = WGPU_TEXTURE_USAGE_COPY_SRC | WGPU_TEXTURE_USAGE_COPY_DST;
  WGpuTexture texture2 = wgpu_device_create_texture(device, &desc2);
  assert(texture2);
  assert(wgpu_texture_format(texture2) == WGPU_TEXTURE_FORMAT_R8UNORM);

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
