// Tests wgpu_device_create_compute_pipeline_async() passing null as the entry
// point, exercising the 'utf8(entryPoint) || void 0' void-0 branch in the
// async variant's JS implementation. The sync variant has a dedicated test
// (wgpu_device_create_compute_pipeline.null_entry_point.cpp) but the async
// path through the same branch had no test.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void PipelineCreated(WGpuDevice device, WGpuPipelineError *error, WGpuPipelineBase pipeline, void *userData)
{
  assert(!error);
  assert(pipeline);
  assert(wgpu_is_compute_pipeline(pipeline));

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Shader with a single compute entry point named "main".
  WGpuShaderModuleDescriptor smdesc = {
    .code = "@compute @workgroup_size(1) fn main() {}",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  // Pass 0 (null) as entryPoint — exercises the 'utf8(entryPoint) || void 0'
  // false branch in $wgpu_device_create_compute_pipeline_async, passing
  // undefined as the entry point so the browser uses the shader's sole entry.
  wgpu_device_create_compute_pipeline_async(device, shader, 0 /*null entryPoint*/,
    WGPU_AUTO_LAYOUT_MODE_AUTO, 0, 0, PipelineCreated, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
