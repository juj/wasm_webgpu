// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  char *str = (char*)malloc(1048576); // Use a huge long string to increase chances that buffer overrun result is catastrophic in this test
  for(int i = 0; i < 1048576; ++i)
    str[i] = '0' + (i%10);
  str[1048575] = 0;

  wgpu_object_set_label(adapter, str);

  char label[16];
  wgpu_object_get_label(adapter, label, sizeof(label)); // Test get_label() call that truncates
  assert(strlen(label) == 15);
  assert(!strcmp(label, "012345678901234"));

  char *str2 = (char*)malloc(1048576);
  wgpu_object_get_label(adapter, str2, 1048576); // Test get_label() call that does not truncate
  assert(strlen(str2) == 1048575);
  assert(!strcmp(str, str2));

  free(str);
  free(str2);

  EM_ASM(window.close());
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
