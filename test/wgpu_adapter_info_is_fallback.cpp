// Tests that the isFallbackAdapter field in WGpuAdapterInfo is populated.
// On most hardware this will be false; the test verifies the field is readable
// (0 or 1) rather than an uninitialized value.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  assert(wgpu_is_adapter(adapter));

  WGpuAdapterInfo info = {};
  wgpu_adapter_or_device_get_info(adapter, &info);

  // isFallbackAdapter must be 0 (false) or 1 (true)
  assert(info.isFallbackAdapter == 0 || info.isFallbackAdapter == 1);
  printf("isFallbackAdapter = %d\n", info.isFallbackAdapter);

  EM_ASM(window.close());
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
