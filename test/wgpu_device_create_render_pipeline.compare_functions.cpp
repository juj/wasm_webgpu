// Tests creating render pipelines with the four depth compare functions not
// covered by existing tests: NEVER, EQUAL, GREATER, GREATER_EQUAL.
// Exercises GPUCompareFunctions[1], [3], [5], [7] in $wgpuReadRenderPipelineDescriptor.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

static WGpuDevice gDevice = 0;
static int step = 0;
static void tryNext(void *);

static const WGPU_COMPARE_FUNCTION kFuncs[] = {
  WGPU_COMPARE_FUNCTION_NEVER,         // index 1
  WGPU_COMPARE_FUNCTION_EQUAL,         // index 3
  WGPU_COMPARE_FUNCTION_GREATER,       // index 5
  WGPU_COMPARE_FUNCTION_GREATER_EQUAL, // index 7
};
static const char *kNames[] = { "never", "equal", "greater", "greater-equal" };

static void onError(WGpuDevice d, WGPU_ERROR_TYPE t, const char *m, void *u)
{
  tryNext(u);
}

static void tryNext(void *userData)
{
  if (step >= (int)(sizeof(kFuncs)/sizeof(kFuncs[0])))
  {
    EM_ASM(window.close());
    return;
  }

  WGPU_TEXTURE_FORMAT fmt = navigator_gpu_get_preferred_canvas_format();
  WGPU_COMPARE_FUNCTION fn = kFuncs[step];
  printf("Testing compare function: %s\n", kNames[step]);
  step++;

  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  var pos = array<vec2f,3>(vec2f(0,1),vec2f(-1,-1),vec2f(1,-1));"
            "  return vec4f(pos[vi], 0.5, 1); }"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(1,0,0,1); }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(gDevice, &smdesc);

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = fmt;

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "vs";
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";
  desc.fragment.targets = &colorTarget;
  desc.fragment.numTargets = 1;
  desc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;
  desc.depthStencil.format = WGPU_TEXTURE_FORMAT_DEPTH32FLOAT;
  desc.depthStencil.depthWriteEnabled = WGPU_FALSE;
  desc.depthStencil.depthCompare = fn; // exercises the untested compare function

  wgpu_device_push_error_scope(gDevice, WGPU_ERROR_FILTER_VALIDATION);
  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(gDevice, &desc);
  (void)pipeline;
  wgpu_device_pop_error_scope_async(gDevice, onError, 0);
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  gDevice = device;
  tryNext(0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
