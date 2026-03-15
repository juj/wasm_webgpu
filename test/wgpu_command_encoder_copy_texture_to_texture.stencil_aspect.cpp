// Tests wgpu_command_encoder_copy_texture_to_texture() with STENCIL_ONLY aspect,
// exercising GPUTextureAspects[2] = 'stencil-only' in $wgpuReadGpuTexelCopyTextureInfo.
// This is the stencil counterpart to the depth-only copy test.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Create source depth-stencil texture
  WGpuTextureDescriptor srcDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  srcDesc.format = WGPU_TEXTURE_FORMAT_DEPTH24PLUS_STENCIL8;
  srcDesc.usage  = WGPU_TEXTURE_USAGE_COPY_SRC | WGPU_TEXTURE_USAGE_COPY_DST
                   | WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  srcDesc.width  = 4;
  srcDesc.height = 4;
  WGpuTexture srcTex = wgpu_device_create_texture(device, &srcDesc);
  assert(srcTex);

  // Create destination stencil-only texture (stencil8 can receive stencil copies)
  WGpuTextureDescriptor dstDesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  dstDesc.format = WGPU_TEXTURE_FORMAT_STENCIL8;
  dstDesc.usage  = WGPU_TEXTURE_USAGE_COPY_DST | WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT;
  dstDesc.width  = 4;
  dstDesc.height = 4;
  WGpuTexture dstTex = wgpu_device_create_texture(device, &dstDesc);
  assert(dstTex);

  WGpuCommandEncoder enc = wgpu_device_create_command_encoder(device, 0);

  WGpuTexelCopyTextureInfo src = WGPU_TEXEL_COPY_TEXTURE_INFO_DEFAULT_INITIALIZER;
  src.texture = srcTex;
  src.aspect  = WGPU_TEXTURE_ASPECT_STENCIL_ONLY;  // exercises GPUTextureAspects[2]

  WGpuTexelCopyTextureInfo dst = WGPU_TEXEL_COPY_TEXTURE_INFO_DEFAULT_INITIALIZER;
  dst.texture = dstTex;
  dst.aspect  = WGPU_TEXTURE_ASPECT_STENCIL_ONLY;

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  wgpu_command_encoder_copy_texture_to_texture(enc, &src, &dst, 4, 4, 1);
  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), wgpu_command_encoder_finish(enc));
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *m, void *) {
    // Stencil-only copy may or may not be supported; either outcome is acceptable
    (void)type; (void)m;
    EM_ASM(window.close());
  }, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
