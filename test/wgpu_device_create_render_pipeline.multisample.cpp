// Tests creating a render pipeline with multisample state (count=4,
// alphaToCoverageEnabled), exercising the 'multisample': multisampleCount ? {...}
// code path in wgpuReadRenderPipelineDescriptor.
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

  // Set multisample state: count=4, mask=0xFFFFFFFF, alphaToCoverageEnabled
  desc.multisample.count = 4;
  desc.multisample.mask = 0xFFFFFFFF;
  desc.multisample.alphaToCoverageEnabled = WGPU_TRUE;

  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &desc);
  assert(pipeline);
  assert(wgpu_is_render_pipeline(pipeline));

  // Exercise the pipeline with an MSAA render pass
  WGpuTextureDescriptor msaaDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  msaaDesc.format = colorFormat;
  msaaDesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  msaaDesc.width = 64;
  msaaDesc.height = 64;
  msaaDesc.sampleCount = 4;
  WGpuTexture msaaTex = wgpu_device_create_texture(device, &msaaDesc);
  assert(msaaTex);

  // Resolve target (single-sample)
  WGpuTextureDescriptor resolveDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  resolveDesc.format = colorFormat;
  resolveDesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT | WGPU_TEXTURE_USAGE_COPY_SRC;
  resolveDesc.width = 64;
  resolveDesc.height = 64;
  WGpuTexture resolveTex = wgpu_device_create_texture(device, &resolveDesc);
  assert(resolveTex);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);
  WGpuRenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  ca.view = wgpu_texture_create_view_simple(msaaTex);
  ca.resolveTarget = wgpu_texture_create_view_simple(resolveTex);
  WGpuRenderPassDescriptor passDesc = { .colorAttachments = &ca, .numColorAttachments = 1 };
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
