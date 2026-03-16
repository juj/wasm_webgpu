// Tests that explicitly destroying a child object (a texture view) while its
// parent (the texture) is still alive correctly exercises the
// `if (o.parentObject) o.parentObject.derivedObjects.delete(object)` branch in
// wgpu_object_destroy. After the view is destroyed the parent texture should
// still be valid, but the view itself must be invalid. Destroying the texture
// afterwards must succeed (it no longer tries to destroy the already-gone view).
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

  WGpuTextureViewDescriptor vdesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER;
  vdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  vdesc.dimension = WGPU_TEXTURE_VIEW_DIMENSION_2D;
  vdesc.aspect = WGPU_TEXTURE_ASPECT_ALL;
  vdesc.mipLevelCount = 1;
  vdesc.arrayLayerCount = 1;
  WGpuTextureView view = wgpu_texture_create_view(texture, &vdesc);
  assert(view);
  assert(wgpu_is_texture_view(view));

  // Destroy the view while the parent texture is still alive. This exercises
  // `if (o.parentObject) o.parentObject.derivedObjects.delete(object)` in
  // wgpu_object_destroy when the parent is still in the wgpu table.
  wgpu_object_destroy(view);

  // The view must be invalid; the parent texture must still be alive.
  assert(!wgpu_is_valid_object(view));
  assert(wgpu_is_texture(texture));

  // Destroying the texture now must succeed without double-freeing the view.
  wgpu_object_destroy(texture);
  assert(!wgpu_is_valid_object(texture));

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
