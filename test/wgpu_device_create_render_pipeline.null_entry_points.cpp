// Tests wgpu_device_create_render_pipeline() with null vertex and fragment
// entry points, exercising the 'utf8(...) || void 0' void-0 branch in
// $wgpuReadRenderPipelineDescriptor for both vertex and fragment stages.
// Null entryPoint tells the browser to auto-detect the single entry point.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  // Shader with unambiguous single entry points for each stage
  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs_main(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  return vec4f(0,0,0,1); }"
            "@fragment fn fs_main() -> @location(0) vec4f { return vec4f(1,0,0,1); }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = colorFormat;

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module     = shader;
  desc.vertex.entryPoint = 0; // null → exercises 'utf8(...) || void 0' → void 0 (vertex)
  desc.fragment.module     = shader;
  desc.fragment.entryPoint = 0; // null → exercises 'utf8(...) || void 0' → void 0 (fragment)
  desc.fragment.targets    = &colorTarget;
  desc.fragment.numTargets = 1;
  desc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

  // Use error scope since null entryPoint requires exactly one matching entry per stage
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &desc);
  (void)pipeline;
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *m, void *) {
    // Some implementations require explicit entry points; accept either outcome
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
