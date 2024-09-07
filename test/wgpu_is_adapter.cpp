// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  assert(wgpu_is_adapter(adapter));

  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  assert(!wgpu_is_adapter(ctx));

  wgpu_object_destroy(adapter);
  assert(!wgpu_is_adapter(adapter));

  EM_ASM(window.close());
}

int main()
{
  assert(!wgpu_is_adapter(0));
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
