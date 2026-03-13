// Verifies that wgpu_compilation_message_type_to_string() returns distinct, non-empty strings for each of the three WGPU_COMPILATION_MESSAGE_TYPE values.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

int main()
{
  // Test that the function returns non-null, non-empty strings for each type
  const char *errorStr = wgpu_compilation_message_type_to_string(WGPU_COMPILATION_MESSAGE_TYPE_ERROR);
  assert(errorStr && strlen(errorStr) > 0);

  const char *warningStr = wgpu_compilation_message_type_to_string(WGPU_COMPILATION_MESSAGE_TYPE_WARNING);
  assert(warningStr && strlen(warningStr) > 0);

  const char *infoStr = wgpu_compilation_message_type_to_string(WGPU_COMPILATION_MESSAGE_TYPE_INFO);
  assert(infoStr && strlen(infoStr) > 0);

  // Verify they are distinct strings
  assert(strcmp(errorStr, warningStr) != 0);
  assert(strcmp(errorStr, infoStr) != 0);
  assert(strcmp(warningStr, infoStr) != 0);

  EM_ASM(window.close());
}
