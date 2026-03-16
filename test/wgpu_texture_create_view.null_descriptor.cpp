// Verifies that wgpu_texture_create_view() accepts a null descriptor and returns a valid default
// texture view. This exercises the false branch of the `descriptorIdx ? {...} : void 0` expression
// in the JS implementation, which passes `undefined` to createView() to get the browser default.
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

  // Pass a null descriptor to exercise the void 0 branch — the browser creates a default view.
  WGpuTextureView view = wgpu_texture_create_view(texture, /*textureViewDesc=*/0);
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
