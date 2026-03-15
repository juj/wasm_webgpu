// Tests wgpu_device_create_bind_group() with a non-zero bufferBindOffset,
// exercising the non-zero offset path in the bind group entry.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a uniform buffer large enough to bind at an offset
  WGpuBufferDescriptor bdesc = {
    .size = 512, // 512 bytes, so we can bind at offset 256
    .usage = WGPU_BUFFER_USAGE_UNIFORM | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &bdesc);
  assert(buffer);

  WGpuBindGroupLayoutEntry layoutEntry = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_VERTEX,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER,
    .layout.buffer = {
      .type = WGPU_BUFFER_BINDING_TYPE_UNIFORM,
      .hasDynamicOffset = WGPU_FALSE,
      .minBindingSize = 0,
    },
  };
  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &layoutEntry, 1);
  assert(bgl);

  // Bind buffer starting at offset 256 (non-zero bufferBindOffset)
  WGpuBindGroupEntry bgentry = {
    .binding = 0,
    .resource = buffer,
    .bufferBindOffset = 256, // non-zero offset exercises the offset path
    .bufferBindSize = 256,   // bind 256 bytes from that offset
  };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, &bgentry, 1);
  assert(bg);
  assert(wgpu_is_bind_group(bg));

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
