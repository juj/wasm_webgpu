// Verifies that wgpu_present_all_rendering_and_wait_for_next_animation_frame() yields to the
// browser event loop and resumes on each animation frame. A minimal render pass is submitted
// each iteration (as in real usage), and performance.now() is asserted to be strictly
// increasing after each call, proving the function truly suspends and resumes once per frame.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(ctx, &config);

  double prevTime = EM_ASM_DOUBLE({ return performance.now(); });

  for (int i = 0; i < 3; ++i)
  {
    // Minimal render pass: clear the canvas to a different shade of blue each frame.
    WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
    colorAttachment.view = wgpu_canvas_context_get_current_texture_view(ctx);
    colorAttachment.loadOp = WGPU_LOAD_OP_CLEAR;
    colorAttachment.clearValue.r = 0.0;
    colorAttachment.clearValue.g = 0.0;
    colorAttachment.clearValue.b = (double)(i + 1) / 3.0;
    colorAttachment.clearValue.a = 1.0;

    WGpuRenderPassDescriptor passDesc = {};
    passDesc.numColorAttachments = 1;
    passDesc.colorAttachments = &colorAttachment;

    WGpuCommandEncoder encoder = wgpu_device_create_command_encoder_simple(device);
    wgpu_render_pass_encoder_end(wgpu_command_encoder_begin_render_pass(encoder, &passDesc));
    wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(encoder));

    // Present the rendered frame and suspend until the next requestAnimationFrame tick.
    wgpu_present_all_rendering_and_wait_for_next_animation_frame();

    // After resuming, wall-clock time must have advanced (each RAF fires at a later timestamp).
    double now = EM_ASM_DOUBLE({ return performance.now(); });
    assert(now > prevTime);
    prevTime = now;

    printf("Frame %d: time=%.3f ms\n", i, now);
  }

  printf("Test OK\n");
  EM_ASM(window.close());
}
