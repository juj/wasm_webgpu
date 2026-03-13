// Verifies that wgpu_texture_dimension() returns WGPU_TEXTURE_DIMENSION_2D and WGPU_TEXTURE_DIMENSION_3D for textures created with those respective dimensions.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  desc.dimension = WGPU_TEXTURE_DIMENSION_2D;
  WGpuTexture texture2d = wgpu_device_create_texture(device, &desc);
  assert(texture2d);
  assert(wgpu_texture_dimension(texture2d) == WGPU_TEXTURE_DIMENSION_2D);

  desc.dimension = WGPU_TEXTURE_DIMENSION_3D;
  desc.depthOrArrayLayers = 4;
  WGpuTexture texture3d = wgpu_device_create_texture(device, &desc);
  assert(texture3d);
  assert(wgpu_texture_dimension(texture3d) == WGPU_TEXTURE_DIMENSION_3D);

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
