// Verifies that wgpu_render_commands_mixin_draw_indirect() draws a triangle using vertex counts read from an indirect buffer without crashing.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

static const char *shaderCode =
  "@vertex fn vs(@builtin(vertex_index) i: u32) -> @builtin(position) vec4f { return vec4f(0,0,0,1); }\n"
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

  WGpuRenderPipelineDescriptor pipeDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  pipeDesc.vertex.module = shader;
  pipeDesc.vertex.entryPoint = "vs";
  pipeDesc.fragment.module = shader;
  pipeDesc.fragment.entryPoint = "fs";
  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = format;
  pipeDesc.fragment.numTargets = 1;
  pipeDesc.fragment.targets = &colorTarget;
  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &pipeDesc);

  // Indirect buffer: [vertexCount=3, instanceCount=1, firstVertex=0, firstInstance=0]
  uint32_t indirectArgs[] = { 3, 1, 0, 0 };
  WGpuBufferDescriptor ibDesc = {
    .size = sizeof(indirectArgs),
    .usage = WGPU_BUFFER_USAGE_INDIRECT,
    .mappedAtCreation = WGPU_TRUE,
  };
  WGpuBuffer indirectBuf = wgpu_device_create_buffer(device, &ibDesc);
  wgpu_buffer_get_mapped_range(indirectBuf, 0);
  wgpu_buffer_write_mapped_range(indirectBuf, 0, 0, indirectArgs, sizeof(indirectArgs));
  wgpu_buffer_unmap(indirectBuf);

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  colorAttachment.view = wgpu_canvas_context_get_current_texture(wgpu_canvas_get_webgpu_context("canvas"));
  WGpuRenderPassDescriptor passDesc = {
    .colorAttachments = &colorAttachment,
    .numColorAttachments = 1,
  };
  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(encoder, &passDesc);

  wgpu_render_commands_mixin_set_pipeline(pass, pipeline);
  wgpu_render_commands_mixin_draw_indirect(pass, indirectBuf, 0);

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
