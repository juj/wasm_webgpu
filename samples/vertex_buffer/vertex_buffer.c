#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <emscripten/em_math.h>
#include "lib_webgpu.h"
#include "lib_demo.h"

#define RECURSION_LIMIT 7

uint64_t numVertices = 0;
uint8_t *bufferData = 0;

WGpuAdapter adapter;
WGpuCanvasContext canvasContext;
WGpuDevice device;
WGpuRenderPipeline renderPipeline;
WGpuBuffer buffer;

typedef struct float2
{
  float x,y;
} float2;

typedef struct vertex
{
  float2 pos;
  float color;
} vertex;

float min(float a, float b, float c)
{
  return a < b && a < c ? a : (b < c ? b : c);
}

float max(float a, float b, float c)
{
  return a > b && a > c ? a : (b > c ? b : c);
}

float2 avg(const float2 *v0, const float2 *v1)
{
  float2 ret = {
    (v0->x + v1->x) * 0.5f,
    (v0->y + v1->y) * 0.5f
  };
  return ret;
}

float2 avg8(const float2 *v0, const float2 *v1)
{
  float2 ret = {
    v0->x * 0.8f + v1->x * 0.2f,
    v0->y * 0.8f + v1->y * 0.2f
  };
  return ret;
}

void divide(const float2 *v0, const float2 *v1, const float2 *v2, int recursionLimit)
{
  if (min(v0->x, v1->x, v2->x) > 1.f) return;
  if (min(v0->y, v1->y, v2->y) > 1.f) return;
  if (max(v0->x, v1->x, v2->x) < -1.f) return;
  if (max(v0->y, v1->y, v2->y) < -1.f) return;

  float2 w1 = avg(v0, v2);
  float2 w2 = avg8(v1, v2);
  float2 w0 = avg(v2, &w2);
  float2 w3 = avg(v0, &w2);

#define COLOR(z) ((recursionLimit == 3 ? 0.7f : 0.4f) * ((z).y*-1.f + 0.3f + (z).x + emscripten_math_sin((z).x*5.f)*0.3f))

  vertex data[] = {
    { *v0, COLOR(*v0) },
    { w2, COLOR(w2) },
    { w2, COLOR(w2) },
    { w1, COLOR(w1) },
    { w1, COLOR(w1) },
    { w0, COLOR(w0) },
    { w1, COLOR(w1) },
    { w3, COLOR(w3) }
  };
  wgpu_buffer_write_mapped_range(buffer, 0, numVertices*sizeof(vertex), data, sizeof(data));
  numVertices += 8;

  if (--recursionLimit > 0)
  {
    divide(&w2, v1, v0, recursionLimit);
    divide(&w3, v0, &w1, recursionLimit);
    divide(&w3, &w2, &w1, recursionLimit);
    divide(&w0, &w1, &w2, recursionLimit);
    divide(&w0, &w1, v2, recursionLimit);
  }
}

void Render()
{
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  colorAttachment.view = wgpu_texture_create_view(wgpu_canvas_context_get_current_texture(canvasContext), 0);
  colorAttachment.loadOp = WGPU_LOAD_OP_CLEAR;

  WGpuRenderPassDescriptor passDesc = {};
  passDesc.numColorAttachments = 1;
  passDesc.colorAttachments = &colorAttachment;

  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(encoder, &passDesc);
  wgpu_render_pass_encoder_set_pipeline(pass, renderPipeline);
  wgpu_render_pass_encoder_set_vertex_buffer(pass, 0, buffer, 0, numVertices*sizeof(vertex));
  wgpu_render_pass_encoder_draw(pass, numVertices, 1, 0, 0);
  wgpu_render_pass_encoder_end(pass);

  WGpuCommandBuffer commandBuffer = wgpu_command_encoder_finish(encoder);

  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), commandBuffer);

  assert(wgpu_get_num_live_objects() < 100); // Check against programming errors from Wasm<->JS WebGPU object leaks
}

void CreateGeometryAndRender()
{
// Upper limit of num vertices written = 8*(1 + 5 + 5^2 + 5^3 + ... + 5^recursionLimit) = 2 * 5^r - 2
#define MAX_VERTICES (2 * (uint64_t)pow(5, RECURSION_LIMIT) - 2)

  bufferData = (uint8_t*)malloc(MAX_VERTICES * sizeof(vertex));
  numVertices = 0;

  float2 v[3] = {
    { -4.f, -4.0f },
    { -4.f,  4.0f },
    { 12.f, -4.0f },
  };

  WGpuBufferDescriptor bufferDesc = {};
  bufferDesc.size = MAX_VERTICES * sizeof(vertex);
  bufferDesc.usage = WGPU_BUFFER_USAGE_VERTEX;
  bufferDesc.mappedAtCreation = WGPU_TRUE;

  wgpu_object_destroy(buffer);
  buffer = wgpu_device_create_buffer(device, &bufferDesc);
  wgpu_buffer_get_mapped_range(buffer, 0, WGPU_MAX_SIZE);

  int w, h;
  emscripten_get_canvas_element_size("canvas", &w, &h);
  float viewportXScale = (float)h/w;
  for(int i = 0; i < 3; ++i)
    v[i].x *= viewportXScale;
  divide(&v[0], &v[1], &v[2], RECURSION_LIMIT);

  wgpu_buffer_unmap(buffer);

  Render();
}

void ObtainedWebGpuDevice(WGpuDevice result, void *userData)
{
  device = result;
  assert(device);

  canvasContext = wgpu_canvas_get_webgpu_context("canvas");

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(canvasContext, &config);

  WGpuShaderModule vs = wgpu_device_create_shader_module(device, &(WGpuShaderModuleDescriptor) {
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
    "}\n"
  });

  WGpuShaderModule fs = wgpu_device_create_shader_module(device, &(WGpuShaderModuleDescriptor) {
    .code = "@fragment\n"
    "fn main(@location(0) inColor : f32) -> @location(0) vec4<f32> {\n"
      "return vec4<f32>(inColor, inColor, abs(inColor), 1.0);\n"
    "}\n"
  });

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
  colorTarget.format = config.format;
  colorTarget.blend.color.operation = WGPU_BLEND_OPERATION_ADD;
  colorTarget.blend.color.dstFactor = WGPU_BLEND_FACTOR_ONE;
  renderPipelineDesc.fragment.numTargets = 1;
  renderPipelineDesc.fragment.targets = &colorTarget;

  renderPipelineDesc.primitive.topology = WGPU_PRIMITIVE_TOPOLOGY_LINE_LIST;

  renderPipeline = wgpu_device_create_render_pipeline(device, &renderPipelineDesc);

  window_resized_callback(CreateGeometryAndRender);
  CreateGeometryAndRender();
}

void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData)
{
  adapter = result;

  WGpuDeviceDescriptor deviceDesc = {};
  wgpu_adapter_request_device_async(adapter, &deviceDesc, ObtainedWebGpuDevice, 0);
}

int main()
{
  WGpuRequestAdapterOptions options = {};
  navigator_gpu_request_adapter_async(&options, ObtainedWebGpuAdapter, 0);
}
