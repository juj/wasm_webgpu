#pragma once

#include <emscripten/html5.h>
#include <stdint.h>

#ifdef __cplusplus
#define _WGPU_DEFAULT_VALUE(x) = x
#else
#define _WGPU_DEFAULT_VALUE(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef int WGpuObjectRef;
typedef int WGpuAdapter;
typedef int WGpuDevice;
typedef int WGpuCanvasContext;
typedef int WGpuTexture;
typedef int WGpuSwapChain;
typedef int WGpuShaderModule;
typedef int WGpuSampler;
typedef int WGpuRenderPipeline;
typedef int WGpuCommandEncoder;
typedef int WGpuBindGroupLayout;
typedef int WGpuTextureView;
typedef int WGpuRenderPassEncoder;
typedef int WGpuQuerySet;
typedef int WGpuCommandBuffer;
typedef int WGpuQueue;
typedef int WGpuBuffer;

// Use double to represent a JavaScript number that can
// address 2^53 == 9007199254740992 = ~9.0 petabytes.
typedef double double_int53_t;

typedef double_int53_t WGpuBufferMappedRangeStartOffset;

#include "lib_webgpu_strings.h"

typedef int WGPU_FEATURES_BITFIELD;
#define WGPU_FEATURE_DEPTH_CLAMPING            0x01
#define WGPU_FEATURE_DEPTH24UNORM_STENCIL8     0x02
#define WGPU_FEATURE_DEPTH32FLOAT_STENCIL8     0x04
#define WGPU_FEATURE_PIPELINE_STATISTICS_QUERY 0x08
#define WGPU_FEATURE_TEXTURE_COMPRESSION_BC    0x10 // TODO what is this? Why not other compression formats in this bitfield? TODO: Propose direct texture format supported queries
#define WGPU_FEATURE_TIMESTAMP_QUERY           0x20

typedef int WGPU_TEXTURE_USAGE_FLAGS;
#define WGPU_TEXTURE_USAGE_COPY_SRC 0x01
#define WGPU_TEXTURE_USAGE_COPY_DST 0x02
#define WGPU_TEXTURE_USAGE_SAMPLED  0x04
#define WGPU_TEXTURE_USAGE_STORAGE  0x08
#define WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT  0x10

typedef int WGPU_COLOR_WRITE_FLAGS;
#define WGPU_COLOR_WRITE_RED   0x01
#define WGPU_COLOR_WRITE_GREEN 0x02
#define WGPU_COLOR_WRITE_BLUE  0x04
#define WGPU_COLOR_WRITE_ALPHA 0x08
#define WGPU_COLOR_WRITE_ALL   0x0F

typedef int WGPU_BUFFER_USAGE_FLAGS;
#define WGPU_BUFFER_USAGE_MAP_READ      0x0001
#define WGPU_BUFFER_USAGE_MAP_WRITE     0x0002
#define WGPU_BUFFER_USAGE_COPY_SRC      0x0004
#define WGPU_BUFFER_USAGE_COPY_DST      0x0008
#define WGPU_BUFFER_USAGE_INDEX         0x0010
#define WGPU_BUFFER_USAGE_VERTEX        0x0020
#define WGPU_BUFFER_USAGE_UNIFORM       0x0040
#define WGPU_BUFFER_USAGE_STORAGE       0x0080
#define WGPU_BUFFER_USAGE_INDIRECT      0x0100
#define WGPU_BUFFER_USAGE_QUERY_RESOLVE 0x0200

typedef int WGPU_MAP_MODE_FLAGS;
#define WGPU_MAP_MODE_READ   0x1
#define WGPU_MAP_MODE_WRITE  0x2

typedef int WGPU_SHADER_STAGE_FLAGS;
#define WGPU_SHADER_STAGE_VERTEX   0x1
#define WGPU_SHADER_STAGE_FRAGMENT 0x2
#define WGPU_SHADER_STAGE_COMPUTE  0x4

