// Tests creating a render pipeline with dual-source blend factors (src1, one-minus-src1,
// src1-alpha, one-minus-src1-alpha), exercising GPUBlendFactors[14..17] in
// $wgpuReadGpuBlendComponent. Requires the dual-source-blending feature.
// Uses error scopes for graceful handling on platforms that don't support it.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

static WGpuDevice gDevice = 0;
static int step = 0;
static void tryNext(void *);

typedef struct { WGPU_BLEND_FACTOR src; WGPU_BLEND_FACTOR dst; const char *name; } BlendCase;

static const BlendCase kCases[] = {
  { WGPU_BLEND_FACTOR_SRC1,                WGPU_BLEND_FACTOR_ONE_MINUS_SRC1,       "src1/one-minus-src1" },
  { WGPU_BLEND_FACTOR_SRC1_ALPHA,          WGPU_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA, "src1-alpha/one-minus-src1-alpha" },
};

static void onError(WGpuDevice d, WGPU_ERROR_TYPE t, const char *m, void *u)
{
  if (t != WGPU_ERROR_TYPE_NO_ERROR)
    printf("Dual-source blend %s: validation error (feature may be unsupported): %s\n",
           kCases[step-1].name, m ? m : "");
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
  printf("Testing dual-source blend factors: %s\n", c->name);

  // Fragment shader with two blend sources via @blend_src attribute
  WGpuShaderModuleDescriptor smdesc = {
    .code = "enable dual_source_blending;\n"
            "struct FragOut {\n"
            "  @location(0) @blend_src(0) color: vec4f,\n"
            "  @location(0) @blend_src(1) blend: vec4f,\n"
            "}\n"
            "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {\n"
            "  return vec4f(0,0,0,1);\n"
            "}\n"
            "@fragment fn fs() -> FragOut {\n"
            "  return FragOut(vec4f(1,1,1,1), vec4f(0.5,0.5,0.5,1));\n"
            "}",
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
  desc.vertex.module     = shader;
  desc.vertex.entryPoint = "vs";
  desc.fragment.module   = shader;
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
  // Request dual-source-blending feature if available
  WGpuDeviceDescriptor desc = {};
  desc.requiredFeatures = WGPU_FEATURE_DUAL_SOURCE_BLENDING;
  wgpu_adapter_request_device_async(adapter, &desc, ObtainedWebGpuDevice, 0);
}

int main()
{
  // https://bugzilla.mozilla.org/show_bug.cgi?id=1924328: Dual Source Blending is not supported by Firefox.
  if (EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    EM_ASM(window.close());
    return 0;
  }

  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
