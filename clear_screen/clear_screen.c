#include <assert.h>
#include <stdio.h>
#include <emscripten/em_math.h>
#include "lib_webgpu.h"

WGpuAdapter adapter;
WGpuSwapChain swapChain;
WGpuDevice device;
WGpuQueue defaultQueue;

double hue2rgb(double v1, double v2, double vh)
{
  if (vh < 0.0) vh += 1.0;
  if (vh > 1.0) vh -= 1.0;
  if (vh < 1.0 / 6.0) return v1 + (v2 - v1) * 6.0 * vh;
  if (vh < 1.0 / 2.0) return v2;
  if (vh < 2.0 / 3.0) return v1 + (v2 - v1) * (2.0 / 3.0 - vh) * 6.0;
  return v1;
}

void hsl2rgb(double hue, double saturation, double luminance, double *outRed, double *outGreen, double *outBlue)
{
  if (saturation < 1e-4)
  {
    *outRed = *outGreen = *outBlue = luminance;
    return;
  }

  double v2 = (luminance < 0.5) ? (luminance * (1.0 + saturation)) : (luminance + saturation - saturation * luminance);
  double v1 = 2.0 * luminance - v2;

  *outRed   = hue2rgb(v1, v2, hue + 1.0 / 3.0);
  *outGreen = hue2rgb(v1, v2, hue);
  *outBlue  = hue2rgb(v1, v2, hue - 1.0 / 3.0);
}

EM_BOOL raf(double time, void *userData)
{
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment colorAttachment = {};
  colorAttachment.view = wgpu_texture_create_view(wgpu_swap_chain_get_current_texture(swapChain), 0);

  double hue = emscripten_math_fmod(time * 0.00005, 1.0);
  hsl2rgb(hue, 1.0, 0.5, &colorAttachment.loadColor[0], &colorAttachment.loadColor[1], &colorAttachment.loadColor[2]);
  colorAttachment.loadColor[3] = 0.05;

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
  defaultQueue = wgpu_device_get_default_queue(device);

  WGpuCanvasContext canvasContext = wgpu_canvas_get_canvas_context("canvas");

  WGpuSwapChainDescriptor swapChainDesc = WGPU_SWAP_CHAIN_DESCRIPTOR_DEFAULT_INITIALIZER;
  swapChainDesc.device = device;
  swapChainDesc.format = wgpu_canvas_context_get_swap_chain_preferred_format(canvasContext, adapter);
  swapChain = wgpu_canvascontext_configure_swap_chain(canvasContext, &swapChainDesc);

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
