// Tests creating render pipelines with various non-default blend operations
// (subtract, reverse-subtract, min, max), exercising all GPUBlendOperations
// enum values in wgpuReadGpuBlendComponent.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

static WGpuDevice gDevice = 0;
static int step = 0;
static void tryNextBlendOp(void *);

typedef struct { WGPU_BLEND_OPERATION op; const char *name; } BlendOpCase;

static const BlendOpCase kOps[] = {
  { WGPU_BLEND_OPERATION_SUBTRACT,         "subtract" },
  { WGPU_BLEND_OPERATION_REVERSE_SUBTRACT, "reverse-subtract" },
  { WGPU_BLEND_OPERATION_MIN,              "min" },
  { WGPU_BLEND_OPERATION_MAX,              "max" },
};

static void onError(WGpuDevice d, WGPU_ERROR_TYPE t, const char *m, void *u)
{
  tryNextBlendOp(u);
}

static void tryNextBlendOp(void *userData)
{
  if (step >= (int)(sizeof(kOps)/sizeof(kOps[0])))
  {
    EM_ASM(window.close());
    return;
  }

  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  return vec4f(0,0,0,1);"
            "}"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(1,1,1,1); }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(gDevice, &smdesc);

  WGpuBlendComponent blendComp = {
    .operation = kOps[step].op,
    .srcFactor = WGPU_BLEND_FACTOR_ONE,
    .dstFactor = WGPU_BLEND_FACTOR_ONE,
  };

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = colorFormat;
  colorTarget.blend.color = blendComp;
  colorTarget.blend.alpha = blendComp;

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "vs";
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";
  desc.fragment.targets = &colorTarget;
  desc.fragment.numTargets = 1;
  desc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

  wgpu_device_push_error_scope(gDevice, WGPU_ERROR_FILTER_VALIDATION);
  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(gDevice, &desc);
  (void)pipeline;
  step++;
  wgpu_device_pop_error_scope_async(gDevice, onError, 0);
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  gDevice = device;
  tryNextBlendOp(0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
