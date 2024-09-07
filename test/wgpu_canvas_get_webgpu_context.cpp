#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  assert(wgpu_is_valid_object(ctx));
  assert(wgpu_is_canvas_context(ctx));

  EM_ASM(window.close());
}
