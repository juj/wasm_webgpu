// Verifies that offscreen_canvas_post_to_worker() transfers OffscreenCanvas ownership to a
// Wasm Worker. Three properties are checked:
//   1. The canvas is valid in the main thread before posting.
//   2. The canvas is no longer valid in the main thread after posting (ownership transferred).
//   3. The canvas is valid in the worker after it has been received.
// flags: -sEXIT_RUNTIME=0 -sWASM_WORKERS

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

static const OffscreenCanvasId kCanvasId = 42;

void done() // runs in main thread, posted back from the worker
{
  printf("Test OK\n");
  EM_ASM(window.close());
}

void worker_main() // runs in worker thread
{
  // Property 3: canvas must be valid in the worker after being received.
  assert(offscreen_canvas_is_valid(kCanvasId));

  // Signal the main thread that all assertions passed.
  emscripten_wasm_worker_post_function_v(EMSCRIPTEN_WASM_WORKER_ID_PARENT, done);
}

int main()
{
  emscripten_wasm_worker_t worker = emscripten_malloc_wasm_worker(1024);

  canvas_transfer_control_to_offscreen("canvas", kCanvasId);

  // Property 1: canvas must be valid in the main thread before posting.
  assert(offscreen_canvas_is_valid(kCanvasId));

  offscreen_canvas_post_to_worker(kCanvasId, worker);

  // Property 2: canvas must no longer be valid in the main thread after posting.
  assert(!offscreen_canvas_is_valid(kCanvasId));

  // The canvas postMessage (above) is queued before this function post, so the
  // worker's message listener will store the canvas before worker_main() runs.
  emscripten_wasm_worker_post_function_v(worker, worker_main);
}
