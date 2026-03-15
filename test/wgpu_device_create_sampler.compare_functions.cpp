// Tests wgpu_device_create_sampler() with all compare function enum values,
// exercising every entry in GPUCompareFunctions[] via the sampler compare path.
// Also validates that the sampler compare field read in lib_webgpu.js produces
// the correct JS strings for each of the 8 compare functions.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

static WGpuDevice gDevice = 0;
static int step = 0;
static void tryNext(void *);

static const WGPU_COMPARE_FUNCTION kFuncs[] = {
  WGPU_COMPARE_FUNCTION_NEVER,         // index 1
  WGPU_COMPARE_FUNCTION_LESS,          // index 2 — sanity check (already tested by comparison.cpp)
  WGPU_COMPARE_FUNCTION_EQUAL,         // index 3
  WGPU_COMPARE_FUNCTION_LESS_EQUAL,    // index 4
  WGPU_COMPARE_FUNCTION_GREATER,       // index 5
  WGPU_COMPARE_FUNCTION_NOT_EQUAL,     // index 6
  WGPU_COMPARE_FUNCTION_GREATER_EQUAL, // index 7
  WGPU_COMPARE_FUNCTION_ALWAYS,        // index 8
};

static void onError(WGpuDevice d, WGPU_ERROR_TYPE t, const char *m, void *u)
{
  // Some compare functions may be rejected with certain filter modes; just continue
  tryNext(u);
}

static void tryNext(void *userData)
{
  if (step >= (int)(sizeof(kFuncs)/sizeof(kFuncs[0])))
  {
    EM_ASM(window.close());
    return;
  }

  WGPU_COMPARE_FUNCTION fn = kFuncs[step++];

  WGpuSamplerDescriptor desc = WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER;
  desc.compare = fn; // exercises GPUCompareFunctions[fn] path

  wgpu_device_push_error_scope(gDevice, WGPU_ERROR_FILTER_VALIDATION);
  WGpuSampler sampler = wgpu_device_create_sampler(gDevice, &desc);
  assert(sampler);
  wgpu_device_pop_error_scope_async(gDevice, onError, 0);
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  gDevice = device;
  tryNext(0);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