#define WGPU_LOAD_OP_CONSTANT_VALUE 0

typedef struct WGpuRequestAdapterOptions
{
  // Optionally provides a hint indicating what class of adapter should be selected from the system’s available adapters.
  // The value of this hint may influence which adapter is chosen, but it must not influence whether an adapter is returned or not.
  // Note: The primary utility of this hint is to influence which GPU is used in a multi-GPU system. For instance, some laptops
  //       have a low-power integrated GPU and a high-performance discrete GPU.
  // Note: Depending on the exact hardware configuration, such as battery status and attached displays or removable GPUs, the user
  //       agent may select different adapters given the same power preference. Typically, given the same hardware configuration and
  //       state and powerPreference, the user agent is likely to select the same adapter.
  WGPU_POWER_PREFERENCE powerPreference;
} WGpuRequestAdapterOptions;

typedef struct WGpuAdapterProperties
{
  char name[256];
  WGPU_FEATURES_BITFIELD features;
  uint32_t maxTextureDimension1D;
  uint32_t maxTextureDimension2D;
  uint32_t maxTextureDimension3D;
  uint32_t maxTextureArrayLayers;
  uint32_t maxBindGroups;
  uint32_t maxDynamicUniformBuffersPerPipelineLayout;
  uint32_t maxDynamicStorageBuffersPerPipelineLayout;
  uint32_t maxSampledTexturesPerShaderStage;
  uint32_t maxSamplersPerShaderStage;
  uint32_t maxStorageBuffersPerShaderStage;
  uint32_t maxStorageTexturesPerShaderStage;
  uint32_t maxUniformBuffersPerShaderStage;
  uint32_t maxUniformBufferBindingSize;
  uint32_t maxStorageBufferBindingSize;
  uint32_t maxVertexBuffers;
  uint32_t maxVertexAttributes;
  uint32_t maxVertexBufferArrayStride;
} WGpuAdapterProperties;

typedef struct WGpuDeviceDescriptor
{
  WGPU_FEATURES_BITFIELD nonGuaranteedFeatures;
  uint32_t maxTextureDimension1D;
  uint32_t maxTextureDimension2D;
  uint32_t maxTextureDimension3D;
  uint32_t maxTextureArrayLayers;
  uint32_t maxBindGroups;
  uint32_t maxDynamicUniformBuffersPerPipelineLayout;
  uint32_t maxDynamicStorageBuffersPerPipelineLayout;
  uint32_t maxSampledTexturesPerShaderStage;
  uint32_t maxSamplersPerShaderStage;
  uint32_t maxStorageBuffersPerShaderStage;
  uint32_t maxStorageTexturesPerShaderStage;
  uint32_t maxUniformBuffersPerShaderStage;
  uint32_t maxUniformBufferBindingSize;
  uint32_t maxStorageBufferBindingSize;
  uint32_t maxVertexBuffers;
  uint32_t maxVertexAttributes;
  uint32_t maxVertexBufferArrayStride;
} WGpuDeviceDescriptor;
extern const WGpuDeviceDescriptor WGPU_DEVICE_DESCRIPTOR_DEFAULT_INITIALIZER;

typedef struct WGpuSwapChainDescriptor
{
  WGpuDevice device;
  WGPU_TEXTURE_FORMAT format;
  WGPU_TEXTURE_USAGE_FLAGS usage;
} WGpuSwapChainDescriptor;
extern const WGpuSwapChainDescriptor WGPU_SWAP_CHAIN_DESCRIPTOR_DEFAULT_INITIALIZER;

typedef struct WGpuShaderModuleDescriptor
{
	const char *code;
} WGpuShaderModuleDescriptor;

typedef struct WGpuVertexAttribute
{
  uint64_t offset;
  uint32_t shaderLocation;
  WGPU_VERTEX_FORMAT format;
} WGpuVertexAttribute;

