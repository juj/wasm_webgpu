#include "lib_webgpu.h"

// Default initializers for WebGPU descriptors, using C++11 standard.
// If you are using C++20, prefer to add lib_webgpu_cpp20.cpp instead to your project.

#ifdef __cplusplus
extern "C" {
#endif

const WGpuRequestAdapterOptions WGPU_REQUEST_ADAPTER_OPTIONS_DEFAULT_INITIALIZER = {
};

const WGpuDeviceDescriptor WGPU_DEVICE_DESCRIPTOR_DEFAULT_INITIALIZER = {
};

const WGpuTextureDescriptor WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER = {
  0, /* width */
  1, /* height */
  1, /* depthOrArrayLayers */
  1, /* mipLevelCount */
  1, /* sampleCount */
  WGPU_TEXTURE_DIMENSION_2D, /* dimension */
  0, /* format */
  0, /* usage */
  0, /* numViewFormats */
  nullptr /* viewFormats */
};

const WGpuTextureViewDescriptor WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER = {
  0, /* format */
  0, /* dimension */
  WGPU_TEXTURE_ASPECT_ALL, /* aspect */
  0, /* baseMipLevel */
  0, /* mipLevelCount */
  0, /* baseArrayLayer */
  0, /* arrayLayerCount */
};

const WGpuExternalTextureDescriptor WGPU_EXTERNAL_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER = {
  0, /* source */
  HTML_PREDEFINED_COLOR_SPACE_SRGB /* colorSpace */
};

const WGpuSamplerDescriptor WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER = {
  WGPU_ADDRESS_MODE_CLAMP_TO_EDGE, /* addressModeU */
  WGPU_ADDRESS_MODE_CLAMP_TO_EDGE, /* addressModeV */
  WGPU_ADDRESS_MODE_CLAMP_TO_EDGE, /* addressModeW */
  WGPU_FILTER_MODE_NEAREST, /* magFilter */
  WGPU_FILTER_MODE_NEAREST, /* minFilter */
  WGPU_MIPMAP_FILTER_MODE_NEAREST, /* mipmapFilter */
  0, /* lodMinClamp*/
  32, /* lodMaxClamp */
  0, /* compare */
  1 /* maxAnisotropy */
};

const WGpuBindGroupLayoutEntry WGPU_BUFFER_BINDING_LAYOUT_ENTRY_DEFAULT_INITIALIZER = {
};

const WGpuBufferBindingLayout WGPU_BUFFER_BINDING_LAYOUT_DEFAULT_INITIALIZER = {
  WGPU_BUFFER_BINDING_TYPE_UNIFORM, /* type */
  EM_FALSE, /* hasDynamicOffset */
  0 /* minBindingSize */
};

const WGpuSamplerBindingLayout WGPU_SAMPLER_BINDING_LAYOUT_DEFAULT_INITIALIZER = {
  WGPU_SAMPLER_BINDING_TYPE_FILTERING /* type */
};

const WGpuTextureBindingLayout WGPU_TEXTURE_BINDING_LAYOUT_DEFAULT_INITIALIZER = {
  WGPU_TEXTURE_SAMPLE_TYPE_FLOAT, /* sampleType */
  WGPU_TEXTURE_VIEW_DIMENSION_2D, /* viewDimension */
  false, /* multisampled */
};

const WGpuBindGroupEntry WGPU_BIND_GROUP_ENTRY_DEFAULT_INITIALIZER = {
};

const WGpuShaderModuleCompilationHint WGPU_SHADER_MODULE_COMPILATION_HINT_DEFAULT_INITIALIZER = {
};

const WGpuCommandEncoderDescriptor WGPU_COMMAND_ENCODER_DESCRIPTOR_DEFAULT_INITIALIZER = {
};

const WGpuImageCopyBuffer WGPU_IMAGE_COPY_BUFFER_DEFAULT_INITIALIZER = {};

const WGpuStorageTextureBindingLayout WGPU_STORAGE_TEXTURE_BINDING_LAYOUT_DEFAULT_INITIALIZER = {
  WGPU_STORAGE_TEXTURE_ACCESS_WRITE_ONLY, /* access */
  WGPU_TEXTURE_FORMAT_INVALID, /* format */
  WGPU_TEXTURE_VIEW_DIMENSION_2D /* viewDimension */
};

const WGpuCanvasConfiguration WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER = {
  0, /* device */
  0, /* format */
  WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT, /* usage */
  0, /* numViewFormats */
  nullptr, /* viewFormats */
  HTML_PREDEFINED_COLOR_SPACE_SRGB, /* colorSpace */
  WGPU_CANVAS_ALPHA_MODE_OPAQUE, /* alphaMode */
};

const WGpuColorTargetState WGPU_COLOR_TARGET_STATE_DEFAULT_INITIALIZER = {
  0, /* format */
  { /* blend */
      { /* color */
        WGPU_BLEND_OPERATION_DISABLED, /* operation */
        WGPU_BLEND_FACTOR_SRC, /*srcFactor */
        WGPU_BLEND_FACTOR_ONE_MINUS_SRC /* dstFactor */
      },
      { /* alpha */
        WGPU_BLEND_OPERATION_ADD, /* operation */
        WGPU_BLEND_FACTOR_ONE, /*srcFactor */
        WGPU_BLEND_FACTOR_ZERO /* dstFactor */
      }
  },
  WGPU_COLOR_WRITE_ALL /* writeMask */
};

const WGpuRenderPipelineDescriptor WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER = {
  { /* vertex */
    0, /* module */
    nullptr, /* entryPoint */
    0, /* numBuffers */
    nullptr, /* buffers */
    0, /* numConstants */
    nullptr, /* constants */
  },
  { /* primitive */
    WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, /* topology */
    0, /* stripIndexFormat */
    WGPU_FRONT_FACE_CCW, /* frontFace */
    WGPU_CULL_MODE_NONE, /* cullMode */
    false /* unclippedDepth */
  },
  { /* depthStencil */
    0, /* format */
    false, /* depthWriteEnabled */
    WGPU_COMPARE_FUNCTION_ALWAYS, /* depthCompare */
    0xFFFFFFFFu, /* stencilReadMask */
    0xFFFFFFFFu, /* stencilWriteMask */
    0, /* depthBias */
    0.0f, /* depthBiasSlopeScale */
    0.0f, /* depthBiasClamp */
    { /* stencilFront */
      WGPU_COMPARE_FUNCTION_ALWAYS, /* compare */
      WGPU_STENCIL_OPERATION_KEEP, /* failOp */
      WGPU_STENCIL_OPERATION_KEEP, /* depthFailOp */
      WGPU_STENCIL_OPERATION_KEEP /* passOp */
    },
    { /* stencilBack */
      WGPU_COMPARE_FUNCTION_ALWAYS, /* compare */
      WGPU_STENCIL_OPERATION_KEEP, /* failOp */
      WGPU_STENCIL_OPERATION_KEEP, /* depthFailOp */
      WGPU_STENCIL_OPERATION_KEEP /* passOp */
    },
    false, /* clampDepth */
  },
  { /* multisample */
    1, /* count */
    0xFFFFFFFFu, /* mask */
    false /* alphaToCoverageEnabled */
  },
  { /* fragment */
    0, /* module */
    nullptr, /* entryPoint */
    0, /* numTargets */
    nullptr, /* targets */
    0, /* numConstants */
    nullptr /* constants */
  },
  0 /* layout */
};

extern const WGpuExtent3D WGPU_EXTENT_3D_DEFAULT_INITIALIZER = {
  0, /* width */
  1, /* height */
  1 /* depthOrArrayLayers */
};

extern const WGpuRenderPassColorAttachment WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER = {
  0, /* view */
  0, /* resolveTarget */
  WGPU_STORE_OP_STORE, /* storeOp */
  WGPU_LOAD_OP_LOAD, /* loadOp */
  { /* clearValue */
    0.0f,
    0.0f,
    0.0f,
    1.0f,
  }
};

extern const WGpuImageCopyExternalImage WGPU_IMAGE_COPY_EXTERNAL_IMAGE_DEFAULT_INITIALIZER = {
  0, /* source */
  { /* origin */
      0,
      0
  },
  false /* flipY */
};

extern const WGpuImageCopyTexture WGPU_IMAGE_COPY_TEXTURE_DEFAULT_INITIALIZER = {
  0, /* texture */
  0, /* mipLevel */
  { /* origin */
      0,
      0,
      0
  },
  WGPU_TEXTURE_ASPECT_ALL /* aspect */
};

extern const WGpuImageCopyTextureTagged WGPU_IMAGE_COPY_TEXTURE_TAGGED_DEFAULT_INITIALIZER = {
  0, /* texture */
  0, /* mipLevel */
  { /* origin */
    0,
    0,
    0
  },
  WGPU_TEXTURE_ASPECT_ALL, /* aspect */
  HTML_PREDEFINED_COLOR_SPACE_SRGB, /* colorSpace */
  EM_FALSE /* premultipliedAlpha */
};

#ifdef __cplusplus
} // ~extern "C"
#endif
