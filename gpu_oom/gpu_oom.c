#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <math.h>
#include <emscripten/em_math.h>
#include <miniprintf.h>
#include "lib_webgpu.h"
#include "lib_demo.h"

WGpuAdapter adapter;
WGpuPresentationContext presentationContext;
WGpuDevice device;
uint64_t gpuAllocatedBytes = 0;
uint64_t largestSuccessfulAllocation = 0;

uint64_t allocSize = 128ULL*1024*1024*1024;

uint32_t initialData[1024*1024];

#define MIN(x, y) ((x) < (y) ? (x) : (y))

void deviceLost(WGpuDevice device, WGPU_DEVICE_LOST_REASON deviceLostReason, const char *message, void *userData)
{
  emscripten_mini_stdio_fprintf(EM_STDERR, "WebGPU device lost! reason: %d, message: \"%s\"\n", deviceLostReason, message);  
}

void oom(WGpuDevice device, WGPU_ERROR_FILTER errorType, const char *errorMessage, void *userData)
{
  if (errorType == WGPU_ERROR_FILTER_OUT_OF_MEMORY)
    emscripten_mini_stdio_fprintf(EM_STDERR, "WebGPU device is out of memory! Error message: %s\n", errorMessage ? errorMessage : "(no error message specified)");
  else
    emscripten_mini_stdio_printf("WebGPU device no oom error reported.\n");
}

void alloc_gpu_buffer()
{
  WGpuBufferDescriptor bufferDesc = {};
  bufferDesc.size = allocSize;
  bufferDesc.usage = WGPU_BUFFER_USAGE_VERTEX;
  bufferDesc.mappedAtCreation = EM_TRUE;
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_OUT_OF_MEMORY);
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &bufferDesc);
  wgpu_device_pop_error_scope_async(device, oom, 0);

  WGpuBufferMappedRangeStartOffset ofs = wgpu_buffer_get_mapped_range(buffer, 0, allocSize);
  if (ofs == WGPU_BUFFER_GET_MAPPED_RANGE_FAILED)
  {
    emscripten_mini_stdio_fprintf(EM_STDERR, "wgpu_buffer_get_mapped_range() of size %f failed!\n", (double)allocSize);
    allocSize = allocSize * 3 / 4;
    return;
  }

  for(uint64_t offset = 0; offset < allocSize; offset += 4*1024*1024)
    wgpu_buffer_write_mapped_range(buffer, 0, offset, initialData, MIN(allocSize-offset, 4*1024*1024));

  if (!largestSuccessfulAllocation) largestSuccessfulAllocation = allocSize;
  gpuAllocatedBytes += allocSize;
}

EM_BOOL raf(double time, void *userData)
{
  alloc_gpu_buffer();
  emscripten_mini_stdio_printf("Total allocated %f bytes. Largest successful allocation size: %f bytes.\n", (double)gpuAllocatedBytes, (double)largestSuccessfulAllocation);
  return allocSize > 0;
}

void ObtainedWebGpuDevice(WGpuDevice result, void *userData)
{
  device = result;
  wgpu_device_set_lost_callback(device, deviceLost, 0);
  presentationContext = wgpu_canvas_get_canvas_context("canvas");
  WGpuPresentationConfiguration config = WGPU_PRESENTATION_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = wgpu_presentation_context_get_preferred_format(presentationContext, adapter);
  wgpu_presentation_context_configure(presentationContext, &config);

  for(int i = 0; i < 1024*1024; ++i)
    initialData[i] = i;

  emscripten_request_animation_frame_loop(raf, 0);
}

void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData)
{
  adapter = result;
  WGpuDeviceDescriptor deviceDesc = {};
  wgpu_adapter_request_device_async(adapter, &deviceDesc, ObtainedWebGpuDevice, 0);
}

int main()
{
  WGpuRequestAdapterOptions options = {};
  navigator_gpu_request_adapter_async(&options, ObtainedWebGpuAdapter, 0);
}
