// Tests clearing the uncaptured error callback by passing null to
// wgpu_device_set_uncapturederror_callback(), exercising the
// 'callback ? function(...) : null' false branch.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

static void MyErrorCallback(WGpuDevice device, WGPU_ERROR_TYPE errorType, const char *errorMessage, void *userData)
{
  // Should not be called after we clear it
  assert(0 && "Error callback should have been cleared");
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // First set a non-null callback
  wgpu_device_set_uncapturederror_callback(device, MyErrorCallback, 0);

  // Now clear the callback by passing null - exercises the 'null' branch
  wgpu_device_set_uncapturederror_callback(device, 0, 0);

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
