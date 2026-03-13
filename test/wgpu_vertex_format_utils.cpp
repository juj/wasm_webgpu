// Verifies the utility functions wgpu_vertex_format_channel_count(), wgpu_vertex_format_byte_size(), wgpu_vertex_format_is_unorm(), wgpu_vertex_format_to_string(), and wgpu_vertex_format_wgsl_element_type() against expected values for several common vertex formats.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

int main()
{
  // wgpu_vertex_format_channel_count
  assert(wgpu_vertex_format_channel_count(WGPU_VERTEX_FORMAT_FLOAT32)   == 1);
  assert(wgpu_vertex_format_channel_count(WGPU_VERTEX_FORMAT_FLOAT32X2) == 2);
  assert(wgpu_vertex_format_channel_count(WGPU_VERTEX_FORMAT_FLOAT32X3) == 3);
  assert(wgpu_vertex_format_channel_count(WGPU_VERTEX_FORMAT_FLOAT32X4) == 4);
  assert(wgpu_vertex_format_channel_count(WGPU_VERTEX_FORMAT_UNORM8X4)  == 4);

  // wgpu_vertex_format_byte_size
  assert(wgpu_vertex_format_byte_size(WGPU_VERTEX_FORMAT_FLOAT32)   == 4);
  assert(wgpu_vertex_format_byte_size(WGPU_VERTEX_FORMAT_FLOAT32X2) == 8);
  assert(wgpu_vertex_format_byte_size(WGPU_VERTEX_FORMAT_FLOAT32X3) == 12);
  assert(wgpu_vertex_format_byte_size(WGPU_VERTEX_FORMAT_FLOAT32X4) == 16);
  assert(wgpu_vertex_format_byte_size(WGPU_VERTEX_FORMAT_UNORM8X4)  == 4);

  // wgpu_vertex_format_is_unorm
  assert( wgpu_vertex_format_is_unorm(WGPU_VERTEX_FORMAT_UNORM8X4));
  assert( wgpu_vertex_format_is_unorm(WGPU_VERTEX_FORMAT_UNORM8X2));
  assert(!wgpu_vertex_format_is_unorm(WGPU_VERTEX_FORMAT_FLOAT32X4));
  assert(!wgpu_vertex_format_is_unorm(WGPU_VERTEX_FORMAT_FLOAT32));

  // wgpu_vertex_format_to_string
  const char *s = wgpu_vertex_format_to_string(WGPU_VERTEX_FORMAT_FLOAT32X2);
  assert(s && strlen(s) > 0);
  const char *s2 = wgpu_vertex_format_to_string(WGPU_VERTEX_FORMAT_UNORM8X4);
  assert(s2 && strlen(s2) > 0);
  assert(strcmp(s, s2) != 0);

  // wgpu_vertex_format_wgsl_element_type
  assert(wgpu_vertex_format_wgsl_element_type(WGPU_VERTEX_FORMAT_FLOAT32X2) == WGPU_WGSL_ELEMENT_TYPE_FLOAT);
  assert(wgpu_vertex_format_wgsl_element_type(WGPU_VERTEX_FORMAT_UNORM8X4)  == WGPU_WGSL_ELEMENT_TYPE_FLOAT);
  assert(wgpu_vertex_format_wgsl_element_type(WGPU_VERTEX_FORMAT_UINT32)    == WGPU_WGSL_ELEMENT_TYPE_UINT);

  EM_ASM(window.close());
}
