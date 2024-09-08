#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  offscreen_canvas_create(42, 300, 100);
  offscreen_canvas_set_size(42, 600, 200);

  assert(EM_ASM_INT({return wgpuOffscreenCanvases[42].width}) == 600);
  assert(EM_ASM_INT({return wgpuOffscreenCanvases[42].height}) == 200);

  EM_ASM(window.close());
}
