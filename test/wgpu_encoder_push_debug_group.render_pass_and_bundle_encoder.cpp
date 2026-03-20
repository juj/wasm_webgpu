// Verifies that wgpu_encoder_push_debug_group(), wgpu_encoder_insert_debug_marker(),
// and wgpu_encoder_pop_debug_group() can be called on a GPURenderPassEncoder and
// a GPURenderBundleEncoder. The existing wgpu_encoder_push_debug_group.cpp only
// exercises GPUCommandEncoder and GPUComputePassEncoder; this test covers the
// remaining two encoder types accepted by the JS implementation.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  return vec4f(0,0,0,1);"
            "}"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(0,0,0,1); }",
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

  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = colorFormat;
  tdesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  tdesc.width = 64;
  tdesc.height = 64;
  WGpuTexture tex = wgpu_device_create_texture(device, &tdesc);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  // --- Test debug functions on a GPURenderPassEncoder ---
  WGpuRenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  ca.view = wgpu_texture_create_view_simple(tex);
  WGpuRenderPassDescriptor passDesc = { .colorAttachments = &ca, .numColorAttachments = 1 };
  WGpuRenderPassEncoder renderPass = wgpu_command_encoder_begin_render_pass(enc, &passDesc);
  wgpu_encoder_push_debug_group(renderPass, "RenderPassGroup");
  wgpu_encoder_insert_debug_marker(renderPass, "RenderPassMarker");
  wgpu_encoder_pop_debug_group(renderPass);
  wgpu_render_pass_encoder_end(renderPass);

  // --- Test debug functions on a GPURenderBundleEncoder ---
  WGPU_TEXTURE_FORMAT bundleColorFormat = colorFormat;
  WGpuRenderBundleEncoderDescriptor bundleDesc = WGPU_RENDER_BUNDLE_ENCODER_DESCRIPTOR_DEFAULT_INITIALIZER;
  bundleDesc.colorFormats = &bundleColorFormat;
  bundleDesc.numColorFormats = 1;
  WGpuRenderBundleEncoder bundleEncoder = wgpu_device_create_render_bundle_encoder(device, &bundleDesc);
  assert(bundleEncoder);
  wgpu_encoder_push_debug_group(bundleEncoder, "BundleGroup");
  wgpu_encoder_insert_debug_marker(bundleEncoder, "BundleMarker");
  wgpu_encoder_pop_debug_group(bundleEncoder);
  wgpu_encoder_finish(bundleEncoder); // produces a GPURenderBundle; discard result

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
