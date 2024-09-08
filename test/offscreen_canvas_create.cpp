#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  offscreen_canvas_create(42, 100, 100);
  assert(EM_ASM_INT(return Object.keys(wgpuOffscreenCanvases).length) == 1);
  assert(EM_ASM_INT(return wgpuOffscreenCanvases[42] instanceof OffscreenCanvas));

  EM_ASM(window.close());
}
