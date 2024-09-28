// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModuleCompilationHint hint = {
    .entryPoint = "main",
    .layout = WGPU_AUTO_LAYOUT_MODE_AUTO
  };

  WGpuShaderModuleDescriptor vsDesc = {
    .code = "struct In {\n"
    "  @location(0) pos : vec2<f32>,\n"
    "  @location(1) color : f32\n"
    "};\n"

    "struct Out {\n"
    "  @builtin(position) pos : vec4<f32>,\n"
    "  @location(0) color : f32\n"
    "};\n"

    "@vertex\n"
    "fn main(in: In) -> Out {\n"
      "var out: Out;\n"
      "out.pos = vec4<f32>(in.pos, 0.0, 1.0);\n"
      "out.color = in.color;\n"
      "return out;\n"
    "}\n",
    .hints = &hint,
    .numHints = 1
  };

  WGpuShaderModule vs = wgpu_device_create_shader_module(device, &vsDesc);

  WGpuShaderModuleDescriptor fsDesc = {
    .code = "@fragment\n"
    "fn main(@location(0) inColor : f32) -> @location(0) vec4<f32> {\n"
      "return vec4<f32>(inColor, inColor, abs(inColor), 1.0);\n"
    "}\n",
    .hints = &hint,
    .numHints = 1
  };

  WGpuShaderModule fs = wgpu_device_create_shader_module(device, &fsDesc);

  WGpuVertexAttribute vertexAttr[2] = {};
  vertexAttr[0].format = WGPU_VERTEX_FORMAT_FLOAT32X2;
  vertexAttr[0].offset = 0;
  vertexAttr[0].shaderLocation = 0;
  vertexAttr[1].format = WGPU_VERTEX_FORMAT_FLOAT32;
  vertexAttr[1].offset = 8;
  vertexAttr[1].shaderLocation = 1;

  WGpuVertexBufferLayout vbLayout = {};
  vbLayout.numAttributes = 2;
  vbLayout.attributes = vertexAttr;
  vbLayout.arrayStride = 12;

  WGpuRenderPipelineDescriptor renderPipelineDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  renderPipelineDesc.vertex.module = vs;
  renderPipelineDesc.vertex.entryPoint = "main";
  renderPipelineDesc.vertex.numBuffers = 1;
  renderPipelineDesc.vertex.buffers = &vbLayout;
  renderPipelineDesc.fragment.module = fs;
  renderPipelineDesc.fragment.entryPoint = "main";

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = navigator_gpu_get_preferred_canvas_format();
  colorTarget.blend.color.operation = WGPU_BLEND_OPERATION_ADD;
  colorTarget.blend.color.dstFactor = WGPU_BLEND_FACTOR_ONE;
  renderPipelineDesc.fragment.numTargets = 1;
  renderPipelineDesc.fragment.targets = &colorTarget;

  renderPipelineDesc.primitive.topology = WGPU_PRIMITIVE_TOPOLOGY_LINE_LIST;

  WGpuRenderPipeline renderPipeline = wgpu_device_create_render_pipeline(device, &renderPipelineDesc);
  assert(renderPipeline);

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
