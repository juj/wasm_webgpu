// Tests that wgpu_destroy_all_objects() correctly clears all live objects, including
// derived child objects (textures, buffers) created from the device, and that the
// library is left in a clean state where new objects can be created and used.
//
// This test verifies:
//   1. Derived child objects are counted and invalidated by the destroy call.
//   2. After the destroy, new adapter/device/GPU objects can be requested and used.
//
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <stdio.h>

// Handles saved across the destroy so we can assert they are invalidated.
static WGpuTexture  gTexture;
static WGpuBuffer   gBuffer;

// ---- Phase 2: run after wgpu_destroy_all_objects() ----

static void Phase2Device(WGpuDevice device, void *userData)
{
  // adapter + device + queue
  assert(wgpu_get_num_live_objects() == 3);
  assert(wgpu_is_valid_object(device));

  // Confirm the old destroyed handles are still invalid.
  assert(!wgpu_is_valid_object(gTexture));
  assert(!wgpu_is_valid_object(gBuffer));

  // Create a new buffer from the fresh device to confirm the library is usable.
  WGpuBufferDescriptor bdesc = {};
  bdesc.size  = 256;
  bdesc.usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_COPY_SRC;
  WGpuBuffer newBuf = wgpu_device_create_buffer(device, &bdesc);
  assert(newBuf);
  assert(wgpu_is_buffer(newBuf));

  // adapter + device + queue + new buffer
  assert(wgpu_get_num_live_objects() == 4);

  printf("Test OK\n");
  EM_ASM(window.close());
}

static void Phase2Adapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, Phase2Device);
}

// ---- Phase 1: build a graph of objects, destroy all, verify ----

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  // adapter + device + queue
  assert(wgpu_get_num_live_objects() == 3);

  // Create a texture (child of device).
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.width  = 4;
  tdesc.height = 4;
  tdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  tdesc.usage  = WGPU_TEXTURE_USAGE_COPY_DST;
  gTexture = wgpu_device_create_texture(device, &tdesc);
  assert(gTexture);

  // Create a buffer (child of device).
  WGpuBufferDescriptor bdesc = {};
  bdesc.size  = 256;
  bdesc.usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_COPY_SRC;
  gBuffer = wgpu_device_create_buffer(device, &bdesc);
  assert(gBuffer);

  // adapter + device + queue + texture + buffer = 5
  assert(wgpu_get_num_live_objects() == 5);

  wgpu_destroy_all_objects();

  // All objects, including derived ones, must be gone.
  assert(wgpu_get_num_live_objects() == 0);
  assert(!wgpu_is_valid_object(device));
  assert(!wgpu_is_valid_object(gTexture));
  assert(!wgpu_is_valid_object(gBuffer));

  // Re-request an adapter to verify the library is in a usable state after destroy.
  navigator_gpu_request_adapter_async_simple(Phase2Adapter);
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
