#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  offscreen_canvas_destroy(0); // Destroying zero canvas ID is a no-op
  canvas_transfer_control_to_offscreen("canvas", 42);
  assert(offscreen_canvas_is_valid(42));
  offscreen_canvas_destroy(42);
  assert(!offscreen_canvas_is_valid(42)); // After deletion, OffscreenCanvas should no longer exist.

  offscreen_canvas_destroy(42); // duplicate deletion should be ok.

  EM_ASM(window.close());
}
