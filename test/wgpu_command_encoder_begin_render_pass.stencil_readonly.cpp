// Tests wgpu_command_encoder_begin_render_pass() with stencilReadOnly=true in the
// depth-stencil attachment, exercising that field in wgpuReadRenderPassDepthStencilAttachment.
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
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(0,1,1,1); }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = colorFormat;

  // Pipeline with no depth/stencil writes (required when stencilReadOnly=true)
  WGpuRenderPipelineDescriptor pipeDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  pipeDesc.vertex.module = shader;
  pipeDesc.vertex.entryPoint = "vs";
  pipeDesc.fragment.module = shader;
  pipeDesc.fragment.entryPoint = "fs";
  pipeDesc.fragment.targets = &colorTarget;
  pipeDesc.fragment.numTargets = 1;
  pipeDesc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;
  pipeDesc.depthStencil.format = WGPU_TEXTURE_FORMAT_DEPTH24PLUS_STENCIL8;
  pipeDesc.depthStencil.depthWriteEnabled = WGPU_FALSE;
  pipeDesc.depthStencil.depthCompare = WGPU_COMPARE_FUNCTION_ALWAYS;
  // Stencil ops default to KEEP (no stencil writes needed when stencilReadOnly=true)

  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &pipeDesc);
  assert(pipeline);

  // Combined depth+stencil texture
  WGpuTextureDescriptor dsDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  dsDesc.format = WGPU_TEXTURE_FORMAT_DEPTH24PLUS_STENCIL8;
  dsDesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  dsDesc.width = 64;
  dsDesc.height = 64;
  WGpuTexture dsTex = wgpu_device_create_texture(device, &dsDesc);
  assert(dsTex);

  // Off-screen color target
  WGpuTextureDescriptor colorDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  colorDesc.format = colorFormat;
  colorDesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  colorDesc.width = 64;
  colorDesc.height = 64;
  WGpuTexture colorTex = wgpu_device_create_texture(device, &colorDesc);
  assert(colorTex);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  ca.view = wgpu_texture_create_view_simple(colorTex);

  WGpuRenderPassDepthStencilAttachment dsAttach = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_DEFAULT_INITIALIZER;
  dsAttach.view = dsTex;
  dsAttach.depthLoadOp = WGPU_LOAD_OP_CLEAR;
  dsAttach.depthClearValue = 1.0f;
  dsAttach.depthStoreOp = WGPU_STORE_OP_DISCARD;
  dsAttach.depthReadOnly = WGPU_FALSE;
  dsAttach.stencilLoadOp = WGPU_LOAD_OP_CLEAR;
  dsAttach.stencilClearValue = 0;
  dsAttach.stencilStoreOp = WGPU_STORE_OP_DISCARD;
  dsAttach.stencilReadOnly = WGPU_TRUE; // exercises the stencilReadOnly=true code path

  WGpuRenderPassDescriptor passDesc = {
    .colorAttachments = &ca,
    .numColorAttachments = 1,
    .depthStencilAttachment = dsAttach,
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
