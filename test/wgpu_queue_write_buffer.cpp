// flags: -sEXIT_RUNTIME=0 -sJSPI -sINITIAL_MEMORY=4294901760

#include "lib_webgpu.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <emscripten/emmalloc.h>

int main()
{
  void *ptr = malloc(1147483648ull); // Make sure all the memory usage is pushed to the upper half of the 4GB range.
  assert(ptr);
  ptr = malloc(1147483648ull); // TODO: Emscripten has a bug that it cannot perform >2GB allocations, so allocate two 1GB chunks.
  assert(ptr);

  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(wgpu_canvas_get_webgpu_context("canvas"), &config);

  WGpuBufferDescriptor desc = {
    .size = 16,
    .usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST,
    .mappedAtCreation = WGPU_FALSE
  };
  WGpuBuffer dstBuffer = wgpu_device_create_buffer(device, &desc);

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);

  uint64_t *data = (uint64_t *)malloc(16);
  *data = 0x0123456789ABCDEF;
  wgpu_queue_write_buffer(wgpu_device_get_queue(device), dstBuffer, 0, data, 16);

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
  printf("Got: 0x%llx, expected: 0x%llx\n", dstData, *data);
  assert(*data == dstData);

  EM_ASM(window.close());
}
