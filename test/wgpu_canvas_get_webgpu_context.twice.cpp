// This test verifies that calling wgpu_canvas_get_webgpu_context() on the
// same canvas multiple times will return the same WGpuCanvasContext object and
// won't create a new one.
#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  WGpuCanvasContext ctx2 = wgpu_canvas_get_webgpu_context("canvas");
  assert(ctx == ctx2); // Verify that getting context several times will return the same one.
  assert(wgpu_get_num_live_objects() == 1); // Only one canvas context object should be alive

  wgpu_object_destroy(ctx);
  WGpuCanvasContext ctx3 = wgpu_canvas_get_webgpu_context("canvas");
  assert(ctx3 != ctx); // Except if the context is destroyed in between and then reacquired,
                       // it should not result in the same context being returned.

  EM_ASM(window.close());
}
