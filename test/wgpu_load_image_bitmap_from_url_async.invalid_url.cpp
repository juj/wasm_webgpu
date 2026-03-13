// Verifies that wgpu_load_image_bitmap_from_url_async() calls the callback with bitmap==0
// and width==height==0 when the URL cannot be loaded.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ImageLoaded(WGpuImageBitmap bitmap, int width, int height, void *userData)
{
  assert(userData == (void*)99);

  // A failed load must be signalled with zero bitmap and zero dimensions.
  assert(!bitmap);
  assert(width == 0);
  assert(height == 0);

  printf("ImageLoaded (failure case): bitmap=%d width=%d height=%d\n", (int)bitmap, width, height);
  printf("Test OK\n");
  EM_ASM(window.close());
}

int main()
{
  wgpu_load_image_bitmap_from_url_async("this-image-does-not-exist.png", WGPU_FALSE, ImageLoaded, (void*)99);
}
