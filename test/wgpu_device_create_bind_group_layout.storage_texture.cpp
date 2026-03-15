// Tests wgpu_device_create_bind_group_layout() with a storage texture entry
// (WGPU_BIND_GROUP_LAYOUT_TYPE_STORAGE_TEXTURE == 4), exercising the
// 'storageTexture' branch in wgpuReadBindGroupLayoutDescriptor.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Bind group layout with a write-only storage texture at binding 0
  WGpuBindGroupLayoutEntry entry = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_COMPUTE,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_STORAGE_TEXTURE,
    .layout.storageTexture = {
      .access = WGPU_STORAGE_TEXTURE_ACCESS_WRITE_ONLY,
      .format = WGPU_TEXTURE_FORMAT_RGBA8UNORM,
      .viewDimension = WGPU_TEXTURE_VIEW_DIMENSION_2D,
    },
  };

  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &entry, 1);
  assert(bgl);
  assert(wgpu_is_bind_group_layout(bgl));

  // Create a matching storage texture and bind group
  WGpuTextureDescriptor texDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  texDesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  texDesc.usage = WGPU_TEXTURE_USAGE_STORAGE_BINDING;
  texDesc.width = 64;
  texDesc.height = 64;
  WGpuTexture tex = wgpu_device_create_texture(device, &texDesc);
  assert(tex);
  WGpuTextureView texView = wgpu_texture_create_view_simple(tex);
  assert(texView);

  WGpuBindGroupEntry bgEntry = {
    .binding = 0,
    .resource = texView,
  };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, &bgEntry, 1);
  assert(bg);
  assert(wgpu_is_bind_group(bg));

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
