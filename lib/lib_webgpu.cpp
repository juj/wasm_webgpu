#include "lib_webgpu.h"

extern "C"
{

static const char *wgpuStrings[] = { 0,"low-power","high-performance","1d","2d","3d","2d-array","cube","cube-array","all","stencil-only","depth-only","r8unorm","r8snorm","r8uint","r8sint","r16uint","r16sint","r16float","rg8unorm","rg8snorm","rg8uint","rg8sint","r32uint","r32sint","r32float","rg16uint","rg16sint","rg16float","rgba8unorm","rgba8unorm-srgb","rgba8snorm","rgba8uint","rgba8sint","bgra8unorm","bgra8unorm-srgb","rgb9e5ufloat","rgb10a2unorm","rg11b10ufloat","rg32uint","rg32sint","rg32float","rgba16uint","rgba16sint","rgba16float","rgba32uint","rgba32sint","rgba32float","stencil8","depth16unorm","depth24plus","depth24plus-stencil8","depth32float","bc1-rgba-unorm","bc1-rgba-unorm-srgb","bc2-rgba-unorm","bc2-rgba-unorm-srgb","bc3-rgba-unorm","bc3-rgba-unorm-srgb","bc4-r-unorm","bc4-r-snorm","bc5-rg-unorm","bc5-rg-snorm","bc6h-rgb-ufloat","bc6h-rgb-float","bc7-rgba-unorm","bc7-rgba-unorm-srgb","depth24unorm-stencil8","depth32float-stencil8","clamp-to-edge","repeat","mirror-repeat","nearest","linear","never","less","equal","less-equal","greater","not-equal","greater-equal","always","uniform","storage","read-only-storage","filtering","non-filtering","comparison","float","unfilterable-float","depth","sint","uint","read-only","write-only","error","warning","info","point-list","line-list","line-strip","triangle-list","triangle-strip","ccw","cw","none","front","back","zero","one","src-color","one-minus-src-color","src-alpha","one-minus-src-alpha","dst-color","one-minus-dst-color","dst-alpha","one-minus-dst-alpha","src-alpha-saturated","blend-color","one-minus-blend-color","add","subtract","reverse-subtract","min","max","keep","replace","invert","increment-clamp","decrement-clamp","increment-wrap","decrement-wrap","uint16","uint32","uchar2","uchar4","char2","char4","uchar2norm","uchar4norm","char2norm","char4norm","ushort2","ushort4","short2","short4","ushort2norm","ushort4norm","short2norm","short4norm","half2","half4","float2","float3","float4","uint2","uint3","uint4","int","int2","int3","int4","vertex","instance","load","store","clear","occlusion","pipeline-statistics","timestamp","vertex-shader-invocations","clipper-invocations","clipper-primitives-out","fragment-shader-invocations","compute-shader-invocations","destroyed","out-of-memory","validation" };

const char *wgpu_enum_to_string(int enumValue)
{
  return (enumValue >= 0 && enumValue < sizeof(wgpuStrings)/sizeof(wgpuStrings[0])) ? wgpuStrings[enumValue] : "(invalid WebGPU enum)";
}

const WGpuRequestAdapterOptions WGPU_REQUEST_ADAPTER_OPTIONS_DEFAULT_INITIALIZER = {
};

const WGpuDeviceDescriptor WGPU_DEVICE_DESCRIPTOR_DEFAULT_INITIALIZER = {
};

const WGpuTextureDescriptor WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER = {
  .height = 1,
  .depthOrArrayLayers = 1,
  .mipLevelCount = 1,
  .sampleCount = 1,
  .dimension = WGPU_TEXTURE_DIMENSION_2D
};

const WGpuTextureViewDescriptor WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER = {
  .aspect = WGPU_TEXTURE_ASPECT_ALL,
  .baseMipLevel = 0,
  .baseArrayLayer = 0
};

const WGpuExternalTextureDescriptor WGPU_EXTERNAL_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER = {
  .colorSpace = WGPU_PREDEFINED_COLOR_SPACE_SRGB
};

const WGpuSamplerDescriptor WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER = {
  .addressModeU = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
  .addressModeV = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
  .addressModeW = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
  .magFilter = WGPU_FILTER_MODE_NEAREST,
  .minFilter = WGPU_FILTER_MODE_NEAREST,
  .mipmapFilter = WGPU_FILTER_MODE_NEAREST,
  .lodMinClamp = 0,
  .lodMaxClamp = 65536 // TODO change this after spec is fixed
};

const WGpuBindGroupLayoutEntry WGPU_BUFFER_BINDING_LAYOUT_ENTRY_DEFAULT_INITIALIZER = {
};

const WGpuBufferBindingLayout WGPU_BUFFER_BINDING_LAYOUT_DEFAULT_INITIALIZER = {
  .type = WGPU_BUFFER_BINDING_TYPE_UNIFORM,
  .hasDynamicOffset = EM_FALSE,
  .minBindingSize = 0
};

const WGpuSamplerBindingLayout WGPU_SAMPLER_BINDING_LAYOUT_DEFAULT_INITIALIZER = {
  .type = WGPU_SAMPLER_BINDING_TYPE_FILTERING
};

const WGpuTextureBindingLayout WGPU_TEXTURE_BINDING_LAYOUT_DEFAULT_INITIALIZER = {
  .sampleType = WGPU_TEXTURE_SAMPLE_TYPE_FLOAT,
  .viewDimension = WGPU_TEXTURE_VIEW_DIMENSION_2D,
};

const WGpuBindGroupEntry WGPU_BIND_GROUP_ENTRY_DEFAULT_INITIALIZER = {
};

const WGpuCommandEncoderDescriptor WGPU_COMMAND_ENCODER_DESCRIPTOR_DEFAULT_INITIALIZER = {
  .measureExecutionTime = EM_FALSE
};

const WGpuImageDataLayout WGPU_IMAGE_DATA_LAYOUT_DEFAULT_INITIALIZER = {
  .offset = 0
};

const WGpuImageCopyBuffer WGPU_IMAGE_COPY_BUFFER_DEFAULT_INITIALIZER = {};

const WGpuStorageTextureBindingLayout WGPU_STORAGE_TEXTURE_BINDING_LAYOUT_DEFAULT_INITIALIZER = {
  .viewDimension = WGPU_TEXTURE_VIEW_DIMENSION_2D
};

const WGpuPresentationConfiguration WGPU_PRESENTATION_CONFIGURATION_DEFAULT_INITIALIZER = {
  .usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT,
  .compositingAlphaMode = WGPU_CANVAS_COMPOSITING_ALPHA_MODE_OPAQUE,
  .size = WGPU_EXTENT_3D_DEFAULT_INITIALIZER
};

const WGpuColorTargetState WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER = {
  .blend = (WGpuBlendState) {
    .color = (WGpuBlendComponent) {
      .srcFactor = WGPU_BLEND_FACTOR_ONE,
      .dstFactor = WGPU_BLEND_FACTOR_ZERO,
      .operation = WGPU_BLEND_OPERATION_ADD
    },
    .alpha = (WGpuBlendComponent) {
      .srcFactor = WGPU_BLEND_FACTOR_ONE,
      .dstFactor = WGPU_BLEND_FACTOR_ZERO,
      .operation = WGPU_BLEND_OPERATION_ADD
    },
  },
  .writeMask = WGPU_COLOR_WRITE_ALL
};

const WGpuRenderPipelineDescriptor WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER = {
  .primitive = (WGpuPrimitiveState) {
    .topology = WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
    .frontFace = WGPU_FRONT_FACE_CCW,
    .cullMode = WGPU_CULL_MODE_NONE
  },
  .depthStencil = (WGpuDepthStencilState) {
    .depthCompare = WGPU_COMPARE_FUNCTION_ALWAYS,
    .stencilReadMask = 0xFFFFFFFFu,
    .stencilWriteMask = 0xFFFFFFFFu,
    .stencilFront = (WGpuStencilFaceState) {
      .compare = WGPU_COMPARE_FUNCTION_ALWAYS,
      .failOp = WGPU_STENCIL_OPERATION_KEEP,
      .depthFailOp = WGPU_STENCIL_OPERATION_KEEP,
      .passOp = WGPU_STENCIL_OPERATION_KEEP
    },
    .stencilBack = (WGpuStencilFaceState) {
      .compare = WGPU_COMPARE_FUNCTION_ALWAYS,
      .failOp = WGPU_STENCIL_OPERATION_KEEP,
      .depthFailOp = WGPU_STENCIL_OPERATION_KEEP,
      .passOp = WGPU_STENCIL_OPERATION_KEEP
    },
  },
  .multisample = (WGpuMultisampleState) {
    .count = 1,
    .mask = 0xFFFFFFFFu,
  },
};

extern const WGpuExtent3D WGPU_EXTENT_3D_DEFAULT_INITIALIZER = {
  .height = 1,
  .depthOrArrayLayers = 1
};

extern const WGpuImageCopyExternalImage WGPU_IMAGE_COPY_EXTERNAL_IMAGE_DEFAULT_INITIALIZER = {
  .origin = (WGpuOrigin2D) {
    .x = 0,
    .y = 0
  }
};

extern const WGpuImageCopyTexture WGPU_IMAGE_COPY_TEXTURE_DEFAULT_INITIALIZER = {
  .mipLevel = 0,
  .origin = (WGpuOrigin3D) {
    .x = 0,
    .y = 0,
    .z = 0
  },
  .aspect = WGPU_TEXTURE_ASPECT_ALL
};

extern const WGpuImageCopyTextureTagged WGPU_IMAGE_COPY_TEXTURE_TAGGED_DEFAULT_INITIALIZER = {
  .mipLevel = 0,
  .origin = (WGpuOrigin3D) {
    .x = 0,
    .y = 0,
    .z = 0
  },
  .aspect = WGPU_TEXTURE_ASPECT_ALL,
  .colorSpace = WGPU_PREDEFINED_COLOR_SPACE_SRGB,
  .premultipliedAlpha = EM_FALSE
};

} // ~extern "C"
