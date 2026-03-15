// Tests wgpu_render_commands_mixin_set_index_buffer() with an explicit
// non-negative size parameter, exercising the 'size < 0 ? void 0 : size'
// true branch (non-void path). Also tests UINT16 index format.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdint.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  var pos = array<vec2f,4>(vec2f(0,1),vec2f(-1,-1),vec2f(1,-1),vec2f(0,0));"
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
  pipeDesc.primitive.topology = WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &pipeDesc);
  assert(pipeline);

  // Index buffer: 4 uint16 indices (8 bytes total), but we'll bind only first 6 bytes (3 indices)
  uint16_t indices[4] = { 0, 1, 2, 3 };
  WGpuBufferDescriptor bdesc = {
    .size = sizeof(indices),
    .usage = WGPU_BUFFER_USAGE_INDEX | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer ibuf = wgpu_device_create_buffer(device, &bdesc);
  assert(ibuf);
  wgpu_queue_write_buffer(wgpu_device_get_queue(device), ibuf, 0, indices, sizeof(indices));

  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = colorFormat;
  tdesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  tdesc.width = 64;
  tdesc.height = 64;
  WGpuTexture tex = wgpu_device_create_texture(device, &tdesc);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);
  WGpuRenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  ca.view = wgpu_texture_create_view_simple(tex);
  WGpuRenderPassDescriptor passDesc = { .colorAttachments = &ca, .numColorAttachments = 1 };
  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(enc, &passDesc);
  wgpu_encoder_set_pipeline(pass, pipeline);
  // Use UINT16 format, explicit offset=0, size=6 (only first 3 uint16 indices)
  // This exercises both the UINT16 index format and the non-negative size path
  wgpu_render_commands_mixin_set_index_buffer(pass, ibuf, WGPU_INDEX_FORMAT_UINT16, 0, 6);
  wgpu_render_commands_mixin_draw_indexed(pass, 3, 1, 0, 0, 0);
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
