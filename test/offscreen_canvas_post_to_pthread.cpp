// Verifies that offscreen_canvas_post_to_pthread() transfers OffscreenCanvas ownership to a
// pthread. Three properties are checked:
//   1. The canvas is valid in the main thread before posting.
//   2. The canvas is no longer valid in the main thread after posting (ownership transferred).
//   3. The canvas is valid in the pthread after it has been received.
// flags: -sEXIT_RUNTIME=0 -pthread -sPTHREAD_POOL_SIZE=1

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>
#include <emscripten/proxying.h>
#include <emscripten/threading.h>

static const OffscreenCanvasId kCanvasId = 42;
static pthread_t thread;
static em_proxying_queue *thread_queue;

void done() // runs in main thread, dispatched back from the pthread
{
  printf("Test OK\n");
  EM_ASM(window.close());
}

void actual_thread_main(void *arg) // runs in pthread
{
  // Property 3: canvas must be valid in the pthread after being received.
  assert(offscreen_canvas_is_valid(kCanvasId));

  // window is not available in a Worker context, so dispatch done() to the main thread.
  emscripten_dispatch_to_thread_async(emscripten_main_runtime_thread_id(), EM_FUNC_SIG_V, done, NULL);
}

void *dummy_thread_main(void *arg) // runs in pthread
{
  // Keep the pthread alive so it can receive proxied calls.
  emscripten_exit_with_live_runtime();
  return 0;
}

int main()
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create(&thread, &attr, dummy_thread_main, 0);

  canvas_transfer_control_to_offscreen("canvas", kCanvasId);

  // Property 1: canvas must be valid in the main thread before posting.
  assert(offscreen_canvas_is_valid(kCanvasId));

  offscreen_canvas_post_to_pthread(kCanvasId, thread);

  // Property 2: canvas must no longer be valid in the main thread after posting.
  assert(!offscreen_canvas_is_valid(kCanvasId));

  // The canvas postMessage (above) is queued before this proxy, so the
  // pthread's message listener will store the canvas before actual_thread_main() runs.
  thread_queue = em_proxying_queue_create();
  emscripten_proxy_async(thread_queue, thread, actual_thread_main, 0);
}
