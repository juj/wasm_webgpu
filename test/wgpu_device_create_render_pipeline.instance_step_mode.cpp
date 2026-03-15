// Tests creating a render pipeline where a vertex buffer uses INSTANCE step mode,
// exercising the [, 'vertex', 'instance'][HEAPU32[vertexBuffersIdx+3]] code path.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuShaderModuleDescriptor smdesc = {
    .code = "struct VSIn {"
            "  @location(0) pos: vec2f,"       // vertex buffer
            "  @location(1) color: vec4f,"     // instance buffer
            "};"
            "@vertex fn vs(in: VSIn, @builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  return vec4f(in.pos, 0, 1);"
            "}"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(1,0,0,1); }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  // Buffer 0: per-vertex positions
  WGpuVertexAttribute posAttr = {};
  posAttr.format = WGPU_VERTEX_FORMAT_FLOAT32X2;
  posAttr.offset = 0;
  posAttr.shaderLocation = 0;

  WGpuVertexBufferLayout vbVertex = {};
  vbVertex.arrayStride = 8;
  vbVertex.stepMode = WGPU_VERTEX_STEP_MODE_VERTEX;
  vbVertex.attributes = &posAttr;
  vbVertex.numAttributes = 1;

  // Buffer 1: per-instance colors (step mode = INSTANCE)
  WGpuVertexAttribute colorAttr = {};
  colorAttr.format = WGPU_VERTEX_FORMAT_FLOAT32X4;
  colorAttr.offset = 0;
  colorAttr.shaderLocation = 1;

  WGpuVertexBufferLayout vbInstance = {};
  vbInstance.arrayStride = 16;
  vbInstance.stepMode = WGPU_VERTEX_STEP_MODE_INSTANCE;
  vbInstance.attributes = &colorAttr;
  vbInstance.numAttributes = 1;

  WGpuVertexBufferLayout buffers[2] = { vbVertex, vbInstance };

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = colorFormat;

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "vs";
  desc.vertex.buffers = buffers;
  desc.vertex.numBuffers = 2;
  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";
  desc.fragment.targets = &colorTarget;
  desc.fragment.numTargets = 1;
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
