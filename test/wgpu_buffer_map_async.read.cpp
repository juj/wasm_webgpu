// Verifies that wgpu_buffer_map_async() with WGPU_MAP_MODE_READ calls the
// callback and allows reading data from the mapped buffer range.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

static WGpuDevice gDevice = 0;
static WGpuBuffer gBuffer = 0;
static const uint32_t kExpectedValue = 0xDEADBEEFu;

void BufferMapped(WGpuBuffer buffer, void *userData, WGPU_MAP_MODE_FLAGS mode, double_int53_t offset, double_int53_t size)
{
  assert(buffer);
  assert(mode == WGPU_MAP_MODE_READ);
  assert(offset == 0);

  // Read back the data
  uint32_t readData = 0;
  wgpu_buffer_get_mapped_range(buffer, 0);
  wgpu_buffer_read_mapped_range(buffer, 0, 0, &readData, sizeof(readData));
  wgpu_buffer_unmap(buffer);

  // GPUBuffer.mapAsync() does not work in Firefox, but reads back 0. https://bugzilla.mozilla.org/show_bug.cgi?id=2023418
  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    assert(readData == kExpectedValue);
  }

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  gDevice = device;

  // Create a MAP_READ buffer
  WGpuBufferDescriptor desc = {
    .size = 256,
    .usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST,
  };
  gBuffer = wgpu_device_create_buffer(device, &desc);
  assert(gBuffer);

  // Write data into the buffer via the queue
  wgpu_queue_write_buffer(wgpu_device_get_queue(device), gBuffer, 0, &kExpectedValue, sizeof(kExpectedValue));

  // Map for read asynchronously
  wgpu_buffer_map_async(gBuffer, BufferMapped, 0, WGPU_MAP_MODE_READ, 0, -1);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
