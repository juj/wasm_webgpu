// Tests wgpu_device_push_error_scope() with WGPU_ERROR_FILTER_OUT_OF_MEMORY,
// exercising the [,'out-of-memory','validation','internal'][filter] code path
// with filter == WGPU_ERROR_FILTER_OUT_OF_MEMORY (index 1) in lib_webgpu.js.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Push an 'out-of-memory' error scope
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_OUT_OF_MEMORY);

  // No operation that causes OOM is performed; just verify clean pop.
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *msg, void *udata) {
    assert(type == WGPU_ERROR_TYPE_NO_ERROR);
    EM_ASM(window.close());
  }, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
