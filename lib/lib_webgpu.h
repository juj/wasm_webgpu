#pragma once

#include <emscripten/html5.h>
#include <stdint.h>

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

typedef int WGPU_POWER_PREFERENCE;
#define WGPU_POWER_PREFERENCE_INVALID 0
#define WGPU_POWER_PREFERENCE_LOW_POWER 1
#define WGPU_POWER_PREFERENCE_HIGH_PERFORMANCE 2

typedef int WGPU_TEXTURE_DIMENSION;
#define WGPU_TEXTURE_DIMENSION_INVALID 0
#define WGPU_TEXTURE_DIMENSION_1D 3
#define WGPU_TEXTURE_DIMENSION_2D 4
#define WGPU_TEXTURE_DIMENSION_3D 5

typedef int WGPU_TEXTURE_VIEW_DIMENSION;
#define WGPU_TEXTURE_VIEW_DIMENSION_INVALID 0
#define WGPU_TEXTURE_VIEW_DIMENSION_1D 3
#define WGPU_TEXTURE_VIEW_DIMENSION_2D 4
#define WGPU_TEXTURE_VIEW_DIMENSION_2D_ARRAY 6
#define WGPU_TEXTURE_VIEW_DIMENSION_CUBE 7
#define WGPU_TEXTURE_VIEW_DIMENSION_CUBE_ARRAY 8
#define WGPU_TEXTURE_VIEW_DIMENSION_3D 5

typedef int WGPU_TEXTURE_ASPECT;
#define WGPU_TEXTURE_ASPECT_INVALID 0
#define WGPU_TEXTURE_ASPECT_ALL 9
#define WGPU_TEXTURE_ASPECT_STENCIL_ONLY 10
#define WGPU_TEXTURE_ASPECT_DEPTH_ONLY 11

