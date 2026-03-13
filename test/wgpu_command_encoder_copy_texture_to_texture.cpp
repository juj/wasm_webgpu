// Verifies that wgpu_command_encoder_copy_texture_to_texture() copies between two same-format textures without producing a validation error.
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

  // Create source and destination textures
  WGpuTextureDescriptor texDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  texDesc.width = 4;
  texDesc.height = 4;
  texDesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  texDesc.usage = WGPU_TEXTURE_USAGE_COPY_SRC | WGPU_TEXTURE_USAGE_COPY_DST;

  WGpuTexture srcTex = wgpu_device_create_texture(device, &texDesc);
  WGpuTexture dstTex = wgpu_device_create_texture(device, &texDesc);

  // Copy texture to texture
  WGpuTexelCopyTextureInfo srcInfo = {
    .texture = srcTex,
    .aspect = WGPU_TEXTURE_ASPECT_ALL,
  };
  WGpuTexelCopyTextureInfo dstInfo = {
    .texture = dstTex,
    .aspect = WGPU_TEXTURE_ASPECT_ALL,
  };

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);
  wgpu_command_encoder_copy_texture_to_texture(encoder, &srcInfo, &dstInfo, 4, 4);

  WGpuCommandBuffer cmdBuf = wgpu_command_encoder_finish(encoder);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), cmdBuf);

  char msg[512];
  WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
  if (strlen(msg) > 0) printf("%s\n", msg);
  assert(!error);

  printf("Test OK\n");
  EM_ASM(window.close());
}
