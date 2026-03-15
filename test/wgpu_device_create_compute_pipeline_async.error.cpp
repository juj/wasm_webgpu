// Exercises the $wgpuPipelineCreationFailed() error-marshalling path by requesting a compute
// pipeline whose entry point does not exist in the shader, causing createComputePipelineAsync()
// to reject. Verifies that the WGpuCreatePipelineCallback receives pipeline==0 and a non-null
// WGpuPipelineError with non-empty name, message, and reason strings.
// -sASSERTIONS=1 ensures the #if ASSERTIONS branch is taken in lib_webgpu.js so that
// $wgpuPipelineCreationFailed() is called (rather than the release-mode stub that passes null).
// flags: -sEXIT_RUNTIME=0 -sASSERTIONS=1

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void PipelineCreated(WGpuDevice device, WGpuPipelineError *error, WGpuPipelineBase pipeline, void *userData)
{
  assert(userData == (void*)42);

  // Creation must have failed: the entry point "nonexistent_fn" is not in the shader.
  assert(!pipeline);

  // With ASSERTIONS enabled wgpuPipelineCreationFailed() marshals the JS GPUPipelineError
  // into a WGpuPipelineError struct. Verify the three string fields are populated.
  assert(error);
  assert(error->name    && strlen(error->name)    > 0);
  assert(error->message && strlen(error->message) > 0);
  assert(error->reason  && strlen(error->reason)  > 0);

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModuleDescriptor shaderDesc = {
    .code = "@compute @workgroup_size(1) fn main() {}"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &shaderDesc);
  assert(shader);

  // Passing an entry point that does not exist causes createComputePipelineAsync()
  // to reject, routing execution through $wgpuPipelineCreationFailed().
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
