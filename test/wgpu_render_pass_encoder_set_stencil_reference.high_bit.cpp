// Verifies that wgpu_render_pass_encoder_set_stencil_reference() accepts a reference value
// that has its highest bit set, such as 0xFFFFFFFF. The parameter is a uint32_t, and WebGPU
// takes a GPUStencilValue (unsigned long), so all 32-bit unsigned values are legal. A Wasm i32
// arrives in JavaScript as a signed number though, so a value >= 0x80000000 must be converted
// back to unsigned before it is handed to the browser, which otherwise throws a TypeError
// ("Value is outside the 'unsigned long' value range").
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ErrorScopePopped(WGpuDevice device, WGPU_ERROR_TYPE errorType, const char *errorMessage, void *userData)
{
  if (errorMessage) printf("%s\n", errorMessage);
  assert(errorType == WGPU_ERROR_TYPE_NO_ERROR);
  printf("Test OK\n");
  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  WGpuTextureDescriptor colorDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  colorDesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  colorDesc.usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  colorDesc.width = 64;
  colorDesc.height = 64;
  WGpuTexture colorTex = wgpu_device_create_texture(device, &colorDesc);
  assert(colorTex);

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  colorAttachment.view = colorTex;

  WGpuRenderPassDescriptor passDesc = {
    .colorAttachments = &colorAttachment,
    .numColorAttachments = 1,
  };
  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(encoder, &passDesc);
  assert(pass);

  wgpu_render_pass_encoder_set_stencil_reference(pass, 0x80000000u);
  wgpu_render_pass_encoder_set_stencil_reference(pass, 0xFFFFFFFFu);

  wgpu_render_pass_encoder_end(pass);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(encoder));

  wgpu_device_pop_error_scope_async(device, ErrorScopePopped, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
