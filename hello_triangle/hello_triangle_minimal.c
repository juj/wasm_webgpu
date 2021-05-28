#include <assert.h>
#include <stdio.h>
#include "lib_webgpu.h"

WGpuAdapter adapter;
WGpuSwapChain swapChain;
WGpuDevice device;
WGpuQueue defaultQueue;
WGpuRenderPipeline renderPipeline;

EM_BOOL raf(double time, void *userData)
{
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment colorAttachment = {};
  colorAttachment.view = wgpu_texture_create_view(wgpu_swap_chain_get_current_texture(swapChain), 0);
  colorAttachment.loadColor.a = 1.0;

  WGpuRenderPassDescriptor passDesc = {};
  passDesc.numColorAttachments = 1;
  passDesc.colorAttachments = &colorAttachment;

  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(encoder, &passDesc);
  wgpu_render_pass_encoder_set_pipeline(pass, renderPipeline);
  wgpu_render_pass_encoder_draw(pass, 3, 1, 0, 0);
  wgpu_render_pass_encoder_end_pass(pass);

  WGpuCommandBuffer commandBuffer = wgpu_command_encoder_finish(encoder);

  wgpu_queue_submit_one_and_destroy(defaultQueue, commandBuffer);

  return EM_FALSE; // Render just one frame, static content
}

void ObtainedWebGpuDevice(WGpuDevice result, void *userData)
{
  device = result;
  defaultQueue = wgpu_device_get_queue(device);

  WGpuCanvasContext canvasContext = wgpu_canvas_get_canvas_context("canvas");

  WGpuSwapChainDescriptor swapChainDesc = WGPU_SWAP_CHAIN_DESCRIPTOR_DEFAULT_INITIALIZER;
  swapChainDesc.device = device;
  swapChainDesc.format = wgpu_canvas_context_get_swap_chain_preferred_format(canvasContext, adapter);

  swapChain = wgpu_canvas_context_configure_swap_chain(canvasContext, &swapChainDesc);

  const char *vertexShader =
    "const pos : array<vec2<f32>, 3> = array<vec2<f32>, 3>("
      "vec2<f32>(0.0, 0.5),"
      "vec2<f32>(-0.5, -0.5),"
      "vec2<f32>(0.5, -0.5)"
    ");"

    "[[builtin(position)]] var<out> Position : vec4<f32>;"
    "[[builtin(vertex_index)]] var<in> VertexIndex : i32;"

    "[[stage(vertex)]]"
    "fn main() -> void {"
      "Position = vec4<f32>(pos[VertexIndex], 0.0, 1.0);"
    "}";

  const char *fragmentShader =
    "[[location(0)]] var<out> outColor : vec4<f32>;"
    "[[stage(fragment)]]"
    "fn main() -> void {"
      "outColor = vec4<f32>(1.0, 0.0, 0.0, 1.0);"
    "}";

  WGpuShaderModuleDescriptor shaderModuleDesc = {};
  shaderModuleDesc.code = vertexShader;
  WGpuShaderModule vs = wgpu_device_create_shader_module(device, &shaderModuleDesc);

  shaderModuleDesc.code = fragmentShader;
  WGpuShaderModule fs = wgpu_device_create_shader_module(device, &shaderModuleDesc);

  WGpuRenderPipelineDescriptor renderPipelineDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  renderPipelineDesc.vertex.module = vs;
  renderPipelineDesc.vertex.entryPoint = "main";
  renderPipelineDesc.fragment.module = fs;
  renderPipelineDesc.fragment.entryPoint = "main";

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = swapChainDesc.format;
  renderPipelineDesc.fragment.numTargets = 1;
  renderPipelineDesc.fragment.targets = &colorTarget;

  renderPipeline = wgpu_device_create_render_pipeline(device, &renderPipelineDesc);

  emscripten_request_animation_frame_loop(raf, 0);
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
