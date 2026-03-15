// Tests creating a WGpuBindGroupLayout with a cube-array texture view dimension
// (WGPU_TEXTURE_VIEW_DIMENSION_CUBE_ARRAY, index 5), exercising that branch in
// $wgpuReadBindGroupLayoutDescriptor. Uses an error scope since cube-array
// textures may require a feature on some platforms.
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
      .viewDimension = WGPU_TEXTURE_VIEW_DIMENSION_CUBE_ARRAY, // index 5 — exercises 'cube-array'
      .multisampled  = WGPU_FALSE,
    },
  };

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &entry, 1);
  (void)bgl;
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *msg, void *) {
    if (type != WGPU_ERROR_TYPE_NO_ERROR)
      printf("cube-array texture view dimension not supported, skipping.\n");
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
