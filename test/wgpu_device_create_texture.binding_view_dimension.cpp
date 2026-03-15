// Tests wgpu_device_create_texture() with a non-default textureBindingViewDimension,
// exercising the 'GPUTextureViewDimensions[HEAPU32[descriptor+11]]' code path
// in wgpu_device_create_texture. This field is mainly used in WebGPU compatibility
// mode but is still a valid field to set in core mode.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a 2D array texture with textureBindingViewDimension = 2D_ARRAY
  // This exercises the textureBindingViewDimension field read in JS.
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  desc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING;
  desc.width = 64;
  desc.height = 64;
  desc.depthOrArrayLayers = 4;
  desc.dimension = WGPU_TEXTURE_DIMENSION_2D;
  desc.textureBindingViewDimension = WGPU_TEXTURE_VIEW_DIMENSION_2D_ARRAY;

  WGpuTexture tex = wgpu_device_create_texture(device, &desc);
  assert(tex);
  assert(wgpu_is_texture(tex));

  // Verify the texture dimensions are correct
  assert(wgpu_texture_depth_or_array_layers(tex) == 4);

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
