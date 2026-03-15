// Verifies that offscreen_canvas_post_to_worker() and offscreen_canvas_post_to_pthread() work
// correctly when both are used in the same program. This specifically exercises the shared
// $wgpuOffscreenCanvasListener __postset, which must register the "message" event listener
// exactly once even though both post functions are linked into the binary.
//
// Four properties are checked:
//   1. Both canvases are valid in the main thread before posting.
//   2. Both canvases are invalid in the main thread after posting (ownership transferred).
//   3. kWorkerCanvasId is valid in the Wasm Worker (and kPthreadCanvasId is not).
//   4. kPthreadCanvasId is valid in the pthread (and kWorkerCanvasId is not).
// flags: -sEXIT_RUNTIME=0 -sWASM_WORKERS -pthread -sPTHREAD_POOL_SIZE=1

#include "lib_webgpu.h"
#include <assert.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>
#include <stdio.h>

static const OffscreenCanvasId kWorkerCanvasId = 42;
static const OffscreenCanvasId kPthreadCanvasId = 43;

static pthread_t thread;
static em_proxying_queue *thread_queue;
// Counts how many threads have finished. When it reaches 2, the test is done.
// Both increments happen on the main thread, so no mutex is needed.
static int completedThreads = 0;

void done() // runs in main thread
{
  if (++completedThreads == 2)
  {
    printf("Test OK\n");
    EM_ASM(window.close());
  }
}

void worker_main() // runs in Wasm Worker
{
  // Property 3: the worker must have received kWorkerCanvasId via the message listener.
  assert(offscreen_canvas_is_valid(kWorkerCanvasId));

  // kPthreadCanvasId was posted to the pthread, not this worker - must not be present here.
  assert(!offscreen_canvas_is_valid(kPthreadCanvasId));

  emscripten_wasm_worker_post_function_v(EMSCRIPTEN_WASM_WORKER_ID_PARENT, done);
}

void actual_thread_main(void *arg) // runs in pthread
{
  // Property 4: the pthread must have received kPthreadCanvasId via the message listener.
  assert(offscreen_canvas_is_valid(kPthreadCanvasId));

  // kWorkerCanvasId was posted to the worker, not this pthread - must not be present here.
  assert(!offscreen_canvas_is_valid(kWorkerCanvasId));

  // window is not accessible in a Worker context, so dispatch done() to the main thread.
  emscripten_dispatch_to_thread_async(emscripten_main_runtime_thread_id(), EM_FUNC_SIG_V, done, NULL);
}

void *dummy_thread_main(void *arg) // runs in pthread
{
  // Keep the pthread alive to receive the proxied actual_thread_main() call.
  emscripten_exit_with_live_runtime();
  return 0;
}

int main()
{
  emscripten_wasm_worker_t worker = emscripten_malloc_wasm_worker(1024);

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create(&thread, &attr, dummy_thread_main, 0);

  // Use the HTML canvas for the worker canvas and a standalone OffscreenCanvas for the pthread canvas.
  canvas_transfer_control_to_offscreen("canvas", kWorkerCanvasId);
  offscreen_canvas_create(kPthreadCanvasId, 300, 150);

  // Property 1: both canvases are valid in the main thread before posting.
  assert(offscreen_canvas_is_valid(kWorkerCanvasId));
  assert(offscreen_canvas_is_valid(kPthreadCanvasId));

  offscreen_canvas_post_to_worker(kWorkerCanvasId, worker);
  offscreen_canvas_post_to_pthread(kPthreadCanvasId, thread);

  // Property 2: both canvases are no longer valid in the main thread after posting.
  assert(!offscreen_canvas_is_valid(kWorkerCanvasId));
  assert(!offscreen_canvas_is_valid(kPthreadCanvasId));

  // The canvas postMessages are queued before the function/proxy calls below, so each
  // thread's message listener will store its canvas before the verification function runs.
  emscripten_wasm_worker_post_function_v(worker, worker_main);

  thread_queue = em_proxying_queue_create();
  emscripten_proxy_async(thread_queue, thread, actual_thread_main, 0);
  // Emscripten bug: must call emscripten_exit_with_live_runtime() here, even though
  // we are building with -sEXIT_RUNTIME=0
  emscripten_exit_with_live_runtime();
}
