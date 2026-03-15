// Tests creating a render pipeline with a sparse (null) color target slot,
// exercising the 'targets.push(HEAPU32[targetsIdx] ? {...} : null)' branch
// in wgpuReadRenderPipelineDescriptor when format == WGPU_TEXTURE_FORMAT_INVALID.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuShaderModuleDescriptor smdesc = {
    // Shader only writes to location 1 (location 0 is sparse/null).
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  var pos = array<vec2f,3>(vec2f(0,1),vec2f(-1,-1),vec2f(1,-1));"
            "  return vec4f(pos[vi], 0, 1);"
            "}"
            "struct Out { @location(1) c: vec4f };"
            "@fragment fn fs() -> Out { return Out(vec4f(1,0,0,1)); }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  // Two target slots: slot 0 is null (sparse), slot 1 is the real color target.
  WGpuColorTargetState targets[2] = {};
  // targets[0].format == 0 == WGPU_TEXTURE_FORMAT_INVALID => null sparse slot
  targets[1].format = colorFormat;
  targets[1].writeMask = WGPU_COLOR_WRITE_ALL;

  // https://bugzilla.mozilla.org/show_bug.cgi?id=2023423: sparse GPUColorTargetState on beginRenderPass not supported
  if (EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    EM_ASM(window.close());
    return;
  }

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "vs";
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";
  desc.fragment.targets = targets;
  desc.fragment.numTargets = 2;
  desc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

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
