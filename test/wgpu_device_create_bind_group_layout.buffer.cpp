// Verifies that a WGpuBindGroupLayout with a uniform buffer entry can be created, and that a bind group referencing that layout and a uniform buffer is valid.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a BGL with one uniform buffer binding at slot 0
  WGpuBindGroupLayoutEntry entry = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_VERTEX | WGPU_SHADER_STAGE_FRAGMENT,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER,
    .layout.buffer = {
      .type = WGPU_BUFFER_BINDING_TYPE_UNIFORM,
      .hasDynamicOffset = WGPU_FALSE,
      .minBindingSize = 0,
    },
  };

  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &entry, 1);
  assert(bgl);
  assert(wgpu_is_bind_group_layout(bgl));

  // Create a uniform buffer and bind it
  WGpuBufferDescriptor bdesc = {
    .size = 256,
    .usage = WGPU_BUFFER_USAGE_UNIFORM | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer ubuf = wgpu_device_create_buffer(device, &bdesc);
  assert(ubuf);

  WGpuBindGroupEntry bgentry = {
    .binding = 0,
    .resource = ubuf,
    .bufferBindOffset = 0,
    .bufferBindSize = 0, // 0 = bind whole buffer
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
