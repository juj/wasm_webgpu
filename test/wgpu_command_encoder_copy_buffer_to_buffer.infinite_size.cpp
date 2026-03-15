// Tests wgpu_command_encoder_copy_buffer_to_buffer() with WGPU_INFINITY as the
// size parameter, exercising the 'size < 1/0 ? size : void 0' code path
// where void 0 (undefined) means copy the whole buffer.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  const uint32_t bufSize = 256;

  WGpuBufferDescriptor srcDesc = {};
  srcDesc.size = bufSize;
  srcDesc.usage = WGPU_BUFFER_USAGE_COPY_SRC | WGPU_BUFFER_USAGE_COPY_DST;
  WGpuBuffer src = wgpu_device_create_buffer(device, &srcDesc);
  assert(src);

  WGpuBufferDescriptor dstDesc = {};
  dstDesc.size = bufSize;
  dstDesc.usage = WGPU_BUFFER_USAGE_COPY_DST;
  WGpuBuffer dst = wgpu_device_create_buffer(device, &dstDesc);
  assert(dst);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  // Use WGPU_INFINITY to copy the whole buffer without specifying size
  wgpu_command_encoder_copy_buffer_to_buffer(enc, src, 0, dst, 0, WGPU_INFINITY);

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
