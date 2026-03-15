// Tests creating a render pipeline with non-zero depthBias, depthBiasSlopeScale,
// and depthBiasClamp, exercising those fields in wgpuReadRenderPipelineDescriptor
// (depthStencilIdx+5, +6, +7 offsets read via HEAP32/HEAPF32).
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  var pos = array<vec2f,3>(vec2f(0,1),vec2f(-1,-1),vec2f(1,-1));"
            "  return vec4f(pos[vi], 0.5, 1);"
            "}"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(1,0.5,0,1); }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = colorFormat;

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "vs";
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";
  desc.fragment.targets = &colorTarget;
  desc.fragment.numTargets = 1;
  desc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

  // Depth state with non-zero bias values
  desc.depthStencil.format = WGPU_TEXTURE_FORMAT_DEPTH32FLOAT;
  desc.depthStencil.depthWriteEnabled = WGPU_TRUE;
  desc.depthStencil.depthCompare = WGPU_COMPARE_FUNCTION_LESS_EQUAL;
  desc.depthStencil.depthBias = 100;           // non-zero integer bias
  desc.depthStencil.depthBiasSlopeScale = 1.5f; // non-zero slope scale
  desc.depthStencil.depthBiasClamp = 0.01f;    // non-zero bias clamp

  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &desc);
  assert(pipeline);
  assert(wgpu_is_render_pipeline(pipeline));

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