typedef int WGPU_TEXTURE_FORMAT;
#define WGPU_TEXTURE_FORMAT_INVALID 0
#define WGPU_TEXTURE_FORMAT_R8UNORM 12
#define WGPU_TEXTURE_FORMAT_R8SNORM 13
#define WGPU_TEXTURE_FORMAT_R8UINT 14
#define WGPU_TEXTURE_FORMAT_R8SINT 15
#define WGPU_TEXTURE_FORMAT_R16UINT 16
#define WGPU_TEXTURE_FORMAT_R16SINT 17
#define WGPU_TEXTURE_FORMAT_R16FLOAT 18
#define WGPU_TEXTURE_FORMAT_RG8UNORM 19
#define WGPU_TEXTURE_FORMAT_RG8SNORM 20
#define WGPU_TEXTURE_FORMAT_RG8UINT 21
#define WGPU_TEXTURE_FORMAT_RG8SINT 22
#define WGPU_TEXTURE_FORMAT_R32UINT 23
#define WGPU_TEXTURE_FORMAT_R32SINT 24
#define WGPU_TEXTURE_FORMAT_R32FLOAT 25
#define WGPU_TEXTURE_FORMAT_RG16UINT 26
#define WGPU_TEXTURE_FORMAT_RG16SINT 27
#define WGPU_TEXTURE_FORMAT_RG16FLOAT 28
#define WGPU_TEXTURE_FORMAT_RGBA8UNORM 29
#define WGPU_TEXTURE_FORMAT_RGBA8UNORM_SRGB 30
#define WGPU_TEXTURE_FORMAT_RGBA8SNORM 31
#define WGPU_TEXTURE_FORMAT_RGBA8UINT 32
#define WGPU_TEXTURE_FORMAT_RGBA8SINT 33
#define WGPU_TEXTURE_FORMAT_BGRA8UNORM 34
#define WGPU_TEXTURE_FORMAT_BGRA8UNORM_SRGB 35
#define WGPU_TEXTURE_FORMAT_RGB9E5UFLOAT 36
#define WGPU_TEXTURE_FORMAT_RGB10A2UNORM 37
#define WGPU_TEXTURE_FORMAT_RG11B10UFLOAT 38
#define WGPU_TEXTURE_FORMAT_RG32UINT 39
#define WGPU_TEXTURE_FORMAT_RG32SINT 40
#define WGPU_TEXTURE_FORMAT_RG32FLOAT 41
#define WGPU_TEXTURE_FORMAT_RGBA16UINT 42
#define WGPU_TEXTURE_FORMAT_RGBA16SINT 43
#define WGPU_TEXTURE_FORMAT_RGBA16FLOAT 44
#define WGPU_TEXTURE_FORMAT_RGBA32UINT 45
#define WGPU_TEXTURE_FORMAT_RGBA32SINT 46
#define WGPU_TEXTURE_FORMAT_RGBA32FLOAT 47
#define WGPU_TEXTURE_FORMAT_STENCIL8 48
#define WGPU_TEXTURE_FORMAT_DEPTH16UNORM 49
#define WGPU_TEXTURE_FORMAT_DEPTH24PLUS 50
#define WGPU_TEXTURE_FORMAT_DEPTH24PLUS_STENCIL8 51
#define WGPU_TEXTURE_FORMAT_DEPTH32FLOAT 52
#define WGPU_TEXTURE_FORMAT_BC1_RGBA_UNORM 53
#define WGPU_TEXTURE_FORMAT_BC1_RGBA_UNORM_SRGB 54
#define WGPU_TEXTURE_FORMAT_BC2_RGBA_UNORM 55
#define WGPU_TEXTURE_FORMAT_BC2_RGBA_UNORM_SRGB 56
#define WGPU_TEXTURE_FORMAT_BC3_RGBA_UNORM 57
#define WGPU_TEXTURE_FORMAT_BC3_RGBA_UNORM_SRGB 58
#define WGPU_TEXTURE_FORMAT_BC4_R_UNORM 59
#define WGPU_TEXTURE_FORMAT_BC4_R_SNORM 60
#define WGPU_TEXTURE_FORMAT_BC5_RG_UNORM 61
#define WGPU_TEXTURE_FORMAT_BC5_RG_SNORM 62
#define WGPU_TEXTURE_FORMAT_BC6H_RGB_UFLOAT 63
#define WGPU_TEXTURE_FORMAT_BC6H_RGB_FLOAT 64
#define WGPU_TEXTURE_FORMAT_BC7_RGBA_UNORM 65
#define WGPU_TEXTURE_FORMAT_BC7_RGBA_UNORM_SRGB 66
#define WGPU_TEXTURE_FORMAT_DEPTH24UNORM_STENCIL8 67
#define WGPU_TEXTURE_FORMAT_DEPTH32FLOAT_STENCIL8 68

typedef int WGPU_ADDRESS_MODE;
#define WGPU_ADDRESS_MODE_INVALID 0
#define WGPU_ADDRESS_MODE_CLAMP_TO_EDGE 69
#define WGPU_ADDRESS_MODE_REPEAT 70
#define WGPU_ADDRESS_MODE_MIRROR_REPEAT 71

typedef int WGPU_FILTER_MODE;
#define WGPU_FILTER_MODE_INVALID 0
#define WGPU_FILTER_MODE_NEAREST 72
#define WGPU_FILTER_MODE_LINEAR 73

typedef int WGPU_COMPARE_FUNCTION;
#define WGPU_COMPARE_FUNCTION_INVALID 0
#define WGPU_COMPARE_FUNCTION_NEVER 74
#define WGPU_COMPARE_FUNCTION_LESS 75
#define WGPU_COMPARE_FUNCTION_EQUAL 76
#define WGPU_COMPARE_FUNCTION_LESS_EQUAL 77
#define WGPU_COMPARE_FUNCTION_GREATER 78
#define WGPU_COMPARE_FUNCTION_NOT_EQUAL 79
#define WGPU_COMPARE_FUNCTION_GREATER_EQUAL 80
#define WGPU_COMPARE_FUNCTION_ALWAYS 81

