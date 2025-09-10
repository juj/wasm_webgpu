// flags: -sJSPI

#include "lib_webgpu.h"
#include <assert.h>

int ok = 1;

void in_between(void *arg)
{
  if (!wgpu_sync_operations_pending()) ok = 0;
  assert(wgpu_sync_operations_pending() == 1);
  EM_ASM({ console.log(`In between, ${$0} sync operations pending`); }, wgpu_sync_operations_pending());
}

int main()
{
  assert(wgpu_sync_operations_pending() == 0); // We should start with no asyncified operations running.
  emscripten_set_timeout(in_between, 0, 0); // We should observe the sync operation.

  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  assert(wgpu_sync_operations_pending() == 0); // All async operations should have resolved now.
  assert(wgpu_is_adapter(adapter)); // Should have got a valid adapter

  if (ok) // TODO: Grab errors and early quit in emrun harness
    EM_ASM(window.close());
}
