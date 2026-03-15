// Tests creating a render pipeline with no fragment stage (depth-only / shadow
// map pipeline), exercising the 'fragment': fragmentModule ? {...} : void 0
// code path in wgpuReadRenderPipelineDescriptor where fragmentModule == 0.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  var pos = array<vec2f,3>(vec2f(0,1),vec2f(-1,-1),vec2f(1,-1));"
            "  return vec4f(pos[vi], 0.5, 1);"
            "}",
  };
  WGpuShaderModule vs = wgpu_device_create_shader_module(device, &smdesc);
  assert(vs);

  // Depth-only render pipeline: no fragment module set.
  WGpuRenderPipelineDescriptor desc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.vertex.module = vs;
  desc.vertex.entryPoint = "vs";
  desc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

  // Depth attachment required when there is no color target
  desc.depthStencil.format = WGPU_TEXTURE_FORMAT_DEPTH32FLOAT;
  desc.depthStencil.depthWriteEnabled = WGPU_TRUE;
  desc.depthStencil.depthCompare = WGPU_COMPARE_FUNCTION_LESS;

  // No fragment stage: fragment.module == 0 (default from DEFAULT_INITIALIZER)
  // fragment.numTargets == 0 so no color targets either.

  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &desc);
  assert(pipeline);
  assert(wgpu_is_render_pipeline(pipeline));

  // Exercise the pipeline with a depth-only render pass
  WGpuTextureDescriptor depthDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  depthDesc.format = WGPU_TEXTURE_FORMAT_DEPTH32FLOAT;
  depthDesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  depthDesc.width = 64;
  depthDesc.height = 64;
  WGpuTexture depthTex = wgpu_device_create_texture(device, &depthDesc);
  assert(depthTex);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassDepthStencilAttachment depthAttach = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_DEFAULT_INITIALIZER;
  depthAttach.view = depthTex;
  depthAttach.depthLoadOp = WGPU_LOAD_OP_CLEAR;
  depthAttach.depthClearValue = 1.0f;
  depthAttach.depthStoreOp = WGPU_STORE_OP_STORE;

  WGpuRenderPassDescriptor passDesc = {
    .numColorAttachments = 0,
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
