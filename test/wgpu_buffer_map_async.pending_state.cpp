// Tests that wgpu_buffer_map_state() returns WGPU_BUFFER_MAP_STATE_PENDING
// between the map request being issued and the callback being called,
// exercising the PENDING state in the GPUBufferMapStates marshalling path.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

static WGpuBuffer gBuffer;

void BufferMapped(WGpuBuffer buffer, void *userData, WGPU_MAP_MODE_FLAGS mode,
                  double_int53_t offset, double_int53_t size)
{
  assert(wgpu_buffer_map_state(buffer) == WGPU_BUFFER_MAP_STATE_MAPPED);
  wgpu_buffer_unmap(buffer);
  assert(wgpu_buffer_map_state(buffer) == WGPU_BUFFER_MAP_STATE_UNMAPPED);

  EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  WGpuBufferDescriptor desc = {
    .size  = 64,
    .usage = WGPU_BUFFER_USAGE_MAP_READ | WGPU_BUFFER_USAGE_COPY_DST,
  };
  gBuffer = wgpu_device_create_buffer(device, &desc);
  assert(gBuffer);

  // Verify unmapped state before mapping
  assert(wgpu_buffer_map_state(gBuffer) == WGPU_BUFFER_MAP_STATE_UNMAPPED);

  // Request mapping — state transitions to PENDING synchronously
  wgpu_buffer_map_async(gBuffer, BufferMapped, 0, WGPU_MAP_MODE_READ, 0, -1);

  // Immediately check the state — should be PENDING (exercises WGPU_BUFFER_MAP_STATE_PENDING)
  WGPU_BUFFER_MAP_STATE state = wgpu_buffer_map_state(gBuffer);
  printf("Map state after map_async: %d (expected %d = PENDING)\n",
         state, WGPU_BUFFER_MAP_STATE_PENDING);
  assert(state == WGPU_BUFFER_MAP_STATE_PENDING);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
