// Tests wgpu_buffer_map_async() with an explicit non-zero offset and size,
// exercising the 'size < 0 ? void 0 : size' code path with size >= 0.
// Also exercises wgpu_buffer_get_mapped_range with explicit size.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

static const uint32_t kData[4] = { 0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u };
static const double_int53_t kMapOffset = 8;   // map starting at byte 8
static const double_int53_t kMapSize   = 8;   // map 8 bytes (2 uint32s)

void BufferMapped(WGpuBuffer buffer, void *userData, WGPU_MAP_MODE_FLAGS mode, double_int53_t offset, double_int53_t size)
{
  assert(buffer);
  assert(mode == WGPU_MAP_MODE_READ);
  assert(offset == kMapOffset);
  assert(size == kMapSize);

  // Only the mapped subrange (bytes 8..15) should be accessible
  uint32_t readback[2] = {};
  wgpu_buffer_get_mapped_range(buffer, kMapOffset, kMapSize); // explicit size path
  wgpu_buffer_read_mapped_range(buffer, kMapOffset, 0, readback, sizeof(readback));
  wgpu_buffer_unmap(buffer);

  // GPUBuffer.mapAsync() does not work in Firefox, but reads back 0. https://bugzilla.mozilla.org/show_bug.cgi?id=2023418
  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    // kData[2] is at byte 8, kData[3] at byte 12
    assert(readback[0] == kData[2]);
    assert(readback[1] == kData[3]);
  }

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuBufferDescriptor desc = {
    .size = sizeof(kData),
    .usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &desc);
  assert(buffer);

  wgpu_queue_write_buffer(wgpu_device_get_queue(device), buffer, 0, kData, sizeof(kData));

  // Map only the subrange [8, 16) - exercises the non-void size path
  wgpu_buffer_map_async(buffer, BufferMapped, 0, WGPU_MAP_MODE_READ, kMapOffset, kMapSize);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
