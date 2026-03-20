// Tests the 'if (device)' FALSE branch in wgpu_adapter_request_device_async()
// when GPUAdapter.requestDevice() rejects because a required feature is not
// supported by the adapter. The callback must be invoked with device==0.
// Exercises the code path where wgpuStoreAndSetParent(device['queue'], device)
// is NOT called (device is undefined/null from the rejected promise).
// Skipped gracefully if the adapter happens to support every known feature.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void DeviceRequestFailed(WGpuDevice device, void *userData)
{
  // The device creation must have failed — device handle must be 0.
  assert(!device);
  printf("Device request correctly failed (device==0).\n");
  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGPU_FEATURES_BITFIELD supported = wgpu_adapter_get_features(adapter);

  // Compute the set of feature bits defined by the library that the adapter
  // does NOT support.  Requesting any unsupported required feature causes
  // GPUAdapter.requestDevice() to reject.
  WGPU_FEATURES_BITFIELD unsupported = ~supported & (WGPU_FEATURE_FIRST_UNUSED_BIT - 1);

  if (!unsupported)
  {
    // Every known feature is supported on this adapter — we cannot trigger a
    // rejection this way, so skip the test gracefully.
    printf("All known features supported; skipping unsupported-feature device-failure test.\n");
    EM_ASM(window.close());
    return;
  }

  // Keep only the lowest unsupported bit to form the minimal failing request.
  WGPU_FEATURES_BITFIELD lowest_unsupported = unsupported & (-unsupported);

  WGpuDeviceDescriptor desc = {};
  desc.requiredFeatures = lowest_unsupported;

  // This request must fail because the adapter does not support the required
  // feature, so the .catch branch fires and calls cb() with undefined (→ 0).
  wgpu_adapter_request_device_async(adapter, &desc, DeviceRequestFailed, 0);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
