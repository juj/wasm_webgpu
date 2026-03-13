// Verifies that wgpu_render_commands_mixin_set_vertex_buffer() correctly binds a vertex buffer and that subsequent draw() of 3 vertices uses the buffer data through a matching pipeline layout.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

static const char *shaderCode =
  "struct In { @location(0) pos: vec2f };\n"
  "@vertex fn vs(in: In) -> @builtin(position) vec4f { return vec4f(in.pos, 0.0, 1.0); }\n"
  "@fragment fn fs() -> @location(0) vec4f { return vec4f(1,0,0,1); }";

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT format = navigator_gpu_get_preferred_canvas_format();

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = format;
  wgpu_canvas_context_configure(wgpu_canvas_get_webgpu_context("canvas"), &config);

  WGpuShaderModuleDescriptor shaderDesc = { .code = shaderCode };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &shaderDesc);

  WGpuVertexAttribute attr = {};
  attr.format = WGPU_VERTEX_FORMAT_FLOAT32X2;
  attr.offset = 0;
  attr.shaderLocation = 0;

  WGpuVertexBufferLayout vbLayout = {};
  vbLayout.attributes = &attr;
  vbLayout.numAttributes = 1;
  vbLayout.arrayStride = 8;

  WGpuRenderPipelineDescriptor pipeDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  pipeDesc.vertex.module = shader;
  pipeDesc.vertex.entryPoint = "vs";
  pipeDesc.vertex.buffers = &vbLayout;
  pipeDesc.vertex.numBuffers = 1;
  pipeDesc.fragment.module = shader;
  pipeDesc.fragment.entryPoint = "fs";
  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = format;
  pipeDesc.fragment.numTargets = 1;
  pipeDesc.fragment.targets = &colorTarget;
  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &pipeDesc);

  // Create a vertex buffer with 3 vertices (triangle)
  float verts[] = { 0.0f, 0.5f, -0.5f, -0.5f, 0.5f, -0.5f };
  WGpuBufferDescriptor vbDesc = {
    .size = sizeof(verts),
    .usage = WGPU_BUFFER_USAGE_VERTEX,
    .mappedAtCreation = WGPU_TRUE,
  };
  WGpuBuffer vertexBuf = wgpu_device_create_buffer(device, &vbDesc);
  wgpu_buffer_get_mapped_range(vertexBuf, 0);
  wgpu_buffer_write_mapped_range(vertexBuf, 0, 0, verts, sizeof(verts));
  wgpu_buffer_unmap(vertexBuf);

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  colorAttachment.view = wgpu_canvas_context_get_current_texture(wgpu_canvas_get_webgpu_context("canvas"));
  WGpuRenderPassDescriptor passDesc = {
    .colorAttachments = &colorAttachment,
    .numColorAttachments = 1,
  };
  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(encoder, &passDesc);

  wgpu_render_commands_mixin_set_pipeline(pass, pipeline);
  wgpu_render_commands_mixin_set_vertex_buffer(pass, 0, vertexBuf, 0, sizeof(verts));
  wgpu_render_commands_mixin_draw(pass, 3, 1, 0, 0);

  wgpu_render_pass_encoder_end(pass);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(encoder));

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
