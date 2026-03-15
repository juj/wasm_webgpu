// Tests wgpu_device_create_render_bundle_encoder() with a sparse (null, format=0)
// color format slot in the colorFormats array, exercising the
// 'GPUTextureAndVertexFormats[HEAPU32[colorFormatsIdx++]]' path where the value
// is 0 → undefined in the JS array (null render target slot).
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Two color format slots: slot 0 is null (0 = INVALID → undefined), slot 1 is real
  WGPU_TEXTURE_FORMAT formats[2] = {
    WGPU_TEXTURE_FORMAT_INVALID,             // null/sparse slot — exercises undefined path
    navigator_gpu_get_preferred_canvas_format(), // real slot
  };

  WGpuRenderBundleEncoderDescriptor desc = {};
  desc.colorFormats    = formats;
  desc.numColorFormats = 2;
  desc.sampleCount = 1;

  // Sparse formats list works in Firefox Nightly, but not yet in Firefox stable.
  // TODO: Remove this when sparse BGLs are supported.
  if (EM_ASM_INT({return navigator.userAgent.includes("Firefox")}))
  {
    EM_ASM(window.close());
    return;
  }

  // Use an error scope since null format slots in render bundle encoders may
  // not be universally supported
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuRenderBundleEncoder encoder = wgpu_device_create_render_bundle_encoder(device, &desc);
  (void)encoder;
  wgpu_device_pop_error_scope_async(device, [](WGpuDevice d, WGPU_ERROR_TYPE type, const char *m, void *) {
    if (type != WGPU_ERROR_TYPE_NO_ERROR)
      printf("Sparse color format slot not supported (validation error): %s\n", m ? m : "");
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
