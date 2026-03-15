// Tests creating render pipelines using float16 and packed vertex attribute formats
// (FLOAT16X2, FLOAT16X4, UNORM10_10_10_2, UNORM8X4_BGRA) to exercise
// GPUTextureAndVertexFormats indices 127, 128, 141, 142 in the marshalling layer.
// These are the highest-index vertex format entries in the shared table.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

static WGpuDevice gDevice = 0;
static int step = 0;
static void tryNext(void *);

typedef struct { WGPU_VERTEX_FORMAT fmt; int numComponents; const char *name; } FmtCase;

static const FmtCase kCases[] = {
  { WGPU_VERTEX_FORMAT_FLOAT16X2,      2, "float16x2"       },  // index 127
  { WGPU_VERTEX_FORMAT_FLOAT16X4,      4, "float16x4"       },  // index 128
  { WGPU_VERTEX_FORMAT_UNORM10_10_10_2,4, "unorm10-10-10-2" },  // index 141
  { WGPU_VERTEX_FORMAT_UNORM8X4_BGRA,  4, "unorm8x4-bgra"   },  // index 142
};

static void onError(WGpuDevice d, WGPU_ERROR_TYPE t, const char *m, void *u)
{
  if (t != WGPU_ERROR_TYPE_NO_ERROR)
    printf("Packed vertex format %s: error %s\n", kCases[step-1].name, m ? m : "");
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
  printf("Testing packed/float16 vertex format: %s\n", c->name);

  // All of these formats decode to float in the shader (vec2f or vec4f)
  const char *shaderCode =
    c->numComponents == 2
    ? "@vertex fn vs(@location(0) a: vec2f) -> @builtin(position) vec4f { return vec4f(a, 0, 1); }"
      "@fragment fn fs() -> @location(0) vec4f { return vec4f(1); }"
    : "@vertex fn vs(@location(0) a: vec4f) -> @builtin(position) vec4f { return a; }"
      "@fragment fn fs() -> @location(0) vec4f { return vec4f(1); }";

  WGpuShaderModuleDescriptor smdesc = { .code = shaderCode };
  WGpuShaderModule shader = wgpu_device_create_shader_module(gDevice, &smdesc);

  WGpuVertexAttribute attr = {};
  attr.format = c->fmt;
  attr.offset = 0;
  attr.shaderLocation = 0;

  WGpuVertexBufferLayout vbl = {};
  vbl.arrayStride = 4; // conservative: all these formats are ≤ 8 bytes, stride covers x2 variants
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
