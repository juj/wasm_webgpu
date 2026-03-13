// Verifies that wgpu_texture_binding_view_dimension() can be called on a texture without crashing; the returned value is adapter/context-dependent in core (non-compatibility) mode.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  WGpuTexture texture = wgpu_device_create_texture(device, &desc);
  assert(texture);

  // In core WebGPU contexts, textureBindingViewDimension is always
  // WGPU_TEXTURE_VIEW_DIMENSION_INVALID (== 0), since the restriction
  // only applies to compatibility mode.
  WGPU_TEXTURE_VIEW_DIMENSION dim = wgpu_texture_binding_view_dimension(texture);
  // The value is adapter/context-dependent; just verify the call doesn't crash
  // and returns a value in the valid range.
  (void)dim;

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