typedef struct WGpuVertexBufferLayout
{
  int numAttributes;
  const WGpuVertexAttribute *attributes;
  uint64_t arrayStride;
  WGPU_INPUT_STEP_MODE stepMode;
} WGpuVertexBufferLayout;

typedef struct WGpuVertexState
{
  WGpuShaderModule module;
  const char *entryPoint;
  int numBuffers;
  const WGpuVertexBufferLayout *buffers;
} WGpuVertexState;

typedef struct WGpuPrimitiveState
{
  WGPU_PRIMITIVE_TOPOLOGY topology; // Defaults to WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ('triangle-list')
  WGPU_INDEX_FORMAT stripIndexFormat; // Defaults to undefined, must be explicitly specified if WGPU_PRIMITIVE_TOPOLOGY_LINE_STRIP ('line-strip') or WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP ('triangle-strip') is used.
  WGPU_FRONT_FACE frontFace; // Defaults to WGPU_FRONT_FACE_CCW ('ccw')
  WGPU_CULL_MODE cullMode; // Defaults to WGPU_CULL_MODE_NONE ('none')
} WGpuPrimitiveState;

typedef struct WGpuStencilFaceState
{
  WGPU_COMPARE_FUNCTION compare;
  WGPU_STENCIL_OPERATION failOp;
  WGPU_STENCIL_OPERATION depthFailOp;
  WGPU_STENCIL_OPERATION passOp;
} WGpuStencilFaceState;

typedef struct WGpuDepthStencilState
{
  // Pass format == WGPU_TEXTURE_FORMAT_INVALID (integer value 0)
  // to disable depth+stenciling altogether.
  WGPU_TEXTURE_FORMAT format;

  EM_BOOL depthWriteEnabled;
  WGPU_COMPARE_FUNCTION depthCompare;

  uint32_t stencilReadMask;
  uint32_t stencilWriteMask;

  int32_t depthBias;
  float depthbiasSlopeScale;
  float depthBiasClamp;

  WGpuStencilFaceState stencilFront;
  WGpuStencilFaceState stencilBack;

  // Enable depth clamping (requires "depth-clamping" feature)
  EM_BOOL clampDepth;
} WGpuDepthStencilState;

typedef struct WGpuMultisampleState
{
  uint32_t count;
  uint32_t mask;
  EM_BOOL alphaToCoverageEnabled;
} WGpuMultisampleState;

typedef struct WGpuBlendComponent
{
  WGPU_BLEND_FACTOR srcFactor;
  WGPU_BLEND_FACTOR dstFactor;
  WGPU_BLEND_OPERATION operation;
} WGpuBlendComponent;

typedef struct WGpuBlendState
{
  WGpuBlendComponent color;
  WGpuBlendComponent alpha;
} WGpuBlendState;

typedef struct WGpuColorTargetState
{
  WGPU_TEXTURE_FORMAT format;

  WGpuBlendState blend;
  WGPU_COLOR_WRITE_FLAGS writeMask;
} WGpuColorTargetState;
extern const WGpuColorTargetState WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER;

typedef struct WGpuFragmentState
{
  WGpuShaderModule module;
  const char *entryPoint;
  int numTargets;
  const WGpuColorTargetState *targets;
} WGpuFragmentState;

typedef struct WGpuRenderPipelineDescriptor
{
  WGpuVertexState vertex;
  WGpuPrimitiveState primitive;
  WGpuDepthStencilState depthStencil;
  WGpuMultisampleState multisample;
  WGpuFragmentState fragment;
} WGpuRenderPipelineDescriptor;
extern const WGpuRenderPipelineDescriptor WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;

typedef struct WGpuCommandEncoderDescriptor
{
  EM_BOOL measureExecutionTime;
} WGpuCommandEncoderDescriptor;

