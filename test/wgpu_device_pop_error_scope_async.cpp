// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void OnError(WGpuDevice device, WGPU_ERROR_TYPE errorType, const char *errorMessage NOTNULL, void *userData)
{
  assert(wgpu_is_device(device));
  assert(errorType == WGPU_ERROR_FILTER_VALIDATION);
  assert(userData == (void*)42);
  printf("%s\n", errorMessage);
  assert(strstr(errorMessage, "texture usage must not be 0"));

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  wgpu_device_create_texture(device, &desc); // Create texture with intentional .usage left to zero (which is not allowed)
  wgpu_device_pop_error_scope_async(device, OnError, (void*)42); // Check that we see that error being diagnosed
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
