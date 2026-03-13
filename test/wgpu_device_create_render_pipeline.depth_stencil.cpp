// Verifies that a render pipeline with a DEPTH32FLOAT depth/stencil state (depth write enabled, LESS compare) can be created and used in a render pass with a depth attachment.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  var pos = array<vec2f,3>(vec2f(0,1),vec2f(-1,-1),vec2f(1,-1));"
            "  return vec4f(pos[vi], 0.5, 1);"
            "}"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(1,0,0,1); }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = colorFormat;

  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = shader;
  desc.vertex.entryPoint = "vs";

  desc.fragment.module = shader;
  desc.fragment.entryPoint = "fs";
  desc.fragment.targets = &colorTarget;
  desc.fragment.numTargets = 1;

  desc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

  // Set up depth/stencil state: DEPTH32FLOAT, depth write enabled, LESS compare
  desc.depthStencil.format = WGPU_TEXTURE_FORMAT_DEPTH32FLOAT;
  desc.depthStencil.depthWriteEnabled = WGPU_TRUE;
  desc.depthStencil.depthCompare = WGPU_COMPARE_FUNCTION_LESS;
  desc.depthStencil.stencilReadMask = 0xFF;
  desc.depthStencil.stencilWriteMask = 0xFF;
  desc.depthStencil.depthBias = 0;
  desc.depthStencil.depthBiasSlopeScale = 0.0f;
  desc.depthStencil.depthBiasClamp = 0.0f;
  desc.depthStencil.stencilFront = (WGpuStencilFaceState){
    .compare = WGPU_COMPARE_FUNCTION_ALWAYS,
    .failOp = WGPU_STENCIL_OPERATION_KEEP,
    .depthFailOp = WGPU_STENCIL_OPERATION_KEEP,
    .passOp = WGPU_STENCIL_OPERATION_KEEP,
  };
  desc.depthStencil.stencilBack = desc.depthStencil.stencilFront;

  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &desc);
  assert(pipeline);
  assert(wgpu_is_render_pipeline(pipeline));

  // Create a color attachment and depth attachment to use the pipeline
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  WGpuCanvasConfiguration cfg = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  cfg.device = device;
  cfg.format = colorFormat;
  wgpu_canvas_context_configure(ctx, &cfg);

  WGpuTextureDescriptor depthDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  depthDesc.format = WGPU_TEXTURE_FORMAT_DEPTH32FLOAT;
  depthDesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  depthDesc.width = 300;
  depthDesc.height = 150;
  WGpuTexture depthTex = wgpu_device_create_texture(device, &depthDesc);
  assert(depthTex);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment colorAttach = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  colorAttach.view = wgpu_canvas_context_get_current_texture(ctx);

  WGpuRenderPassDepthStencilAttachment depthAttach = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_DEFAULT_INITIALIZER;
  depthAttach.view = depthTex;
  depthAttach.depthLoadOp = WGPU_LOAD_OP_CLEAR;
  depthAttach.depthClearValue = 1.0f;
  depthAttach.depthStoreOp = WGPU_STORE_OP_STORE;

  WGpuRenderPassDescriptor passDesc = {
    .colorAttachments = &colorAttach,
    .numColorAttachments = 1,
    .depthStencilAttachment = depthAttach,
  };

  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(enc, &passDesc);
  wgpu_encoder_set_pipeline(pass, pipeline);
  wgpu_render_commands_mixin_draw(pass, 3, 1, 0, 0);
  wgpu_render_pass_encoder_end(pass);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));

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
