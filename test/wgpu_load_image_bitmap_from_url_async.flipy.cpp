// Verifies that wgpu_load_image_bitmap_from_url_async() succeeds with flipY=WGPU_TRUE, exercising
// the imageOrientation:'flipY' path in createImageBitmap().
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

// Minimal 1x1 pixel PNG encoded as a data URL, so no network or filesystem dependency.
static const char *kTestImageUrl =
  "data:image/png;base64,"
  "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJ"
  "AAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";

void ImageLoaded(WGpuImageBitmap bitmap, int width, int height, void *userData)
{
  assert(userData == (void*)7);

  // flipY must not prevent a successful load.
  assert(bitmap);
  assert(wgpu_is_valid_object(bitmap));
  assert(width > 0);
  assert(height > 0);

  printf("ImageLoaded (flipY): bitmap=%d width=%d height=%d\n", (int)bitmap, width, height);
  printf("Test OK\n");
  EM_ASM(window.close());
}

int main()
{
  wgpu_load_image_bitmap_from_url_async(kTestImageUrl, WGPU_TRUE, ImageLoaded, (void*)7);
}
