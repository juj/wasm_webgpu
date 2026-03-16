// Tests wgpu_device_set_lost_callback() by triggering device loss via
// wgpu_object_destroy(). According to the WebGPU spec, destroying the device
// resolves the device.lost Promise with reason 'destroyed', which exercises
// the `deviceLostInfo['reason'] == 'destroyed' ? 1 : 0` TRUE branch in the
// JS implementation and reports WGPU_DEVICE_LOST_REASON_DESTROYED (1) to
// the callback.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void DeviceLost(WGpuDevice device, WGPU_DEVICE_LOST_REASON reason, const char *message, void *userData)
{
  assert(reason == WGPU_DEVICE_LOST_REASON_DESTROYED);
  assert(userData == (void*)42);
  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Register the lost callback before destroying the device.
  wgpu_device_set_lost_callback(device, DeviceLost, (void*)42);

  // Destroy the device — this should resolve device.lost with reason 'destroyed',
  // causing DeviceLost() to be invoked with WGPU_DEVICE_LOST_REASON_DESTROYED.
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
