// Tests wgpu_command_encoder_begin_render_pass() with a render attachment that
// specifies a depthSlice into a 3D texture, exercising the
// 'depthSlice': HEAP32[colorAttachmentsIdx+1] < 0 ? void 0 : HEAP32[...]
// code path with a non-negative depthSlice value.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGPU_TEXTURE_FORMAT colorFormat = WGPU_TEXTURE_FORMAT_RGBA8UNORM;

  WGpuShaderModuleDescriptor smdesc = {
    .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
            "  var pos = array<vec2f,3>(vec2f(0,1),vec2f(-1,-1),vec2f(1,-1));"
            "  return vec4f(pos[vi], 0, 1);"
            "}"
            "@fragment fn fs() -> @location(0) vec4f { return vec4f(0,0,1,1); }",
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

  // Create a 3D texture with depth 4 to render to one of its z-slices
  WGpuTextureDescriptor texDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  texDesc.format = colorFormat;
  texDesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT | WGPU_TEXTURE_USAGE_COPY_SRC;
  texDesc.width = 64;
  texDesc.height = 64;
  texDesc.depthOrArrayLayers = 4;
  texDesc.dimension = WGPU_TEXTURE_DIMENSION_3D;
  WGpuTexture tex3D = wgpu_device_create_texture(device, &texDesc);
  assert(tex3D);

  // Create a view of the whole 3D texture (all slices)
  WGpuTextureViewDescriptor viewDesc = WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER;
  viewDesc.dimension = WGPU_TEXTURE_VIEW_DIMENSION_3D;
  WGpuTextureView view3D = wgpu_texture_create_view(tex3D, &viewDesc);
  assert(view3D);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  ca.view = view3D;
  ca.depthSlice = 2; // render into z-slice 2 of the 3D texture

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
