#include <assert.h>
#include <stdio.h>
#include <emscripten/em_math.h>
#include "lib_webgpu.h"

WGpuAdapter adapter;
WGpuSwapChain swapChain;
WGpuDevice device;
WGpuQueue defaultQueue;

double hue2color(double hue)
{
  hue = emscripten_math_fmod(hue, 1.0);
  if (hue < 1.0 / 6.0) return 6.0 * hue;
  if (hue < 1.0 / 2.0) return 1;
  if (hue < 2.0 / 3.0) return 4.0 - 6.0 * hue;
  return 0;
}

EM_BOOL raf(double time, void *userData)
{
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment colorAttachment = {};
  colorAttachment.view = wgpu_texture_create_view(wgpu_swap_chain_get_current_texture(swapChain), 0);

  double hue = time * 0.00005;
  colorAttachment.loadColor.r = hue2color(hue + 1.0 / 3.0);
  colorAttachment.loadColor.g = hue2color(hue);
  colorAttachment.loadColor.b = hue2color(hue - 1.0 / 3.0);
  colorAttachment.loadColor.a = 1.0;
  colorAttachment.storeOp = WGPU_STORE_OP_STORE;

  WGpuRenderPassDescriptor passDesc = {};
  passDesc.numColorAttachments = 1;
  passDesc.colorAttachments = &colorAttachment;

  wgpu_render_pass_encoder_end_pass(wgpu_command_encoder_begin_render_pass(encoder, &passDesc));
  wgpu_queue_submit_one_and_destroy(defaultQueue, wgpu_command_encoder_finish(encoder));

  return EM_TRUE;
}

void ObtainedWebGpuDevice(WGpuDevice result, void *userData)
{
  device = result;
  defaultQueue = wgpu_device_get_queue(device);

  WGpuCanvasContext canvasContext = wgpu_canvas_get_canvas_context("canvas");

  WGpuSwapChainDescriptor swapChainDesc = WGPU_SWAP_CHAIN_DESCRIPTOR_DEFAULT_INITIALIZER;
  swapChainDesc.device = device;
  swapChainDesc.format = wgpu_canvas_context_get_swap_chain_preferred_format(canvasContext, adapter);
  swapChain = wgpu_canvas_context_configure_swap_chain(canvasContext, &swapChainDesc);

  emscripten_request_animation_frame_loop(raf, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData)
{
  adapter = result;

  WGpuDeviceDescriptor deviceDesc = WGPU_DEVICE_DESCRIPTOR_DEFAULT_INITIALIZER;
  wgpu_adapter_request_device_async(adapter, &deviceDesc, ObtainedWebGpuDevice, 0);
}

int main(int argc, char **argv)
{
  WGpuRequestAdapterOptions options = {};
  navigator_gpu_request_adapter_async(&options, ObtainedWebGpuAdapter, 0);
}
