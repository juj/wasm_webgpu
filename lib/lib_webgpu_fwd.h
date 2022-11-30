#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// The type 'double_int53_t' shall be an integer-like
// type that can represent at least 53-bits of consecutive
// unsigned integers. On the web, this will be a 'double',
// on other platforms it will be a 64-bit unsigned integer.
// The reason for using double on the web is due to the
// JavaScript WebGPU API does not not allow utilizing
// BigInt to represent real 64-bit integers, so only 53 bits
// of integer values is available.
#ifdef __EMSCRIPTEN__
// Use double to represent a JavaScript number that can
// address 2^53 == 9007199254740992 = ~9.0 petabytes.
typedef double double_int53_t;
#else
// On other platforms than the web, double_int53_t is a real
// integer type.
typedef uint64_t double_int53_t;
#endif

#ifdef __GNUC__
#define NOTNULL __attribute__((nonnull))
#else
#define NOTNULL
#endif

// The following should be kept sorted in the order of the WebIDL for easier diffing across changes to the spec: https://www.w3.org/TR/webgpu/#idl-index
// with the exception that the callback typedefs should appear last in this file to see the necessary definitions.
#ifdef __EMSCRIPTEN__
typedef int WGpuObjectBase;
#else
typedef struct _WGpuObject *WGpuObjectBase;
#endif

typedef struct WGpuObjectDescriptorBase WGpuObjectDescriptorBase;
typedef struct WGpuSupportedLimits WGpuSupportedLimits;
typedef int WGPU_FEATURES_BITFIELD;
typedef int HTML_PREDEFINED_COLOR_SPACE;
typedef struct WGpuAdapterInfo WGpuAdapterInfo;
typedef struct WGpuRequestAdapterOptions WGpuRequestAdapterOptions;
typedef int WGPU_POWER_PREFERENCE;
typedef WGpuObjectBase WGpuAdapter;
typedef struct WGpuDeviceDescriptor WGpuDeviceDescriptor;
typedef WGpuObjectBase WGpuDevice;
typedef WGpuObjectBase WGpuBuffer;
typedef int WGPU_BUFFER_MAP_STATE;
typedef struct WGpuBufferDescriptor WGpuBufferDescriptor;
typedef int WGPU_BUFFER_USAGE_FLAGS;
typedef int WGPU_MAP_MODE_FLAGS;
typedef WGpuObjectBase WGpuTexture;
typedef struct WGpuTextureDescriptor WGpuTextureDescriptor;
typedef int WGPU_TEXTURE_DIMENSION;
typedef int WGPU_TEXTURE_USAGE_FLAGS;
typedef WGpuObjectBase WGpuTextureView;
typedef struct WGpuTextureViewDescriptor WGpuTextureViewDescriptor;
typedef int WGPU_TEXTURE_VIEW_DIMENSION;
typedef int WGPU_TEXTURE_ASPECT;
typedef int WGPU_TEXTURE_FORMAT;
typedef WGpuObjectBase WGpuExternalTexture;
typedef struct WGpuExternalTextureDescriptor WGpuExternalTextureDescriptor;
typedef WGpuObjectBase WGpuSampler;
typedef struct WGpuSamplerDescriptor WGpuSamplerDescriptor;
typedef int WGPU_ADDRESS_MODE;
typedef int WGPU_FILTER_MODE;
typedef int WGPU_MIPMAP_FILTER_MODE;
typedef int WGPU_COMPARE_FUNCTION;
typedef WGpuObjectBase WGpuBindGroupLayout;
// Not used:
//typedef struct WGpuBindGroupLayoutDescriptor WGpuBindGroupLayoutDescriptor;
typedef int WGPU_SHADER_STAGE_FLAGS;
typedef struct WGpuBindGroupLayoutEntry WGpuBindGroupLayoutEntry;
typedef int WGPU_BUFFER_BINDING_TYPE;
typedef struct WGpuBufferBindingLayout WGpuBufferBindingLayout;
typedef int WGPU_SAMPLER_BINDING_TYPE;
typedef struct WGpuSamplerBindingLayout WGpuSamplerBindingLayout;
typedef int WGPU_TEXTURE_SAMPLE_TYPE;
typedef struct WGpuTextureBindingLayout WGpuTextureBindingLayout;
typedef int WGPU_STORAGE_TEXTURE_ACCESS;
typedef struct WGpuStorageTextureBindingLayout WGpuStorageTextureBindingLayout;
typedef struct WGpuExternalTextureBindingLayout WGpuExternalTextureBindingLayout;
typedef WGpuObjectBase WGpuBindGroup;
// Not used:
//typedef struct WGpuBindGroupDescriptor WGpuBindGroupDescriptor;
typedef struct WGpuBindGroupEntry WGpuBindGroupEntry;
typedef WGpuObjectBase WGpuPipelineLayout;
// Not used:
//typedef struct WGpuPipelineLayoutDescriptor WGpuPipelineLayoutDescriptor;
typedef WGpuObjectBase WGpuShaderModule;
typedef struct WGpuShaderModuleDescriptor WGpuShaderModuleDescriptor;
typedef int WGPU_COMPILATION_MESSAGE_TYPE;
typedef struct WGpuCompilationMessage WGpuCompilationMessage;
typedef struct WGpuCompilationInfo WGpuCompilationInfo;
typedef int WGPU_AUTO_LAYOUT_MODE;
typedef struct WGpuPipelineConstant WGpuPipelineConstant;
typedef WGpuObjectBase WGpuComputePipeline;
// Not used:
//typedef struct WGpuComputePipelineDescriptor WGpuComputePipelineDescriptor;
typedef WGpuObjectBase WGpuRenderPipeline;
typedef WGpuObjectBase WGpuPipelineBase;
typedef struct WGpuRenderPipelineDescriptor WGpuRenderPipelineDescriptor;
typedef int WGPU_PRIMITIVE_TOPOLOGY;
typedef struct WGpuPrimitiveState WGpuPrimitiveState;
typedef int WGPU_FRONT_FACE;
typedef int WGPU_CULL_MODE;
typedef struct WGpuMultisampleState WGpuMultisampleState;
typedef struct WGpuFragmentState WGpuFragmentState;
typedef struct WGpuColorTargetState WGpuColorTargetState;
typedef struct WGpuBlendState WGpuBlendState;
typedef int WGPU_COLOR_WRITE_FLAGS;
typedef struct WGpuBlendComponent WGpuBlendComponent;
typedef int WGPU_BLEND_FACTOR;
typedef int WGPU_BLEND_OPERATION;
typedef struct WGpuDepthStencilState WGpuDepthStencilState;
typedef struct WGpuStencilFaceState WGpuStencilFaceState;
typedef int WGPU_STENCIL_OPERATION;
typedef int WGPU_INDEX_FORMAT;
typedef int WGPU_VERTEX_FORMAT;
typedef int WGPU_VERTEX_STEP_MODE;
typedef struct WGpuVertexState WGpuVertexState;
typedef struct WGpuVertexBufferLayout WGpuVertexBufferLayout;
typedef struct WGpuVertexAttribute WGpuVertexAttribute;
typedef WGpuObjectBase WGpuCommandBuffer;
typedef struct WGpuCommandBufferDescriptor WGpuCommandBufferDescriptor;
typedef WGpuObjectBase WGpuCommandEncoder;
typedef struct WGpuCommandEncoderDescriptor WGpuCommandEncoderDescriptor;
typedef struct WGpuImageCopyBuffer WGpuImageCopyBuffer;
typedef struct WGpuImageCopyTexture WGpuImageCopyTexture;
typedef struct WGpuImageCopyTextureTagged WGpuImageCopyTextureTagged;
typedef struct WGpuImageCopyExternalImage WGpuImageCopyExternalImage;
typedef WGpuObjectBase WGpuBindingCommandsMixin;
typedef WGpuObjectBase WGpuComputePassEncoder;
typedef struct WGpuComputePassDescriptor WGpuComputePassDescriptor;
typedef WGpuObjectBase WGpuRenderCommandsMixin;
typedef WGpuObjectBase WGpuRenderPassEncoder;
typedef struct WGpuRenderPassDescriptor WGpuRenderPassDescriptor;
typedef struct WGpuRenderPassColorAttachment WGpuRenderPassColorAttachment;
typedef struct WGpuRenderPassDepthStencilAttachment WGpuRenderPassDepthStencilAttachment;
typedef int WGPU_LOAD_OP;
typedef int WGPU_STORE_OP;
typedef WGpuObjectBase WGpuRenderBundle;
typedef struct WGpuRenderBundleDescriptor WGpuRenderBundleDescriptor;
typedef WGpuObjectBase WGpuRenderBundleEncoder;
typedef struct WGpuRenderBundleEncoderDescriptor WGpuRenderBundleEncoderDescriptor;
typedef WGpuObjectBase WGpuQueue;
typedef WGpuObjectBase WGpuQuerySet;
typedef struct WGpuQuerySetDescriptor WGpuQuerySetDescriptor;
typedef int WGPU_QUERY_TYPE;
typedef int WGPU_PIPELINE_STATISTIC_NAME;
typedef WGpuObjectBase WGpuCanvasContext;
typedef int WGPU_CANVAS_ALPHA_MODE;
typedef struct WGpuCanvasConfiguration WGpuCanvasConfiguration;
typedef int WGPU_DEVICE_LOST_REASON;
typedef int WGPU_ERROR_FILTER;
typedef int WGPU_ERROR_TYPE;
typedef struct WGpuColor WGpuColor;
typedef struct WGpuOrigin2D WGpuOrigin2D;
typedef struct WGpuOrigin3D WGpuOrigin3D;
typedef struct WGpuExtent3D WGpuExtent3D;
typedef struct WGpuBindGroupLayoutEntry WGpuBindGroupLayoutEntry;
typedef WGpuObjectBase WGpuImageBitmap;

