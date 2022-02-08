#include <assert.h>
#include <stdio.h>
#include "lib_webgpu.h"
#include "lib_demo.h"
#include <miniprintf.h>

WGpuAdapter adapter;
WGpuCanvasContext canvasContext;
WGpuDevice device;
WGpuQueue queue;
WGpuRenderPipeline renderPipeline;
WGpuBuffer buffer = 0;
WGpuTexture texture;
int imageWidth, imageHeight;
WGpuSampler sampler;
WGpuBindGroup bindGroup;

float v[] = {
  // x,    y,   u,   v
  -1.f, -1.f, 0.f, 0.f,
   1.f, -1.f, 1.f, 0.f,
  -1.f,  1.f, 0.f, 1.f,
  -1.f,  1.f, 0.f, 1.f,
   1.f, -1.f, 1.f, 0.f,
   1.f,  1.f, 1.f, 1.f
};  

void CreateGeometryAndRender()
{
  WGpuBufferDescriptor bufferDesc = {};
  bufferDesc.size = sizeof(v);
  bufferDesc.usage = WGPU_BUFFER_USAGE_VERTEX;
  bufferDesc.mappedAtCreation = EM_TRUE;

  int canvasWidth, canvasHeight;
  emscripten_get_canvas_element_size("canvas", &canvasWidth, &canvasHeight);

  float scaleX = (float)canvasWidth/imageWidth;
  float scaleY = (float)canvasHeight/imageHeight;
  float scale = scaleY < scaleX ? scaleY : scaleX;
  float letterbox = imageWidth * scale / canvasWidth;
  float pillarbox = imageHeight * scale / canvasHeight;
  v[0] = v[8] = v[12] = -letterbox;
  v[4] = v[16] = v[20] = letterbox;
  v[1] = v[5] = v[17] = -pillarbox;
  v[9] = v[13] = v[21] = pillarbox;

  wgpu_object_destroy(buffer);
  buffer = wgpu_device_create_buffer(device, &bufferDesc);
  wgpu_buffer_get_mapped_range(buffer, 0, WGPU_MAP_MAX_LENGTH);
  wgpu_buffer_write_mapped_range(buffer, 0, 0, v, sizeof(v));
  wgpu_buffer_unmap(buffer);

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  colorAttachment.view = wgpu_texture_create_view(wgpu_canvas_context_get_current_texture(canvasContext), 0);

  WGpuRenderPassDescriptor passDesc = {};
  passDesc.numColorAttachments = 1;
  passDesc.colorAttachments = &colorAttachment;

  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(encoder, &passDesc);
  wgpu_render_pass_encoder_set_pipeline(pass, renderPipeline);
  wgpu_render_pass_encoder_set_vertex_buffer(pass, 0, buffer, 0, sizeof(v));
  wgpu_render_pass_encoder_set_bind_group(pass, 0, bindGroup, 0, 0);
  wgpu_render_pass_encoder_draw(pass, 6, 1, 0, 0);
  wgpu_render_pass_encoder_end(pass);

  wgpu_queue_submit_one_and_destroy(queue, wgpu_command_encoder_finish(encoder));

  assert(wgpu_get_num_live_objects() < 100); // Check against programming errors from Wasm<->JS WebGPU object leaks
}

