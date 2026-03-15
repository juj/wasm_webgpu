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

  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.width = 8;
  desc.height = 8;
  desc.format = WGPU_TEXTURE_FORMAT_R8UNORM;
  desc.usage = WGPU_TEXTURE_USAGE_COPY_SRC | WGPU_TEXTURE_USAGE_COPY_DST;
  WGpuTexture texture = wgpu_device_create_texture(device, &desc);

  WGpuBufferDescriptor bufferDesc = {
    .size = 256*8,
    .usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST,
    .mappedAtCreation = WGPU_FALSE
  };
  WGpuBuffer dstBuffer = wgpu_device_create_buffer(device, &bufferDesc);

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);

  uint8_t *data = (uint8_t *)malloc(8*8);
  for(int y = 0; y < 8; ++y)
    for(int x = 0; x < 8; ++x)
      data[y*8+x] = y*8+x;

  WGpuTexelCopyTextureInfo copyTexture = {
    .texture = texture,
    .aspect = WGPU_TEXTURE_ASPECT_ALL
  };

  // GPUBuffer.mapAsync() does not work in Firefox, but reads back 0. https://bugzilla.mozilla.org/show_bug.cgi?id=2023418
  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    wgpu_queue_write_texture(wgpu_device_get_queue(device), &copyTexture, data, 8, 8, 8, 8);

    WGpuTexelCopyBufferInfo copyBuffer = {
      .bytesPerRow = 256,
      .rowsPerImage = 8,
      .buffer = dstBuffer,
    };

    wgpu_command_encoder_copy_texture_to_buffer(encoder, &copyTexture, &copyBuffer, 8, 8);

    WGpuCommandBuffer commandBuffer = wgpu_command_encoder_finish(encoder);
    wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), commandBuffer);

    char msg[512];
    WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
    if (strlen(msg) > 0) printf("%s\n", msg);
    assert(!error);

    uint8_t *dstData = (uint8_t*)malloc(256*8);
    wgpu_buffer_map_sync(dstBuffer, WGPU_MAP_MODE_READ);
    wgpu_buffer_get_mapped_range(dstBuffer, 0);
    wgpu_buffer_read_mapped_range(dstBuffer, 0, 0, dstData, 256*8);
    for(int y = 0; y < 8; ++y)
    {
      for(int x = 0; x < 8; ++x) printf("%d ", dstData[y*256+x]);
      printf("\n");
      assert(!memcmp(data + y*8, dstData + y*256, 8));
    }
  }
  printf("Test OK\n");

  EM_ASM(window.close());
}
