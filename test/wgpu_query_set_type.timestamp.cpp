// Verifies that wgpu_query_set_type() returns WGPU_QUERY_TYPE_TIMESTAMP (value 2) for a query
// set created with that type. The existing wgpu_query_set_type.cpp only checks OCCLUSION (value 1),
// leaving the GPUQueryTypes[2] = 'timestamp' mapping untested.
// Requires WGPU_FEATURE_TIMESTAMP_QUERY; test is skipped gracefully if not supported.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  if (!wgpu_device_supports_feature(device, WGPU_FEATURE_TIMESTAMP_QUERY))
  {
    printf("WGPU_FEATURE_TIMESTAMP_QUERY not supported, skipping test.\n");
    EM_ASM(window.close());
    return;
  }

  WGpuQuerySetDescriptor desc = {
    .type = WGPU_QUERY_TYPE_TIMESTAMP,
    .count = 2,
  };
  WGpuQuerySet querySet = wgpu_device_create_query_set(device, &desc);
  assert(querySet);
  assert(wgpu_is_query_set(querySet));
  assert(wgpu_query_set_type(querySet) == WGPU_QUERY_TYPE_TIMESTAMP);

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGpuDeviceDescriptor desc = {};
  desc.requiredFeatures = WGPU_FEATURE_TIMESTAMP_QUERY;
  wgpu_adapter_request_device_async(adapter, &desc, ObtainedWebGpuDevice, 0);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
