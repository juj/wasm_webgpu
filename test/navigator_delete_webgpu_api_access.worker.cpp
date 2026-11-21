// Verifies that navigator_delete_webgpu_api_access() works when called in a Wasm Worker. In a
// Worker the navigator object is an instance of WorkerNavigator, and the Navigator interface
// that the main thread has does not exist, so access to navigator.gpu must be removed via
// the prototype of whichever of the two interfaces the calling thread has.
// flags: -sEXIT_RUNTIME=0 -sWASM_WORKERS

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void done() // runs in main thread, posted back from the worker
{
  // Removing access in the worker does not affect the main thread.
  assert(navigator_gpu_available());

  printf("Test OK\n");
  EM_ASM(window.close());
}

void worker_main() // runs in worker thread
{
  assert(navigator_gpu_available());

  navigator_delete_webgpu_api_access();
  assert(!navigator_gpu_available());

  // Calling a second time is a no-op.
  navigator_delete_webgpu_api_access();
  assert(!navigator_gpu_available());

  // Signal the main thread that all assertions passed.
  emscripten_wasm_worker_post_function_v(EMSCRIPTEN_WASM_WORKER_ID_PARENT, done);
}

int main()
{
  assert(navigator_gpu_available());

  emscripten_wasm_worker_t worker = emscripten_malloc_wasm_worker(64*1024);
  emscripten_wasm_worker_post_function_v(worker, worker_main);
}
