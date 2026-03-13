// Verifies that wgpu_device_pop_error_scope_sync() returns WGPU_ERROR_TYPE_NO_ERROR for a clean scope and WGPU_ERROR_TYPE_VALIDATION with a non-empty message when a validation error occurs within the scope.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  // Test 1: No error - should return WGPU_ERROR_TYPE_NO_ERROR
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  char msg1[512] = {};
  WGPU_ERROR_TYPE err1 = wgpu_device_pop_error_scope_sync(device, msg1, sizeof(msg1));
  assert(err1 == WGPU_ERROR_TYPE_NO_ERROR);
  assert(strlen(msg1) == 0);
  printf("Test 1 (no error): OK\n");

  // Test 2: Validation error - should return WGPU_ERROR_TYPE_VALIDATION
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  desc.usage = 0; // Invalid: usage must not be 0
  wgpu_device_create_texture(device, &desc);
  char msg2[512] = {};
  WGPU_ERROR_TYPE err2 = wgpu_device_pop_error_scope_sync(device, msg2, sizeof(msg2));
  printf("Test 2 (validation error): errorType=%d msg=%s\n", err2, msg2);
  assert(err2 == WGPU_ERROR_TYPE_VALIDATION);
  assert(strlen(msg2) > 0);

  printf("Test OK\n");
  EM_ASM(window.close());
}
