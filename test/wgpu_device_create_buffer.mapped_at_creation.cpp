// Tests wgpu_device_create_buffer() with mappedAtCreation = true,
// exercising the '!!HEAPU32[descriptor+3]' code path in wgpu_device_create_buffer.
// The buffer is immediately available for mapping without a mapAsync call.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  const uint32_t kValue = 0x12345678u;

  WGpuBufferDescriptor desc = {
    .size = 256,
    .usage = WGPU_BUFFER_USAGE_COPY_SRC | WGPU_BUFFER_USAGE_MAP_WRITE,
    .mappedAtCreation = WGPU_TRUE, // exercises the !!HEAPU32[descriptor+3] == 1 path
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &desc);
  assert(buffer);
  assert(wgpu_is_buffer(buffer));

  // Buffer is already mapped; map state should be MAPPED
  assert(wgpu_buffer_map_state(buffer) == WGPU_BUFFER_MAP_STATE_MAPPED);

  // Write data immediately (no async map needed)
  wgpu_buffer_get_mapped_range(buffer, 0);
  wgpu_buffer_write_mapped_range(buffer, 0, 0, &kValue, sizeof(kValue));
  wgpu_buffer_unmap(buffer);

  assert(wgpu_buffer_map_state(buffer) == WGPU_BUFFER_MAP_STATE_UNMAPPED);

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
