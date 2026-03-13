// Verifies that wgpu_shader_module_get_compilation_info_async() delivers a WGpuCompilationInfo to the callback and that a valid shader produces no error-type compilation messages.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdlib.h>

void GotCompilationInfo(WGpuShaderModule shaderModule, WGpuCompilationInfo *compilationInfo, void *userData)
{
  assert(shaderModule);
  assert(compilationInfo);
  assert(userData == (void*)123);

  // A valid shader should have no error-type messages (may have warnings/info)
  for (int i = 0; i < compilationInfo->numMessages; ++i)
    assert(compilationInfo->messages[i].type != WGPU_COMPILATION_MESSAGE_TYPE_ERROR);

  wgpu_free_compilation_info(compilationInfo);

  EM_ASM(window.close());
}

void GotCompilationInfoWithError(WGpuShaderModule shaderModule, WGpuCompilationInfo *compilationInfo, void *userData)
{
  assert(shaderModule);
  assert(compilationInfo);

  // An invalid shader should produce at least one error message
  assert(compilationInfo->numMessages > 0);
  assert(compilationInfo->messages[0].type == WGPU_COMPILATION_MESSAGE_TYPE_ERROR);

  wgpu_free_compilation_info(compilationInfo);

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Test with a valid shader - expect no compilation errors
  WGpuShaderModuleDescriptor desc = {
    .code = "@compute @workgroup_size(1) fn main() {}"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &desc);
  assert(shader);

  wgpu_shader_module_get_compilation_info_async(shader, GotCompilationInfo, (void*)123);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
