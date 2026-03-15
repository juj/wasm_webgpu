// Tests creating render pipelines with all untested blend factor enum values,
// exercising GPUBlendFactors[1..13] in $wgpuReadGpuBlendComponent.
// Loops through ZERO, SRC, ONE_MINUS_SRC, DST, ONE_MINUS_DST, DST_ALPHA,
// ONE_MINUS_DST_ALPHA, SRC_ALPHA_SATURATED, CONSTANT, ONE_MINUS_CONSTANT.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

static WGpuDevice gDevice = 0;
static int step = 0;
static void tryNext(void *);

typedef struct { WGPU_BLEND_FACTOR src; WGPU_BLEND_FACTOR dst; const char *name; } BlendCase;

static const BlendCase kCases[] = {
  { WGPU_BLEND_FACTOR_ZERO,                WGPU_BLEND_FACTOR_ONE,                 "zero/one" },
  { WGPU_BLEND_FACTOR_SRC,                 WGPU_BLEND_FACTOR_ONE_MINUS_SRC,       "src/one-minus-src" },
  { WGPU_BLEND_FACTOR_DST,                 WGPU_BLEND_FACTOR_ONE_MINUS_DST,       "dst/one-minus-dst" },
  { WGPU_BLEND_FACTOR_DST_ALPHA,           WGPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA, "dst-alpha/one-minus-dst-alpha" },
  { WGPU_BLEND_FACTOR_SRC_ALPHA_SATURATED, WGPU_BLEND_FACTOR_ONE,                 "src-alpha-saturated/one" },
  { WGPU_BLEND_FACTOR_CONSTANT,            WGPU_BLEND_FACTOR_ONE_MINUS_CONSTANT,  "constant/one-minus-constant" },
};

static void onError(WGpuDevice d, WGPU_ERROR_TYPE t, const char *m, void *u)
{
  tryNext(u);
}

static void tryNext(void *userData)
{
  if (step >= (int)(sizeof(kCases)/sizeof(kCases[0])))
  {
    EM_ASM(window.close());
    return;
  }

  WGPU_TEXTURE_FORMAT fmt = navigator_gpu_get_preferred_canvas_format();
  const BlendCase *c = &kCases[step++];
  printf("Testing blend factors: %s\n", c->name);

  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  return vec4f(0,0,0,1); }"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(1,1,1,1); }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(gDevice, &smdesc);

  WGpuBlendComponent blendComp = {
    .operation = WGPU_BLEND_OPERATION_ADD,
    .srcFactor  = c->src,
    .dstFactor  = c->dst,
  };
  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = fmt;
  colorTarget.blend.color = blendComp;
  colorTarget.blend.alpha = blendComp;

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module   = shader;
  desc.vertex.entryPoint = "vs";
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";
  desc.fragment.targets    = &colorTarget;
  desc.fragment.numTargets = 1;
  desc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

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
