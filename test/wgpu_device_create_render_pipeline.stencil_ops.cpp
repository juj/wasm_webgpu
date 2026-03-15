// Tests creating a render pipeline with non-default stencil operations
// (REPLACE on pass, INVERT on fail, INCREMENT_CLAMP on depth-fail),
// exercising different GPUStencilOperations values in wgpuReadGpuStencilFaceState.
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
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(0,0,1,1); }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = colorFormat;

  // Stencil ops: front face uses REPLACE on pass, INVERT on stencil fail, INCREMENT_CLAMP on depth fail
  WGpuStencilFaceState stencilFront = {
    .compare = WGPU_COMPARE_FUNCTION_ALWAYS,
    .failOp = WGPU_STENCIL_OPERATION_INVERT,
    .depthFailOp = WGPU_STENCIL_OPERATION_INCREMENT_CLAMP,
    .passOp = WGPU_STENCIL_OPERATION_REPLACE,
  };
  // Back face uses DECREMENT_WRAP on pass, ZERO on fail, DECREMENT_CLAMP on depth fail
  WGpuStencilFaceState stencilBack = {
    .compare = WGPU_COMPARE_FUNCTION_NOT_EQUAL,
    .failOp = WGPU_STENCIL_OPERATION_ZERO,
    .depthFailOp = WGPU_STENCIL_OPERATION_DECREMENT_CLAMP,
    .passOp = WGPU_STENCIL_OPERATION_DECREMENT_WRAP,
  };

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "vs";
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";
  desc.fragment.targets = &colorTarget;
  desc.fragment.numTargets = 1;
  desc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

  desc.depthStencil.format = WGPU_TEXTURE_FORMAT_DEPTH24PLUS_STENCIL8;
  desc.depthStencil.depthWriteEnabled = WGPU_FALSE;
  desc.depthStencil.depthCompare = WGPU_COMPARE_FUNCTION_ALWAYS;
  desc.depthStencil.stencilReadMask = 0xFF;
  desc.depthStencil.stencilWriteMask = 0xFF;
  desc.depthStencil.stencilFront = stencilFront;
  desc.depthStencil.stencilBack = stencilBack;

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
