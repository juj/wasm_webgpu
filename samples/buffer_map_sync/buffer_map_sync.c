#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <emscripten/em_math.h>
#include "lib_webgpu.h"
#include "lib_demo.h"

WGpuAdapter adapter;
WGpuCanvasContext canvasContext;
WGpuDevice device;
WGpuRenderPipeline renderPipeline;
WGpuBuffer stagingBuffer, renderBuffer;

WGPU_BOOL raf(double time, void *userData)
{
  wgpu_buffer_map_sync(stagingBuffer, WGPU_MAP_MODE_WRITE, 0, WGPU_MAX_SIZE);

  float scale = (emscripten_math_sin(time * 0.005f) + 1.f) / 2.f + 0.5f;
  float triangle[2*3] = {
    //   x,      y
      0.0f,  scale,
    -scale, -scale,
     scale, -scale
  };

  wgpu_buffer_get_mapped_range(stagingBuffer, 0, WGPU_MAX_SIZE);
  wgpu_buffer_write_mapped_range(stagingBuffer, 0, 0, triangle, sizeof(triangle));
  wgpu_buffer_unmap(stagingBuffer);

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  colorAttachment.view = wgpu_texture_create_view(wgpu_canvas_context_get_current_texture(canvasContext), 0);
  colorAttachment.loadOp = WGPU_LOAD_OP_CLEAR;

  WGpuRenderPassDescriptor passDesc = {
    .numColorAttachments = 1,
    .colorAttachments = &colorAttachment
  };

  wgpu_command_encoder_copy_buffer_to_buffer(encoder, stagingBuffer, 0, renderBuffer, 0, sizeof(triangle));

  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(encoder, &passDesc);
  wgpu_render_pass_encoder_set_pipeline(pass, renderPipeline);
  wgpu_render_pass_encoder_set_vertex_buffer(pass, 0, renderBuffer, 0, WGPU_MAX_SIZE);
  wgpu_render_pass_encoder_draw(pass, 3, 1, 0, 0);
  wgpu_render_pass_encoder_end(pass);

  WGpuCommandBuffer commandBuffer = wgpu_command_encoder_finish(encoder);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), commandBuffer);

  assert(wgpu_get_num_live_objects() < 100); // Check against programming errors from Wasm<->JS WebGPU object leaks
  return EM_TRUE;
}

int main()
{
  WGpuRequestAdapterOptions options = {};
  adapter = navigator_gpu_request_adapter_sync(&options);

  WGpuDeviceDescriptor deviceDesc = {};
  device = wgpu_adapter_request_device_sync(adapter, &deviceDesc);

  canvasContext = wgpu_canvas_get_webgpu_context("canvas");

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(canvasContext, &config);

  WGpuShaderModule vs = wgpu_device_create_shader_module(device, &(WGpuShaderModuleDescriptor) {
    .code = "struct In {\n"
    "  @location(0) pos : vec2<f32>\n"
    "};\n"

    "struct Out {\n"
    "  @builtin(position) pos : vec4<f32>\n"
    "};\n"

    "@vertex\n"
    "fn main(in: In) -> Out {\n"
      "var out: Out;\n"
      "out.pos = vec4<f32>(in.pos, 0.0, 1.0);\n"
      "return out;\n"
    "}\n"
  });

  WGpuShaderModule fs = wgpu_device_create_shader_module(device, &(WGpuShaderModuleDescriptor) {
    .code = "@fragment\n"
    "fn main() -> @location(0) vec4<f32> {\n"
      "return vec4<f32>(0.3, 0.5, 0.8, 1.0);\n"
    "}\n"
  });

  WGpuVertexAttribute vertexAttr[1] = {};
  vertexAttr[0].format = WGPU_VERTEX_FORMAT_FLOAT32X2;
  vertexAttr[0].offset = 0;
  vertexAttr[0].shaderLocation = 0;

  WGpuVertexBufferLayout vbLayout = {};
  vbLayout.numAttributes = 1;
  vbLayout.attributes = vertexAttr;
  vbLayout.arrayStride = 2*sizeof(float);

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = config.format;
  colorTarget.blend.color.operation = WGPU_BLEND_OPERATION_ADD;
  colorTarget.blend.color.dstFactor = WGPU_BLEND_FACTOR_ONE;

  WGpuRenderPipelineDescriptor renderPipelineDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  renderPipelineDesc.vertex.module = vs;
  renderPipelineDesc.vertex.entryPoint = "main";
  renderPipelineDesc.vertex.numBuffers = 1;
  renderPipelineDesc.vertex.buffers = &vbLayout;
  renderPipelineDesc.fragment.module = fs;
  renderPipelineDesc.fragment.entryPoint = "main";
  renderPipelineDesc.fragment.numTargets = 1;
  renderPipelineDesc.fragment.targets = &colorTarget;
  renderPipelineDesc.primitive.topology = WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  renderPipeline = wgpu_device_create_render_pipeline(device, &renderPipelineDesc);

  WGpuBufferDescriptor bufferDesc = {};
  bufferDesc.size = 6*sizeof(float);
  bufferDesc.usage = WGPU_BUFFER_USAGE_COPY_SRC | WGPU_BUFFER_USAGE_MAP_WRITE;
  stagingBuffer = wgpu_device_create_buffer(device, &bufferDesc);

  bufferDesc.size = 6*sizeof(float);
  bufferDesc.usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_VERTEX;
  renderBuffer = wgpu_device_create_buffer(device, &bufferDesc);

  wgpu_request_animation_frame_loop(raf, 0);
}