typedef int WGPU_BUFFER_BINDING_TYPE;
#define WGPU_BUFFER_BINDING_TYPE_INVALID 0
#define WGPU_BUFFER_BINDING_TYPE_UNIFORM 82
#define WGPU_BUFFER_BINDING_TYPE_STORAGE 83
#define WGPU_BUFFER_BINDING_TYPE_READ_ONLY_STORAGE 84

typedef int WGPU_SAMPLER_BINDING_TYPE;
#define WGPU_SAMPLER_BINDING_TYPE_INVALID 0
#define WGPU_SAMPLER_BINDING_TYPE_FILTERING 85
#define WGPU_SAMPLER_BINDING_TYPE_NON_FILTERING 86
#define WGPU_SAMPLER_BINDING_TYPE_COMPARISON 87

typedef int WGPU_TEXTURE_SAMPLE_TYPE;
#define WGPU_TEXTURE_SAMPLE_TYPE_INVALID 0
#define WGPU_TEXTURE_SAMPLE_TYPE_FLOAT 88
#define WGPU_TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT 89
#define WGPU_TEXTURE_SAMPLE_TYPE_DEPTH 90
#define WGPU_TEXTURE_SAMPLE_TYPE_SINT 91
#define WGPU_TEXTURE_SAMPLE_TYPE_UINT 92

typedef int WGPU_STORAGE_TEXTURE_ACCESS;
#define WGPU_STORAGE_TEXTURE_ACCESS_INVALID 0
#define WGPU_STORAGE_TEXTURE_ACCESS_READ_ONLY 93
#define WGPU_STORAGE_TEXTURE_ACCESS_WRITE_ONLY 94

typedef int WGPU_COMPILATION_MESSAGE_TYPE;
#define WGPU_COMPILATION_MESSAGE_TYPE_INVALID 0
#define WGPU_COMPILATION_MESSAGE_TYPE_ERROR 95
#define WGPU_COMPILATION_MESSAGE_TYPE_WARNING 96
#define WGPU_COMPILATION_MESSAGE_TYPE_INFO 97

typedef int WGPU_PRIMITIVE_TOPOLOGY;
#define WGPU_PRIMITIVE_TOPOLOGY_INVALID 0
#define WGPU_PRIMITIVE_TOPOLOGY_POINT_LIST 98
#define WGPU_PRIMITIVE_TOPOLOGY_LINE_LIST 99
#define WGPU_PRIMITIVE_TOPOLOGY_LINE_STRIP 100
#define WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST 101
#define WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP 102

typedef int WGPU_FRONT_FACE;
#define WGPU_FRONT_FACE_INVALID 0
#define WGPU_FRONT_FACE_CCW 103
#define WGPU_FRONT_FACE_CW 104

typedef int WGPU_CULL_MODE;
#define WGPU_CULL_MODE_INVALID 0
#define WGPU_CULL_MODE_NONE 105
#define WGPU_CULL_MODE_FRONT 106
#define WGPU_CULL_MODE_BACK 107

typedef int WGPU_BLEND_FACTOR;
#define WGPU_BLEND_FACTOR_INVALID 0
#define WGPU_BLEND_FACTOR_ZERO 108
#define WGPU_BLEND_FACTOR_ONE 109
#define WGPU_BLEND_FACTOR_SRC_COLOR 110
#define WGPU_BLEND_FACTOR_ONE_MINUS_SRC_COLOR 111
#define WGPU_BLEND_FACTOR_SRC_ALPHA 112
#define WGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA 113
#define WGPU_BLEND_FACTOR_DST_COLOR 114
#define WGPU_BLEND_FACTOR_ONE_MINUS_DST_COLOR 115
#define WGPU_BLEND_FACTOR_DST_ALPHA 116
#define WGPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA 117
#define WGPU_BLEND_FACTOR_SRC_ALPHA_SATURATED 118
#define WGPU_BLEND_FACTOR_BLEND_COLOR 119
#define WGPU_BLEND_FACTOR_ONE_MINUS_BLEND_COLOR 120

