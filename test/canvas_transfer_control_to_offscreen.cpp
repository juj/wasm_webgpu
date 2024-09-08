#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  canvas_transfer_control_to_offscreen("canvas", 42);
  assert(EM_ASM_INT(return Object.keys(wgpuOffscreenCanvases).length) == 1);
  assert(EM_ASM_INT(return wgpuOffscreenCanvases[42] instanceof OffscreenCanvas));

  EM_ASM(window.close());
}
