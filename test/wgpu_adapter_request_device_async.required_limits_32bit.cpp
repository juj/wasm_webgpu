// Tests $wgpuReadSupportedLimits with a non-zero 32-bit required limit field,
// exercising the `if ((v = HEAPU32[heap32Idx++])) requiredLimits[limitName] = v;`
// true branch for 32-bit limits. The existing wgpu_adapter_request_device_async.cpp
// only sets a 64-bit limit (maxStorageBufferBindingSize), so every 32-bit slot is zero
// and only the false branch (skip) is exercised for all 32-bit limits. Here we request
// maxTextureDimension2D = 8192 (the WebGPU-guaranteed minimum) to make the 32-bit
// non-zero branch execute and add the limit to requiredLimits on the JS side.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  assert(wgpu_is_device(device));

  // Verify the device honours the requested 32-bit limit.
  WGpuSupportedLimits limits = {};
  wgpu_device_get_limits(device, &limits);
  assert(limits.maxTextureDimension2D >= 8192);

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGpuDeviceDescriptor desc = {};
  // Set a 32-bit required limit to exercise the true branch of
  // `if ((v = HEAPU32[heap32Idx++])) requiredLimits[limitName] = v;`
  // in $wgpuReadSupportedLimits.
  desc.requiredLimits.maxTextureDimension2D = 8192;
  wgpu_adapter_request_device_async(adapter, &desc, ObtainedWebGpuDevice, 0);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
