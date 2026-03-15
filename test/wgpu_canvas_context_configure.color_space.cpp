// Tests wgpu_canvas_context_configure() with non-default color spaces
// (HTML_PREDEFINED_COLOR_SPACE_SRGB_LINEAR, _DISPLAY_P3, _DISPLAY_P3_LINEAR).
// This exercises HTMLPredefinedColorSpaces[2/3/4] paths in lib_webgpu.js.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

static WGpuDevice gDevice = 0;
static int step = 0;

static void tryNextColorSpace(void *);

static void errorCallback(WGpuDevice d, WGPU_ERROR_TYPE type, const char *msg, void *userData)
{
  // Ignore validation errors (color space not supported), just move on.
  (void)type; (void)msg;
  tryNextColorSpace(userData);
}

static HTML_PREDEFINED_COLOR_SPACE colorSpaces[] = {
  HTML_PREDEFINED_COLOR_SPACE_SRGB_LINEAR,
  HTML_PREDEFINED_COLOR_SPACE_DISPLAY_P3,
  HTML_PREDEFINED_COLOR_SPACE_DISPLAY_P3_LINEAR,
};

static void tryNextColorSpace(void *userData)
{
  if (step >= (int)(sizeof(colorSpaces)/sizeof(colorSpaces[0])))
  {
    EM_ASM(window.close());
    return;
  }

  WGpuCanvasContext ctx = wgpu_canvas_get_webgpu_context("canvas");
  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = gDevice;
  config.format = navigator_gpu_get_preferred_canvas_format();
  config.colorSpace = colorSpaces[step++];

  wgpu_device_push_error_scope(gDevice, WGPU_ERROR_FILTER_VALIDATION);
  wgpu_canvas_context_configure(ctx, &config);
  wgpu_device_pop_error_scope_async(gDevice, errorCallback, 0);
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  gDevice = device;
  tryNextColorSpace(0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
