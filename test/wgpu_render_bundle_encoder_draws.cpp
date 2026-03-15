// Tests recording draw calls (draw, draw_indexed, draw_indirect,
// draw_indexed_indirect) into a WGpuRenderBundleEncoder, then executing
// the bundle in a render pass. This exercises the draw commands mixin
// through the GPURenderBundleEncoder path.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  var pos = array<vec2f,3>(vec2f(0,1),vec2f(-1,-1),vec2f(1,-1));"
            "  return vec4f(pos[vi], 0, 1);"
            "}"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(1,1,0,1); }",
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

  // Create an indirect draw buffer: {vertexCount=3, instanceCount=1, firstVertex=0, firstInstance=0}
  uint32_t drawArgs[4] = { 3, 1, 0, 0 };
  WGpuBufferDescriptor indirectDesc = {};
  indirectDesc.size = sizeof(drawArgs);
  indirectDesc.usage = WGPU_BUFFER_USAGE_INDIRECT | WGPU_BUFFER_USAGE_COPY_DST;
  WGpuBuffer indirectBuf = wgpu_device_create_buffer(device, &indirectDesc);
  assert(indirectBuf);
  wgpu_queue_write_buffer(wgpu_device_get_queue(device), indirectBuf, 0, drawArgs, sizeof(drawArgs));

  // Create an index buffer with uint16 indices: [0, 1, 2]
  uint16_t indices[3] = { 0, 1, 2 };
  WGpuBufferDescriptor idxDesc = {};
  idxDesc.size = (sizeof(indices) + 3) / 4 * 4;
  idxDesc.usage = WGPU_BUFFER_USAGE_INDEX | WGPU_BUFFER_USAGE_COPY_DST;
  WGpuBuffer idxBuf = wgpu_device_create_buffer(device, &idxDesc);
  assert(idxBuf);
  wgpu_queue_write_buffer(wgpu_device_get_queue(device), idxBuf, 0, indices, (sizeof(indices) + 3) / 4 * 4);

  // Create an indexed indirect draw buffer:
  // {indexCount=3, instanceCount=1, firstIndex=0, baseVertex=0, firstInstance=0}
  uint32_t idxDrawArgs[5] = { 3, 1, 0, 0, 0 };
  WGpuBufferDescriptor idxIndirectDesc = {};
  idxIndirectDesc.size = sizeof(idxDrawArgs);
  idxIndirectDesc.usage = WGPU_BUFFER_USAGE_INDIRECT | WGPU_BUFFER_USAGE_COPY_DST;
  WGpuBuffer idxIndirectBuf = wgpu_device_create_buffer(device, &idxIndirectDesc);
  assert(idxIndirectBuf);
  wgpu_queue_write_buffer(wgpu_device_get_queue(device), idxIndirectBuf, 0, idxDrawArgs, sizeof(idxDrawArgs));

  // --- Record a render bundle with all four draw call types ---
  WGPU_TEXTURE_FORMAT colorFormats[1] = { colorFormat };
  WGpuRenderBundleEncoderDescriptor rbeDesc = {};
  rbeDesc.colorFormats = colorFormats;
  rbeDesc.numColorFormats = 1;
  WGpuRenderBundleEncoder bundleEnc = wgpu_device_create_render_bundle_encoder(device, &rbeDesc);
  assert(bundleEnc);

  wgpu_render_commands_mixin_set_pipeline(bundleEnc, pipeline);

  // 1. draw()
  wgpu_render_commands_mixin_draw(bundleEnc, 3, 1, 0, 0);

  // 2. draw_indirect()
  wgpu_render_commands_mixin_draw_indirect(bundleEnc, indirectBuf, 0);

  // 3. draw_indexed() — requires setting index buffer first
  wgpu_render_commands_mixin_set_index_buffer(bundleEnc, idxBuf, WGPU_INDEX_FORMAT_UINT16, 0, -1);
  wgpu_render_commands_mixin_draw_indexed(bundleEnc, 3, 1, 0, 0, 0);

  // 4. draw_indexed_indirect()
  wgpu_render_commands_mixin_draw_indexed_indirect(bundleEnc, idxIndirectBuf, 0);

  WGpuRenderBundle bundle = wgpu_render_bundle_encoder_finish(bundleEnc);
  assert(bundle);
  assert(wgpu_is_render_bundle(bundle));

  // --- Execute the bundle in a render pass ---
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
  wgpu_render_pass_encoder_execute_bundles(pass, &bundle, 1);
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
