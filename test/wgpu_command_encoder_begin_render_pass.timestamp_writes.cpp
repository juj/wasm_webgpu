// flags: -sEXIT_RUNTIME=0

#include "lib_webgpu.h"
#include <stdio.h>
#include <assert.h>

WGpuAdapter adapter;
bool no_error = true;

void OnError(WGpuDevice device, WGPU_ERROR_TYPE errorType, const char *errorMessage, void *userData)
{
  assert(wgpu_is_device(device));
  assert(errorType == WGPU_ERROR_FILTER_NO_ERROR);
  assert(userData == (void*)42);
  assert(errorMessage == 0);
  no_error = true;
}

void bufferCallback(WGpuBuffer buffer, void *userData, WGPU_MAP_MODE_FLAGS mode, double_int53_t offset, double_int53_t size)
{
  double_int53_t start = wgpu_buffer_get_mapped_range(buffer, 0);
  uint64_t data[3] = { (uint64_t)-1, (uint64_t)-1, (uint64_t)-1 };
  wgpu_buffer_read_mapped_range(buffer, 0, 0, data, sizeof(data));
  printf("Timestamps: %llu %llu %llu\n", data[0], data[1], data[2]);
  printf("timestamp delta: %llu\n", data[2] - data[1]);
  assert(data[0] == 0); // We didn't write to this index.
  assert(data[1] > 0);
  assert(data[2] > data[1]);
  assert(data[2] - data[1] < 10'000'000);

  if (no_error) EM_ASM(window.close());
}

void ObtainedWebGpuDevice(WGpuDevice device, void *userData)
{
  wgpu_device_push_error_scope(device, WGPU_ERROR_FILTER_VALIDATION);

  WGpuCanvasConfiguration config = WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;
  config.device = device;
  config.format = navigator_gpu_get_preferred_canvas_format();
  wgpu_canvas_context_configure(wgpu_canvas_get_webgpu_context("canvas"), &config);

  const char *vertexShader =
    "@vertex\n"
    "fn main(@builtin(vertex_index) vertexIndex : u32) -> @builtin(position) vec4<f32> {\n"
      "var pos = array<vec2<f32>, 3>(\n"
        "vec2<f32>(0.0, 0.5),\n"
        "vec2<f32>(-0.5, -0.5),\n"
        "vec2<f32>(0.5, -0.5)\n"
      ");\n"

      "return vec4<f32>(pos[vertexIndex], 0.0, 1.0);\n"
    "}\n";

  const char *fragmentShader =
    "@fragment\n"
    "fn main() -> @location(0) vec4<f32> {\n"
      "return vec4<f32>(1.0, 0.5, 0.3, 1.0);\n"
    "}\n";

  WGpuShaderModuleDescriptor shaderModuleDesc = { .code = vertexShader };
  WGpuShaderModule vs = wgpu_device_create_shader_module(device, &shaderModuleDesc);

  shaderModuleDesc.code = fragmentShader;
  WGpuShaderModule fs = wgpu_device_create_shader_module(device, &shaderModuleDesc);

  WGpuRenderPipelineDescriptor renderPipelineDesc = WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;
  renderPipelineDesc.vertex.module = vs;
  renderPipelineDesc.vertex.entryPoint = "main";
  renderPipelineDesc.fragment.module = fs;
  renderPipelineDesc.fragment.entryPoint = "main";

  WGpuColorTargetState colorTarget = WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;
  colorTarget.format = config.format;
  renderPipelineDesc.fragment.numTargets = 1;
  renderPipelineDesc.fragment.targets = &colorTarget;

  WGpuRenderPipeline renderPipeline = wgpu_device_create_render_pipeline(device, &renderPipelineDesc);

  WGpuCommandEncoder encoder = wgpu_device_create_command_encoder(device, 0);

  WGpuRenderPassColorAttachment colorAttachment = WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;
  colorAttachment.view = wgpu_texture_create_view(wgpu_canvas_context_get_current_texture(wgpu_canvas_get_webgpu_context("canvas")), 0);

  WGpuQuerySetDescriptor queryDesc = {
    .type = WGPU_QUERY_TYPE_TIMESTAMP,
    .count = 3,
  };
  WGpuQuerySet querySet = wgpu_device_create_query_set(device, &queryDesc);
  assert(querySet);

  WGpuRenderPassDescriptor passDesc = {
    .colorAttachments = &colorAttachment,
    .numColorAttachments = 1,
    .timestampWrites = (WGpuRenderPassTimestampWrites) {
      .querySet = querySet,
      .beginningOfPassWriteIndex = 1,
      .endOfPassWriteIndex = 2,
    }
  };
  WGpuRenderPassEncoder pass = wgpu_command_encoder_begin_render_pass(encoder, &passDesc);

  WGpuBufferDescriptor bufferDesc = {
    .size = 24,
    .usage = WGPU_BUFFER_USAGE_COPY_SRC | WGPU_BUFFER_USAGE_QUERY_RESOLVE,
  };
  WGpuBuffer buffer = wgpu_device_create_buffer(device, &bufferDesc);
  assert(buffer);
  bufferDesc.usage = WGPU_BUFFER_USAGE_COPY_DST | WGPU_BUFFER_USAGE_MAP_READ;
  WGpuBuffer buffer2 = wgpu_device_create_buffer(device, &bufferDesc);

  wgpu_device_pop_error_scope_async(device, OnError, (void*)42); // Check that we see that error being diagnosed

  wgpu_render_pass_encoder_set_pipeline(pass, renderPipeline);
  wgpu_render_pass_encoder_draw(pass, 3, 1, 0, 0);
  wgpu_render_pass_encoder_end(pass);

  wgpu_command_encoder_resolve_query_set(encoder, querySet, 0, 3, buffer, 0);
  wgpu_command_encoder_copy_buffer_to_buffer(encoder, buffer, 0, buffer2, 0, 24);

  WGpuCommandBuffer commandBuffer = wgpu_command_encoder_finish(encoder);

  wgpu_queue_submit_one_and_destroy(wgpu_device_get_queue(device), commandBuffer);

  wgpu_buffer_map_async(buffer2, bufferCallback, 0, WGPU_MAP_MODE_READ);
}

void ObtainedWebGpuAdapter(WGpuAdapter result, void *userData)
{
  adapter = result;
  WGpuDeviceDescriptor desc = {};
  desc.requiredFeatures = WGPU_FEATURE_TIMESTAMP_QUERY; // This test needs timestamp queries to be supported.
  wgpu_adapter_request_device_async(adapter, &desc, ObtainedWebGpuDevice, 0);
}

int main()
{
  navigator_gpu_request_adapter_async_simple(ObtainedWebGpuAdapter);
}
