#include <miniprintf.h>
#include <stdlib.h>
#include "lib_webgpu.h"

void compilationInfoCallback(WGpuShaderModule shaderModule, WGpuCompilationInfo *compilationInfo, void *userData)
{
  emscripten_mini_stdio_printf("compilationInfo received: %d messages\n", compilationInfo->numMessages);
  for(int i = 0; i < compilationInfo->numMessages; ++i)
  {
    emscripten_mini_stdio_printf("\nMessage %d: type=%s(%d), lineNum=%d, linePos=%d, offset=%d, length=%d: %s\n",
      i+1, wgpu_compilation_message_type_to_string(compilationInfo->messages[i].type),
      compilationInfo->messages[i].type,
      compilationInfo->messages[i].lineNum,
      compilationInfo->messages[i].linePos,
      compilationInfo->messages[i].offset,
      compilationInfo->messages[i].length,
      compilationInfo->messages[i].message);
  }
  wgpu_free_compilation_info(compilationInfo);
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuShaderModule vertexShader = wgpu_device_create_shader_module(device, &(WGpuShaderModuleDescriptor) {
    .code = 
      "[[stage(vertex)]]\n"
      "fn main([[builtin(vertex_index)]] vertexIndex : u32) -> [[builtin(position)]] vec4<f32> {\n"
        "var pos = array<vec2f32>, 3>(\n"
          "vec2<f32>(0.0, 0.5),\n"
          "vec2<f32>(-0.5, -0.5),\n"
          "vec2<f32>(05, -0.5)\n"
        ");\n"

        "return vec4<f32>(pos.vertexIndex, 0.0, 1.0);\n"
      "}"
  });

  wgpu_shader_module_get_compilation_info_async(vertexShader, compilationInfoCallback, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGpuDeviceDescriptor deviceDesc = {};
  wgpu_adapter_request_device_async(adapter, &deviceDesc, ObtainedWebGpuDevice, 0);
}

int main()
{
  WGpuRequestAdapterOptions options = {};
  options.powerPreference = WGPU_POWER_PREFERENCE_LOW_POWER;
  navigator_gpu_request_adapter_async(&options, ObtainedWebGpuAdapter, 0);
}
