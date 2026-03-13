// Verifies that wgpu_query_set_type() returns WGPU_QUERY_TYPE_OCCLUSION for a query set created with that type.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuQuerySetDescriptor desc = {
    .type = WGPU_QUERY_TYPE_OCCLUSION,
    .count = 2,
  };
  WGpuQuerySet querySet = wgpu_device_create_query_set(device, &desc);
  assert(querySet);
  assert(wgpu_query_set_type(querySet) == WGPU_QUERY_TYPE_OCCLUSION);

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
