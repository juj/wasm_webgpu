var enums = {};

enums['GPUPowerPreference'] = [
    "low-power",
    "high-performance"
];
/*
enums['GPUFeatureName'] = [
    "depth-clamping",
    "depth24unorm-stencil8",
    "depth32float-stencil8",
    "pipeline-statistics-query",
    "texture-compression-bc",
    "timestamp-query",
];
*/
enums['GPUTextureDimension'] = [
    "1d",
    "2d",
    "3d",
];

enums['GPUTextureViewDimension'] = [
    "1d",
    "2d",
    "2d-array",
    "cube",
    "cube-array",
    "3d"
];

enums['GPUTextureAspect'] = [
    "all",
    "stencil-only",
    "depth-only"
];

enums['GPUTextureFormat'] = [
    // 8-bit formats
    "r8unorm",
    "r8snorm",
    "r8uint",
    "r8sint",

    // 16-bit formats
    "r16uint",
    "r16sint",
    "r16float",
    "rg8unorm",
    "rg8snorm",
    "rg8uint",
    "rg8sint",

    // 32-bit formats
    "r32uint",
    "r32sint",
    "r32float",
    "rg16uint",
    "rg16sint",
    "rg16float",
    "rgba8unorm",
    "rgba8unorm-srgb",
    "rgba8snorm",
    "rgba8uint",
    "rgba8sint",
    "bgra8unorm",
    "bgra8unorm-srgb",
    // Packed 32-bit formats
    "rgb9e5ufloat",
    "rgb10a2unorm",
    "rg11b10ufloat",

    // 64-bit formats
    "rg32uint",
    "rg32sint",
    "rg32float",
    "rgba16uint",
    "rgba16sint",
    "rgba16float",

    // 128-bit formats
    "rgba32uint",
    "rgba32sint",
    "rgba32float",

    // Depth and stencil formats
    "stencil8",
    "depth16unorm",
    "depth24plus",
    "depth24plus-stencil8",
    "depth32float",

    // BC compressed formats usable if "texture-compression-bc" is both
    // supported by the device/user agent and enabled in requestDevice.
    "bc1-rgba-unorm",
    "bc1-rgba-unorm-srgb",
    "bc2-rgba-unorm",
    "bc2-rgba-unorm-srgb",
    "bc3-rgba-unorm",
    "bc3-rgba-unorm-srgb",
    "bc4-r-unorm",
    "bc4-r-snorm",
    "bc5-rg-unorm",
    "bc5-rg-snorm",
    "bc6h-rgb-ufloat",
    "bc6h-rgb-float",
    "bc7-rgba-unorm",
    "bc7-rgba-unorm-srgb",

    // "depth24unorm-stencil8" feature
    "depth24unorm-stencil8",

    // "depth32float-stencil8" feature
    "depth32float-stencil8",
];

enums['GPUAddressMode'] = [
    "clamp-to-edge",
    "repeat",
    "mirror-repeat"
];

enums['GPUFilterMode'] = [
    "nearest",
    "linear"
];

enums['GPUCompareFunction'] = [
    "never",
    "less",
    "equal",
    "less-equal",
    "greater",
    "not-equal",
    "greater-equal",
    "always"
];

enums['GPUBufferBindingType'] = [
    "uniform",
    "storage",
    "read-only-storage",
];

enums['GPUSamplerBindingType'] = [
    "filtering",
    "non-filtering",
    "comparison",
];

enums['GPUTextureSampleType'] = [
  "float",
  "unfilterable-float",
  "depth",
  "sint",
  "uint",
];

enums['GPUStorageTextureAccess'] = [
    "read-only",
    "write-only",
];

enums['GPUCompilationMessageType'] = [
    "error",
    "warning",
    "info"
];

enums['GPUPrimitiveTopology'] = [
    "point-list",
    "line-list",
    "line-strip",
    "triangle-list",
    "triangle-strip"
];

enums['GPUFrontFace'] = [
    "ccw",
    "cw"
];

enums['GPUCullMode'] = [
    "none",
    "front",
    "back"
];

enums['GPUBlendFactor'] = [
    "zero",
    "one",
    "src-color",
    "one-minus-src-color",
    "src-alpha",
    "one-minus-src-alpha",
    "dst-color",
    "one-minus-dst-color",
    "dst-alpha",
    "one-minus-dst-alpha",
    "src-alpha-saturated",
    "blend-color",
    "one-minus-blend-color"
];

enums['GPUBlendOperation'] = [
    "add",
    "subtract",
    "reverse-subtract",
    "min",
    "max"
];

enums['GPUStencilOperation'] = [
    "keep",
    "zero",
    "replace",
    "invert",
    "increment-clamp",
    "decrement-clamp",
    "increment-wrap",
    "decrement-wrap"
];

enums['GPUIndexFormat'] = [
    "uint16",
    "uint32"
];

enums['GPUVertexFormat'] = [
    "uint8x2",
    "uint8x4",
    "sint8x2",
    "sint8x4",
    "unorm8x2",
    "unorm8x4",
    "snorm8x2",
    "snorm8x4",
    "uint16x2",
    "uint16x4",
    "sint16x2",
    "sint16x4",
    "unorm16x2",
    "unorm16x4",
    "snorm16x2",
    "snorm16x4",
    "float16x2",
    "float16x4",
    "float32",
    "float32x2",
    "float32x3",
    "float32x4",
    "uint32",
    "uint32x2",
    "uint32x3",
    "uint32x4",
    "sint32",
    "sint32x2",
    "sint32x3",
    "sint32x4"
];

enums['GPUInputStepMode'] = [
    "vertex",
    "instance"
];

enums['GPULoadOp'] = [
    "load"
];

enums['GPUStoreOp'] = [
    "store",
    "clear"
];

enums['GPUQueryType'] = [
    "occlusion",
    "pipeline-statistics",
    "timestamp"
];

enums['GPUPipelineStatisticName'] = [
    "vertex-shader-invocations",
    "clipper-invocations",
    "clipper-primitives-out",
    "fragment-shader-invocations",
    "compute-shader-invocations"
];

enums['GPUDeviceLostReason'] = [
    "destroyed",
];

enums['GPUErrorFilter'] = [
    "out-of-memory",
    "validation"
];

function convertEnumNameToCppIdentifier(enumName, enumValue) {
  var val = 'WGPU'+enumName.replace('GPU', '').replace(/([A-Z])/g, '_$1').toUpperCase();
  if (enumValue) val += '_' + enumValue.toUpperCase().replace(/-/g, '_');
  return val;
}

var webgpu_strings_h = `#pragma once

`;

var wgpuStrings = [undefined];
for(const [key, value] of Object.entries(enums)) {
  webgpu_strings_h += `typedef int ${convertEnumNameToCppIdentifier(key, '')};\n`;
  webgpu_strings_h += `#define ${convertEnumNameToCppIdentifier(key, 'INVALID')} 0\n`;
  for (const s of value) {
    var id = wgpuStrings.indexOf(s);
    if (id < 0) {
      id = wgpuStrings.length;
      wgpuStrings.push(s);
    }
    webgpu_strings_h += `#define ${convertEnumNameToCppIdentifier(key, s)} ${id}\n`;
  }
  webgpu_strings_h += '\n';
}

const fs = require('fs');
fs.writeFileSync('lib_webgpu_strings.h', webgpu_strings_h);

console.log(`var wgpuStrings = ${JSON.stringify(wgpuStrings)};`);

