#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  offscreen_canvas_create(42, 300, 100);
  int width, height;
  offscreen_canvas_size(42, &width, &height);
  assert(width == 300);
  assert(height == 100);

  EM_ASM(window.close());
}
