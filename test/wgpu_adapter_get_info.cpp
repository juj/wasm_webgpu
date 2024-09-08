// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGpuAdapterInfo info;
  memset(&info, 0xFF, sizeof(info)); // Fill with garbage to test that all fields are filled.

  wgpu_adapter_get_info(adapter, &info);

  printf("Vendor: %s\n", info.vendor);
  printf("Architecture: %s\n", info.architecture);
  printf("Device: %s\n", info.device);
  printf("Description: %s\n", info.description);

  assert(strlen(info.vendor) < 512);
  assert(strlen(info.architecture) < 512);
  assert(strlen(info.device) < 512);
  assert(strlen(info.description) < 512);

  EM_ASM(window.close());
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
