#include <assert.h>
#include <stdio.h>
#include <emscripten/em_math.h>
#include "lib_webgpu.h"

WGpuAdapter adapter;
WGpuDevice device;
WGpuQueue defaultQueue;
WGpuCanvasContext canvasContext;

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
  colorAttachment.view = wgpu_texture_create_view(wgpu_canvas_context_get_current_texture(canvasContext), 0);

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

  canvasContext = wgpu_canvas_get_webgpu_context("canvas");

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = wgpu_canvas_context_get_preferred_format(canvasContext, adapter);
  wgpu_canvas_context_configure(canvasContext, &config);

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