void DownloadedImage(WGpuImageBitmap bitmap, int width, int height, void *userData)
{
  imageWidth = width;
  imageHeight = height;
  emscripten_mini_stdio_printf("DownloadedImage: width: %d, height: %d\n", width, height);
  if (!width)
    return;

  queue = wgpu_device_get_queue(device);

  canvasContext = wgpu_canvas_get_webgpu_context("canvas");

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = wgpu_canvas_context_get_preferred_format(canvasContext, adapter);
  wgpu_canvas_context_configure(canvasContext, &config);

  const char *vertexShader =
    "struct In {"
    "  [[location(0)]] pos : vec2<f32>;"
    "  [[location(1)]] uv : vec2<f32>;"
    "};"

    "struct Out {"
    "  [[builtin(position)]] pos : vec4<f32>;"
    "  [[location(0)]] uv : vec2<f32>;"
    "};"

    "[[stage(vertex)]]"
    "fn main(in: In) -> Out {"
      "var out: Out;"
      "out.pos = vec4<f32>(in.pos, 0.0, 1.0);"
      "out.uv = in.uv;"
      "return out;"
    "}";

  const char *fragmentShader =
    "[[group(0), binding(0)]] var myTexture : texture_2d<f32>;\n"
    "[[group(0), binding(1)]] var mySampler : sampler;\n"
    "[[stage(fragment)]]\n"
    "fn main([[location(0)]] uv : vec2<f32>) -> [[location(0)]] vec4<f32> {\n"
      "return textureSample(myTexture, mySampler, uv);"
    "}";

  WGpuRenderPipelineDescriptor renderPipelineDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;

  WGpuVertexAttribute vertexAttr[2] = {};
  vertexAttr[0].format = WGPU_VERTEX_FORMAT_FLOAT32X2;
  vertexAttr[0].offset = 0;
  vertexAttr[0].shaderLocation = 0;
  vertexAttr[1].format = WGPU_VERTEX_FORMAT_FLOAT32X2;
  vertexAttr[1].offset = 8;
  vertexAttr[1].shaderLocation = 1;

  WGpuVertexBufferLayout vbLayout = {};
  vbLayout.numAttributes = 2;
  vbLayout.attributes = vertexAttr;
  vbLayout.arrayStride = 16;

  renderPipelineDesc.vertex.numBuffers = 1;
  renderPipelineDesc.vertex.buffers = &vbLayout;

  WGpuShaderModuleDescriptor shaderModuleDesc = {};
  shaderModuleDesc.code = vertexShader;
  renderPipelineDesc.vertex.module = wgpu_device_create_shader_module(device, &shaderModuleDesc);
  renderPipelineDesc.vertex.entryPoint = "main";

  shaderModuleDesc.code = fragmentShader;
  renderPipelineDesc.fragment.module = wgpu_device_create_shader_module(device, &shaderModuleDesc);
  renderPipelineDesc.fragment.entryPoint = "main";

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = config.format;
  renderPipelineDesc.fragment.numTargets = 1;
  renderPipelineDesc.fragment.targets = &colorTarget;

  renderPipeline = wgpu_device_create_render_pipeline(device, &renderPipelineDesc);

  WGpuTextureDescriptor textureDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  textureDesc.width = width;
  textureDesc.height = height;
  textureDesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  textureDesc.usage = WGPU_TEXTURE_USAGE_COPY_DST | WGPU_TEXTURE_USAGE_TEXTURE_BINDING | WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;

  texture = wgpu_device_create_texture(device, &textureDesc);

  WGpuImageCopyExternalImage src = {};
  src.source = bitmap;

  WGpuImageCopyTextureTagged dst = {};
  dst.texture = texture;

  wgpu_queue_copy_external_image_to_texture(queue, &src, &dst, width, height, 1);

  WGpuSamplerDescriptor samplerDesc = WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER;
  sampler = wgpu_device_create_sampler(device, &samplerDesc);

  WGpuBindGroupEntry bindGroupEntries[2] = {};
  bindGroupEntries[0].binding = 0;
  bindGroupEntries[0].resource = wgpu_texture_create_view(texture, 0);
  bindGroupEntries[1].binding = 1;
  bindGroupEntries[1].resource = sampler;

  bindGroup = wgpu_device_create_bind_group(device, wgpu_pipeline_get_bind_group_layout(renderPipeline, 0), bindGroupEntries, 2);

  window_resized_callback(CreateGeometryAndRender);
  CreateGeometryAndRender();
}

void ObtainedWebGpuDevice(WGpuDevice result, void *userData)
{
  device = result;
  wgpu_load_image_bitmap_from_url_async("fish.png", EM_TRUE, DownloadedImage, 0);
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
  options.powerPreference = WGPU_POWER_PREFERENCE_LOW_POWER;
  navigator_gpu_request_adapter_async(&options, ObtainedWebGpuAdapter, 0);
}
