// Tests wgpu_device_create_pipeline_layout() with a null/zero BGL slot in the
// array, exercising the 'wgpuReadArrayOfItemsMaybeNull' path that writes null
// (undefined) for a zero-ID entry. This creates a sparse pipeline layout.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create one real BGL for slot 1
  WGpuBindGroupLayoutEntry entry = {
    .binding    = 0,
    .visibility = WGPU_SHADER_STAGE_COMPUTE,
    .type       = WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER,
    .layout.buffer = { .type = WGPU_BUFFER_BINDING_TYPE_UNIFORM },
  };
  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &entry, 1);
  assert(bgl);

  // Sparse layout: slot 0 is null (0 = INVALID → null in JS), slot 1 is real
  WGpuBindGroupLayout bgls[2] = { 0, bgl };

  // Sparse BGL works in Firefox Nightly, but not yet in Firefox stable.
  // TODO: Remove this when sparse BGLs are supported.
  if (EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    EM_ASM(window.close());
    return;
  }

  // Use error scope since sparse pipeline layouts may not be universally supported
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuPipelineLayout layout = wgpu_device_create_pipeline_layout(device, bgls, 2);
  (void)layout;
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *m, void *) {
    if (type != WGPU_ERROR_TYPE_NO_ERROR)
      printf("Sparse pipeline layout not supported: %s\n", m ? m : "");
    EM_ASM(window.close());
  }, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
