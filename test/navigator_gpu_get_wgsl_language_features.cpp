#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main()
{
  const char * const * features = navigator_gpu_get_wgsl_language_features();
  assert(features);
  int num_supported_features = 0;
  int supports_pointer_composite_access = 0;
  while(*features)
  {
    ++num_supported_features;
    assert(strlen(*features) > 0);
    printf("Feature %d: %s\n", num_supported_features, *features);
    supports_pointer_composite_access |= !strcmp(*features, "pointer_composite_access");
    ++features;
  }
  assert(num_supported_features > 0); // Surely at least one feature is supported by each implementation?
  assert(supports_pointer_composite_access); // This test checks against one feature that Chrome supports. Your impl. might support different features.

  EM_ASM(window.close());
}