typedef struct GPUTextureViewDescriptor
{
  WGPU_TEXTURE_FORMAT format;
  WGPU_TEXTURE_VIEW_DIMENSION dimension;
  WGPU_TEXTURE_ASPECT aspect;
  uint32_t baseMipLevel;
  uint32_t mipLevelCount;
  uint32_t baseArrayLayer;
  uint32_t arrayLayerCount;
} GPUTextureViewDescriptor;
// TODO WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER

typedef struct WGpuRenderPassDepthStencilAttachment
{
  WGpuTextureView view;

  WGPU_LOAD_OP depthLoadOp; // Either WGPU_LOAD_OP_CONSTANT_VALUE (== default, 0) or WGPU_LOAD_OP_LOAD
  float depthLoadValue;

  WGPU_STORE_OP depthStoreOp;
  EM_BOOL depthReadOnly;

  WGPU_LOAD_OP stencilLoadOp;  // Either WGPU_LOAD_OP_CONSTANT_VALUE (== default, 0) or WGPU_LOAD_OP_LOAD
  uint32_t stencilLoadValue;
  WGPU_STORE_OP stencilStoreOp;
  EM_BOOL stencilReadOnly;
} WGpuRenderPassDepthStencilAttachment;

typedef struct WGpuRenderPassColorAttachment
{
  WGpuTextureView view;
  WGpuTextureView resolveTarget;

  WGPU_STORE_OP storeOp;

  WGPU_LOAD_OP loadOp; // Either WGPU_LOAD_OP_CONSTANT_VALUE (== default, 0) or WGPU_LOAD_OP_LOAD
  double loadColor[4]; // r, g, b, a
} WGpuRenderPassColorAttachment;

typedef struct WGpuRenderPassDescriptor
{
  int numColorAttachments;
  const WGpuRenderPassColorAttachment *colorAttachments;
  WGpuRenderPassDepthStencilAttachment depthStencilAttachment;
  WGpuQuerySet occlusionQuerySet;
} WGpuRenderPassDescriptor;

typedef struct WGpuBufferDescriptor
{
  uint64_t size;
  WGPU_BUFFER_USAGE_FLAGS usage;
  EM_BOOL mappedAtCreation; // Note: it is valid to set mappedAtCreation to true without MAP_READ or MAP_WRITE in usage. This can be used to set the buffer’s initial data.
} WGpuBufferDescriptor;

/*
typedef struct WGpuSamplerDescriptor
{
  // TODO https://gpuweb.github.io/gpuweb/#dictdef-gpusamplerdescriptor
} WGpuSamplerDescriptor;
*/

const char *wgpu_enum_to_string(int enumValue);

// Tests if given object represents a currently valid WebGPU adapter. Returns false if navigator_gpu_request_adapter_async promise is still pending, so this
// function can be used to poll for completion of the request (if refactoring the code for a callback structure is infeasible)
EM_BOOL wgpu_is_adapter(WGpuAdapter adapter);

// Tests if given object represents a currently valid WebGPU device. Returns false if navigator_gpu_request_device_async promise is still pending, so this
// function can be used to poll for completion of the request (if refactoring the code for a callback structure is infeasible)
EM_BOOL wgpu_is_device(WGpuDevice device);

// The following functions test whether the passed object is a valid WebGPU object of the specified type.
EM_BOOL wgpu_is_canvas_context(WGpuCanvasContext canvasContext);
EM_BOOL wgpu_is_swap_chain(WGpuSwapChain swapChain);
EM_BOOL wgpu_is_texture(WGpuTexture texture);
EM_BOOL wgpu_is_shader_module(WGpuShaderModule shaderModule);
EM_BOOL wgpu_is_sampler(WGpuSampler sampler);
EM_BOOL wgpu_is_render_pipeline(WGpuRenderPipeline renderPipeline);
EM_BOOL wgpu_is_command_encoder(WGpuCommandEncoder commandEncoder);
EM_BOOL wgpu_is_bind_group_layout(WGpuBindGroupLayout bindGroupLayout);
EM_BOOL wgpu_is_texture_view(WGpuTextureView textureView);
EM_BOOL wgpu_is_render_pass_encoder(WGpuRenderPassEncoder renderPassEncoder);
EM_BOOL wgpu_is_query_set(WGpuQuerySet querySet);
EM_BOOL wgpu_is_command_buffer(WGpuCommandBuffer commandBuffer);
EM_BOOL wgpu_is_queue(WGpuQueue queue);
// TODO other wgpu_is_*

