// Tests that creating a render bundle encoder with a sparse (null) color format
// slot does NOT produce a WebGPU validation error.
//
// The JS marshalling at lib_webgpu.js:1508 does:
//
//   colorFormats.push(GPUTextureAndVertexFormats[HEAPU32[colorFormatsIdx++]]);
//
// When the slot value is 0 (WGPU_TEXTURE_FORMAT_INVALID), GPUTextureAndVertexFormats[0]
// is `undefined` (empty array slot from the leading comma) rather than `null`.
// The WebGPU spec type for sparse color format entries is GPUTextureFormat? (nullable),
// where null is the correct value and undefined is not. This test fails if the browser
// rejects the `undefined` value with a validation error.
//
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Two-slot format list: slot 0 is sparse (INVALID = 0 → GPUTextureAndVertexFormats[0]
  // = undefined in JS, which should be null per spec), slot 1 is a real format.
  WGPU_TEXTURE_FORMAT formats[2] = {
    WGPU_TEXTURE_FORMAT_INVALID,
    navigator_gpu_get_preferred_canvas_format(),
  };

  WGpuRenderBundleEncoderDescriptor desc = {};
  desc.colorFormats    = formats;
  desc.numColorFormats = 2;
  desc.sampleCount     = 1;

  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);
  WGpuRenderBundleEncoder encoder = wgpu_device_create_render_bundle_encoder(device, &desc);
  (void)encoder;
  wgpu_device_pop_error_scope_async(device,
    [](WGpuDevice, WGPU_ERROR_TYPE type, const char *msg, void *)
    {
      if (msg && strlen(msg) > 0) printf("%s\n", msg);
      // Fails if the browser rejects `undefined` in place of `null` for the
      // sparse color format slot — i.e. the JS marshalling bug is present.
      assert(type == WGPU_ERROR_TYPE_NO_ERROR);
      printf("Test OK\n");
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