typedef int WGPU_BLEND_OPERATION;
#define WGPU_BLEND_OPERATION_INVALID 0
#define WGPU_BLEND_OPERATION_ADD 121
#define WGPU_BLEND_OPERATION_SUBTRACT 122
#define WGPU_BLEND_OPERATION_REVERSE_SUBTRACT 123
#define WGPU_BLEND_OPERATION_MIN 124
#define WGPU_BLEND_OPERATION_MAX 125

typedef int WGPU_STENCIL_OPERATION;
#define WGPU_STENCIL_OPERATION_INVALID 0
#define WGPU_STENCIL_OPERATION_KEEP 126
#define WGPU_STENCIL_OPERATION_ZERO 108
#define WGPU_STENCIL_OPERATION_REPLACE 127
#define WGPU_STENCIL_OPERATION_INVERT 128
#define WGPU_STENCIL_OPERATION_INCREMENT_CLAMP 129
#define WGPU_STENCIL_OPERATION_DECREMENT_CLAMP 130
#define WGPU_STENCIL_OPERATION_INCREMENT_WRAP 131
#define WGPU_STENCIL_OPERATION_DECREMENT_WRAP 132

typedef int WGPU_INDEX_FORMAT;
#define WGPU_INDEX_FORMAT_INVALID 0
#define WGPU_INDEX_FORMAT_UINT16 133
#define WGPU_INDEX_FORMAT_UINT32 134

typedef int WGPU_VERTEX_FORMAT;
#define WGPU_VERTEX_FORMAT_INVALID 0
#define WGPU_VERTEX_FORMAT_UCHAR2 135
#define WGPU_VERTEX_FORMAT_UCHAR4 136
#define WGPU_VERTEX_FORMAT_CHAR2 137
#define WGPU_VERTEX_FORMAT_CHAR4 138
#define WGPU_VERTEX_FORMAT_UCHAR2NORM 139
#define WGPU_VERTEX_FORMAT_UCHAR4NORM 140
#define WGPU_VERTEX_FORMAT_CHAR2NORM 141
#define WGPU_VERTEX_FORMAT_CHAR4NORM 142
#define WGPU_VERTEX_FORMAT_USHORT2 143
#define WGPU_VERTEX_FORMAT_USHORT4 144
#define WGPU_VERTEX_FORMAT_SHORT2 145
#define WGPU_VERTEX_FORMAT_SHORT4 146
#define WGPU_VERTEX_FORMAT_USHORT2NORM 147
#define WGPU_VERTEX_FORMAT_USHORT4NORM 148
#define WGPU_VERTEX_FORMAT_SHORT2NORM 149
#define WGPU_VERTEX_FORMAT_SHORT4NORM 150
#define WGPU_VERTEX_FORMAT_HALF2 151
#define WGPU_VERTEX_FORMAT_HALF4 152
#define WGPU_VERTEX_FORMAT_FLOAT 88
#define WGPU_VERTEX_FORMAT_FLOAT2 153
#define WGPU_VERTEX_FORMAT_FLOAT3 154
#define WGPU_VERTEX_FORMAT_FLOAT4 155
#define WGPU_VERTEX_FORMAT_UINT 92
#define WGPU_VERTEX_FORMAT_UINT2 156
#define WGPU_VERTEX_FORMAT_UINT3 157
#define WGPU_VERTEX_FORMAT_UINT4 158
#define WGPU_VERTEX_FORMAT_INT 159
#define WGPU_VERTEX_FORMAT_INT2 160
#define WGPU_VERTEX_FORMAT_INT3 161
#define WGPU_VERTEX_FORMAT_INT4 162

