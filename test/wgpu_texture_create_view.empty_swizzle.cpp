// Tests that wgpu_texture_create_view() uses 'rgba' as the swizzle default when the
// swizzle field in the descriptor is an empty string, exercising the
// `'swizzle': UTF8ToString(descriptorByteIdx + 32) || 'rgba'` fallback branch in JS.
// All other tests use WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER which initialises
// swizzle to "rgba" (non-empty and truthy), so the || fallback is never reached by those
// tests. Here we start from the default initializer and then zero out swizzle[0] to
// produce an empty string, making UTF8ToString() return "" (falsy) and triggering the
// || 'rgba' path in the JS implementation.
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
  // Clear the swizzle field to an empty string. UTF8ToString() will return "" (falsy),
  // triggering the `|| 'rgba'` fallback branch in $wgpu_texture_create_view.
  vdesc.swizzle[0] = '\0';

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
