#include "lib_webgpu.h"

extern "C"
{

static const char *wgpuStrings[] = { 0,"low-power","high-performance","1d","2d","3d","2d-array","cube","cube-array","all","stencil-only","depth-only","r8unorm","r8snorm","r8uint","r8sint","r16uint","r16sint","r16float","rg8unorm","rg8snorm","rg8uint","rg8sint","r32uint","r32sint","r32float","rg16uint","rg16sint","rg16float","rgba8unorm","rgba8unorm-srgb","rgba8snorm","rgba8uint","rgba8sint","bgra8unorm","bgra8unorm-srgb","rgb9e5ufloat","rgb10a2unorm","rg11b10ufloat","rg32uint","rg32sint","rg32float","rgba16uint","rgba16sint","rgba16float","rgba32uint","rgba32sint","rgba32float","stencil8","depth16unorm","depth24plus","depth24plus-stencil8","depth32float","bc1-rgba-unorm","bc1-rgba-unorm-srgb","bc2-rgba-unorm","bc2-rgba-unorm-srgb","bc3-rgba-unorm","bc3-rgba-unorm-srgb","bc4-r-unorm","bc4-r-snorm","bc5-rg-unorm","bc5-rg-snorm","bc6h-rgb-ufloat","bc6h-rgb-float","bc7-rgba-unorm","bc7-rgba-unorm-srgb","depth24unorm-stencil8","depth32float-stencil8","clamp-to-edge","repeat","mirror-repeat","nearest","linear","never","less","equal","less-equal","greater","not-equal","greater-equal","always","uniform","storage","read-only-storage","filtering","non-filtering","comparison","float","unfilterable-float","depth","sint","uint","read-only","write-only","error","warning","info","point-list","line-list","line-strip","triangle-list","triangle-strip","ccw","cw","none","front","back","zero","one","src-color","one-minus-src-color","src-alpha","one-minus-src-alpha","dst-color","one-minus-dst-color","dst-alpha","one-minus-dst-alpha","src-alpha-saturated","blend-color","one-minus-blend-color","add","subtract","reverse-subtract","min","max","keep","replace","invert","increment-clamp","decrement-clamp","increment-wrap","decrement-wrap","uint16","uint32","uchar2","uchar4","char2","char4","uchar2norm","uchar4norm","char2norm","char4norm","ushort2","ushort4","short2","short4","ushort2norm","ushort4norm","short2norm","short4norm","half2","half4","float2","float3","float4","uint2","uint3","uint4","int","int2","int3","int4","vertex","instance","load","store","clear","occlusion","pipeline-statistics","timestamp","vertex-shader-invocations","clipper-invocations","clipper-primitives-out","fragment-shader-invocations","compute-shader-invocations","destroyed","out-of-memory","validation" };

const char *wgpu_enum_to_string(int enumValue)
{
  return (enumValue >= 0 && enumValue < sizeof(wgpuStrings)/sizeof(wgpuStrings[0])) ? wgpuStrings[enumValue] : "(invalid WebGPU enum)";
}

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

} // ~extern "C"
