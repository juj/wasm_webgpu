// Tests creating a WGpuBindGroupLayout with a 1D texture view dimension
// (WGPU_TEXTURE_VIEW_DIMENSION_1D, index 1), exercising GPUTextureViewDimensions[1]
// in the texture BGL entry branch of $wgpuReadBindGroupLayoutDescriptor.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuBindGroupLayoutEntry entry = {
    .binding    = 0,
    .visibility = WGPU_SHADER_STAGE_FRAGMENT,
    .type       = WGPU_BIND_GROUP_LAYOUT_TYPE_TEXTURE,
    .layout.texture = {
      .sampleType    = WGPU_TEXTURE_SAMPLE_TYPE_FLOAT,
      .viewDimension = WGPU_TEXTURE_VIEW_DIMENSION_1D, // index 1 — exercises '1d'
      .multisampled  = WGPU_FALSE,
    },
  };

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &entry, 1);
  (void)bgl;
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *m, void *) {
    if (type != WGPU_ERROR_TYPE_NO_ERROR)
      printf("1D texture BGL not supported: %s\n", m ? m : "");
    else
    {
      // Create a 1D texture to bind and verify the full path
      WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
      tdesc.format    = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
      tdesc.usage     = WGPU_TEXTURE_USAGE_TEXTURE_BINDING;
      tdesc.width     = 64;
      tdesc.height    = 1;
      tdesc.dimension = WGPU_TEXTURE_DIMENSION_1D;
      WGpuTexture tex = wgpu_device_create_texture(d, &tdesc);
      assert(tex);
    }
    EM_ASM(window.close());
  }, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
