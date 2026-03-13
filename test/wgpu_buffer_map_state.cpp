// Verifies that wgpu_buffer_map_state() transitions correctly between UNMAPPED and MAPPED states as wgpu_buffer_map_sync() and wgpu_buffer_unmap() are called.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  WGpuBufferDescriptor desc = {
    .size = 256,
    .usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST,
    .mappedAtCreation = WGPU_FALSE,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &desc);
  assert(buffer);

  // Buffer starts unmapped
  assert(wgpu_buffer_map_state(buffer) == WGPU_BUFFER_MAP_STATE_UNMAPPED);

  // Map the buffer and check mapped state
  wgpu_buffer_map_sync(buffer, WGPU_MAP_MODE_READ);
  assert(wgpu_buffer_map_state(buffer) == WGPU_BUFFER_MAP_STATE_MAPPED);

  // Unmap and check unmapped state again
  wgpu_buffer_unmap(buffer);
  assert(wgpu_buffer_map_state(buffer) == WGPU_BUFFER_MAP_STATE_UNMAPPED);

  EM_ASM(window.close());
}
