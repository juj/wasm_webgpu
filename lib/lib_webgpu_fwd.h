#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef double double_int53_t;

typedef double_int53_t WGpuBufferMappedRangeStartOffset;

// The following should be kept sorted in the order of the WebIDL for easier diffing across changes to the spec: https://www.w3.org/TR/webgpu/#idl-index
// with the exception that the callback typedefs should appear last in this file to see the necessary definitions.

typedef int WGpuObjectBase;
typedef struct WGpuObjectDescriptorBase WGpuObjectDescriptorBase;
typedef struct WGpuSupportedLimits WGpuSupportedLimits;
typedef int WGPU_FEATURES_BITFIELD;
typedef int WGPU_PREDEFINED_COLOR_SPACE;
typedef struct WGpuRequestAdapterOptions WGpuRequestAdapterOptions;
typedef int WGPU_POWER_PREFERENCE;
typedef int WGpuAdapter;
typedef struct WGpuDeviceDescriptor WGpuDeviceDescriptor;
typedef int WGPU_FEATURE_NAME;
typedef int WGpuDevice;
typedef int WGpuBuffer;
typedef struct WGpuBufferDescriptor WGpuBufferDescriptor;
typedef int WGPU_BUFFER_USAGE_FLAGS;
typedef int WGPU_MAP_MODE_FLAGS;
typedef int WGpuTexture;
typedef struct WGpuTextureDescriptor WGpuTextureDescriptor;
typedef int WGPU_TEXTURE_DIMENSION;
typedef int WGPU_TEXTURE_USAGE_FLAGS;
typedef int WGpuTextureView;
typedef struct WGpuTextureViewDescriptor WGpuTextureViewDescriptor;
typedef int WGPU_TEXTURE_VIEW_DIMENSION;
typedef int WGPU_TEXTURE_ASPECT;
typedef int WGPU_TEXTURE_FORMAT;
typedef int WGpuExternalTexture;
typedef struct WGpuExternalTextureDescriptor WGpuExternalTextureDescriptor;
typedef int WGpuSampler;
typedef struct WGpuSamplerDescriptor WGpuSamplerDescriptor;
typedef int WGPU_ADDRESS_MODE;
typedef int WGPU_FILTER_MODE;
typedef int WGPU_COMPARE_FUNCTION;
typedef int WGpuBindGroupLayout;
typedef struct WGpuBindGroupLayoutDescriptor WGpuBindGroupLayoutDescriptor;
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
typedef int WGpuBindGroup;
typedef struct WGpuBindGroupDescriptor WGpuBindGroupDescriptor;
typedef struct WGpuBindGroupEntry WGpuBindGroupEntry;
typedef struct WGpuBufferBinding WGpuBufferBinding;
typedef int WGpuPipelineLayout;
typedef struct WGpuPipelineLayoutDescriptor WGpuPipelineLayoutDescriptor;
typedef int WGpuShaderModule;
typedef struct WGpuShaderModuleDescriptor WGpuShaderModuleDescriptor;
typedef int WGPU_COMPILATION_MESSAGE_TYPE;
typedef int WGpuComputePipeline;
typedef struct WGpuComputePipelineDescriptor WGpuComputePipelineDescriptor;
typedef int WGpuRenderPipeline;
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
typedef int WGPU_INPUT_STEP_MODE;
typedef struct WGpuVertexState WGpuVertexState;
typedef struct WGpuVertexBufferLayout WGpuVertexBufferLayout;
typedef struct WGpuVertexAttribute WGpuVertexAttribute;
typedef int WGpuCommandBuffer;
typedef struct WGpuCommandBufferDescriptor WGpuCommandBufferDescriptor;
typedef int WGpuCommandEncoder;
typedef struct WGpuCommandEncoderDescriptor WGpuCommandEncoderDescriptor;
typedef struct WGpuImageDataLayout WGpuImageDataLayout;
typedef struct WGpuImageCopyBuffer WGpuImageCopyBuffer;
typedef struct WGpuImageCopyTexture WGpuImageCopyTexture;
typedef struct WGpuImageCopyTextureTagged WGpuImageCopyTextureTagged;
typedef struct WGpuImageCopyExternalImage WGpuImageCopyExternalImage;
typedef int WGpuProgrammablePassEncoder;
typedef int WGpuComputePassEncoder;
typedef struct WGpuComputePassDescriptor WGpuComputePassDescriptor;
typedef int WGpuRenderEncoderBase;
typedef int WGpuRenderPassEncoder;
typedef struct WGpuRenderPassDescriptor WGpuRenderPassDescriptor;
typedef struct WGpuRenderPassColorAttachment WGpuRenderPassColorAttachment;
typedef struct WGpuRenderPassDepthStencilAttachment WGpuRenderPassDepthStencilAttachment;
typedef int WGPU_LOAD_OP;
typedef int WGPU_STORE_OP;
typedef int WGpuRenderBundle;
typedef struct WGpuRenderBundleDescriptor WGpuRenderBundleDescriptor;
typedef int WGpuRenderBundleEncoder;
typedef struct WGpuRenderBundleEncoderDescriptor WGpuRenderBundleEncoderDescriptor;
typedef int WGpuQueue;
typedef int WGpuQuerySet;
typedef struct WGpuQuerySetDescriptor WGpuQuerySetDescriptor;
typedef int WGPU_QUERY_TYPE;
typedef int WGPU_PIPELINE_STATISTIC_NAME;
typedef int WGpuCanvasContext;
typedef int WGPU_CANVAS_COMPOSITING_ALPHA_MODE;
typedef struct WGpuPresentationConfiguration WGpuPresentationConfiguration;
typedef int WGPU_DEVICE_LOST_REASON;
typedef int WGpuDeviceLostInfo;
typedef int WGPU_ERROR_FILTER;
typedef int WGpuError;
typedef struct WGpuColor WGpuColor;
typedef struct WGpuOrigin2D WGpuOrigin2D;
typedef struct WGpuOrigin3D WGpuOrigin3D;
typedef struct WGpuExtent3D WGpuExtent3D;

// Callbacks in the order of appearance in the WebIDL:

typedef void (*WGpuRequestAdapterCallback)(WGpuAdapter adapter, void *userData);
typedef void (*WGpuBufferMapCallback)(WGpuBuffer buffer, void *userData);
typedef void (*WGpuOnSubmittedWorkDoneCallback)(WGpuQueue queue, void *userData);
typedef void (*WGpuDeviceLostCallback)(WGpuDevice device, WGpuDeviceLostInfo deviceLostInfo, void *userData);
typedef void (*WGpuDeviceErrorCallback)(WGpuDevice device, WGpuError deviceLostInfo, void *userData); // TODO implement

#ifdef __cplusplus
} // ~extern "C"
#endif