typedef int WGPU_INPUT_STEP_MODE;
#define WGPU_INPUT_STEP_MODE_INVALID 0
#define WGPU_INPUT_STEP_MODE_VERTEX 163
#define WGPU_INPUT_STEP_MODE_INSTANCE 164

typedef int WGPU_LOAD_OP;
#define WGPU_LOAD_OP_INVALID 0
#define WGPU_LOAD_OP_LOAD 165

typedef int WGPU_STORE_OP;
#define WGPU_STORE_OP_INVALID 0
#define WGPU_STORE_OP_STORE 166
#define WGPU_STORE_OP_CLEAR 167

typedef int WGPU_QUERY_TYPE;
#define WGPU_QUERY_TYPE_INVALID 0
#define WGPU_QUERY_TYPE_OCCLUSION 168
#define WGPU_QUERY_TYPE_PIPELINE_STATISTICS 169
#define WGPU_QUERY_TYPE_TIMESTAMP 170

typedef int WGPU_PIPELINE_STATISTIC_NAME;
#define WGPU_PIPELINE_STATISTIC_NAME_INVALID 0
#define WGPU_PIPELINE_STATISTIC_NAME_VERTEX_SHADER_INVOCATIONS 171
#define WGPU_PIPELINE_STATISTIC_NAME_CLIPPER_INVOCATIONS 172
#define WGPU_PIPELINE_STATISTIC_NAME_CLIPPER_PRIMITIVES_OUT 173
#define WGPU_PIPELINE_STATISTIC_NAME_FRAGMENT_SHADER_INVOCATIONS 174
#define WGPU_PIPELINE_STATISTIC_NAME_COMPUTE_SHADER_INVOCATIONS 175

typedef int WGPU_DEVICE_LOST_REASON;
#define WGPU_DEVICE_LOST_REASON_INVALID 0
#define WGPU_DEVICE_LOST_REASON_DESTROYED 176

typedef int WGPU_ERROR_FILTER;
#define WGPU_ERROR_FILTER_INVALID 0
#define WGPU_ERROR_FILTER_OUT_OF_MEMORY 177
#define WGPU_ERROR_FILTER_VALIDATION 178

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
  WGPU_VERTEX_FORMAT format;
  uint64_t offset;
  uint32_t shaderLocation;
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
  WGPU_PRIMITIVE_TOPOLOGY topology;
  WGPU_INDEX_FORMAT stripIndexFormat;
  WGPU_FRONT_FACE frontFace;
  WGPU_CULL_MODE cullMode;
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
WGpuSwapChain wgpu_canvascontext_configure_swap_chain(WGpuCanvasContext canvasContext, const WGpuSwapChainDescriptor *descriptor);

WGpuAdapter wgpu_device_get_adapter(WGpuDevice device);
WGpuShaderModule wgpu_device_create_shader_module(WGpuDevice device, const WGpuShaderModuleDescriptor *descriptor);
WGpuRenderPipeline wgpu_device_create_render_pipeline(WGpuDevice device, const WGpuRenderPipelineDescriptor *renderPipelineDesc);
WGpuCommandEncoder wgpu_device_create_command_encoder(WGpuDevice device, const WGpuCommandEncoderDescriptor *commandEncoderDesc);
WGpuQueue wgpu_device_get_default_queue(WGpuDevice device);
// TODO: Add wgpu_device_get_features - or a common wgpu_device_get_properties() that returns both features+limits, like with adapter?
// TODO: Add wgpu_device_get_limits
// TODO: Add wgpu_device_create_buffer
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

// wgpu_buffer_map_async()
// wgpu_buffer_get_mapped_range()
// wgpu_buffer_unmap()

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
// wgpu_render_pass_encoder_set_vertex_buffer()
void wgpu_render_pass_encoder_draw(WGpuRenderPassEncoder passEncoder, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance);
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
