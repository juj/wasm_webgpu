// This sample shows how to perform OffscreenCanvas rendering with WebGPU
// using Wasm Workers.

#include <stdio.h>
#include <emscripten/em_math.h>
#include <math.h>
#include "lib_webgpu.h"

#define HUE2COLOR(hue) fmax(0.0, fmin(1.0, 2.0 - fabs(emscripten_math_fmod((hue), 6.0) - 3.0)))

WGpuAdapter adapter;
WGpuDevice device;
WGpuCanvasContext canvasContext;
OffscreenCanvasId canvasId;
emscripten_wasm_worker_t worker;

void worker_main(void);
void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData);
void ObtainedWebGpuDevice(WGpuDevice result, void *userData);
WGPU_BOOL raf(double time, void *userData);
EM_BOOL window_resize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData);

int main(int argc, char **argv) // runs in main thread
{
  // To render to an existing HTML Canvas element from a Wasm Worker using WebGPU:

  // 1. Let's first create a Wasm Worker to do the rendering.
  worker = emscripten_malloc_wasm_worker(1024);

  // 2. convert the HTML Canvas element to be renderable from a Worker
  //    by transferring its control to an OffscreenCanvas:

  //    This function must be given a custom ID number, that will be used to
  //    identify that OffscreenCanvas element in C/C++ side code. If you want to
  //    render to multiple Canvases from different Workers, you should devise
  //    some kind of Canvas ID counter scheme here. We'll use a fixed ID 42.

  //    After control has been transferred offscreen, this canvas element will
  //    be permanently offscreen-controlled, there is no browser API to undo
  //    this. (except to delete the DOM element and create a new one)
  canvasId = 42;
  canvas_transfer_control_to_offscreen("canvas", canvasId);

  // 3. Each OffscreenCanvas is owned by exactly one thread, so we must
  //    postMessage() our newly created OffscreenCanvas to the Worker to pass
  //    the ownership. Here we pass the ID of the OffscreenCanvas we created
  //    above. After this line, our main thread cannot access this
  //    OffscreenCanvas anymore.
  offscreen_canvas_post_to_worker(canvasId, worker);

  // 4. Now the OffscreenCanvas is available for the Worker, so make it start
  //    executing its main function to initialize WebGPU in the Worker and
  //    start rendering.
  emscripten_wasm_worker_post_function_v(worker, worker_main);

  // 5. One particular gotcha with rendering with OffscreenCanvas is that resizing the canvas
  //    becomes more complicated. If we want to perform e.g. fullscreen rendering,
  //    we need a custom resize strategy. See the window_resize handler below.
  emscripten_set_resize_callback(EMSCRIPTEN_EVENT_TARGET_WINDOW, 0, 0, window_resize);
}

void worker_main() // runs in worker thread
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}

void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData) // runs in worker thread
{
  adapter = result;
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

void ObtainedWebGpuDevice(WGpuDevice result, void *userData) // runs in worker thread
{
  device = result;

  // 6. Instead of using the usual main thread function
  //      wgpu_canvas_get_webgpu_context(cssSelector);
  //    to initialize a WebGPU context on the canvas, use the Offscreen Canvas
  //    variant of this function
  //      wgpu_offscreen_canvas_get_webgpu_context(OffscreenCanvasId id);
  //    to initialize the context.
  canvasContext = wgpu_offscreen_canvas_get_webgpu_context(canvasId);

  // 7. The rest of the WebGPU engine usage proceeds as it did in the main
  //    thread case.
  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(canvasContext, &config);

  // In particular, Web Workers can use requestAnimationFrame() like the main
  // thread to perform rendering.
  wgpu_request_animation_frame_loop(raf, 0);
}

WGPU_BOOL raf(double time, void *userData) // runs in worker thread
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

EM_BOOL window_resize(int eventType, const EmscriptenUiEvent *uiEvent, void *userData) // runs in main thread
{
  double innerWidth = EM_ASM_DOUBLE(return window.innerWidth);
  double innerHeight = EM_ASM_DOUBLE(return window.innerHeight);
  double dpr = EM_ASM_DOUBLE(return window.devicePixelRatio);

  // Recall that the canvas size in web browsers is dictated by two different size fields:
  // a) the canvas CSS style width and height (of type double) that govern the visible CSS
  //    size of the Canvas element on the web page, and
  // b) the render target size of the Canvas front buffer that WebGPU(/WebGL) renders to.
  //    (of type integer).

  // The Canvas render target is stretched to cover the HTML area that the Canvas element
  // encompasses.
  // If we want pixel-perfect rendering, these two sizes need to be modified in sync
  // whenever we want to change the visible canvas size. The size a) is governed from the main
  // thread; however, when using OffscreenCanvas, the size b) must be controlled by the Web
  // Worker that holds ownership of the OffscreenCanvas.

  // Therefore to update the size, modify size a) here in the main browser thread:
  emscripten_set_element_css_size("canvas", innerWidth, innerHeight);

  // ... and update size b) by posting a message to the Worker that owns the OffscreenCanvas
  // so that the render target size is correctly updated in the Worker thread.

  int w = (int)(innerWidth*dpr);
  int h = (int)(innerHeight*dpr);
  emscripten_wasm_worker_post_function_viii(worker, offscreen_canvas_set_size, canvasId, w, h);
  EM_ASM({console.error(`Resized Offscreen Canvas to new size ${$0}x${$1}`)}, w, h);
  return EM_TRUE;
}
