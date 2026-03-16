// Verifies that wgpu_texture_create_view() correctly marshals a non-zero usage field from
// WGpuTextureViewDescriptor into the JS createView() call. This exercises the
// `'usage': HEAPU32[descriptorIdx+2]` field path in the JS implementation which is not
// exercised by any other test (all other view tests leave usage at its default value of 0).
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  tdesc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING | WGPU_TEXTURE_USAGE_COPY_DST;
  tdesc.width = 4;
  tdesc.height = 4;
  WGpuTexture texture = wgpu_device_create_texture(device, &tdesc);
  assert(texture);

  WGpuTextureViewDescriptor vdesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER;
  vdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  vdesc.dimension = WGPU_TEXTURE_VIEW_DIMENSION_2D;
  vdesc.aspect = WGPU_TEXTURE_ASPECT_ALL;
  vdesc.mipLevelCount = 1;
  vdesc.arrayLayerCount = 1;
  // Set a non-zero usage (a subset of the texture's usage flags) to exercise the usage field
  // marshalling path in lib_webgpu.js. WGPU_TEXTURE_USAGE_TEXTURE_BINDING = 4.
  vdesc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING;

  WGpuTextureView view = wgpu_texture_create_view(texture, &vdesc);
  assert(view);
  assert(wgpu_is_texture_view(view));

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
