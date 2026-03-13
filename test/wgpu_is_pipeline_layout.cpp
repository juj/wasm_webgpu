// Verifies that wgpu_is_pipeline_layout() returns true for a WGpuPipelineLayout and false for a WGpuBindGroupLayout.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, 0, 0);
  WGpuPipelineLayout layout = wgpu_device_create_pipeline_layout(device, &bgl, 1);
  assert(layout);
  assert(wgpu_is_pipeline_layout(layout));
  assert(!wgpu_is_pipeline_layout(bgl)); // a bind group layout is not a pipeline layout

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
