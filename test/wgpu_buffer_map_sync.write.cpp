// Verifies that wgpu_buffer_map_sync() with WGPU_MAP_MODE_WRITE allows writing a known data pattern into the buffer and correctly transitions map state.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  // Create a buffer mappable for write
  WGpuBufferDescriptor desc = {
    .size = 256,
    .usage = WGPU_BUFFER_USAGE_MAP_WRITE | WGPU_BUFFER_USAGE_COPY_SRC,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &desc);
  assert(buffer);

  // Map sync for WRITE
  assert(wgpu_buffer_map_state(buffer) == WGPU_BUFFER_MAP_STATE_UNMAPPED);
  wgpu_buffer_map_sync(buffer, WGPU_MAP_MODE_WRITE);
  assert(wgpu_buffer_map_state(buffer) == WGPU_BUFFER_MAP_STATE_MAPPED);

  // Write a known pattern
  const uint64_t pattern = 0xCAFEBABEDEADBEEFULL;
  wgpu_buffer_get_mapped_range(buffer, 0);
  wgpu_buffer_write_mapped_range(buffer, 0, 0, &pattern, sizeof(pattern));
  wgpu_buffer_unmap(buffer);

  assert(wgpu_buffer_map_state(buffer) == WGPU_BUFFER_MAP_STATE_UNMAPPED);

  printf("Test OK\n");
  EM_ASM(window.close());
}
