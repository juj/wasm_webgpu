// Tests creating a render pipeline with unclippedDepth = true,
// exercising the '!!HEAPU32[primitiveIdx+4]' code path in wgpuReadRenderPipelineDescriptor.
// Requires WGPU_FEATURE_DEPTH_CLIP_CONTROL; test skips gracefully if not supported.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  if (!wgpu_device_supports_feature(device, WGPU_FEATURE_DEPTH_CLIP_CONTROL))
  {
    printf("WGPU_FEATURE_DEPTH_CLIP_CONTROL not supported, skipping.\n");
    EM_ASM(window.close());
    return;
  }

  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuShaderModuleDescriptor smdesc = {
    // Depth > 1 should normally be clipped, but with unclippedDepth=true it's not.
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  var pos = array<vec2f,3>(vec2f(0,1),vec2f(-1,-1),vec2f(1,-1));"
            "  return vec4f(pos[vi], 2.0, 1);" // depth = 2.0 (outside [0,1])
            "}"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(0,1,0,1); }",
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
  desc.primitive.topology = WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  desc.primitive.unclippedDepth = WGPU_TRUE; // exercises !!HEAPU32[primitiveIdx+4] == 1

  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &desc);
  assert(pipeline);
  assert(wgpu_is_render_pipeline(pipeline));

  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  WGpuCanvasConfiguration cfg = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  cfg.device = device;
  cfg.format = colorFormat;
  wgpu_canvas_context_configure(ctx, &cfg);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);
  WGpuRenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  ca.view = wgpu_canvas_context_get_current_texture(ctx);
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
  // Request device with depth clip control
  WGpuDeviceDescriptor desc = {};
  desc.requiredFeatures = WGPU_FEATURE_DEPTH_CLIP_CONTROL;
  wgpu_adapter_request_device_async(adapter, &desc, ObtainedWebGpuDevice, 0);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
