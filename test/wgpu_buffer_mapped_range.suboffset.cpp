// Verifies that wgpu_buffer_write_mapped_range() and wgpu_buffer_read_mapped_range() correctly write and read values at multiple sub-offsets within a single mapped range.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  // Create a 256-byte buffer, mapped at creation for writing
  WGpuBufferDescriptor desc = {
    .size = 256,
    .usage = WGPU_BUFFER_USAGE_MAP_WRITE | WGPU_BUFFER_USAGE_COPY_SRC,
    .mappedAtCreation = WGPU_TRUE,
  };
  WGpuBuffer wbuf = wgpu_device_create_buffer(device, &desc);
  assert(wbuf);

  // Map the full range starting at offset 0
  wgpu_buffer_get_mapped_range(wbuf, 0);

  // Write values at different sub-offsets within the mapped range
  const uint32_t val0 = 0xAAAAAAAAu;
  const uint32_t val1 = 0xBBBBBBBBu;
  const uint32_t val2 = 0xCCCCCCCCu;
  wgpu_buffer_write_mapped_range(wbuf, 0,  0, &val0, sizeof(val0));
  wgpu_buffer_write_mapped_range(wbuf, 0,  4, &val1, sizeof(val1));
  wgpu_buffer_write_mapped_range(wbuf, 0, 16, &val2, sizeof(val2));
  wgpu_buffer_unmap(wbuf);

  // Copy to a readable buffer via command encoder
  WGpuBufferDescriptor rdesc = {
    .size = 256,
    .usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer rbuf = wgpu_device_create_buffer(device, &rdesc);
  assert(rbuf);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder_simple(device);
  wgpu_command_encoder_copy_buffer_to_buffer(enc, wbuf, 0, rbuf, 0, 256);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));

  // Read back and verify sub-offsets
  wgpu_buffer_map_sync(rbuf, WGPU_MAP_MODE_READ);
  wgpu_buffer_get_mapped_range(rbuf, 0);

  uint32_t r0 = 0, r1 = 0, r2 = 0;
  wgpu_buffer_read_mapped_range(rbuf, 0,  0, &r0, sizeof(r0));
  wgpu_buffer_read_mapped_range(rbuf, 0,  4, &r1, sizeof(r1));
  wgpu_buffer_read_mapped_range(rbuf, 0, 16, &r2, sizeof(r2));

  printf("r0=0x%x r1=0x%x r2=0x%x\n", r0, r1, r2);
  assert(r0 == val0);
  assert(r1 == val1);
  assert(r2 == val2);
  wgpu_buffer_unmap(rbuf);

  printf("Test OK\n");
  EM_ASM(window.close());
}
