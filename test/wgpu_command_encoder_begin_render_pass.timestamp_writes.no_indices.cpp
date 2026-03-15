// Tests wgpu_command_encoder_begin_render_pass() with a timestamp querySet set
// but both beginningOfPassWriteIndex and endOfPassWriteIndex equal to -1.
// This exercises the case where querySet != 0 but both '>=0' branches in
// $wgpuReadTimestampWrites are false, producing a timestampWrites object with
// only a querySet and no index properties.
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
            "  return vec4f(0,0,0,1); }"
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

  // A query set with 2 slots (neither will be written)
  WGpuQuerySetDescriptor qsDesc = {};
  qsDesc.type  = WGPU_QUERY_TYPE_TIMESTAMP;
  qsDesc.count = 2;
  WGpuQuerySet querySet = wgpu_device_create_query_set(device, &qsDesc);
  assert(querySet);

  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = colorFormat;
  tdesc.usage  = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  tdesc.width  = 64;
  tdesc.height = 64;
  WGpuTexture tex = wgpu_device_create_texture(device, &tdesc);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);
  WGpuRenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  ca.view = wgpu_texture_create_view_simple(tex);

  WGpuRenderPassDescriptor passDesc = WGPU_RENDER_PASS_DESCRIPTOR_DEFAULT_INITIALIZER;
  passDesc.colorAttachments    = &ca;
  passDesc.numColorAttachments = 1;
  // querySet is set but both write indices are -1 (neither beginning nor end written)
  passDesc.timestampWrites.querySet                  = querySet;
  passDesc.timestampWrites.beginningOfPassWriteIndex = -1; // NOT written
  passDesc.timestampWrites.endOfPassWriteIndex       = -1; // NOT written

  // Use an error scope — some implementations may reject a timestampWrites with no indices
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(enc, &passDesc);
  wgpu_encoder_set_pipeline(pass, pipeline);
  wgpu_render_commands_mixin_draw(pass, 3, 1, 0, 0);
  wgpu_render_pass_encoder_end(pass);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *m, void *) {
    if (type != WGPU_ERROR_TYPE_NO_ERROR)
      printf("timestampWrites with no indices produced validation error (acceptable).\n");
    EM_ASM(window.close());
  }, 0);
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
