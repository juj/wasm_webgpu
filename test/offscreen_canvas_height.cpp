#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  offscreen_canvas_create(42, 300, 100);
  assert(offscreen_canvas_height(42) == 100);

  EM_ASM(window.close());
}
