// Verifies that wgpu_is_query_set() returns true for a WGpuQuerySet and false for a WGpuDevice.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuQuerySetDescriptor desc = {
    .type = WGPU_QUERY_TYPE_OCCLUSION,
    .count = 4,
  };
  WGpuQuerySet querySet = wgpu_device_create_query_set(device, &desc);
  assert(querySet);
  assert(wgpu_is_query_set(querySet));
  assert(!wgpu_is_query_set(device)); // a device is not a query set

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
