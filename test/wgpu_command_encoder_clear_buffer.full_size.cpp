// Tests wgpu_command_encoder_clear_buffer() with size == -1 (WGPU_MAX_SIZE),
// exercising the 'size < 0 ? void 0 : size' code path where void 0 means
// clear the whole buffer from the given offset.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  const uint32_t bufSize = 256;

  WGpuBufferDescriptor bufDesc = {};
  bufDesc.size = bufSize;
  bufDesc.usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_MAP_READ;
  WGpuBuffer buf = wgpu_device_create_buffer(device, &bufDesc);
  assert(buf);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  // Clear with WGPU_MAX_SIZE (-1): clears from offset 0 to end of buffer.
  wgpu_command_encoder_clear_buffer(enc, buf, 0, WGPU_MAX_SIZE);

  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));

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
