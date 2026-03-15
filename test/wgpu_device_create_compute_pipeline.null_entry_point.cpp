// Tests wgpu_device_create_compute_pipeline() passing null as the entry point,
// exercising the 'utf8(entryPoint) || void 0' void-0 branch in lib_webgpu.js,
// which tells the browser to use the shader module's default entry point.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Shader with a single compute entry point named "main"
  WGpuShaderModuleDescriptor smdesc = {
    .code = "@compute @workgroup_size(1) fn main() {}",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  // Pass null entryPoint — exercises 'utf8(entryPoint) || void 0' → void 0
  WGpuComputePipeline pipeline = wgpu_device_create_compute_pipeline(
    device, shader, 0 /*null entryPoint*/, WGPU_AUTO_LAYOUT_MODE_AUTO, 0, 0);
  assert(pipeline);
  assert(wgpu_is_compute_pipeline(pipeline));

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
