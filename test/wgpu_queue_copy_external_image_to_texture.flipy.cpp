// Tests wgpu_queue_copy_external_image_to_texture() with flipY=true on the
// source, exercising the '!!HEAPU32[source+3]' true branch in lib_webgpu.js.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

// Minimal 1x1 pixel PNG as a data URL.
static const char *kTestImageUrl =
  "data:image/png;base64,"
  "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJ"
  "AAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==";

static WGpuDevice gDevice;

void ErrorScopePopped(WGpuDevice device, WGPU_ERROR_TYPE errorType, const char *errorMessage, void *userData)
{
  if (errorMessage) printf("%s\n", errorMessage);
  assert(errorType == WGPU_ERROR_TYPE_NO_ERROR);
  EM_ASM(window.close());
}

void ImageLoaded(WGpuImageBitmap bitmap, int width, int height, void *userData)
{
  assert(bitmap);

  wgpu_device_push_error_scope(gDevice, WGPU_ERROR_FILTER_VALIDATION);

  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.width  = (uint32_t)width;
  tdesc.height = (uint32_t)height;
  tdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  tdesc.usage  = WGPU_TEXTURE_USAGE_COPY_DST | WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  WGpuTexture texture = wgpu_device_create_texture(gDevice, &tdesc);
  assert(texture);

  WGpuCopyExternalImageSourceInfo src = WGPU_COPY_EXTERNAL_IMAGE_SOURCE_INFO_DEFAULT_INITIALIZER;
  src.source = bitmap;
  src.flipY  = WGPU_TRUE; // exercises '!!HEAPU32[source+3]' true branch

  WGpuCopyExternalImageDestInfo dst = WGPU_COPY_EXTERNAL_IMAGE_DEST_INFO_DEFAULT_INITIALIZER;
  dst.texture = texture;

  wgpu_queue_copy_external_image_to_texture(
    wgpu_device_get_queue(gDevice), &src, &dst, (uint32_t)width, (uint32_t)height);

  wgpu_device_pop_error_scope_async(gDevice, ErrorScopePopped, 0);
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  gDevice = device;
  wgpu_load_image_bitmap_from_url_async(kTestImageUrl, WGPU_FALSE, ImageLoaded, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
