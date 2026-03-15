// Tests wgpu_device_create_bind_group_layout() with an external texture entry
// (WGPU_BIND_GROUP_LAYOUT_TYPE_EXTERNAL_TEXTURE == 5), exercising the
// 'externalTexture' branch in wgpuReadBindGroupLayoutDescriptor.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Bind group layout with an external texture at binding 0
  WGpuBindGroupLayoutEntry entry = {
    .binding = 0,
    .visibility = WGPU_SHADER_STAGE_FRAGMENT,
    .type = WGPU_BIND_GROUP_LAYOUT_TYPE_EXTERNAL_TEXTURE,
  };

  // Push error scope because external texture support may be limited.
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuBindGroupLayout bgl = wgpu_device_create_bind_group_layout(device, &entry, 1);
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *msg, void *udata) {
    if (type == WGPU_ERROR_TYPE_NO_ERROR)
    {
      // Succeeded: verify the object is valid
      // (bgl captured via userData)
      WGpuBindGroupLayout pBgl = (WGpuBindGroupLayout)udata;
      assert(pBgl);
      assert(wgpu_is_bind_group_layout(pBgl));
    }
    // Either success or validation error is acceptable.
    EM_ASM(window.close());
  }, (void*)bgl);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
