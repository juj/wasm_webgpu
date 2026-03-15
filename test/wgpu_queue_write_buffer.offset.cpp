// Verifies that wgpu_queue_write_buffer() with a non-zero bufferOffset writes data only to the specified offset, leaving earlier bytes zeroed.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <emscripten/heap.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  // Create a buffer large enough for two 32-byte blocks
  WGpuBufferDescriptor desc = {
    .size = 512,
    .usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_MAP_READ,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &desc);
  assert(buffer);

  // Write a known pattern at bufferOffset=256 (one block in)
  // bufferOffset must be a multiple of 4
  uint32_t data[4] = { 0x11223344u, 0x55667788u, 0xAABBCCDDu, 0xEEFF0011u };

  wgpu_queue_write_buffer(wgpu_device_get_queue(device), buffer, 256, data, sizeof(data));

  char msg[512];
  WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
  if (strlen(msg) > 0) printf("%s\n", msg);
  assert(!error);

  // GPUBuffer.mapAsync() does not work in Firefox, but reads back 0. https://bugzilla.mozilla.org/show_bug.cgi?id=2023418
  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    // Read back and verify the written region
    wgpu_buffer_map_sync(buffer, WGPU_MAP_MODE_READ);
    wgpu_buffer_get_mapped_range(buffer, 0);

    uint32_t readback[4] = { 0 };
    wgpu_buffer_read_mapped_range(buffer, 0, 256, readback, sizeof(readback));

    printf("readback[0]=0x%x (expected 0x%x)\n", readback[0], data[0]);
    assert(readback[0] == data[0]);
    assert(readback[1] == data[1]);
    assert(readback[2] == data[2]);
    assert(readback[3] == data[3]);

    // Bytes before offset 256 should be zero (default GPU buffer init)
    uint32_t before[4] = { 0xDEADu };
    wgpu_buffer_read_mapped_range(buffer, 0, 0, before, sizeof(before));
    assert(before[0] == 0);
    assert(before[1] == 0);
    assert(before[2] == 0);
    assert(before[3] == 0);

    wgpu_buffer_unmap(buffer);
  }

  printf("Test OK\n");
  EM_ASM(window.close());
}
