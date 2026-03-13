// Verifies that wgpu_command_encoder_resolve_query_set() resolves occlusion query results into a buffer at a specified destination offset without errors.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  WGPU_TEXTURE_FORMAT colorFormat = navigator_gpu_get_preferred_canvas_format();

  // Create an occlusion query set with 4 queries
  WGpuQuerySetDescriptor qdesc = {
    .type = WGPU_QUERY_TYPE_OCCLUSION,
    .count = 4,
  };
  WGpuQuerySet querySet = wgpu_device_create_query_set(device, &qdesc);
  assert(querySet);

  // Destination buffer: 4 queries × 8 bytes each = 32 bytes
  // destinationOffset = 256 (skip the first 256 bytes)
  // so we need at least 256 + 4*8 = 40 bytes, rounded up to 512
  WGpuBufferDescriptor bdesc = {
    .size = 512,
    .usage = WGPU_BUFFER_USAGE_QUERY_RESOLVE | WGPU_BUFFER_USAGE_COPY_SRC,
  };
  WGpuBuffer queryBuf = wgpu_device_create_buffer(device, &bdesc);
  assert(queryBuf);

  // Readback buffer
  WGpuBufferDescriptor rdesc = {
    .size = 512,
    .usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_MAP_READ,
  };
  WGpuBuffer rbuf = wgpu_device_create_buffer(device, &rdesc);
  assert(rbuf);

  // Canvas setup for the render pass
  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  WGpuCanvasConfiguration cfg = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  cfg.device = device;
  cfg.format = colorFormat;
  wgpu_canvas_context_configure(ctx, &cfg);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  // Render pass with occlusion query set
  WGpuRenderPassColorAttachment ca = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  ca.view = wgpu_canvas_context_get_current_texture(ctx);
  WGpuRenderPassDescriptor passDesc = {
    .colorAttachments = &ca,
    .numColorAttachments = 1,
    .occlusionQuerySet = querySet,
  };
  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(enc, &passDesc);
  // Begin/end a query at index 0 and index 2 (leaving 1 and 3 empty)
  wgpu_render_pass_encoder_begin_occlusion_query(pass, 0);
  wgpu_render_pass_encoder_end_occlusion_query(pass);
  wgpu_render_pass_encoder_begin_occlusion_query(pass, 2);
  wgpu_render_pass_encoder_end_occlusion_query(pass);
  wgpu_render_pass_encoder_end(pass);

  // Resolve queries 0..3 into queryBuf, with destinationOffset=256
  wgpu_command_encoder_resolve_query_set(enc, querySet, 0, 4, queryBuf, /*destinationOffset=*/256);

  // Copy to readback
  wgpu_command_encoder_copy_buffer_to_buffer(enc, queryBuf, 0, rbuf, 0, 512);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));

  char msg[512];
  WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
  if (strlen(msg) > 0) printf("%s\n", msg);
  assert(!error);

  // Verify resolve didn't crash and buffer is readable
  wgpu_buffer_map_sync(rbuf, WGPU_MAP_MODE_READ);
  wgpu_buffer_get_mapped_range(rbuf, 0);
  uint64_t results[4] = { 0 };
  wgpu_buffer_read_mapped_range(rbuf, 0, 256, results, sizeof(results));
  printf("Query results: q0=%llu q1=%llu q2=%llu q3=%llu\n",
    results[0], results[1], results[2], results[3]);
  wgpu_buffer_unmap(rbuf);

  printf("Test OK\n");
  EM_ASM(window.close());
}
