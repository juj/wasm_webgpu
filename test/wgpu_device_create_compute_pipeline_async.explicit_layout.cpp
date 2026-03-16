// Verifies that wgpu_device_create_compute_pipeline_async() correctly handles an explicit
// WGpuPipelineLayout (layout > 1) rather than WGPU_AUTO_LAYOUT_MODE_AUTO. This exercises the
// `layout > 1 ? wgpu[layout] : GPUAutoLayoutMode` true branch in the JS implementation.
// All other compute pipeline async tests pass WGPU_AUTO_LAYOUT_MODE_AUTO.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void PipelineCreated(WGpuDevice device, WGpuPipelineError *error, WGpuPipelineBase pipeline, void *userData)
{
  assert(!error);
  assert(pipeline);
  assert(wgpu_is_compute_pipeline(pipeline));
  assert(userData == (void*)55);

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModuleDescriptor shaderDesc = {
    .code = "@compute @workgroup_size(1) fn main() {}"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &shaderDesc);
  assert(shader);

  // Create an explicit empty pipeline layout (no bind groups) to pass as the layout argument.
  WGpuPipelineLayout layout = wgpu_device_create_pipeline_layout(device, /*bindGroupLayouts=*/0, /*numLayouts=*/0);
  assert(layout);

  wgpu_device_create_compute_pipeline_async(device, shader, "main",
    layout, /*constants=*/0, /*numConstants=*/0,
    PipelineCreated, (void*)55);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
