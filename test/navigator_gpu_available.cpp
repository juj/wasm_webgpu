#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  assert(navigator_gpu_available());
  navigator_delete_webgpu_api_access();
  assert(!navigator_gpu_available());

  EM_ASM(window.close());
}
