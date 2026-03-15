// Tests wgpu_command_encoder_begin_render_pass() with partial timestamp writes
// (only the end-of-pass timestamp is written, beginning index set to -1).
// This exercises the 'if ((i = HEAP32[...+1]) >= 0)' false branch in
// wgpuReadTimestampWrites where -1 means "do not write this timestamp".
// Requires WGPU_FEATURE_TIMESTAMP_QUERY; skipped gracefully if not supported.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  if (!wgpu_device_supports_feature(device, WGPU_FEATURE_TIMESTAMP_QUERY))
  {
    printf("WGPU_FEATURE_TIMESTAMP_QUERY not supported, skipping.\n");
    EM_ASM(window.close());
    return;
  }

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

  WGpuQuerySetDescriptor qsDesc = {};
  qsDesc.type = WGPU_QUERY_TYPE_TIMESTAMP;
  qsDesc.count = 2;
  WGpuQuerySet querySet = wgpu_device_create_query_set(device, &qsDesc);
  assert(querySet);

  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  WGpuCanvasConfiguration cfg = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  cfg.device = device;
  cfg.format = colorFormat;
  wgpu_canvas_context_configure(ctx, &cfg);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);
  WGpuRenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  ca.view = wgpu_canvas_context_get_current_texture(ctx);

  WGpuRenderPassDescriptor passDesc = WGPU_RENDER_PASS_DESCRIPTOR_DEFAULT_INITIALIZER;
  passDesc.colorAttachments = &ca;
  passDesc.numColorAttachments = 1;

  // beginningOfPassWriteIndex = -1 (skip), endOfPassWriteIndex = 0
  // This exercises the 'HEAP32[...+1] >= 0' FALSE branch for the beginning write.
  passDesc.timestampWrites.querySet = querySet;
  passDesc.timestampWrites.beginningOfPassWriteIndex = -1; // NOT written
  passDesc.timestampWrites.endOfPassWriteIndex = 0;        // written at end

  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(enc, &passDesc);
  wgpu_encoder_set_pipeline(pass, pipeline);
  wgpu_render_commands_mixin_draw(pass, 3, 1, 0, 0);
  wgpu_render_pass_encoder_end(pass);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGpuDeviceDescriptor desc = {};
  desc.requiredFeatures = WGPU_FEATURE_TIMESTAMP_QUERY;
  wgpu_adapter_request_device_async(adapter, &desc, ObtainedWebGpuDevice, 0);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
