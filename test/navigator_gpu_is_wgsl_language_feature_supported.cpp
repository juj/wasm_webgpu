#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

int main()
{
  // Test that each advertised feature is reported as supported.
  const char * const * features = navigator_gpu_get_wgsl_language_features();
  char str[64];
  while(*features)
  {
    assert(navigator_gpu_is_wgsl_language_feature_supported(*features));
    strcpy(str, *features);
    ++features;
  }

  // Test that dummy feature name comes out as unsupported.
  assert(!navigator_gpu_is_wgsl_language_feature_supported("dummy_feature_that_does_not_exist"));

  // Test that concatenating garbage after a feature name does not find it supported.
  assert(navigator_gpu_is_wgsl_language_feature_supported(str));
  int len = strlen(str);
  strcat(str, "foo");
  assert(!navigator_gpu_is_wgsl_language_feature_supported(str));

  // Test that cutting off last char of a feature name does not find it supported.
  str[len-1] = 0;
  assert(!navigator_gpu_is_wgsl_language_feature_supported(str));

  EM_ASM(window.close());
}
