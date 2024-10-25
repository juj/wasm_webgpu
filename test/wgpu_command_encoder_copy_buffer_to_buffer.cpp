// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(wgpu_canvas_get_webgpu_context("canvas"), &config);

  const uint64_t data = 0x0123456789ABCDEF;
  WGpuBufferDescriptor desc = {
    .size = sizeof(data),
    .usage = WGPU_BUFFER_USAGE_COPY_SRC,
    .mappedAtCreation = WGPU_TRUE
  };
  WGpuBuffer srcBuffer = wgpu_device_create_buffer(device, &desc);
  wgpu_buffer_get_mapped_range(srcBuffer, 0);
  wgpu_buffer_write_mapped_range(srcBuffer, 0, 0, &data, sizeof(data));
  wgpu_buffer_unmap(srcBuffer);

  desc.usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST;
  desc.mappedAtCreation = WGPU_FALSE;
  WGpuBuffer dstBuffer = wgpu_device_create_buffer(device, &desc);

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  wgpu_command_encoder_copy_buffer_to_buffer(encoder, srcBuffer, 0, dstBuffer, 0, sizeof(data));
  WGpuCommandBuffer commandBuffer = wgpu_command_encoder_finish(encoder);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), commandBuffer);

  char msg[512];
  WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
  if (strlen(msg) > 0) printf("%s\n", msg);
  assert(!error);

  uint64_t dstData;
  wgpu_buffer_map_sync(dstBuffer, WGPU_MAP_MODE_READ);
  wgpu_buffer_get_mapped_range(dstBuffer, 0);
  wgpu_buffer_read_mapped_range(dstBuffer, 0, 0, &dstData, sizeof(dstData));
  printf("Got: 0x%llx, expected: 0x%llx\n", dstData, data);
  assert(data == dstData);

  EM_ASM(window.close());
}