// Callbacks in the order of appearance in the WebIDL:

typedef void (*WGpuRequestAdapterCallback)(WGpuAdapter adapter, void *userData);
typedef void (*WGpuRequestDeviceCallback)(WGpuDevice device, void *userData);
typedef void (*WGpuRequestAdapterInfoCallback)(WGpuAdapter adapter, const WGpuAdapterInfo *adapterInfo NOTNULL, void *userData);
typedef void (*WGpuCreatePipelineCallback)(WGpuDevice device, WGpuPipelineBase pipeline, void *userData);
typedef void (*WGpuBufferMapCallback)(WGpuBuffer buffer, void *userData, WGPU_MAP_MODE_FLAGS mode, double_int53_t offset, double_int53_t size);
typedef void (*WGpuGetCompilationInfoCallback)(WGpuShaderModule shaderModule, WGpuCompilationInfo *compilationInfo NOTNULL, void *userData);
typedef void (*WGpuOnSubmittedWorkDoneCallback)(WGpuQueue queue, void *userData);
typedef void (*WGpuDeviceLostCallback)(WGpuDevice device, WGPU_DEVICE_LOST_REASON deviceLostReason, const char *message, void *userData);
typedef void (*WGpuDeviceErrorCallback)(WGpuDevice device, WGPU_ERROR_TYPE errorType, const char *errorMessage, void *userData);
typedef void (*WGpuLoadImageBitmapCallback)(WGpuImageBitmap bitmap, int width, int height, void *userData);

#ifdef __cplusplus
} // ~extern "C"
#endif
