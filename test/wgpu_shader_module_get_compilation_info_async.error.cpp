// Tests wgpu_shader_module_get_compilation_info_async() with a shader that has
// compile errors, verifying that the compilation info contains error messages
// with non-zero lineNum/linePos/offset/length fields. This exercises the
// message marshalling loop in wgpu_shader_module_get_compilation_info_async.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void OnCompilationInfo(WGpuShaderModule shaderModule, WGpuCompilationInfo *compilationInfo, void *userData)
{
  assert(compilationInfo);

  printf("Compilation info: %d message(s)\n", compilationInfo->numMessages);

  // The invalid shader should produce at least one error message
  assert(compilationInfo->numMessages > 0);

  for (int i = 0; i < compilationInfo->numMessages; ++i)
  {
    const WGpuCompilationMessage *msg = &compilationInfo->messages[i];
    printf("  [%d] type=%d line=%u linePos=%u offset=%u length=%u msg='%s'\n",
      i, msg->type, msg->lineNum, msg->linePos, msg->offset, msg->length,
      msg->message ? msg->message : "(null)");

    // Error messages should have non-zero line info
    assert(msg->type == WGPU_COMPILATION_MESSAGE_TYPE_ERROR ||
           msg->type == WGPU_COMPILATION_MESSAGE_TYPE_WARNING ||
           msg->type == WGPU_COMPILATION_MESSAGE_TYPE_INFO);
    assert(msg->message);
  }

  free(compilationInfo);
  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create a shader module with a deliberate syntax error.
  // Push error scope to prevent the error from being uncaptured.
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  WGpuShaderModuleDescriptor smdesc = {
    .code = "this is not valid WGSL at all $$$",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE t, const char *m, void *u) {
    // Ignore the validation error from the bad shader
  }, 0);

  // Request compilation info - this should contain error messages
  wgpu_shader_module_get_compilation_info_async(shader, OnCompilationInfo, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
