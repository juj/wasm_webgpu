#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  assert(!offscreen_canvas_is_valid(0));
  assert(!offscreen_canvas_is_valid(42));
  canvas_transfer_control_to_offscreen("canvas", 42);
  assert(offscreen_canvas_is_valid(42));

  EM_ASM(window.close());
}
