// Verifies that wgpu_offscreen_canvas_get_webgpu_context() works in a program that has not
// requested a WebGPU adapter or device (yet). This is a regression test for a missing JS
// library __deps declaration: the function registers the context to the object table with
// wgpuStore(), so it must declare a dependency to it. In other tests that dependency is pulled
// in by the adapter and device request functions. Emscripten versions that do not infer JS
// library dependencies automatically (such as 6.0.13) otherwise leave wgpuStore() out of
// the build, and the call throws "ReferenceError: wgpuStore is not defined".
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  offscreen_canvas_create(1, 64, 64);

  WGpuCanvasContext ctx = wgpu_offscreen_canvas_get_webgpu_context(1);
  assert(ctx);
  assert(wgpu_is_canvas_context(ctx));

  // Asking again returns the same context handle.
  assert(wgpu_offscreen_canvas_get_webgpu_context(1) == ctx);

  offscreen_canvas_destroy(1);

  EM_ASM(window.close());
}
