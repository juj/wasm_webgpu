// Tests wgpu_command_encoder_begin_render_pass() with loadOp='load' on the
// color attachment, exercising the WGPU_LOAD_OP_LOAD code path.
// The first pass clears to a known color; the second pass uses loadOp=load to
// preserve that content (exercises the non-clear loadOp branch).
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  var pos = array<vec2f,3>(vec2f(0,1),vec2f(-1,-1),vec2f(1,-1));"
            "  return vec4f(pos[vi], 0, 1);"
            "}"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(0,1,0,1); }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = colorFormat;

  WGpuRenderPipelineDescriptor pipeDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  pipeDesc.vertex.module = shader;
  pipeDesc.vertex.entryPoint = "vs";
  pipeDesc.fragment.module = shader;
  pipeDesc.fragment.entryPoint = "fs";
  pipeDesc.fragment.targets = &colorTarget;
  pipeDesc.fragment.numTargets = 1;
  pipeDesc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &pipeDesc);
  assert(pipeline);

  // Render to an off-screen texture (first pass clears, second pass loads)
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = colorFormat;
  tdesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  tdesc.width = 64;
  tdesc.height = 64;
  WGpuTexture tex = wgpu_device_create_texture(device, &tdesc);
  assert(tex);
  WGpuTextureView view = wgpu_texture_create_view_simple(tex);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  // First pass: clear
  WGpuRenderPassColorAttachment ca1 = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  ca1.view = view;
  ca1.loadOp = WGPU_LOAD_OP_CLEAR;
  ca1.clearValue = (WGpuColor){ .r = 1, .g = 0, .b = 0, .a = 1 };
  ca1.storeOp = WGPU_STORE_OP_STORE;
  WGpuRenderPassDescriptor pass1Desc = { .colorAttachments = &ca1, .numColorAttachments = 1 };
  WGpuRenderPassEncoder pass1 = wgpu_command_encoder_begin_render_pass(enc, &pass1Desc);
  wgpu_render_pass_encoder_end(pass1);

  // Second pass: load (exercises WGPU_LOAD_OP_LOAD code path)
  WGpuRenderPassColorAttachment ca2 = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  ca2.view = view;
  ca2.loadOp = WGPU_LOAD_OP_LOAD; // preserves the cleared content from first pass
  ca2.storeOp = WGPU_STORE_OP_STORE;
  WGpuRenderPassDescriptor pass2Desc = { .colorAttachments = &ca2, .numColorAttachments = 1 };
  WGpuRenderPassEncoder pass2 = wgpu_command_encoder_begin_render_pass(enc, &pass2Desc);
  wgpu_encoder_set_pipeline(pass2, pipeline);
  wgpu_render_commands_mixin_draw(pass2, 3, 1, 0, 0);
  wgpu_render_pass_encoder_end(pass2);

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
