// Verifies that a WGpuBindGroupLayout with hasDynamicOffset=true on a storage buffer binding can be created and used to build a valid bind group.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  // Create a BGL with hasDynamicOffset=true on the storage buffer binding
  WGpuBindGroupLayoutEntry entry = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_COMPUTE,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER,
    .layout.buffer = {
      .type = WGPU_BUFFER_BINDING_TYPE_STORAGE,
      .hasDynamicOffset = WGPU_TRUE,
      .minBindingSize = 0,
    },
  };

  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &entry, 1);
  assert(bgl);

  // Create a storage buffer large enough for dynamic offsetting
  // Dynamic offsets must be multiples of minStorageBufferOffsetAlignment (256)
  WGpuSupportedLimits limits;
  wgpu_device_get_limits(device, &limits);
  uint32_t align = limits.minStorageBufferOffsetAlignment;

  WGpuBufferDescriptor bdesc = {
    .size = (uint64_t)align * 4,
    .usage = WGPU_BUFFER_USAGE_STORAGE | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer sbuf = wgpu_device_create_buffer(device, &bdesc);
  assert(sbuf);

  // Bind the buffer at offset 0, bind size = align (one "slot")
  WGpuBindGroupEntry bgentry = {
    .binding = 0,
    .resource = sbuf,
    .bufferBindOffset = 0,
    .bufferBindSize = align,
  };
  WGpuBindGroup bg = wgpu_device_create_bind_group(device, bgl, &bgentry, 1);
  assert(bg);
  assert(wgpu_is_bind_group(bg));

  char msg[512];
  WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
  if (strlen(msg) > 0) printf("%s\n", msg);
  assert(!error);

  printf("Test OK\n");
  EM_ASM(window.close());
}
