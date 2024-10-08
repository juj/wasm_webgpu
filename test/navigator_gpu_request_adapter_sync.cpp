// flags: -sJSPI
// expected-output: navigator.gpu.requestAdapter(options={"forceFallbackAdapter":true,"powerPreference":"high-performance"})

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
  WGpuRequestAdapterOptions options = {
    .powerPreference = WGPU_POWER_PREFERENCE_HIGH_PERFORMANCE,
    .forceFallbackAdapter = WGPU_TRUE
  };

  assert(wgpu_sync_operations_pending() == 0); // We should start with no asyncified operations running.
  emscripten_set_timeout(in_between, 0, 0); // We should observe the sync operation.

  WGpuAdapter adapter = navigator_gpu_request_adapter_sync(&options);
  assert(wgpu_sync_operations_pending() == 0); // All async operations should have resolved now.
  assert(wgpu_is_adapter(adapter)); // Should have got a valid adapter
  assert(wgpu_adapter_is_fallback_adapter(adapter)); // That is a fallback adapter like we requested

  if (ok) // TODO: Grab errors and early quit in emrun harness
    EM_ASM(window.close());
}
