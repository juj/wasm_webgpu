// Verifies that wgpu_sync_operations_pending() can be called in a JSPI build that uses no other
// asyncified wasm_webgpu function. This is a regression test for a missing JS library __deps
// declaration: the function reads the internal pending operations counter, so it must declare
// a dependency to it. Emscripten versions that do not infer JS library dependencies
// automatically (such as 6.0.13) otherwise leave the counter out of the build, and the call
// throws "ReferenceError: __wgpuNumAsyncifiedOperationsPending is not defined".
// flags: -sJSPI

#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  assert(wgpu_sync_operations_pending() == 0);

  EM_ASM(window.close());
}
