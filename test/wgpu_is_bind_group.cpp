// Verifies that wgpu_is_bind_group() returns true for a WGpuBindGroup and false for a WGpuBindGroupLayout.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuBindGroupLayout layout = wgpu_device_create_bind_group_layout(device, 0, 0);
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, layout, 0, 0);
  assert(bg);
  assert(wgpu_is_bind_group(bg));
  assert(!wgpu_is_bind_group(layout)); // a bind group layout is not a bind group

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
