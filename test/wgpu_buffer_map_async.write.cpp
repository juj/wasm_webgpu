// Verifies that wgpu_buffer_map_async() with WGPU_MAP_MODE_WRITE calls the callback and allows writing data into the mapped buffer range.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

static const uint32_t kWriteData = 0xABCD1234u;

void BufferMapped(WGpuBuffer buffer, void *userData, WGPU_MAP_MODE_FLAGS mode, double_int53_t offset, double_int53_t size)
{
  assert(buffer);
  assert(mode == WGPU_MAP_MODE_WRITE);
  assert(offset == 0);

  // Write data into the mapped buffer
  wgpu_buffer_get_mapped_range(buffer, 0);
  wgpu_buffer_write_mapped_range(buffer, 0, 0, &kWriteData, sizeof(kWriteData));
  wgpu_buffer_unmap(buffer);

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuBufferDescriptor desc = {
    .size = 256,
    .usage = WGPU_BUFFER_USAGE_MAP_WRITE | WGPU_BUFFER_USAGE_COPY_SRC,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &desc);
  assert(buffer);

  // Map for write asynchronously
  wgpu_buffer_map_async(buffer, BufferMapped, (void*)42, WGPU_MAP_MODE_WRITE);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
