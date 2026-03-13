// Verifies that the device-lost callback registered via wgpu_device_set_lost_callback() is invoked with WGPU_DEVICE_LOST_REASON_DESTROYED when the device is explicitly destroyed.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void OnDeviceLost(WGpuDevice device, WGPU_DEVICE_LOST_REASON reason, const char *message, void *userData)
{
  printf("Device lost: reason=%d message=%s\n", reason, message ? message : "(null)");
  assert(userData == (void*)55);
  // When destroyed explicitly, reason should be WGPU_DEVICE_LOST_REASON_DESTROYED
  assert(reason == WGPU_DEVICE_LOST_REASON_DESTROYED);

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  wgpu_device_set_lost_callback(device, OnDeviceLost, (void*)55);

  // Destroy the device to trigger the lost callback
  wgpu_object_destroy(device);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
