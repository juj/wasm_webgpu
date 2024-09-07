#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  assert(!wgpu_is_valid_object(0));
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  assert(wgpu_is_valid_object(ctx));
  wgpu_object_destroy(ctx);
  assert(!wgpu_is_valid_object(ctx));

  WGpuCanvasContext ctx2 = wgpu_canvas_get_webgpu_context("canvas");
  assert(wgpu_is_valid_object(ctx2));
  assert(!wgpu_is_valid_object(ctx));

  EM_ASM(window.close());
}
