// This sample is like offscreen_canvas_pthread.c, but instead shows how to perform OffscreenCanvas
// rendering with WebGPU using Emscripten's -sPROXY_TO_PTHREAD linker flag.

#include <stdio.h>
#include <emscripten/em_math.h>
#include <emscripten/proxying.h>
#include <math.h>
#include "lib_webgpu.h"

#define HUE2COLOR(hue) fmax(0.0, fmin(1.0, 2.0 - fabs(emscripten_math_fmod((hue), 6.0) - 3.0)))

WGpuAdapter adapter;
WGpuDevice device;
WGpuCanvasContext canvasContext;

void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData);
void ObtainedWebGpuDevice(WGpuDevice result, void *userData);
WGPU_BOOL raf(double time, void *userData);

int main(int argc, char **argv) // runs in a pthread that is transparently created by Emscripten (since building with -sPROXY_TO_PTHREAD)
{
  // 1. Request a WebGPU adapter. This call is asynchronous, and will continue
  //    execution at ObtainedWebGpuAdapter() when complete.
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);

  // Do not quit the program when falling out of main, but lie dormant for async
  // events to be processed in this pthread context.
  emscripten_exit_with_live_runtime();
}

void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData)
{
  adapter = result;
  // 2. Request a WebGPU device. This call is asynchronous too, and will
  //    continue in ObtainedWebGpuDevice().
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

void ObtainedWebGpuDevice(WGpuDevice result, void *userData)
{
  device = result;

  // 3. Search for an OffscreenCanvas using a CSS ID.
  //    For this call to find the right canvas, it is also necessary to set
  //    the DOM element ID of the Canvas element to 'canvas'. If you want to
  //    use some other DOM element ID here, you will need to specify the
  //    -sOFFSCREENCANVASES_TO_PTHREAD=#myCustomCanvasId as an Emscripten link
  //    directive, and then pass "myCustomCanvasId" below.
  canvasContext = wgpu_canvas_get_webgpu_context("canvas");

  // 4. The rest of the WebGPU engine usage proceeds as it would in the main
  //    thread case.
  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(canvasContext, &config);

  // In particular, Web Workers can use requestAnimationFrame() like the main
  // thread to perform rendering.
  wgpu_request_animation_frame_loop(raf, 0);
}

WGPU_BOOL raf(double time, void *userData) // runs in pthread
{
  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder_simple(device);

  WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  colorAttachment.view = wgpu_canvas_context_get_current_texture_view(canvasContext);

  double hue = time * 0.0005;
  colorAttachment.clearValue.r = HUE2COLOR(hue + 2.0);
  colorAttachment.clearValue.g = HUE2COLOR(hue);
  colorAttachment.clearValue.b = HUE2COLOR(hue - 2.0);
  colorAttachment.clearValue.a = 1.0;
  colorAttachment.loadOp = WGPU_LOAD_OP_CLEAR;

  WGpuRenderPassDescriptor passDesc = {};
  passDesc.numColorAttachments = 1;
  passDesc.colorAttachments = &colorAttachment;

  wgpu_render_pass_encoder_end(wgpu_command_encoder_begin_render_pass(encoder, &passDesc));
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(encoder));

  return EM_TRUE;
}
