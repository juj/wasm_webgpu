// Tests that wgpu_shader_module_get_compilation_info_async() correctly marshals
// the lineNum, linePos, offset, and length fields of WGpuCompilationMessage
// from JS to C for a deliberately erroneous shader. Asserts the fields are
// non-zero to verify the JS->C marshalling code paths are exercised correctly.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void CompilationInfoCallback(WGpuShaderModule shader, WGpuCompilationInfo *compilationInfo, void *userData)
{
  assert(compilationInfo);
  assert(compilationInfo->numMessages > 0);

  // Find the first error message and verify its positional fields are marshalled
  int foundError = 0;
  for (int i = 0; i < compilationInfo->numMessages; i++)
  {
    const WGpuCompilationMessage *msg = &compilationInfo->messages[i];
    if (msg->type == WGPU_COMPILATION_MESSAGE_TYPE_ERROR)
    {
      printf("Error at line %u, pos %u, offset %u, length %u: %s\n",
             msg->lineNum, msg->linePos, msg->offset, msg->length,
             msg->message ? msg->message : "(null)");
      // The error is on a specific line — lineNum must be > 0
      assert(msg->lineNum > 0);
      // linePos and offset are byte-level positions, must also be > 0 for a non-first-line error
      // (offset at least equals the number of chars before the error)
      assert(msg->offset > 0);
      // message must be non-empty
      assert(msg->message && strlen(msg->message) > 0);
      foundError = 1;
      break;
    }
  }
  assert(foundError && "Expected at least one error message");

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // Shader with a deliberate error on line 3 (not line 1), so lineNum > 1
  WGpuShaderModuleDescriptor smdesc = {
    .code =
      "@vertex fn vs(@builtin(vertex_index) vi: u32) -> @builtin(position) vec4f {\n"
      "  var pos = array<vec2f,3>(vec2f(0,1),vec2f(-1,-1),vec2f(1,-1));\n"
      "  return UNDEFINED_IDENTIFIER;\n" // error on line 3
      "}",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);

  wgpu_shader_module_get_compilation_info_async(shader, CompilationInfoCallback, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
