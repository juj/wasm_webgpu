// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

int num_calls = 0;

double previous_time;
WGPU_BOOL raf(double time, void *userData)
{
  assert(time > previous_time);
  previous_time = time;

  assert(userData == (void*)42);
  if (++num_calls > 100)
  {
    EM_ASM(window.close());
    return WGPU_FALSE;
  }
  return WGPU_TRUE;
}

int main()
{
  wgpu_request_animation_frame_loop(raf, (void*)42);
}
