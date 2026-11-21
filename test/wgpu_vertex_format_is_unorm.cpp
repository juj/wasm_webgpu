// Verifies wgpu_vertex_format_is_unorm() for every WGPU_VERTEX_FORMAT value: it must return
// true for exactly the unsigned normalized formats, and false for everything else, in
// particular for the signed normalized (snorm) formats such as
// WGPU_VERTEX_FORMAT_SNORM10_10_10_2.

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

int main()
{
  const WGPU_VERTEX_FORMAT unormFormats[] = {
    WGPU_VERTEX_FORMAT_UNORM8,
    WGPU_VERTEX_FORMAT_UNORM8X2,
    WGPU_VERTEX_FORMAT_UNORM8X4,
    WGPU_VERTEX_FORMAT_UNORM16,
    WGPU_VERTEX_FORMAT_UNORM16X2,
    WGPU_VERTEX_FORMAT_UNORM16X4,
    WGPU_VERTEX_FORMAT_UNORM10_10_10_2,
    WGPU_VERTEX_FORMAT_UNORM8X4_BGRA,
  };
  const int numUnormFormats = sizeof(unormFormats) / sizeof(unormFormats[0]);

  // WGPU_VERTEX_FORMAT_SNORM10_10_10_2 is the last vertex format.
  for(WGPU_VERTEX_FORMAT format = WGPU_VERTEX_FORMAT_FIRST_VALUE; format <= WGPU_VERTEX_FORMAT_SNORM10_10_10_2; ++format)
  {
    WGPU_BOOL expected = WGPU_FALSE;
    for(int i = 0; i < numUnormFormats; ++i)
      if (unormFormats[i] == format)
        expected = WGPU_TRUE;

    WGPU_BOOL isUnorm = wgpu_vertex_format_is_unorm(format);
    if (!isUnorm != !expected)
      printf("wgpu_vertex_format_is_unorm(%s) returned %d, expected %d\n", wgpu_vertex_format_to_string(format), (int)isUnorm, (int)expected);
    assert(!isUnorm == !expected);
  }

  // Spot check the snorm formats explicitly.
  assert(!wgpu_vertex_format_is_unorm(WGPU_VERTEX_FORMAT_SNORM8));
  assert(!wgpu_vertex_format_is_unorm(WGPU_VERTEX_FORMAT_SNORM8X4));
  assert(!wgpu_vertex_format_is_unorm(WGPU_VERTEX_FORMAT_SNORM16X2));
  assert(!wgpu_vertex_format_is_unorm(WGPU_VERTEX_FORMAT_SNORM10_10_10_2));

  // Values that are not vertex formats are not unorm either.
  assert(!wgpu_vertex_format_is_unorm(WGPU_VERTEX_FORMAT_INVALID));

  EM_ASM(window.close());
}
