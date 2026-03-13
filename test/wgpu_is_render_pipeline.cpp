// Verifies that wgpu_is_render_pipeline() returns true for a WGpuRenderPipeline and false for a WGpuShaderModule.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModuleDescriptor shaderDesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) i: u32) -> @builtin(position) vec4f { return vec4f(0,0,0,1); }\n"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(1,0,0,1); }"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &shaderDesc);

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "vs";
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = navigator_gpu_get_preferred_canvas_format();
  desc.fragment.numTargets = 1;
  desc.fragment.targets = &colorTarget;

  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &desc);
  assert(pipeline);
  assert(wgpu_is_render_pipeline(pipeline));
  assert(!wgpu_is_render_pipeline(shader)); // a shader module is not a render pipeline

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
