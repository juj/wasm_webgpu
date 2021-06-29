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

size_t allocSize = 1024*1024*1024;

uint32_t initialData[1024*1024];

#define MIN(x, y) ((x) < (y) ? (x) : (y))

void alloc_gpu_buffer()
{
  WGpuBufferDescriptor bufferDesc = {};
  bufferDesc.size = allocSize;
  bufferDesc.usage = WGPU_BUFFER_USAGE_VERTEX;
  bufferDesc.mappedAtCreation = EM_TRUE;
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &bufferDesc);

  WGpuBufferMappedRangeStartOffset ofs = wgpu_buffer_get_mapped_range(buffer, 0, allocSize);
  if (ofs == WGPU_BUFFER_GET_MAPPED_RANGE_FAILED)
  {
    allocSize /= 2;
    return;
  }

  for(uint32_t offset = 0; offset < allocSize; offset += 4*1024*1024)
    wgpu_buffer_write_mapped_range(buffer, 0, offset, initialData, MIN(allocSize-offset, 4*1024*1024));

  gpuAllocatedBytes += allocSize;
}

EM_BOOL raf(double time, void *userData)
{
  alloc_gpu_buffer();
  emscripten_mini_stdio_printf("Total allocated %f bytes\n", (double)gpuAllocatedBytes);
  return allocSize > 0;
}

void ObtainedWebGpuDevice(WGpuDevice result, void *userData)
{
  device = result;
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
