#include <stdio.h>
#include <emscripten/em_math.h>
#include "lib_webgpu.h"
#include <math.h>

#define HUE2COLOR(hue) fmax(0.0, fmin(1.0, 2.0 - fabs(emscripten_math_fmod((hue), 6.0) - 3.0)))

int main(int argc, char **argv)
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  WGpuCanvasContext canvasContext = wgpu_canvas_get_webgpu_context("canvas");

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(canvasContext, &config);

  for(;;)
  {
    WGpuCommandEncoder encoder = wgpu_device_create_command_encoder_simple(device);

    WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
    colorAttachment.view = wgpu_canvas_context_get_current_texture_view(canvasContext);

    double hue = emscripten_performance_now() * 0.0005;
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

    // This sample shows how to use a synchronous-looking infinite rendering loop,
    // where the following function yields back to the browser event loop using
    // JSPI. This allows porting applications that use infinite for() loops.
    // Using this function requires linking with -sJSPI.
    // Alternatively, one can use more traditional wgpu_request_animation_frame_loop()
    // callbacks.
    // Note: there is a possibility that wgpu_request_animation_frame_loop() offers
    // better performance and input latency compared to using this method, so
    // prefer using the callback variant whenever possible.
    wgpu_present_all_rendering_and_wait_for_next_animation_frame();
  }
}
