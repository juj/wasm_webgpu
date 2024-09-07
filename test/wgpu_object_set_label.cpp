// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_object_set_label(adapter, "test");
  EM_ASM({ if (wgpu[$0]['label'] != 'test') throw wgpu[$0]['label']; }, adapter);

  EM_ASM(window.close());
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
