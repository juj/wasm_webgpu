// Tests wgpu_device_create_texture() with textureBindingViewDimension=CUBE,
// exercising GPUTextureViewDimensions[4] → 'cube' in the texture descriptor
// marshalling in lib_webgpu.js.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format                      = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  tdesc.usage                       = WGPU_TEXTURE_USAGE_TEXTURE_BINDING;
  tdesc.width                       = 4;
  tdesc.height                      = 4;
  tdesc.depthOrArrayLayers          = 6; // 6 faces for a cube
  tdesc.textureBindingViewDimension = WGPU_TEXTURE_VIEW_DIMENSION_CUBE; // index 4 → 'cube'

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuTexture texture = wgpu_device_create_texture(device, &tdesc);
  (void)texture;
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *m, void *) {
    if (type != WGPU_ERROR_TYPE_NO_ERROR)
      printf("textureBindingViewDimension=cube not supported: %s\n", m ? m : "");
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
