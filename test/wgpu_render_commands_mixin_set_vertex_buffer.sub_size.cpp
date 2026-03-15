// Tests wgpu_render_commands_mixin_set_vertex_buffer() with an explicit
// non-negative size parameter, exercising the 'size < 0 ? void 0 : size'
// true branch (non-void path) in the JS implementation.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  WGpuShaderModuleDescriptor smdesc = {
    .code = "struct Vert { @location(0) pos: vec2f };"
            "@vertex fn vs(v: Vert) -> @builtin(position) vec4f {"
            "  return vec4f(v.pos, 0, 1);"
            "}"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(1,0,0,1); }",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  WGpuVertexAttribute attr = { .format = WGPU_VERTEX_FORMAT_FLOAT32X2, .offset = 0, .shaderLocation = 0 };
  WGpuVertexBufferLayout vbl = {
    .arrayStride = 8,
    .attributes = &attr,
    .numAttributes = 1,
  };

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = colorFormat;

  WGpuRenderPipelineDescriptor pipeDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  pipeDesc.vertex.module = shader;
  pipeDesc.vertex.entryPoint = "vs";
  pipeDesc.vertex.buffers = &vbl;
  pipeDesc.vertex.numBuffers = 1;
  pipeDesc.fragment.module = shader;
  pipeDesc.fragment.entryPoint = "fs";
  pipeDesc.fragment.targets = &colorTarget;
  pipeDesc.fragment.numTargets = 1;
  pipeDesc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

  WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &pipeDesc);
  assert(pipeline);

  // Create a vertex buffer with 3 vertices (24 bytes) but only expose the first 16 bytes
  float verts[3][2] = { {0,1}, {-1,-1}, {1,-1} };
  WGpuBufferDescriptor bdesc = {
    .size = sizeof(verts),
    .usage = WGPU_BUFFER_USAGE_VERTEX | WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer vbuf = wgpu_device_create_buffer(device, &bdesc);
  assert(vbuf);
  wgpu_queue_write_buffer(wgpu_device_get_queue(device), vbuf, 0, verts, sizeof(verts));

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
  // Use explicit offset=0, size=16 (covers only first 2 vertices) — exercises size >= 0 path
  wgpu_render_commands_mixin_set_vertex_buffer(pass, 0, vbuf, 0, 16);
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
