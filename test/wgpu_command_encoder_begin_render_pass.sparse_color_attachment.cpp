// Tests wgpu_command_encoder_begin_render_pass() with a null (sparse) color
// attachment as the first slot, exercising the
// 'colorAttachments.push(HEAPU32[...] ? {...} : null)' null branch.
// A null attachment slot is represented by setting view == 0 in the
// WGpuRenderPassColorAttachment struct.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // https://bugzilla.mozilla.org/show_bug.cgi?id=2023423: sparse GPUColorTargetState on beginRenderPass not supported
  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

    // Shader writes only to @location(1); @location(0) is absent => sparse slot 0.
    WGpuShaderModuleDescriptor smdesc = {
      .code = "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {"
              "  var pos = array<vec2f,3>(vec2f(0,1),vec2f(-1,-1),vec2f(1,-1));"
              "  return vec4f(pos[vi], 0, 1);"
              "}"
              "struct FSOut { @location(1) c: vec4f };"
              "@fragment fn fs() -> FSOut { return FSOut(vec4f(0,1,0,1)); }",
    };
    WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
    assert(shader);

    // Pipeline: slot 0 is null/sparse, slot 1 is real color target
    WGpuColorTargetState targets[2] = {};
    // targets[0] has format==0 (INVALID) => null sparse target
    targets[1].format = colorFormat;
    targets[1].writeMask = WGPU_COLOR_WRITE_ALL;

    WGpuRenderPipelineDescriptor pipeDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
    pipeDesc.vertex.module = shader;
    pipeDesc.vertex.entryPoint = "vs";
    pipeDesc.fragment.module = shader;
    pipeDesc.fragment.entryPoint = "fs";
    pipeDesc.fragment.targets = targets;
    pipeDesc.fragment.numTargets = 2;
    pipeDesc.layout = WGPU_AUTO_LAYOUT_MODE_AUTO;

    WGpuRenderPipeline pipeline = wgpu_device_create_render_pipeline(device, &pipeDesc);
    assert(pipeline);

    // Create a real texture for slot 1
    WGpuTextureDescriptor texDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
    texDesc.format = colorFormat;
    texDesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
    texDesc.width = 64;
    texDesc.height = 64;
    WGpuTexture realTex = wgpu_device_create_texture(device, &texDesc);
    assert(realTex);

    WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

    // Color attachment array: slot 0 has view==0 (sparse/null), slot 1 is the real texture
    WGpuRenderPassColorAttachment ca[2] = {};
    ca[0] = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
    ca[0].view = 0; // null/sparse - this exercises the null branch in JS
    ca[1] = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
    ca[1].view = wgpu_texture_create_view_simple(realTex);

    WGpuRenderPassDescriptor passDesc = { .colorAttachments = ca, .numColorAttachments = 2 };
    WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(enc, &passDesc);
    wgpu_encoder_set_pipeline(pass, pipeline);
    wgpu_render_commands_mixin_draw(pass, 3, 1, 0, 0);
    wgpu_render_pass_encoder_end(pass);
    wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));
  }

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
