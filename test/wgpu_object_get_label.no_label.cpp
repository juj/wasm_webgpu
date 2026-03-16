// Verifies that wgpu_object_get_label() behaves correctly when the object has no label
// set: the destination buffer should receive an empty string (null terminator only) and
// the return value should be 0. This exercises the stringToUTF8('',...) path in the JS
// implementation, where wgpu[o]['label'] is an empty string (WebGPU default).
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a buffer with no label set — its label defaults to empty string.
  WGpuBufferDescriptor bdesc = {
    .size = 64,
    .usage = WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &bdesc);
  assert(buffer);

  // Fill buffer with non-zero sentinel to verify the terminator is written.
  char dst[32];
  memset(dst, 0xFF, sizeof(dst));

  int n = wgpu_object_get_label(buffer, dst, sizeof(dst));

  // No label was set: return value must be 0 and buffer must contain empty string.
  assert(n == 0);
  assert(dst[0] == '\0');

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
