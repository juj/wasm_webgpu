// Tests wgpu_device_create_bind_group() with bufferBindSize == 0, which should
// bind the whole buffer. This exercises the
// 'size: wgpuReadI53FromU64HeapIdx(entries+4) || void 0' code path
// where size==0 is converted to undefined (bind whole buffer).
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a uniform buffer BGL
  WGpuBindGroupLayoutEntry bglEntry = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_VERTEX,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER,
    .layout.buffer = {
      .type = WGPU_BUFFER_BINDING_TYPE_UNIFORM,
      .hasDynamicOffset = WGPU_FALSE,
      .minBindingSize = 0,
    },
  };
  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &bglEntry, 1);
  assert(bgl);

  // Create a uniform buffer
  WGpuBufferDescriptor bufDesc = {};
  bufDesc.size = 256;
  bufDesc.usage = WGPU_BUFFER_USAGE_UNIFORM | WGPU_BUFFER_USAGE_COPY_DST;
  WGpuBuffer buf = wgpu_device_create_buffer(device, &bufDesc);
  assert(buf);

  // Create bind group with bufferBindSize == 0 (bind whole buffer)
  WGpuBindGroupEntry entry = {
    .binding = 0,
    .resource = buf,
    .bufferBindOffset = 0,
    .bufferBindSize = 0, // 0 means bind whole buffer, converted to undefined in JS
  };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, &entry, 1);
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