// Returns the number of WebGPU objects referenced by the WebGPU JS library.
uint32_t wgpu_get_num_live_objects(void);

// Calls .destroy() on the given WebGPU object (if it has such a member function) and releases the JS side reference to it. Use this function
// to release memory for all types of WebGPU objects after you are done with them.
// Note that deleting a GPUTexture will also delete all GPUTextureViews that have been created from it.
// Similar to free(), calling wgpu_object_destroy() on null, or an object that has already been destroyed before is safe, and no-op. (so no need to
// do excess "if (wgpuObject) wgpu_object_destroy(wgpuObject);")
void wgpu_object_destroy(WGpuObjectRef wgpuObject);

// TODO: Add support for getting and setting label member field: https://gpuweb.github.io/gpuweb/#gpuobjectbase
// void wgpu_object_set_label(WGpuObjectRef wgpuObject, const char *label);
// const char *wgpu_object_get_label(WGpuObjectRef wgpuObject);

typedef void (*WGpuRequestAdapterCallback)(WGpuAdapter adapter, void *userData);

// Requests an adapter from the user agent. The user agent chooses whether to return an adapter, and, if so, chooses according to the provided options.
// If WebGPU is not supported by the browser, returns 0. Otherwise returns an ID for a WebGPU adapter.
WGpuAdapter navigator_gpu_request_adapter_async(const WGpuRequestAdapterOptions *options, WGpuRequestAdapterCallback adapterCallback, void *userData);

// Writes the properties of the given adapter (adapter.name, adapter.features and adapter.limits) to the given properties structure.
void wgpu_adapter_get_properties(WGpuAdapter adapter, WGpuAdapterProperties *properties);

typedef void (*WGpuRequestDeviceCallback)(WGpuDevice device, void *userData);

void wgpu_adapter_request_device_async(WGpuAdapter adapter, const WGpuDeviceDescriptor *descriptor, WGpuRequestDeviceCallback deviceCallback, void *userData);

// WGpuCanvasContext functions:
WGpuCanvasContext wgpu_canvas_get_canvas_context(const char *canvasSelector);

WGpuTexture wgpu_swap_chain_get_current_texture(WGpuSwapChain swapChain);

// Returns an optimal GPUTextureFormat to use for swap chains with this context and the given device.
// TODO: How can a single "optimal" format work? (optimal for which usage? gamma? sRGB? HDR? other color spaces?)
WGPU_TEXTURE_FORMAT wgpu_canvas_context_get_swap_chain_preferred_format(WGpuCanvasContext canvasContext, WGpuAdapter adapter);

// Configures the swap chain for this canvas, and returns a new GPUSwapChain object representing it. Destroys any swapchain previously returned by configureSwapChain, including all of the textures it has produced.
WGpuSwapChain wgpu_canvascontext_configure_swap_chain(WGpuCanvasContext canvasContext, const WGpuSwapChainDescriptor *swapChainDesc);

