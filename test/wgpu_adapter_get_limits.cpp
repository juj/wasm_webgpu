// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  WGpuSupportedLimits limits;
  memset(&limits, 0xFF, sizeof(limits)); // Fill with garbage to test that all fields are filled.

  wgpu_adapter_get_limits(adapter, &limits);

#define TEST64(x, lowerBound) assert(x >= lowerBound); assert(x < 8ull*1024*1024*1024);
#define TEST32(x, lowerBound) assert(x >= lowerBound); assert(x < 2ull*1024*1024*1024);

  TEST64(limits.maxUniformBufferBindingSize, 65536);
  TEST64(limits.maxStorageBufferBindingSize, 128*1024*1024);
  TEST64(limits.maxBufferSize, 256*1024*1024);

  TEST32(limits.maxTextureDimension1D, 8192);
  TEST32(limits.maxTextureDimension2D, 8192);
  TEST32(limits.maxTextureDimension3D, 2048);
  TEST32(limits.maxTextureArrayLayers, 256);
  TEST32(limits.maxBindGroups, 4);
  TEST32(limits.maxBindGroupsPlusVertexBuffers, 24);
  TEST32(limits.maxBindingsPerBindGroup, 1000);
  TEST32(limits.maxDynamicUniformBuffersPerPipelineLayout, 8);
  TEST32(limits.maxDynamicStorageBuffersPerPipelineLayout, 4);
  TEST32(limits.maxSampledTexturesPerShaderStage, 16);
  TEST32(limits.maxSamplersPerShaderStage, 16);
  TEST32(limits.maxStorageBuffersPerShaderStage, 8);
  TEST32(limits.maxStorageTexturesPerShaderStage, 4);
  TEST32(limits.maxUniformBuffersPerShaderStage, 12);
  TEST32(limits.minUniformBufferOffsetAlignment, 256);
  TEST32(limits.minStorageBufferOffsetAlignment, 256);
  TEST32(limits.maxVertexBuffers, 8);
  TEST32(limits.maxVertexAttributes , 16);
  TEST32(limits.maxVertexBufferArrayStride, 2048);
  TEST32(limits.maxInterStageShaderVariables, 16);
  TEST32(limits.maxColorAttachments, 8);
  TEST32(limits.maxColorAttachmentBytesPerSample, 32);
  TEST32(limits.maxComputeWorkgroupStorageSize, 16384);
  TEST32(limits.maxComputeInvocationsPerWorkgroup, 256);
  TEST32(limits.maxComputeWorkgroupSizeX, 256);
  TEST32(limits.maxComputeWorkgroupSizeY, 256);
  TEST32(limits.maxComputeWorkgroupSizeZ, 64);

  EM_ASM(window.close());
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
