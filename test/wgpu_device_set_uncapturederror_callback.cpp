// Verifies that the uncaptured-error callback registered via wgpu_device_set_uncapturederror_callback() is invoked with WGPU_ERROR_TYPE_VALIDATION and a non-empty message when a validation error occurs outside any error scope.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void OnUncapturedError(WGpuDevice device, WGPU_ERROR_TYPE errorType, const char *errorMessage, void *userData)
{
  printf("Uncaptured error: type=%d msg=%s\n", errorType, errorMessage ? errorMessage : "(null)");
  assert(wgpu_is_device(device));
  assert(userData == (void*)88);
  assert(errorType == WGPU_ERROR_TYPE_VALIDATION);
  assert(errorMessage && strlen(errorMessage) > 0);

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  wgpu_device_set_uncapturederror_callback(device, OnUncapturedError, (void*)88);

  // Intentionally cause a validation error outside of an error scope
  // Creating a texture with usage=0 is invalid
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  desc.usage = 0; // Invalid: usage must not be 0
  wgpu_device_create_texture(device, &desc);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