WGpuAdapter wgpu_device_get_adapter(WGpuDevice device);
WGpuShaderModule wgpu_device_create_shader_module(WGpuDevice device, const WGpuShaderModuleDescriptor *shaderModuleDesc);
WGpuRenderPipeline wgpu_device_create_render_pipeline(WGpuDevice device, const WGpuRenderPipelineDescriptor *renderPipelineDesc);
WGpuCommandEncoder wgpu_device_create_command_encoder(WGpuDevice device, const WGpuCommandEncoderDescriptor *commandEncoderDesc);
WGpuQueue wgpu_device_get_queue(WGpuDevice device);
// TODO: Add wgpu_device_get_features - or a common wgpu_device_get_properties() that returns both features+limits, like with adapter?
// TODO: Add wgpu_device_get_limits
WGpuBuffer wgpu_device_create_buffer(WGpuDevice device, const WGpuBufferDescriptor *bufferDesc);
// TODO: Add wgpu_device_create_texture
// TODO: Add wgpu_device_create_sampler
// TODO: Add wgpu_device_create_bind_group_layout
// TODO: Add wgpu_device_create_pipeline_layout
// TODO: Add wgpu_device_create_bind_group
// TODO: Add wgpu_device_create_shader_module
// TODO: Add wgpu_device_create_compute_pipeline
// TODO: Add wgpu_device_create_compute_pipeline_async
// TODO: Add wgpu_device_create_render_pipeline_async
// TODO: Add wgpu_device_create_create_render_bundle_encoder
// TODO: Add wgpu_device_create_query_set
// wgpu_device_push_error_scope()?
// wgpu_device_pop_error_scope()?

// TODO: If there are wgpu_device_create_compute_pipeline_async() and wgpu_device_create_render_pipeline_async(), why not wgpu_device_create_shader_module_async()?

// TODO: wgpu_buffer_map_async()
// Calls buffer.getMappedRange(). Returns `startOffset`, which is used as an ID token to wgpu_buffer_read/write_mapped_range().
#define WGPU_MAP_MAX_LENGTH -1

WGpuBufferMappedRangeStartOffset wgpu_buffer_get_mapped_range(WGpuBuffer buffer, double_int53_t startOffset, double_int53_t size _WGPU_DEFAULT_VALUE(WGPU_MAP_MAX_LENGTH));
void wgpu_buffer_read_mapped_range(WGpuBuffer buffer, WGpuBufferMappedRangeStartOffset startOffset, double_int53_t subOffset, void *dst, double_int53_t size);
void wgpu_buffer_write_mapped_range(WGpuBuffer buffer, WGpuBufferMappedRangeStartOffset startOffset, double_int53_t subOffset, const void *src, double_int53_t size);
void wgpu_buffer_unmap(WGpuBuffer buffer);

// TODO: Add WGpuBindGroupLayout wgpu_render_pipeline_get_bind_group_layout(unsigned long index);

WGpuTextureView wgpu_texture_create_view(WGpuTexture texture, const GPUTextureViewDescriptor *textureViewDesc);

// wgpu_shader_module_compilation_info();

WGpuRenderPassEncoder wgpu_command_encoder_begin_render_pass(WGpuCommandEncoder commandEncoder, const WGpuRenderPassDescriptor *renderPassDesc);
// wgpu_command_encoder_begin_compute_pass()
// wgpu_command_encoder_copy_buffer_to_buffer()
// wgpu_command_encoder_copy_buffer_to_texture()
// wgpu_command_encoder_copy_texture_to_buffer()
// wgpu_command_encoder_copy_texture_to_texture()
// wgpu_command_encoder_push_debug_group()
// wgpu_command_encoder_pop_debug_group()
// wgpu_command_encoder_insert_debug_marker()
// wgpu_command_encoder_write_timestamp()
// wgpu_command_encoder_resolve_query_set()
WGpuCommandBuffer wgpu_command_encoder_finish(WGpuCommandEncoder commandEncoder);

