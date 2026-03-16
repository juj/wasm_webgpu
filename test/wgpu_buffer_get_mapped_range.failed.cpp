// Verifies that wgpu_buffer_get_mapped_range() returns WGPU_BUFFER_GET_MAPPED_RANGE_FAILED
// (-1) when called on a buffer that is not currently in the mapped state. This exercises
// the try-catch path in the JS implementation where getMappedRange() throws a DOMException
// (because the buffer is unmapped), and the catch block returns -1.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a mappable buffer but do NOT map it — it starts in the 'unmapped' state.
  WGpuBufferDescriptor bdesc = {
    .size = 64,
    .usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &bdesc);
  assert(buffer);

  // Calling getMappedRange on an unmapped buffer throws in JS, which the
  // library catches and converts to the sentinel return value -1.
  double_int53_t result = wgpu_buffer_get_mapped_range(buffer, 0, 64);
  assert(result == WGPU_BUFFER_GET_MAPPED_RANGE_FAILED);

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
