#include "lib_webgpu.h"

// Default initializers for WebGPU descriptors, using C++20 standard.
// Only add either this file or lib_webgpu_cpp11.cpp to your project, but not both.

#ifdef __cplusplus
extern "C" {
#endif

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
  .baseArrayLayer = 0,
  .swizzle = "rgba"
};

const WGpuExternalTextureDescriptor WGPU_EXTERNAL_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER = {
  .colorSpace = HTML_PREDEFINED_COLOR_SPACE_SRGB
};

const WGpuSamplerDescriptor WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER = {
  .addressModeU = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
  .addressModeV = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
  .addressModeW = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE,
  .magFilter = WGPU_FILTER_MODE_NEAREST,
  .minFilter = WGPU_FILTER_MODE_NEAREST,
  .mipmapFilter = WGPU_MIPMAP_FILTER_MODE_NEAREST,
  .lodMinClamp = 0,
  .lodMaxClamp = 32,
  .maxAnisotropy = 1

};

const WGpuBindGroupLayoutEntry WGPU_BUFFER_BINDING_LAYOUT_ENTRY_DEFAULT_INITIALIZER = {
};

const WGpuBufferBindingLayout WGPU_BUFFER_BINDING_LAYOUT_DEFAULT_INITIALIZER = {
  .type = WGPU_BUFFER_BINDING_TYPE_UNIFORM,
  .hasDynamicOffset = WGPU_FALSE,
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

const WGpuShaderModuleCompilationHint WGPU_SHADER_MODULE_COMPILATION_HINT_DEFAULT_INITIALIZER = {
};

const WGpuCommandEncoderDescriptor WGPU_COMMAND_ENCODER_DESCRIPTOR_DEFAULT_INITIALIZER = {
};

const WGpuTexelCopyBufferInfo WGPU_TEXEL_COPY_BUFFER_INFO_DEFAULT_INITIALIZER = {};

const WGpuComputePassTimestampWrites WGPU_COMPUTE_PASS_TIMESTAMP_WRITES_DEFAULT_INITIALIZER = {
  .querySet = 0,
  .beginningOfPassWriteIndex = -1,
  .endOfPassWriteIndex = -1
};

const WGpuComputePassDescriptor WGPU_COMPUTE_PASS_DESCRIPTOR_DEFAULT_INITIALIZER = {
  .timestampWrites = WGPU_COMPUTE_PASS_TIMESTAMP_WRITES_DEFAULT_INITIALIZER
};

const WGpuRenderPassDepthStencilAttachment WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_DEFAULT_INITIALIZER = {
  .view = 0,

  .depthLoadOp = WGPU_LOAD_OP_LOAD,
  .depthClearValue = WGPU_NAN,
  .depthStoreOp = WGPU_STORE_OP_UNDEFINED,
  .depthReadOnly = false,

  .stencilLoadOp = WGPU_LOAD_OP_LOAD,
  .stencilClearValue = 0,
  .stencilStoreOp = WGPU_STORE_OP_UNDEFINED,
  .stencilReadOnly = false
};

const WGpuStorageTextureBindingLayout WGPU_STORAGE_TEXTURE_BINDING_LAYOUT_DEFAULT_INITIALIZER = {
  .access = WGPU_STORAGE_TEXTURE_ACCESS_WRITE_ONLY,
  .viewDimension = WGPU_TEXTURE_VIEW_DIMENSION_2D
};

const WGpuCanvasToneMapping WGPU_CANVAS_TONE_MAPPING_DEFAULT_INITIALIZER = {
  .mode = WGPU_CANVAS_TONE_MAPPING_MODE_STANDARD
};

const WGpuCanvasConfiguration WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER = {
  .usage = WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT,
  .colorSpace = HTML_PREDEFINED_COLOR_SPACE_SRGB,
  .toneMapping = WGPU_CANVAS_TONE_MAPPING_DEFAULT_INITIALIZER,
  .alphaMode = WGPU_CANVAS_ALPHA_MODE_OPAQUE,
};

const WGpuRenderPassTimestampWrites WGPU_RENDER_PASS_TIMESTAMP_WRITES_DEFAULT_INITIALIZER = {
  .querySet = 0,
  .beginningOfPassWriteIndex = -1,
  .endOfPassWriteIndex = -1
};

const WGpuRenderPassDescriptor WGPU_RENDER_PASS_DESCRIPTOR_DEFAULT_INITIALIZER = {
  .maxDrawCount = 0,
  .colorAttachments = 0,
  .numColorAttachments = 0,
  .depthStencilAttachment = WGPU_RENDER_PASS_DEPTH_STENCIL_ATTACHMENT_DEFAULT_INITIALIZER,
  .occlusionQuerySet = 0,
  .timestampWrites = WGPU_RENDER_PASS_TIMESTAMP_WRITES_DEFAULT_INITIALIZER
};

const WGpuColorTargetState WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER = {
  .blend = (WGpuBlendState) {
    .color = (WGpuBlendComponent) {
      .operation = WGPU_BLEND_OPERATION_DISABLED, // Color blending is disabled by default. Set to WGPU_BLEND_OPERATION_ADD to enable.
      .srcFactor = WGPU_BLEND_FACTOR_SRC,
      .dstFactor = WGPU_BLEND_FACTOR_ONE_MINUS_SRC
    },
    .alpha = (WGpuBlendComponent) {
      .operation = WGPU_BLEND_OPERATION_ADD, // If blending is enabled, default to copying src alpha. (no blend in alpha value)
      .srcFactor = WGPU_BLEND_FACTOR_ONE,
      .dstFactor = WGPU_BLEND_FACTOR_ZERO
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
    .depthCompare = WGPU_COMPARE_FUNCTION_INVALID,
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

extern const WGpuRenderPassColorAttachment WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER = {
  .depthSlice = -1,
  .storeOp = WGPU_STORE_OP_STORE,
  .loadOp = WGPU_LOAD_OP_LOAD,
  .clearValue = (WGpuColor) {
    .r = 0.0,
    .g = 0.0,
    .b = 0.0,
    .a = 1.0,
  }
};

extern const WGpuCopyExternalImageSourceInfo WGPU_COPY_EXTERNAL_IMAGE_SOURCE_INFO_DEFAULT_INITIALIZER = {
  .origin = (WGpuOrigin2D) {
    .x = 0,
    .y = 0
  }
};

extern const WGpuTexelCopyTextureInfo WGPU_TEXEL_COPY_TEXTURE_INFO_DEFAULT_INITIALIZER = {
  .mipLevel = 0,
  .origin = (WGpuOrigin3D) {
    .x = 0,
    .y = 0,
    .z = 0
  },
  .aspect = WGPU_TEXTURE_ASPECT_ALL
};

extern const WGpuCopyExternalImageDestInfo WGPU_COPY_EXTERNAL_IMAGE_DEST_INFO_DEFAULT_INITIALIZER = {
  .mipLevel = 0,
  .origin = (WGpuOrigin3D) {
    .x = 0,
    .y = 0,
    .z = 0
  },
  .aspect = WGPU_TEXTURE_ASPECT_ALL,
  .colorSpace = HTML_PREDEFINED_COLOR_SPACE_SRGB,
  .premultipliedAlpha = WGPU_FALSE
};

#ifdef __cplusplus
} // ~extern "C"
#endif
