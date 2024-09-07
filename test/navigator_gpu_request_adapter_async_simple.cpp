// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  assert(wgpu_is_adapter(adapter));
  assert(userData == (void*)0);

  EM_ASM(window.close());
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
