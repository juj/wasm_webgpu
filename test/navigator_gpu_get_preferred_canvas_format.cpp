#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  WGPU_TEXTURE_FORMAT fmt = navigator_gpu_get_preferred_canvas_format();
  assert(fmt != WGPU_TEXTURE_FORMAT_INVALID);
  assert(fmt > 0);
  assert(fmt < WGPU_TEXTURE_FORMAT_LAST_VALUE);

  // N.b. this is what Chrome just happens to report for me on Windows, for now keep using it.
  // There probably exists only very few reasonable preferred formats, so if this test fails
  // and your target implementation has another preferred format, probably good way to
  // build here a whitelist of tested formats.
  assert(fmt == WGPU_TEXTURE_FORMAT_BGRA8UNORM);

  EM_ASM(window.close());
}
