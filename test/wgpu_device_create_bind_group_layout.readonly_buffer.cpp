// Tests creating a WGpuBindGroupLayout with a read-only storage buffer binding
// (WGPU_BUFFER_BINDING_TYPE_READ_ONLY_STORAGE, index 3), exercising that branch
// in $wgpuReadBindGroupLayoutDescriptor. Binds a real storage buffer to verify.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuBindGroupLayoutEntry entry = {
    .binding    = 0,
    .visibility = WGPU_SHADER_STAGE_COMPUTE,
    .type       = WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER,
    .layout.buffer = {
      .type             = WGPU_BUFFER_BINDING_TYPE_READ_ONLY_STORAGE, // index 3 — untested
      .hasDynamicOffset = WGPU_FALSE,
      .minBindingSize   = 0,
    },
  };

  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &entry, 1);
  assert(bgl);
  assert(wgpu_is_bind_group_layout(bgl));

  // Create a small buffer to bind as read-only storage
  WGpuBufferDescriptor bdesc = {
    .size  = 64,
    .usage = WGPU_BUFFER_USAGE_STORAGE | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer buf = wgpu_device_create_buffer(device, &bdesc);
  assert(buf);

  WGpuBindGroupEntry bgentry = { .binding = 0, .resource = buf };
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
