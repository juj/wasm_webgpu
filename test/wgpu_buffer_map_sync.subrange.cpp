// Verifies that mapping only a subrange of the full buffer in a call to wgpu_buffer_map_sync() works.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  // Create a 1 KiB buffer (must be multiple of 256 for mapping offset alignment)
  WGpuBufferDescriptor desc = {
    .size = 1024,
    .usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST,
    .mappedAtCreation = WGPU_TRUE,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &desc);
  assert(buffer);

  // GPUBuffer.mapAsync() does not work in Firefox, but reads back 0. https://bugzilla.mozilla.org/show_bug.cgi?id=2023418
  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    // Write known data into the second 256-byte block (offset=256, size=256)
    const uint64_t sentinel = 0x1122334455667788ULL;
    wgpu_buffer_get_mapped_range(buffer, 0);
    // Write sentinel at offset 256
    wgpu_buffer_write_mapped_range(buffer, 0, 256, &sentinel, sizeof(sentinel));
    wgpu_buffer_unmap(buffer);

    // Map only a sub-range: offset=256, size=256
    wgpu_buffer_map_sync(buffer, WGPU_MAP_MODE_READ, 256, 256);
    assert(wgpu_buffer_map_state(buffer) == WGPU_BUFFER_MAP_STATE_MAPPED);

    uint64_t readback = 0;
    // getMappedRange at startOffset=256 (same offset as used in map call)
    wgpu_buffer_get_mapped_range(buffer, 256, 256);
    wgpu_buffer_read_mapped_range(buffer, 256, 0, &readback, sizeof(readback));
    printf("Subrange readback: 0x%llx (expected 0x%llx)\n", readback, sentinel);
    assert(readback == sentinel);
    wgpu_buffer_unmap(buffer);
  }
  printf("Test OK\n");
  EM_ASM(window.close());
}
