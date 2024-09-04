#include <assert.h>
#include <stdio.h>
#include <emscripten/em_math.h>
#include "lib_webgpu.h"

WGpuAdapter adapter;
WGpuDevice device;
WGpuQueue queue;
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
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder_simple(device);

  WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  colorAttachment.view = wgpu_canvas_context_get_current_texture_view(canvasContext);

  double hue = time * 0.00005;
  colorAttachment.clearValue.r = hue2color(hue + 1.0 / 3.0);
  colorAttachment.clearValue.g = hue2color(hue);
  colorAttachment.clearValue.b = hue2color(hue - 1.0 / 3.0);
  colorAttachment.clearValue.a = 1.0;
  colorAttachment.loadOp = WGPU_LOAD_OP_CLEAR;

  WGpuRenderPassDescriptor passDesc = {};
  passDesc.numColorAttachments = 1;
  passDesc.colorAttachments = &colorAttachment;

  wgpu_render_pass_encoder_end(wgpu_command_encoder_begin_render_pass_1color_0depth(encoder, &passDesc));
  wgpu_queue_submit_one_and_destroy(queue, wgpu_command_encoder_finish(encoder));

  assert(wgpu_get_num_live_objects() < 100); // Check against programming errors from Wasm<->JS WebGPU object leaks

  return EM_TRUE;
}

void ObtainedWebGpuDevice(WGpuDevice result, void *userData)
{
  device = result;
  queue = wgpu_device_get_queue(device);

  canvasContext = wgpu_canvas_get_webgpu_context("canvas");

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(canvasContext, &config);

  wgpu_request_animation_frame_loop(raf, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData)
{
  adapter = result;
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main(int argc, char **argv)
{
  assert(navigator_gpu_available());
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
