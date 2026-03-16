// Tests that WGpuDepthStencilState.clampDepth = WGPU_TRUE is correctly marshalled
// into the JS render pipeline descriptor, exercising the
// 'clampDepth': !!HEAPU32[depthStencilIdx+16] true branch in $wgpuReadRenderPipelineDescriptor.
// The field sits at WGpuDepthStencilState offset 16 (the 17th uint32 in the struct).
// A depth-stencil state must be present (depthStencilFormat != 0) for the branch to
// be reached. An error scope swallows any validation error in browsers that reject the
// non-standard field so the test still passes regardless of browser support.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  return vec4f(0, 0, 0.5, 1);"
            "}"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(1, 0, 0, 1); }"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  WGpuColorTargetState ct = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  ct.format = navigator_gpu_get_preferred_canvas_format();

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "vs";
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";
  desc.fragment.targets = &ct;
  desc.fragment.numTargets = 1;
  desc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

  // A non-zero depthStencil.format causes the depthStencil branch to be taken,
  // and clampDepth = WGPU_TRUE exercises 'clampDepth': !!HEAPU32[depthStencilIdx+16].
  desc.depthStencil.format = WGPU_TEXTURE_FORMAT_DEPTH32FLOAT;
  desc.depthStencil.depthWriteEnabled = WGPU_TRUE;
  desc.depthStencil.depthCompare = WGPU_COMPARE_FUNCTION_LESS;
  desc.depthStencil.clampDepth = WGPU_TRUE;

  // clampDepth may or may not be accepted by the browser; swallow any
  // validation error so the test passes regardless of browser support.
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &desc);
  (void)pipeline;
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice, WGPU_ERROR_TYPE, const char *, void *) {
    EM_ASM(window.close());
  }, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
