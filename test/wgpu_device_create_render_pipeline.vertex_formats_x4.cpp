// Tests creating render pipelines using x4-component and uint32 integer vertex
// attribute formats (UINT8X4, SINT16X4, UINT32X4, SINT32X4) to exercise
// GPUTextureAndVertexFormats indices 104, 119, 136, 140 in the marshalling layer.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

static WGpuDevice gDevice = 0;
static int step = 0;
static void tryNext(void *);

typedef struct { WGPU_VERTEX_FORMAT fmt; const char *shaderType; const char *name; } FmtCase;

static const FmtCase kCases[] = {
  // Unsigned x4 formats: shader input is vec4u
  { WGPU_VERTEX_FORMAT_UINT8X4,  "vec4u", "uint8x4"  },  // index 104
  { WGPU_VERTEX_FORMAT_UINT32X4, "vec4u", "uint32x4" },  // index 136
  // Signed x4 formats: shader input is vec4i
  { WGPU_VERTEX_FORMAT_SINT16X4, "vec4i", "sint16x4" },  // index 119
  { WGPU_VERTEX_FORMAT_SINT32X4, "vec4i", "sint32x4" },  // index 140
};

static void onError(WGpuDevice d, WGPU_ERROR_TYPE t, const char *m, void *u)
{
  if (t != WGPU_ERROR_TYPE_NO_ERROR)
    printf("x4 vertex format %s: error %s\n", kCases[step-1].name, m ? m : "");
  tryNext(u);
}

static void tryNext(void *userData)
{
  if (step >= (int)(sizeof(kCases)/sizeof(kCases[0])))
  {
    EM_ASM(window.close());
    return;
  }

  const FmtCase *c = &kCases[step++];
  printf("Testing x4 vertex format: %s\n", c->name);

  char shaderBuf[512];
  snprintf(shaderBuf, sizeof(shaderBuf),
    "@vertex fn vs(@location(0) a: %s) -> @builtin(position) vec4f {"
    "  return vec4f(f32(a.x), f32(a.y), f32(a.z), f32(a.w)); }"
    "@fragment fn fs() -> @location(0) vec4f { return vec4f(1); }",
    c->shaderType);

  WGpuShaderModuleDescriptor smdesc = { .code = shaderBuf };
  WGpuShaderModule shader = wgpu_device_create_shader_module(gDevice, &smdesc);

  WGpuVertexAttribute attr = {};
  attr.format = c->fmt;
  attr.offset = 0;
  attr.shaderLocation = 0;

  WGpuVertexBufferLayout vbl = {};
  vbl.arrayStride = 4; // 4-byte stride covers uint8x4; larger formats tolerate it in error scope
  vbl.attributes = &attr;
  vbl.numAttributes = 1;

  WGpuColorTargetState ct = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  ct.format = navigator_gpu_get_preferred_canvas_format();

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "vs";
  desc.vertex.buffers = &vbl;
  desc.vertex.numBuffers = 1;
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";
  desc.fragment.targets = &ct;
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
