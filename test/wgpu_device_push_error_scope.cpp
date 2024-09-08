// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_OUT_OF_MEMORY);
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_INTERNAL);

//  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
