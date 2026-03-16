// Exercises the $wgpuPipelineCreationFailed() error-marshalling path by requesting a render
// pipeline whose vertex entry point does not exist in the shader, causing
// createRenderPipelineAsync() to reject. Verifies that the WGpuCreatePipelineCallback receives
// pipeline==0 and a non-null WGpuPipelineError with non-empty name, message, and reason strings.
// -sASSERTIONS=1 ensures the #if ASSERTIONS branch is taken in lib_webgpu.js so that
// $wgpuPipelineCreationFailed() is called (rather than the release-mode stub that passes null).
// flags: -sEXIT_RUNTIME=0 -sASSERTIONS=1

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void PipelineCreated(WGpuDevice device, WGpuPipelineError *error, WGpuPipelineBase pipeline, void *userData)
{
  assert(userData == (void*)99);

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
    .code = "@vertex fn vs(@builtin(vertex_index) i: u32) -> @builtin(position) vec4f { return vec4f(0,0,0,1); }\n"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(1,0,0,1); }"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &shaderDesc);
  assert(shader);

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  // Use an entry point that does not exist to force createRenderPipelineAsync() to reject.
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "nonexistent_fn";
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = navigator_gpu_get_preferred_canvas_format();
  desc.fragment.numTargets = 1;
  desc.fragment.targets = &colorTarget;

  wgpu_device_create_render_pipeline_async(device, &desc, PipelineCreated, (void*)99);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
