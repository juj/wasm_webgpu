// Tests that wgpu_queue_write_texture() passes a data buffer of the correct size to the
// browser's WebGPU implementation when writing multiple array layers at once.
//
// Background: the Firefox/Wasm4GB workaround path (compiled when CAN_ADDRESS_2GB || MEMORY64)
// creates a plain Uint8Array view so Firefox can accept it, but sizes the view as:
//
//   bytesPerBlockRow * blockRowsPerImage   (one layer only)
//
// For a write spanning D > 1 layers, the WebGPU spec requires at least:
//
//   bytesPerBlockRow * (blockRowsPerImage * (D - 1) + (H - 1)) + W   bytes
//
// A view that is too small must cause a GPUValidationError. This test catches that bug
// by writing two layers and asserting that no validation error is produced.
//
// The test passes trivially when built without CAN_ADDRESS_2GB / MEMORY64 (the #else
// branch passes HEAPU8 directly and is always large enough). Run with --wasm4gb or
// --wasm64 to exercise the workaround path.
// flags: -sEXIT_RUNTIME=0 -sJSPI

#include "lib_webgpu.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main()
{
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  WGpuDevice device = wgpu_adapter_request_device_sync_simple(adapter);

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  // 2-layer 2D texture array, R8UNORM: 1 byte per texel, 1x1 block size.
  const uint32_t W = 4, H = 4, D = 2;
  const uint32_t bytesPerBlockRow  = W; // 4 bytes per row
  const uint32_t blockRowsPerImage = H; // 4 rows per layer

  // Full data covering both layers: bytesPerBlockRow * blockRowsPerImage * D = 32 bytes.
  //
  // With the bug present the JS-side Uint8Array view is only
  //   bytesPerBlockRow * blockRowsPerImage = 16 bytes  (first layer only).
  //
  // The WebGPU spec required size for D=2, H=4, W=4, bytesPerBlockRow=4:
  //   bytesPerBlockRow * (blockRowsPerImage * (D-1) + (H-1)) + W
  //   = 4 * (4*1 + 3) + 4 = 32 bytes
  //
  // 16 < 32, so the browser must generate a GPUValidationError.
  uint8_t data[bytesPerBlockRow * blockRowsPerImage * D];
  for (uint32_t i = 0; i < sizeof(data); ++i)
    data[i] = (uint8_t)i;

  WGpuTextureDescriptor desc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.width              = W;
  desc.height             = H;
  desc.depthOrArrayLayers = D;
  desc.format             = WGPU_TEXTURE_FORMAT_R8UNORM;
  desc.usage              = WGPU_TEXTURE_USAGE_COPY_DST;
  WGpuTexture texture = wgpu_device_create_texture(device, &desc);
  assert(texture);

  WGpuTexelCopyTextureInfo dst = WGPU_TEXEL_COPY_TEXTURE_INFO_DEFAULT_INITIALIZER;
  dst.texture = texture;
  dst.aspect  = WGPU_TEXTURE_ASPECT_ALL;

  // Write both layers. If the Firefox workaround path is active and unfixed, the
  // Uint8Array view handed to writeTexture() will be 16 bytes while 32 are required,
  // producing a GPUValidationError captured by the error scope above.
  wgpu_queue_write_texture(wgpu_device_get_queue(device), &dst,
    data, bytesPerBlockRow, blockRowsPerImage, W, H, D);

  char msg[512];
  WGPU_ERROR_TYPE error = wgpu_device_pop_error_scope_sync(device, msg, sizeof(msg));
  if (strlen(msg) > 0) printf("%s\n", msg);
  assert(!error);

  printf("Test OK\n");
  EM_ASM(window.close());
}
