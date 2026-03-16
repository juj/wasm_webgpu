// Tests wgpu_device_create_compute_pipeline_async() with override constants
// (numConstants > 0), exercising the while-loop body inside $wgpuReadConstants
// via the async compute pipeline creation path. The existing
// wgpu_device_create_compute_pipeline_async.cpp passes constants=0, numConstants=0
// so the while-loop body never runs in that test. Complements the sync variant
// wgpu_device_create_compute_pipeline.constants.cpp.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>

void PipelineCreated(WGpuDevice device, WGpuPipelineError *error, WGpuPipelineBase pipeline, void *userData)
{
  assert(!error);
  assert(pipeline);
  assert(wgpu_is_compute_pipeline(pipeline));
  assert(userData == (void*)77);

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Shader with an override constant whose default value we will override.
  WGpuShaderModuleDescriptor smdesc = {
    .code = "override kFactor: u32 = 1u;"
            "@compute @workgroup_size(1) fn main() { _ = kFactor; }"
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  // Pass a non-empty constants array to exercise the $wgpuReadConstants while-loop
  // body in the async pipeline creation path.
  WGpuPipelineConstant constants[] = {
    { .name = "kFactor", .value = 42.0 },
  };

  wgpu_device_create_compute_pipeline_async(device, shader, "main",
    WGPU_AUTO_LAYOUT_MODE_AUTO, constants, 1, PipelineCreated, (void*)77);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
