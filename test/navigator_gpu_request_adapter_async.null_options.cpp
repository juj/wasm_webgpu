// Verifies that navigator_gpu_request_adapter_async() can be called with a null options
// pointer, as lib_webgpu.h documents: "options: may be null to request an adapter without
// specific options." Debug builds must not assert on the null pointer either, and the
// parameter must not be declared NOTNULL, so that passing a null pointer does not make the
// compiler emit a -Wnonnull warning.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  assert(userData == (void*)42);
  assert(adapter);
  assert(wgpu_is_adapter(adapter));

  printf("Test OK\n");
  EM_ASM(window.close());
}

int main()
{
  WGPU_BOOL success = navigator_gpu_request_adapter_async(0, ObtainedWebGpuAdapter, (void*)42);
  assert(success);
}
