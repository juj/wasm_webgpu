// flags: -sJSPI=1

#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);
  assert(wgpu_is_device(device));

  EM_ASM(window.close());
}
