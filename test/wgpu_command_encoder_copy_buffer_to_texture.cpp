// Verifies that wgpu_command_encoder_copy_buffer_to_texture() successfully copies pixel data from a buffer into a texture without validation errors.
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

  // Create a source buffer with data
  const uint32_t width = 256, height = 4;
  uint8_t srcData[width * height];
  for (uint32_t i = 0; i < width * height; ++i) srcData[i] = (uint8_t)i;

  WGpuBufferDescriptor bufDesc = {
    .size = sizeof(srcData),
    .usage = WGPU_BUFFER_USAGE_COPY_SRC,
    .mappedAtCreation = WGPU_TRUE,
  };
  WGpuBuffer srcBuf = wgpu_device_create_buffer(device, &bufDesc);
  wgpu_buffer_get_mapped_range(srcBuf, 0);
  wgpu_buffer_write_mapped_range(srcBuf, 0, 0, srcData, sizeof(srcData));
  wgpu_buffer_unmap(srcBuf);

  // Create a destination texture
  WGpuTextureDescriptor texDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  texDesc.width = width;
  texDesc.height = height;
  texDesc.format = WGPU_TEXTURE_FORMAT_R8UNORM;
  texDesc.usage = WGPU_TEXTURE_USAGE_COPY_SRC | WGPU_TEXTURE_USAGE_COPY_DST;
  WGpuTexture dstTex = wgpu_device_create_texture(device, &texDesc);

  // Copy buffer to texture
  WGpuTexelCopyBufferInfo srcInfo = {
    .bytesPerRow = width,
    .rowsPerImage = height,
    .buffer = srcBuf,
  };
  WGpuTexelCopyTextureInfo dstInfo = {
    .texture = dstTex,
    .aspect = WGPU_TEXTURE_ASPECT_ALL,
  };

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  wgpu_command_encoder_copy_buffer_to_texture(encoder, &srcInfo, &dstInfo, width, height);

  WGpuCommandBuffer cmdBuf = wgpu_command_encoder_finish(encoder);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), cmdBuf);

  char msg[512];
  WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
  if (strlen(msg) > 0) printf("%s\n", msg);
  assert(!error);

  printf("Test OK\n");
  EM_ASM(window.close());
}
