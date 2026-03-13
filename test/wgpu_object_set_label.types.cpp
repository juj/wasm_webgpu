// Verifies that wgpu_object_set_label() and wgpu_object_get_label() work correctly on several different WebGPU object types: buffer, texture, shader module, queue, and sampler.
// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <assert.h>
#include <string.h>

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  char label[128];

  // Label on a buffer
  WGpuBufferDescriptor bdesc = {
    .size = 64,
    .usage = WGPU_BUFFER_USAGE_COPY_DST,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &bdesc);
  assert(buffer);
  wgpu_object_set_label(buffer, "test-buffer");
  wgpu_object_get_label(buffer, label, sizeof(label));
  assert(strcmp(label, "test-buffer") == 0);

  // Label on a texture
  WGpuTextureDescriptor tdesc = WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;
  tdesc.format = WGPU_TEXTURE_FORMAT_RGBA8UNORM;
  tdesc.usage = WGPU_TEXTURE_USAGE_TEXTURE_BINDING;
  WGpuTexture texture = wgpu_device_create_texture(device, &tdesc);
  assert(texture);
  wgpu_object_set_label(texture, "test-texture");
  wgpu_object_get_label(texture, label, sizeof(label));
  assert(strcmp(label, "test-texture") == 0);

  // Label on a shader module
  WGpuShaderModuleDescriptor smdesc = {
    .code = "@compute @workgroup_size(1) fn main() {}",
  };
  WGpuShaderModule shader = wgpu_device_create_shader_module(device, &smdesc);
  assert(shader);
  wgpu_object_set_label(shader, "test-shader");
  wgpu_object_get_label(shader, label, sizeof(label));
  assert(strcmp(label, "test-shader") == 0);

  // Label on the device's queue
  WGpuQueue queue = wgpu_device_get_queue(device);
  assert(queue);
  wgpu_object_set_label(queue, "test-queue");
  wgpu_object_get_label(queue, label, sizeof(label));
  assert(strcmp(label, "test-queue") == 0);

  // Label on a sampler
  WGpuSamplerDescriptor sdesc = WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER;
  WGpuSampler sampler = wgpu_device_create_sampler(device, &sdesc);
  assert(sampler);
  wgpu_object_set_label(sampler, "test-sampler");
  wgpu_object_get_label(sampler, label, sizeof(label));
  assert(strcmp(label, "test-sampler") == 0);

  EM_ASM(window.close());
}

void ObtainedWebGpuAdapter(WGpuAdapter adapter, void *userData)
{
  wgpu_adapter_request_device_async_simple(adapter, ObtainedWebGpuDevice);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
