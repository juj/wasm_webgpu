// Verifies that wgpu_query_set_count() returns the exact count (7) specified in the WGpuQuerySetDescriptor at creation.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuQuerySetDescriptor desc = {
    .type = WGPU_QUERY_TYPE_OCCLUSION,
    .count = 7,
  };
  WGpuQuerySet querySet = wgpu_device_create_query_set(device, &desc);
  assert(querySet);
  assert(wgpu_query_set_count(querySet) == 7);

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
