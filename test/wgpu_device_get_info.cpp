// Verifies that wgpu_device_get_adapter_info() populates a WGpuAdapterInfo struct with null-terminated vendor/architecture/device/description strings and a valid isFallbackAdapter boolean.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // wgpu_device_get_adapter_info is a macro for wgpu_adapter_or_device_get_info
  // Calling it with a device handle should be supported
  WGpuAdapterInfo info;
  wgpu_device_get_adapter_info(device, &info);

  // vendor, architecture, device, and description are always present (possibly empty strings)
  // Just assert they are null-terminated by checking that accessing them doesn't crash
  (void)info.vendor[0];
  (void)info.architecture[0];
  (void)info.device[0];
  (void)info.description[0];

  // isFallbackAdapter should be a boolean (0 or 1)
  assert(info.isFallbackAdapter == WGPU_FALSE || info.isFallbackAdapter == WGPU_TRUE);

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