void wgpu_render_pass_encoder_set_pipeline(WGpuRenderPassEncoder passEncoder, WGpuRenderPipeline renderPipeline);
// wgpu_render_pass_encoder_set_bind_group()
// wgpu_render_pass_encoder_push_debug_group()
// wgpu_render_pass_encoder_pop_debug_group()
// wgpu_render_pass_encoder_insert_debug_marker()
// wgpu_render_pass_encoder_set_index_buffer()
void wgpu_render_pass_encoder_set_vertex_buffer(WGpuRenderPassEncoder passEncoder, int32_t slot, WGpuBuffer buffer, double_int53_t offset _WGPU_DEFAULT_VALUE(0), double_int53_t size _WGPU_DEFAULT_VALUE(0));
void wgpu_render_pass_encoder_draw(WGpuRenderPassEncoder passEncoder, uint32_t vertexCount, uint32_t instanceCount _WGPU_DEFAULT_VALUE(1), uint32_t firstVertex _WGPU_DEFAULT_VALUE(0), uint32_t firstInstance _WGPU_DEFAULT_VALUE(0));
// wgpu_render_pass_encoder_draw_indexed()
// wgpu_render_pass_encoder_draw_indirect()
// wgpu_render_pass_encoder_draw_indexed_indirect()
// wgpu_render_pass_encoder_set_viewport()
// wgpu_render_pass_encoder_set_scissor_rect()
// wgpu_render_pass_encoder_set_blend_color()
// wgpu_render_pass_encoder_set_stencil_reference()
// wgpu_render_pass_encoder_begin_occlusion_query()
// wgpu_render_pass_encoder_end_occlusion_query()
// wgpu_render_pass_encoder_begin_pipeline_statistics_query()
// wgpu_render_pass_encoder_end_pipeline_statistics_query()
// wgpu_render_pass_encoder_write_timestamp()
// wgpu_render_pass_encoder_execute_bundles()
void wgpu_render_pass_encoder_end_pass(WGpuRenderPassEncoder passEncoder);

// Submits one command buffer to the given queue for rendering. The command buffer is held alive for later resubmission to another queue.
void wgpu_queue_submit_one(WGpuQueue queue, WGpuCommandBuffer commandBuffer);
// Submits one command buffer to the given queue for rendering. The command buffer is destroyed after rendering by calling wgpu_object_destroy() on it.
// (this is a helper function to help remind that wasm side references to WebGPU JS objects need to be destroyed or a reference leak occurs. See
// function wgpu_get_num_live_objects() to help debug the number of live references)
void wgpu_queue_submit_one_and_destroy(WGpuQueue queue, WGpuCommandBuffer commandBuffer);

// Submits multiple command buffers to the given queue for rendering. The command buffers are held alive for later resubmission to another queue.
void wgpu_queue_submit_multiple(WGpuQueue queue, int numCommandBuffers, const WGpuCommandBuffer *commandBuffers);
// Submits multiple command buffers to the given queue for rendering. The command buffers are destroyed after rendering by calling wgpu_object_destroy() on them.
// (this is a helper function to help remind that wasm side references to WebGPU JS objects need to be destroyed or a reference leak occurs. See
// function wgpu_get_num_live_objects() to help debug the number of live references)
void wgpu_queue_submit_multiple_and_destroy(WGpuQueue queue, int numCommandBuffers, const WGpuCommandBuffer *commandBuffers);

// wgpu_queue_on_submitted_work_done()
// wgpu_queue_write_buffer()
// wgpu_queue_write_texture()
// wgpu_queue_copy_image_bitmap_to_texture()

// wgpu_programmable_pass_encoder_set_bind_group
// wgpu_programmable_pass_encoder_push_debug_group
// wgpu_programmable_pass_encoder_pop_debug_group
// wgpu_programmable_pass_encoder_insert_debug_marker

// wgpu_compute_pass_encoder_set_pipeline
// wgpu_compute_pass_encoder_dispatch
// wgpu_compute_pass_encoder_dispatch_indirect
// wgpu_compute_pass_encoder_begin_pipeline_statistics_query
// wgpu_compute_pass_encoder_end_pipeline_statistics_query
// wgpu_compute_pass_encoder_write_timestamp
// wgpu_compute_pass_encoder_end_pass

#ifdef __cplusplus
} // ~extern "C"
#endif
