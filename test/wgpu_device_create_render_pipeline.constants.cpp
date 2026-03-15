// Tests wgpu_device_create_render_pipeline() with pipeline override constants
// set in both the vertex and fragment stages, exercising $wgpuReadConstants
// from both marshalling call sites in the render pipeline descriptor path.
// (Distinct from the compute pipeline constants path tested separately.)
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Shader with override constants in both stages
  WGpuShaderModuleDescriptor smdesc = {
    .code = "override kScale: f32 = 1.0;\n"       // used in vertex stage
            "override kBrightness: f32 = 0.5;\n"  // used in fragment stage
            "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {\n"
            "  return vec4f(0, 0, 0, kScale);\n"
            "}\n"
            "@fragment fn fs() -> @location(0) vec4f {\n"
            "  return vec4f(kBrightness, kBrightness, kBrightness, 1);\n"
            "}",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);

  // Vertex stage constant: kScale = 2.0
  WGpuPipelineConstant vsConstants[] = {
    { .name = "kScale", .value = 2.0 },
  };

  // Fragment stage constant: kBrightness = 1.0
  WGpuPipelineConstant fsConstants[] = {
    { .name = "kBrightness", .value = 1.0 },
  };

  WGpuColorTargetState ct = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  ct.format = navigator_gpu_get_preferred_canvas_format();

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "vs";
  desc.vertex.constants = vsConstants;
  desc.vertex.numConstants = 1;
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";
  desc.fragment.targets = &ct;
  desc.fragment.numTargets = 1;
  desc.fragment.constants = fsConstants;
  desc.fragment.numConstants = 1;
  desc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &desc);
  (void)pipeline;
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *m, void *) {
    if (type != WGPU_ERROR_TYPE_NO_ERROR)
      printf("Render pipeline constants: error: %s\n", m ? m : "");
    else
      printf("Render pipeline with vertex+fragment constants: OK\n");
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
