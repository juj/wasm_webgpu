// Verifies that wgpu_command_encoder_clear_buffer() zeroes the contents of a buffer that was previously written with non-zero data.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  // Create a buffer with initial data
  const uint64_t initData = 0xDEADBEEFCAFEBABEULL;
  WGpuBufferDescriptor desc = {
    .size = sizeof(initData),
    .usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_MAP_READ,
    .mappedAtCreation = WGPU_TRUE,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &desc);
  wgpu_buffer_get_mapped_range(buffer, 0);
  wgpu_buffer_write_mapped_range(buffer, 0, 0, &initData, sizeof(initData));
  wgpu_buffer_unmap(buffer);

  // Clear the buffer
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  wgpu_command_encoder_clear_buffer(encoder, buffer, 0, sizeof(initData));
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(encoder));

  char msg[512];
  WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
  if (strlen(msg) > 0) printf("%s\n", msg);
  assert(!error);

  // Read back and verify zeroed
  uint64_t readback = 0xFFFFFFFFFFFFFFFFULL;
  wgpu_buffer_map_sync(buffer, WGPU_MAP_MODE_READ);
  wgpu_buffer_get_mapped_range(buffer, 0);
  wgpu_buffer_read_mapped_range(buffer, 0, 0, &readback, sizeof(readback));
  printf("After clear: 0x%llx (expected 0)\n", readback);
  assert(readback == 0);

  printf("Test OK\n");
  EM_ASM(window.close());
}
