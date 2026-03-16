// Tests that destroying a texture via wgpu_object_destroy() also destroys all
// texture views that were created from it, exercising the
// `if (o.derivedObjects) o.derivedObjects.forEach(...)` branch in
// wgpu_object_destroy when the destroyed object has child derived objects.
// (wgpu_object_destroy.cpp only tests the device→queue chain; this test
// exercises the texture→view chain to confirm the derived-object hierarchy
// is properly cleaned up.)
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  tdesc.usage  = WGPU_TEXTURE_USAGE_TEXTURE_BINDING | WGPU_TEXTURE_USAGE_COPY_DST;
  tdesc.width  = 4;
  tdesc.height = 4;
  WGpuTexture texture = wgpu_device_create_texture(device, &tdesc);
  assert(texture);
  assert(wgpu_is_texture(texture));

  // Create two views so that the derivedObjects Map contains multiple entries.
  WGpuTextureViewDescriptor vdesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER;
  vdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  vdesc.dimension = WGPU_TEXTURE_VIEW_DIMENSION_2D;
  vdesc.aspect = WGPU_TEXTURE_ASPECT_ALL;
  vdesc.mipLevelCount = 1;
  vdesc.arrayLayerCount = 1;
  WGpuTextureView view1 = wgpu_texture_create_view(texture, &vdesc);
  WGpuTextureView view2 = wgpu_texture_create_view(texture, &vdesc);
  assert(view1);
  assert(view2);
  assert(wgpu_is_texture_view(view1));
  assert(wgpu_is_texture_view(view2));

  // Destroying the texture must also destroy both derived views, exercising
  // the `if (o.derivedObjects) o.derivedObjects.forEach(...)` branch.
  wgpu_object_destroy(texture);

  // The texture and both views must now be invalid.
  assert(!wgpu_is_valid_object(texture));
  assert(!wgpu_is_valid_object(view1));
  assert(!wgpu_is_valid_object(view2));

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
