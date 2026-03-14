// Verifies that wgpu_command_encoder_copy_texture_to_buffer() copies texel data from a GPU texture into a mappable buffer and that the readback matches the original pixel data.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <emscripten/heap.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  const uint32_t W = 4, H = 4;
  const uint32_t bytesPerRow = W * 4; // 4 bytes per RGBA8UNORM texel, must be multiple of 256 for copy
  // bytesPerRow for copy_texture_to_buffer must be a multiple of 256
  const uint32_t alignedBytesPerRow = 256;
  const uint32_t bufSize = alignedBytesPerRow * H;

  // Fill a 4x4 RGBA8UNORM texture with known pixel data
  uint8_t srcPixels[W * H * 4];
  for (uint32_t i = 0; i < W * H * 4; ++i)
    srcPixels[i] = (uint8_t)i;

  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  tdesc.usage = WGPU_TEXTURE_USAGE_COPY_SRC | WGPU_TEXTURE_USAGE_COPY_DST;
  tdesc.width = W;
  tdesc.height = H;
  WGpuTexture texture = wgpu_device_create_texture(device, &tdesc);
  assert(texture);

  if (!EM_ASM_INT({return navigator.userAgent.includes("Firefox")}) || emscripten_get_heap_max() <= (size_t)0xFFFFFFFF)
  {
    // Upload pixel data to texture
    WGpuTexelCopyTextureInfo texDst = WGPU_TEXEL_COPY_TEXTURE_INFO_DEFAULT_INITIALIZER;
    texDst.texture = texture;
    wgpu_queue_write_texture(wgpu_device_get_queue(device), &texDst,
      srcPixels, bytesPerRow, H, W, H);

    // Create destination buffer (bytesPerRow must be ≥ 256 and multiple of 256)
    WGpuBufferDescriptor bdesc = {
      .size = bufSize,
      .usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_MAP_READ,
    };
    WGpuBuffer dstBuf = wgpu_device_create_buffer(device, &bdesc);
    assert(dstBuf);

    // Copy texture to buffer
    WGpuTexelCopyTextureInfo texSrc = WGPU_TEXEL_COPY_TEXTURE_INFO_DEFAULT_INITIALIZER;
    texSrc.texture = texture;

    WGpuTexelCopyBufferInfo bufDst = {
      .offset = 0,
      .bytesPerRow = alignedBytesPerRow,
      .rowsPerImage = H,
      .buffer = dstBuf,
    };

    WGpuCommandEncoder enc = wgpu_device_create_command_encoder_simple(device);
    wgpu_command_encoder_copy_texture_to_buffer(enc, &texSrc, &bufDst, W, H);
    wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));

    char msg[512];
    WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
    if (strlen(msg) > 0) printf("%s\n", msg);
    assert(!error);

    // Read back and verify each row
    wgpu_buffer_map_sync(dstBuf, WGPU_MAP_MODE_READ);
    wgpu_buffer_get_mapped_range(dstBuf, 0);
    for (uint32_t row = 0; row < H; ++row)
    {
      uint8_t rowData[W * 4] = { 0 };
      wgpu_buffer_read_mapped_range(dstBuf, 0, row * alignedBytesPerRow, rowData, sizeof(rowData));
      for (uint32_t col = 0; col < W * 4; ++col)
      {
        uint8_t expected = srcPixels[row * bytesPerRow + col];
        if (rowData[col] != expected)
        {
          printf("Mismatch at row=%u col=%u: got %u expected %u\n", row, col, rowData[col], expected);
          assert(0);
        }
      }
    }
    wgpu_buffer_unmap(dstBuf);
  }

  printf("Test OK\n");
  EM_ASM(window.close());
}
