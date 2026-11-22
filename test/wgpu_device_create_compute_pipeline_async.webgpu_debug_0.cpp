// Verifies that building with -jsDWEBGPU_DEBUG=0 gives the same behavior as not defining
// WEBGPU_DEBUG at all. Values of -jsD defines reach the JS library preprocessor as strings, so
// "0" must be parsed as a number before it is tested: the string "0" is truthy.
// The observable difference that this test uses is the 'error' parameter of
// WGpuCreatePipelineCallback: lib_webgpu.h documents that it carries the failure details only
// in WEBGPU_DEBUG (and ASSERTIONS) builds, and that it is null in release builds. So with
// both ASSERTIONS and WEBGPU_DEBUG explicitly disabled, a failed pipeline creation must report
// error == null.
// flags: -sEXIT_RUNTIME=0 -sASSERTIONS=0 -jsDWEBGPU_DEBUG=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void PipelineCreated(WGpuDevice device, WGpuPipelineError *error, WGpuPipelineBase pipeline, void *userData)
{
  assert(userData == (void*)42);

  // Creation must have failed: the entry point "nonexistent_fn" is not in the shader.
  assert(!pipeline);

  // This is not a debug build, so no error details are marshalled.
  assert(!error);

  printf("Test OK\n");
  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModuleDescriptor shaderDesc = {
    .code = "@compute @workgroup_size(1) fn main() {}"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &shaderDesc);
  assert(shader);

  wgpu_device_create_compute_pipeline_async(device, shader, "nonexistent_fn",
    WGPU_AUTO_LAYOUT_MODE_AUTO, /*constants=*/0, /*numConstants=*/0,
    PipelineCreated, (void*)42);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
