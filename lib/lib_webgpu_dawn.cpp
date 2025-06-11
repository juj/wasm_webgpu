#ifndef __EMSCRIPTEN__
#include "lib_webgpu.h"

#include "dawn/webgpu.h"
#include "dawn/dawn_proc.h"
#include "dawn/native/DawnNative.h"

#include <map>
#include <vector>
#include <assert.h>
#ifdef _WIN32
#include <Windows.h>
#endif
#define DAWN_UNSAFE_API 1

enum _WgpuObjectType {
  kWebGPUInvalidObject = 0,
  kWebGPUAdapter,
  kWebGPUDevice,
  kWebGPUBindGroupLayout,
  kWebGPUBuffer,
  kWebGPUTexture,
  kWebGPUTextureView,
  kWebGPUExternalTexture,
  kWebGPUSampler,
  kWebGPUBindGroup,
  kWebGPUPipelineLayout,
  kWebGPUShaderModule,
  kWebGPUComputePipeline,
  kWebGPURenderPipeline,
  kWebGPUCommandBuffer,
  kWebGPUCommandEncoder,
  kWebGPUComputePassEncoder,
  kWebGPURenderPassEncoder,
  kWebGPURenderBundle,
  kWebGPURenderBundleEncoder,
  kWebGPUQueue,
  kWebGPUQuerySet,
  kWebGPUCanvasContext
};

enum _WGpuBufferMapState {
  kWebGPUBufferMapStateUnmapped,
  kWebGPUBufferMapStatePending,
  kWebGPUBufferMapStateMappedForWriting,
  kWebGPUBufferMapStateMappedForReading,
};

// lib_webgpu.js tracks derived objects and deletes them when the parent object is deleted.
// This may potentially cause bad pointers because WGpuObjectBase is a pointer to a _WGpuObject,
// and if the app still has a pointer to a derived object after it deletes the parent object,
// it would be a bad pointer. The app should make sure to delete derived objects before a parent
// object, such as deleting all texture_view's before deleting the texture.

struct _WGpuObject {
  _WgpuObjectType type; // C/Dawn doesn't have the RTTI that JS has.
  void* dawnObject;
};

struct _WGpuObjectBuffer : _WGpuObject {
  _WGpuBufferMapState state;
};

namespace {

const WGPU_BUFFER_MAP_STATE Dawn_to_WGPU_BUFFER_MAP_STATE[] = {
  WGPU_BUFFER_MAP_STATE_UNMAPPED,
  WGPU_BUFFER_MAP_STATE_PENDING,
  WGPU_BUFFER_MAP_STATE_MAPPED,
  WGPU_BUFFER_MAP_STATE_MAPPED
};

template<typename T>
class RuntimeStatic {
public:
  RuntimeStatic()
    : _self(nullptr)
  {}

  ~RuntimeStatic() {
    delete _self;
  }

  T* operator->() {
    if (!_self)
      _self = new T;
    return _self;
  }

  T& operator*() {
    if (!_self)
      _self = new T;
    return *_self;
  }
private:
  T* _self;
};

RuntimeStatic<std::map<void*, WGpuObjectBase>> _dawn_to_webgpu;
RuntimeStatic<std::map<void*, void*>> _webgpu_to_dawn;

// Translate lib_webgpu enums to Dawn enums
const WGPUFeatureName WGPU_FEATURES_BITFIELD_to_Dawn[] = {
  WGPUFeatureName_DepthClipControl,
  WGPUFeatureName_Depth32FloatStencil8,
  WGPUFeatureName_TextureCompressionBC,
  WGPUFeatureName_Force32, // WGPU_FEATURE_TEXTURE_COMPRESSION_BC_SLICED_3D, no Dawn equivalent
  WGPUFeatureName_TextureCompressionETC2,
  WGPUFeatureName_TextureCompressionASTC,
  WGPUFeatureName_TimestampQuery,
  WGPUFeatureName_IndirectFirstInstance,
  WGPUFeatureName_ShaderF16,
  WGPUFeatureName_RG11B10UfloatRenderable,
  WGPUFeatureName_BGRA8UnormStorage,
  WGPUFeatureName_Float32Filterable,
  WGPUFeatureName_ClipDistances,
  WGPUFeatureName_DualSourceBlending,
};
const int _wgpu_num_features = 13;

const WGPUPowerPreference WGPU_POWER_PREFERENCE_to_Dawn[] = {
  WGPUPowerPreference_Undefined,
  WGPUPowerPreference_LowPower,
  WGPUPowerPreference_HighPerformance,
};
#define wgpu_power_preference_to_dawn(mode) WGPU_POWER_PREFERENCE_to_Dawn[mode]

#define wgpu_buffer_usage_to_dawn(usage) (WGPUBufferUsage)usage

#define wgpu_map_mode_to_dawn(mode) (WGPUMapMode)mode

const WGPUTextureDimension WGPU_TEXTURE_DIMENSION_to_Dawn[] = {
  WGPUTextureDimension_Force32,
  WGPUTextureDimension_1D,
  WGPUTextureDimension_2D,
  WGPUTextureDimension_3D,
};
#define wgpu_texture_dimension_to_dawn(dim) WGPU_TEXTURE_DIMENSION_to_Dawn[dim]

const WGPU_TEXTURE_DIMENSION Dawn_to_WGPU_TEXTURE_DIMENSION[] = {
  WGPU_TEXTURE_DIMENSION_1D,
  WGPU_TEXTURE_DIMENSION_2D,
  WGPU_TEXTURE_DIMENSION_3D,
};
#define dawn_to_wgpu_texture_dimension(dim) Dawn_to_WGPU_TEXTURE_DIMENSION[dim]

#define wgpu_texture_usage_flags_to_dawn(flags) (WGPUTextureUsage)flags

const WGPUTextureViewDimension WGPU_TEXTURE_VIEW_DIMENSION_to_Dawn[] = {
  WGPUTextureViewDimension_Undefined,
  WGPUTextureViewDimension_1D,
  WGPUTextureViewDimension_2D,
  WGPUTextureViewDimension_2DArray,
  WGPUTextureViewDimension_Cube,
  WGPUTextureViewDimension_CubeArray,
  WGPUTextureViewDimension_3D,
};
#define wgpu_texture_view_dimension_to_dawn(dim) WGPU_TEXTURE_VIEW_DIMENSION_to_Dawn[dim]

WGPUTextureAspect WGPU_TEXTURE_ASPECT_to_Dawn[] = {
  WGPUTextureAspect_Force32,
  WGPUTextureAspect_All,
  WGPUTextureAspect_StencilOnly,
  WGPUTextureAspect_DepthOnly
};
#define wgpu_texture_aspect_to_dawn(aspect) WGPU_TEXTURE_ASPECT_to_Dawn[aspect]

const WGPUTextureFormat WGPU_TEXTURE_FORMAT_to_Dawn[] = {
  WGPUTextureFormat_Undefined, // WGPU_TEXTURE_FORMAT_INVALID 0
  // 8-bit formats
  WGPUTextureFormat_R8Unorm, // WGPU_TEXTURE_FORMAT_R8UNORM 1
  WGPUTextureFormat_R8Snorm, // WGPU_TEXTURE_FORMAT_R8SNORM 2
  WGPUTextureFormat_R8Uint, // WGPU_TEXTURE_FORMAT_R8UINT 3
  WGPUTextureFormat_R8Sint, // WGPU_TEXTURE_FORMAT_R8SINT
  // 16-bit formats
  WGPUTextureFormat_R16Uint, // WGPU_TEXTURE_FORMAT_R16UINT  5
  WGPUTextureFormat_R16Sint, // WGPU_TEXTURE_FORMAT_R16SINT  6
  WGPUTextureFormat_R16Float, // WGPU_TEXTURE_FORMAT_R16FLOAT 7
  WGPUTextureFormat_RG8Unorm, // WGPU_TEXTURE_FORMAT_RG8UNORM 8
  WGPUTextureFormat_RG8Snorm, // WGPU_TEXTURE_FORMAT_RG8SNORM 9
  WGPUTextureFormat_RG8Uint, // WGPU_TEXTURE_FORMAT_RG8UINT  10
  WGPUTextureFormat_RG8Sint, // WGPU_TEXTURE_FORMAT_RG8SINT  11
  // 32-bit formats
  WGPUTextureFormat_R32Uint, // WGPU_TEXTURE_FORMAT_R32UINT 12
  WGPUTextureFormat_R32Sint, // WGPU_TEXTURE_FORMAT_R32SINT 13
  WGPUTextureFormat_R32Float, // WGPU_TEXTURE_FORMAT_R32FLOAT 14
  WGPUTextureFormat_RG16Uint, // WGPU_TEXTURE_FORMAT_RG16UINT 15
  WGPUTextureFormat_RG16Sint, // WGPU_TEXTURE_FORMAT_RG16SINT 16
  WGPUTextureFormat_RG16Float, // WGPU_TEXTURE_FORMAT_RG16FLOAT 17
  WGPUTextureFormat_RGBA8Unorm, // WGPU_TEXTURE_FORMAT_RGBA8UNORM 18
  WGPUTextureFormat_RGBA8UnormSrgb, // WGPU_TEXTURE_FORMAT_RGBA8UNORM_SRGB 19
  WGPUTextureFormat_RGBA8Snorm, // WGPU_TEXTURE_FORMAT_RGBA8SNORM 20
  WGPUTextureFormat_RGBA8Uint, // WGPU_TEXTURE_FORMAT_RGBA8UINT 21
  WGPUTextureFormat_RGBA8Sint, // WGPU_TEXTURE_FORMAT_RGBA8SINT 22
  WGPUTextureFormat_BGRA8Unorm, // WGPU_TEXTURE_FORMAT_BGRA8UNORM 23
  WGPUTextureFormat_BGRA8UnormSrgb, // WGPU_TEXTURE_FORMAT_BGRA8UNORM_SRGB 24
  // Packed 32-bit formats
  WGPUTextureFormat_RGB9E5Ufloat, // WGPU_TEXTURE_FORMAT_RGB9E5UFLOAT 25
  WGPUTextureFormat_Undefined, // WGPU_TEXTURE_FORMAT_RGB10A2UINT 26
  WGPUTextureFormat_RGB10A2Unorm, // WGPU_TEXTURE_FORMAT_RGB10A2UNORM 27
  WGPUTextureFormat_RG11B10Ufloat, // WGPU_TEXTURE_FORMAT_RG11B10UFLOAT 28
  // 64-bit formats
  WGPUTextureFormat_RG32Uint, // WGPU_TEXTURE_FORMAT_RG32UINT 29
  WGPUTextureFormat_RG32Sint, // WGPU_TEXTURE_FORMAT_RG32SINT 30
  WGPUTextureFormat_RG32Float, // WGPU_TEXTURE_FORMAT_RG32FLOAT 31
  WGPUTextureFormat_RGBA16Uint, // WGPU_TEXTURE_FORMAT_RGBA16UINT 32
  WGPUTextureFormat_RGBA16Sint, // WGPU_TEXTURE_FORMAT_RGBA16SINT 33
  WGPUTextureFormat_RGBA16Float, // WGPU_TEXTURE_FORMAT_RGBA16FLOAT 34
  // 128-bit formats
  WGPUTextureFormat_RGBA32Uint, // WGPU_TEXTURE_FORMAT_RGBA32UINT 35
  WGPUTextureFormat_RGBA32Sint, // WGPU_TEXTURE_FORMAT_RGBA32SINT 36
  WGPUTextureFormat_RGBA32Float, // WGPU_TEXTURE_FORMAT_RGBA32FLOAT 37
  // Depth/stencil formats
  WGPUTextureFormat_Stencil8, // WGPU_TEXTURE_FORMAT_STENCIL8 38
  WGPUTextureFormat_Depth16Unorm,
  WGPUTextureFormat_Depth24Plus,
  WGPUTextureFormat_Depth24PlusStencil8,
  WGPUTextureFormat_Depth32Float,
  WGPUTextureFormat_Depth32FloatStencil8,
  // BC compressed formats usable if "texture-compression-bc" is both
  // supported by the device/user agent and enabled in requestDevice.
  WGPUTextureFormat_BC1RGBAUnorm,
  WGPUTextureFormat_BC1RGBAUnormSrgb,
  WGPUTextureFormat_BC2RGBAUnorm,
  WGPUTextureFormat_BC2RGBAUnormSrgb,
  WGPUTextureFormat_BC3RGBAUnorm,
  WGPUTextureFormat_BC3RGBAUnormSrgb,
  WGPUTextureFormat_BC4RUnorm,
  WGPUTextureFormat_BC4RSnorm,
  WGPUTextureFormat_BC5RGUnorm,
  WGPUTextureFormat_BC5RGSnorm,
  WGPUTextureFormat_BC6HRGBUfloat,
  WGPUTextureFormat_BC6HRGBFloat,
  WGPUTextureFormat_BC7RGBAUnorm,
  WGPUTextureFormat_BC7RGBAUnormSrgb,
  // ETC2 compressed formats usable if "texture-compression-etc2" is both
  // supported by the device/user agent and enabled in requestDevice.
  WGPUTextureFormat_ETC2RGB8Unorm,
  WGPUTextureFormat_ETC2RGB8UnormSrgb,
  WGPUTextureFormat_ETC2RGB8A1Unorm,
  WGPUTextureFormat_ETC2RGB8A1UnormSrgb,
  WGPUTextureFormat_ETC2RGBA8Unorm,
  WGPUTextureFormat_ETC2RGBA8UnormSrgb,
  WGPUTextureFormat_EACR11Unorm,
  WGPUTextureFormat_EACR11Snorm,
  WGPUTextureFormat_EACRG11Unorm,
  WGPUTextureFormat_EACRG11Snorm,
  // ASTC compressed formats usable if "texture-compression-astc" is both
  // supported by the device/user agent and enabled in requestDevice.
  WGPUTextureFormat_ASTC4x4Unorm,
  WGPUTextureFormat_ASTC4x4UnormSrgb,
  WGPUTextureFormat_ASTC5x4Unorm,
  WGPUTextureFormat_ASTC5x4UnormSrgb,
  WGPUTextureFormat_ASTC5x5Unorm,
  WGPUTextureFormat_ASTC5x5UnormSrgb,
  WGPUTextureFormat_ASTC6x5Unorm,
  WGPUTextureFormat_ASTC6x5UnormSrgb,
  WGPUTextureFormat_ASTC6x6Unorm,
  WGPUTextureFormat_ASTC6x6UnormSrgb,
  WGPUTextureFormat_ASTC8x5Unorm,
  WGPUTextureFormat_ASTC8x5UnormSrgb,
  WGPUTextureFormat_ASTC8x6Unorm,
  WGPUTextureFormat_ASTC8x6UnormSrgb,
  WGPUTextureFormat_ASTC8x8Unorm,
  WGPUTextureFormat_ASTC8x8UnormSrgb,
  WGPUTextureFormat_ASTC10x5Unorm,
  WGPUTextureFormat_ASTC10x5UnormSrgb,
  WGPUTextureFormat_ASTC10x6Unorm,
  WGPUTextureFormat_ASTC10x6UnormSrgb,
  WGPUTextureFormat_ASTC10x8Unorm,
  WGPUTextureFormat_ASTC10x8UnormSrgb,
  WGPUTextureFormat_ASTC10x10Unorm,
  WGPUTextureFormat_ASTC10x10UnormSrgb,
  WGPUTextureFormat_ASTC12x10Unorm,
  WGPUTextureFormat_ASTC12x10UnormSrgb,
  WGPUTextureFormat_ASTC12x12Unorm,
  WGPUTextureFormat_ASTC12x12UnormSrgb,
  // ?
  WGPUTextureFormat_R8BG8Biplanar420Unorm,
};
#define wgpu_texture_format_to_dawn(format) WGPU_TEXTURE_FORMAT_to_Dawn[format]

const WGPU_TEXTURE_FORMAT Dawn_to_WGPU_TEXTURE_FORMAT[] = {
  WGPU_TEXTURE_FORMAT_INVALID,
    // 8-bit formats
  WGPU_TEXTURE_FORMAT_R8UNORM,
  WGPU_TEXTURE_FORMAT_R8SNORM,
  WGPU_TEXTURE_FORMAT_R8UINT,
  WGPU_TEXTURE_FORMAT_R8SINT,
    // 16-bit formats
  WGPU_TEXTURE_FORMAT_R16UINT,
  WGPU_TEXTURE_FORMAT_R16SINT,
  WGPU_TEXTURE_FORMAT_R16FLOAT,
  WGPU_TEXTURE_FORMAT_RG8UNORM,
  WGPU_TEXTURE_FORMAT_RG8SNORM,
  WGPU_TEXTURE_FORMAT_RG8UINT,
  WGPU_TEXTURE_FORMAT_RG8SINT,
    // 32-bit formats
  WGPU_TEXTURE_FORMAT_R32FLOAT,
  WGPU_TEXTURE_FORMAT_R32UINT,
  WGPU_TEXTURE_FORMAT_R32SINT,
  WGPU_TEXTURE_FORMAT_RG16UINT,
  WGPU_TEXTURE_FORMAT_RG16SINT,
  WGPU_TEXTURE_FORMAT_RG16FLOAT,
  WGPU_TEXTURE_FORMAT_RGBA8UNORM,
  WGPU_TEXTURE_FORMAT_RGBA8UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_RGBA8SNORM,
  WGPU_TEXTURE_FORMAT_RGBA8UINT,
  WGPU_TEXTURE_FORMAT_RGBA8SINT,
  WGPU_TEXTURE_FORMAT_BGRA8UNORM,
  WGPU_TEXTURE_FORMAT_BGRA8UNORM_SRGB,
    // Packed 32-bit formats
  WGPU_TEXTURE_FORMAT_RGB10A2UINT,
  WGPU_TEXTURE_FORMAT_RGB10A2UNORM,
  WGPU_TEXTURE_FORMAT_RG11B10UFLOAT,
  WGPU_TEXTURE_FORMAT_RGB9E5UFLOAT,
    // 64-bit formats
  WGPU_TEXTURE_FORMAT_RG32FLOAT,
  WGPU_TEXTURE_FORMAT_RG32UINT,
  WGPU_TEXTURE_FORMAT_RG32SINT,
  WGPU_TEXTURE_FORMAT_RGBA16UINT,
  WGPU_TEXTURE_FORMAT_RGBA16SINT,
  WGPU_TEXTURE_FORMAT_RGBA16FLOAT,
    // 128-bit formats
  WGPU_TEXTURE_FORMAT_RGBA32FLOAT,
  WGPU_TEXTURE_FORMAT_RGBA32UINT,
  WGPU_TEXTURE_FORMAT_RGBA32SINT,
    // Depth/stencil formats
  WGPU_TEXTURE_FORMAT_STENCIL8,
  WGPU_TEXTURE_FORMAT_DEPTH16UNORM,
  WGPU_TEXTURE_FORMAT_DEPTH24PLUS,
  WGPU_TEXTURE_FORMAT_DEPTH24PLUS_STENCIL8,
  WGPU_TEXTURE_FORMAT_DEPTH32FLOAT,
  WGPU_TEXTURE_FORMAT_DEPTH32FLOAT_STENCIL8,
    // BC compressed formats usable if "texture-compression-bc" is both
    // supported by the device/user agent and enabled in requestDevice.
  WGPU_TEXTURE_FORMAT_BC1_RGBA_UNORM,
  WGPU_TEXTURE_FORMAT_BC1_RGBA_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_BC2_RGBA_UNORM,
  WGPU_TEXTURE_FORMAT_BC2_RGBA_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_BC3_RGBA_UNORM,
  WGPU_TEXTURE_FORMAT_BC3_RGBA_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_BC4_R_UNORM,
  WGPU_TEXTURE_FORMAT_BC4_R_SNORM,
  WGPU_TEXTURE_FORMAT_BC5_RG_UNORM,
  WGPU_TEXTURE_FORMAT_BC5_RG_SNORM,
  WGPU_TEXTURE_FORMAT_BC6H_RGB_UFLOAT,
  WGPU_TEXTURE_FORMAT_BC6H_RGB_FLOAT,
  WGPU_TEXTURE_FORMAT_BC7_RGBA_UNORM,
  WGPU_TEXTURE_FORMAT_BC7_RGBA_UNORM_SRGB,
    // ETC2 compressed formats usable if "texture-compression-etc2" is both
    // supported by the device/user agent and enabled in requestDevice.
  WGPU_TEXTURE_FORMAT_ETC2_RGB8UNORM,
  WGPU_TEXTURE_FORMAT_ETC2_RGB8UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ETC2_RGB8A1UNORM,
  WGPU_TEXTURE_FORMAT_ETC2_RGB8A1UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ETC2_RGBA8UNORM,
  WGPU_TEXTURE_FORMAT_ETC2_RGBA8UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_EAC_R11UNORM,
  WGPU_TEXTURE_FORMAT_EAC_R11SNORM,
  WGPU_TEXTURE_FORMAT_EAC_RG11UNORM,
  WGPU_TEXTURE_FORMAT_EAC_RG11SNORM,
    // ASTC compressed formats usable if "texture-compression-astc" is both
    // supported by the device/user agent and enabled in requestDevice.
  WGPU_TEXTURE_FORMAT_ASTC_4X4_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_4X4_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_5X4_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_5X4_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_5X5_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_5X5_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_6X5_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_6X5_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_6X6_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_6X6_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_8X5_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_8X5_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_8X6_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_8X6_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_8X8_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_8X8_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_10X5_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_10X5_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_10X6_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_10X6_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_10X8_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_10X8_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_10X10_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_10X10_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_12X10_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_12X10_UNORM_SRGB,
  WGPU_TEXTURE_FORMAT_ASTC_12X12_UNORM,
  WGPU_TEXTURE_FORMAT_ASTC_12X12_UNORM_SRGB,
};
#define dawn_to_wgpu_texture_format(format) Dawn_to_WGPU_TEXTURE_FORMAT[format]

const WGPUAddressMode WGPU_ADDRESS_MODE_to_Dawn[] = {
  WGPUAddressMode_Force32,
  WGPUAddressMode_ClampToEdge,
  WGPUAddressMode_Repeat,
  WGPUAddressMode_MirrorRepeat
};
#define wgpu_address_mode_to_dawn(mode) WGPU_ADDRESS_MODE_to_Dawn[mode]

const WGPUFilterMode WGPU_FILTER_MODE_to_Dawn[] = {
  WGPUFilterMode_Force32,
  WGPUFilterMode_Nearest,
  WGPUFilterMode_Linear
};
#define wgpu_filter_mode_to_dawn(mode) WGPU_FILTER_MODE_to_Dawn[mode]

const WGPUMipmapFilterMode WGPU_MIPMAP_FILTER_MODE_to_Dawn[] = {
  WGPUMipmapFilterMode_Force32,
  WGPUMipmapFilterMode_Nearest,
  WGPUMipmapFilterMode_Linear
};
#define wgpu_mipmap_filter_mode_to_dawn(mode) WGPU_MIPMAP_FILTER_MODE_to_Dawn[mode]

const WGPUCompareFunction WGPU_COMPARE_FUNCTION_to_Dawn[] = {
  WGPUCompareFunction_Undefined,
  WGPUCompareFunction_Never,
  WGPUCompareFunction_Less,
  WGPUCompareFunction_Equal,
  WGPUCompareFunction_LessEqual,
  WGPUCompareFunction_Greater,
  WGPUCompareFunction_NotEqual,
  WGPUCompareFunction_GreaterEqual,
  WGPUCompareFunction_Always,
};
#define wgpu_compare_function_to_dawn(function) WGPU_COMPARE_FUNCTION_to_Dawn[function]

#define wgpu_shader_storage_flags_to_dawn(flags) (WGPUShaderStageFlags)flags

const WGPUBufferBindingType WGPU_BUFFER_BINDING_TYPE_to_Dawn[] = {
  WGPUBufferBindingType_Undefined,
  WGPUBufferBindingType_Uniform,
  WGPUBufferBindingType_Storage,
  WGPUBufferBindingType_ReadOnlyStorage,
};
#define wgpu_buffer_binding_type_to_dawn(type) WGPU_BUFFER_BINDING_TYPE_to_Dawn[type]

const WGPUSamplerBindingType WGPU_SAMPLER_BINDING_TYPE_to_Dawn[] = {
  WGPUSamplerBindingType_Undefined,
  WGPUSamplerBindingType_Filtering,
  WGPUSamplerBindingType_NonFiltering,
  WGPUSamplerBindingType_Comparison,
};
#define wgpu_sampler_binding_type_to_dawn(type) WGPU_SAMPLER_BINDING_TYPE_to_Dawn[type];

const WGPUTextureSampleType WGPU_TEXTURE_SAMPLE_TYPE_to_Dawn[] = {
  WGPUTextureSampleType_Undefined,
  WGPUTextureSampleType_Float,
  WGPUTextureSampleType_UnfilterableFloat,
  WGPUTextureSampleType_Depth,
  WGPUTextureSampleType_Sint,
  WGPUTextureSampleType_Uint
};
#define wgpu_texture_sample_type_to_dawn(type) WGPU_TEXTURE_SAMPLE_TYPE_to_Dawn[type]

const WGPUStorageTextureAccess WGPU_STORAGE_TEXTURE_ACCESS_to_Dawn[] = {
  WGPUStorageTextureAccess_Undefined,
  WGPUStorageTextureAccess_WriteOnly,
  WGPUStorageTextureAccess_ReadOnly,
  WGPUStorageTextureAccess_ReadWrite
};
#define wgpu_storage_texture_access_to_dawn(access) WGPU_STORAGE_TEXTURE_ACCESS_to_Dawn[access]

const WGPUPrimitiveTopology WGPU_PRIMITIVE_TOPOLOGY_to_Dawn[] = {
  WGPUPrimitiveTopology_Force32,
  WGPUPrimitiveTopology_PointList,
  WGPUPrimitiveTopology_LineList,
  WGPUPrimitiveTopology_LineStrip,
  WGPUPrimitiveTopology_TriangleList,
  WGPUPrimitiveTopology_TriangleStrip,
};
#define wgpu_primitive_topology_to_dawn(topology) WGPU_PRIMITIVE_TOPOLOGY_to_Dawn[topology]

const WGPUFrontFace WGPU_FRONT_FACE_to_Dawn[] = {
  WGPUFrontFace_Force32,
  WGPUFrontFace_CCW,
  WGPUFrontFace_CW,
};
#define wgpu_front_face_to_dawn(type) WGPU_FRONT_FACE_to_Dawn[type]

const WGPUCullMode WGPU_CULL_MODE_to_Dawn[] = {
  WGPUCullMode_Force32,
  WGPUCullMode_None,
  WGPUCullMode_Front,
  WGPUCullMode_Back,
};
#define wgpu_cull_mode_to_dawn(mode) WGPU_CULL_MODE_to_Dawn[mode]

#define wgpu_color_write_flags_to_dawn(flags) (WGPUColorWriteMaskFlags)flags

const WGPUBlendFactor WGPU_BLEND_FACTOR_to_Dawn[] = {
  WGPUBlendFactor_Force32, //WGPU_BLEND_FACTOR_INVALID
  WGPUBlendFactor_Zero, //WGPU_BLEND_FACTOR_ZERO,
  WGPUBlendFactor_One, //WGPU_BLEND_FACTOR_ONE,
  WGPUBlendFactor_Src, //WGPU_BLEND_FACTOR_SRC,
  WGPUBlendFactor_OneMinusSrc, //WGPU_BLEND_FACTOR_ONE_MINUS_SRC,
  WGPUBlendFactor_SrcAlpha, //WGPU_BLEND_FACTOR_SRC_ALPHA,
  WGPUBlendFactor_OneMinusSrcAlpha, //WGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
  WGPUBlendFactor_Dst, //WGPU_BLEND_FACTOR_DST,
  WGPUBlendFactor_OneMinusDst, //WGPU_BLEND_FACTOR_ONE_MINUS_DST,
  WGPUBlendFactor_DstAlpha, //WGPU_BLEND_FACTOR_DST_ALPHA,
  WGPUBlendFactor_OneMinusDstAlpha, //WGPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
  WGPUBlendFactor_SrcAlphaSaturated, //WGPU_BLEND_FACTOR_SRC_ALPHA_SATURATED,
  WGPUBlendFactor_Constant, //WGPU_BLEND_FACTOR_CONSTANT,
  WGPUBlendFactor_OneMinusConstant, //WGPU_BLEND_FACTOR_ONE_MINUS_CONSTANT
};
#define wgpu_blend_factor_to_dawn(factor) WGPU_BLEND_FACTOR_to_Dawn[factor]

const WGPUBlendOperation WGPU_BLEND_OPERATION_to_Dawn[] = {
  WGPUBlendOperation_Force32,
  WGPUBlendOperation_Add,
  WGPUBlendOperation_Subtract,
  WGPUBlendOperation_ReverseSubtract,
  WGPUBlendOperation_Min,
  WGPUBlendOperation_Max,
};
#define wgpu_blend_operation_to_dawn(op) WGPU_BLEND_OPERATION_to_Dawn[op]

const WGPUStencilOperation WGPU_STENCIL_OPERATION_to_Dawn[] = {
  WGPUStencilOperation_Force32,
  WGPUStencilOperation_Keep,
  WGPUStencilOperation_Zero,
  WGPUStencilOperation_Replace,
  WGPUStencilOperation_Invert,
  WGPUStencilOperation_IncrementClamp,
  WGPUStencilOperation_DecrementClamp,
  WGPUStencilOperation_IncrementWrap,
  WGPUStencilOperation_DecrementWrap,
};
#define wgpu_stencil_operation_to_dawn(op) WGPU_STENCIL_OPERATION_to_Dawn[op]

const WGPUIndexFormat WGPU_INDEX_FORMAT_to_Dawn[] = {
  WGPUIndexFormat_Undefined,
  WGPUIndexFormat_Uint16,
  WGPUIndexFormat_Uint32,
};
#define wgpu_index_format_to_dawn(format) WGPU_INDEX_FORMAT_to_Dawn[format]

const WGPUVertexFormat WGPU_VERTEX_FORMAT_to_Dawn[] = {
  WGPUVertexFormat_Force32,
  WGPUVertexFormat_Uint8x2,
  WGPUVertexFormat_Uint8x4,
  WGPUVertexFormat_Sint8x2,
  WGPUVertexFormat_Sint8x4,
  WGPUVertexFormat_Unorm8x2,
  WGPUVertexFormat_Unorm8x4,
  WGPUVertexFormat_Snorm8x2,
  WGPUVertexFormat_Snorm8x4,
  WGPUVertexFormat_Uint16x2,
  WGPUVertexFormat_Uint16x4,
  WGPUVertexFormat_Sint16x2,
  WGPUVertexFormat_Sint16x4,
  WGPUVertexFormat_Unorm16x2,
  WGPUVertexFormat_Unorm16x4,
  WGPUVertexFormat_Snorm16x2,
  WGPUVertexFormat_Snorm16x4,
  WGPUVertexFormat_Float16x2,
  WGPUVertexFormat_Float16x4,
  WGPUVertexFormat_Float32,
  WGPUVertexFormat_Float32x2,
  WGPUVertexFormat_Float32x3,
  WGPUVertexFormat_Float32x4,
  WGPUVertexFormat_Uint32,
  WGPUVertexFormat_Uint32x2,
  WGPUVertexFormat_Uint32x3,
  WGPUVertexFormat_Uint32x4,
  WGPUVertexFormat_Sint32,
  WGPUVertexFormat_Sint32x2,
  WGPUVertexFormat_Sint32x3,
  WGPUVertexFormat_Sint32x4,
  WGPUVertexFormat_Unorm10_10_10_2
};
#define wgpu_vertex_format_to_dawn(format) (format == 0 ? WGPUVertexFormat_Force32 : WGPU_VERTEX_FORMAT_to_Dawn[format - (WGPU_VERTEX_FORMAT_UINT8X2 - 1)])

const WGPUVertexStepMode WGPU_VERTEX_STEP_MODE_to_Dawn[] = {
  WGPUVertexStepMode_VertexBufferNotUsed,
  WGPUVertexStepMode_Vertex,
  WGPUVertexStepMode_Instance
};
#define wgpu_vertex_step_mode_to_dawn(mode) WGPU_VERTEX_STEP_MODE_to_Dawn[mode]

const WGPULoadOp WGPU_LOAD_OP_to_Dawn[] = {
  WGPULoadOp_Undefined,
  WGPULoadOp_Load,
  WGPULoadOp_Clear
};
#define wgpu_load_op_to_dawn(op) WGPU_LOAD_OP_to_Dawn[op]

const WGPUStoreOp WGPU_STORE_OP_to_Dawn[] = {
  WGPUStoreOp_Undefined,
  WGPUStoreOp_Store,
  WGPUStoreOp_Discard
};
#define wgpu_store_op_to_dawn(op) WGPU_STORE_OP_to_Dawn[op]

const WGPUQueryType WGPU_QUERY_TYPE_to_Dawn[] = {
  WGPUQueryType_Force32,
  WGPUQueryType_Occlusion,
  WGPUQueryType_Timestamp,
};
#define wgpu_query_type_to_dawn(type) WGPU_QUERY_TYPE_to_Dawn[type]

const WGPU_QUERY_TYPE Dawn_to_WGPU_QUERY_TYPE[] = {
  WGPU_QUERY_TYPE_OCCLUSION,
  WGPU_QUERY_TYPE_TIMESTAMP,
};
#define dawn_to_wgpu_query_type(type) Dawn_to_WGPU_QUERY_TYPE[type]

const WGPUErrorFilter WGPU_ERROR_FILTER_to_Dawn[] = {
  WGPUErrorFilter_Force32,
  WGPUErrorFilter_OutOfMemory,
  WGPUErrorFilter_Validation,
  WGPUErrorFilter_Internal
};
#define wgpu_error_filter_to_dawn(filter) WGPU_ERROR_FILTER_to_Dawn[filter]

WGPU_DEVICE_LOST_REASON Dawn_to_WGPU_DEVICE_LOST_REASON[] = {
  WGPU_DEVICE_LOST_REASON_UNKNOWN,
  WGPU_DEVICE_LOST_REASON_DESTROYED
};
#define dawn_to_wgpu_device_lost_reason(reason) Dawn_to_WGPU_DEVICE_LOST_REASON[reason]

const WGPUCompositeAlphaMode WGPU_CANVAS_ALPHA_MODE_to_Dawn[] = {
  WGPUCompositeAlphaMode_Force32,
  WGPUCompositeAlphaMode_Opaque,
  WGPUCompositeAlphaMode_Premultiplied
};
#define wgpu_canvas_alpha_mode_to_dawn(alphaMode) WGPU_CANVAS_ALPHA_MODE_to_Dawn[alphaMode]

//////////////////////////////////////////////////////////////////////

#ifdef DAWN_UNSAFE_API
static const char* enabledToggles = "allow_unsafe_apis";

static WGPUDawnTogglesDescriptor instanceTogglesDesc{ { nullptr, WGPUSType_DawnTogglesDescriptor}, 1, &enabledToggles };
static WGPUInstanceDescriptor instanceDesc{ reinterpret_cast<WGPUChainedStruct*>(&instanceTogglesDesc) };

#endif
static dawn::native::Instance& GetDawnInstance() {
#if DAWN_UNSAFE_API
	static dawn::native::Instance instance(&instanceDesc);
#else
	static dawn::native::Instance instance;
#endif
  return instance;
}

struct _WGpuCanvasContext {
  WGPUSurface surface;
};

// Returns the number of leading zeros.
// Is there a standard C/C++ function for this?
static int clz32(int x) {
  int r = 0;
  if (!x)
    return 32;
  if (!(x & 0xffff0000u)) {
    x <<= 16;
    r += 16;
  }
  if (!(x & 0xff000000u)) {
    x <<= 8;
    r += 8;
  }
  if (!(x & 0xf0000000u)) {
    x <<= 4;
    r += 4;
  }
  if (!(x & 0xc0000000u)) {
    x <<= 2;
    r += 2;
  }
  if (!(x & 0x80000000u)) {
    x <<= 1;
    r += 1;
  }
  return r;
}

static inline _WGpuObject* _wgpu_get(WGpuObjectBase id) {
  return (_WGpuObject*)id;
}

template<typename T> inline _WgpuObjectType _wgpu_get_type() { return kWebGPUInvalidObject; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUAdapter>() { return kWebGPUAdapter; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUDevice>() { return kWebGPUDevice; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUBindGroupLayout>() { return kWebGPUBindGroupLayout; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUBuffer>() { return kWebGPUBuffer; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUTexture>() { return kWebGPUTexture; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUTextureView>() { return kWebGPUTextureView; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUExternalTexture>() { return kWebGPUExternalTexture; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUSampler>() { return kWebGPUSampler; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUBindGroup>() { return kWebGPUBindGroup; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUPipelineLayout>() { return kWebGPUPipelineLayout; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUShaderModule>() { return kWebGPUShaderModule; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUComputePipeline>() { return kWebGPUComputePipeline; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPURenderPipeline>() { return kWebGPURenderPipeline; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUCommandEncoder>() { return kWebGPUCommandEncoder; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUComputePassEncoder>() { return kWebGPUComputePassEncoder; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPURenderPassEncoder>() { return kWebGPURenderPassEncoder; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUCommandBuffer>() { return kWebGPUCommandBuffer; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUQuerySet>() { return kWebGPUQuerySet; }
template<> inline _WgpuObjectType _wgpu_get_type<WGPUQueue>() { return kWebGPUQueue; }
template<> inline _WgpuObjectType _wgpu_get_type<_WGpuCanvasContext*>() { return kWebGPUCanvasContext; }

template<typename T>
static inline T _wgpu_get_dawn(WGpuObjectBase id) {
  if (!id)
    return nullptr;
  assert(_wgpu_get_type<T>() == ((_WGpuObject*)id)->type);
  return (T)(((_WGpuObject*)id)->dawnObject);
}

static WGpuObjectBase _wgpu_store(_WgpuObjectType type, void* dawnObject) {
  _WGpuObject* wgpu;
  if (type == kWebGPUBuffer)
    wgpu = new _WGpuObjectBuffer{ type, dawnObject, kWebGPUBufferMapStateUnmapped };
  else
    wgpu = new _WGpuObject{ type, dawnObject };

  (*_dawn_to_webgpu)[dawnObject] = wgpu;

  return wgpu;
}

static WGpuObjectBase _wgpu_store_and_set_parent(_WgpuObjectType type, void* dawnObject, WGpuObjectBase parent) {
  WGpuObjectBase id = _wgpu_store(type, dawnObject);
  // derived objects currently aren't being tracked because deleting them can cause potential for memory errors.
  return id;
}

void _wgpu_object_destroy(_WGpuObject* obj) {
  // Dawn has separate Destroy functions for the different object types.
  switch (obj->type) {
  case kWebGPUAdapter: {
    auto a = (*_webgpu_to_dawn).find(obj->dawnObject);
    if (a != (*_webgpu_to_dawn).end()) {
      delete (*a).second;
      (*_webgpu_to_dawn).erase(a);
    }
    wgpuAdapterRelease((WGPUAdapter)obj->dawnObject);
    break;
  }
  case kWebGPUDevice:
    wgpuDeviceDestroy((WGPUDevice)obj->dawnObject);
    break;
  case kWebGPUBindGroupLayout:
    wgpuBindGroupLayoutRelease((WGPUBindGroupLayout)obj->dawnObject);
    break;
  case kWebGPUBuffer:
    wgpuBufferDestroy((WGPUBuffer)obj->dawnObject);
    break;
  case kWebGPUTexture:
    wgpuTextureDestroy((WGPUTexture)obj->dawnObject);
    break;
  case kWebGPUTextureView:
    wgpuTextureViewRelease((WGPUTextureView)obj->dawnObject);
    break;
  case kWebGPUExternalTexture:
    wgpuExternalTextureDestroy((WGPUExternalTexture)obj->dawnObject);
    break;
  case kWebGPUSampler:
    wgpuSamplerRelease((WGPUSampler)obj->dawnObject);
    break;
  case kWebGPUBindGroup:
    wgpuBindGroupRelease((WGPUBindGroup)obj->dawnObject);
    break;
  case kWebGPUPipelineLayout:
    wgpuPipelineLayoutRelease((WGPUPipelineLayout)obj->dawnObject);
    break;
  case kWebGPUShaderModule:
    wgpuShaderModuleRelease((WGPUShaderModule)obj->dawnObject);
    break;
  case kWebGPUComputePipeline:
    wgpuComputePipelineRelease((WGPUComputePipeline)obj->dawnObject);
    break;
  case kWebGPURenderPipeline:
    wgpuRenderPipelineRelease((WGPURenderPipeline)obj->dawnObject);
    break;
  case kWebGPUCommandBuffer:
    wgpuCommandBufferRelease((WGPUCommandBuffer)obj->dawnObject);
    break;
  case kWebGPUCommandEncoder:
    wgpuCommandEncoderRelease((WGPUCommandEncoder)obj->dawnObject);
    break;
  case kWebGPUComputePassEncoder:
    wgpuComputePassEncoderRelease((WGPUComputePassEncoder)obj->dawnObject);
    break;
  case kWebGPURenderPassEncoder:
    wgpuRenderPassEncoderRelease((WGPURenderPassEncoder)obj->dawnObject);
    break;
  case kWebGPUQueue:
    wgpuQueueRelease((WGPUQueue)obj->dawnObject);
    break;
  case kWebGPUCanvasContext:
    break;
  case kWebGPUQuerySet:
    wgpuQuerySetDestroy((WGPUQuerySet)obj->dawnObject);
    break;
  default:
    assert(false);
    break;
  }
  obj->dawnObject = 0;
  obj->type = kWebGPUInvalidObject;
}

} // namespace

extern "C" {

uint32_t wgpu_get_num_live_objects() {
  return _dawn_to_webgpu->size();
}

void wgpu_object_destroy(WGpuObjectBase wgpuObject) {
  if (wgpuObject == 0)
    return;

  _WGpuObject* obj = (_WGpuObject*)wgpuObject;
  auto id = _dawn_to_webgpu->find(obj->dawnObject);
  if (id == _dawn_to_webgpu->end())
    return;
  _dawn_to_webgpu->erase(id);

  _wgpu_object_destroy(obj);

  delete obj;
}

void wgpu_destroy_all_objects() {
  for (auto i : *_dawn_to_webgpu) {
    _WGpuObject* obj = (_WGpuObject*)i.second;
    _wgpu_object_destroy(obj);
    delete obj;
  }
  _dawn_to_webgpu->clear();
}

WGpuCanvasContext wgpu_canvas_get_webgpu_context(void *hwnd) {
  WGPUSurfaceDescriptor surfaceDesc{};

#ifdef _WIN32
  WGPUSurfaceSourceWindowsHWND chainedDesc = WGPU_SURFACE_SOURCE_WINDOWS_HWND_INIT;
  chainedDesc.hinstance = GetModuleHandle(NULL);
  chainedDesc.hwnd = hwnd;
  chainedDesc.chain = { nullptr, WGPUSType_SurfaceSourceWindowsHWND };
  surfaceDesc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&chainedDesc);
#endif

  _WGpuCanvasContext* context = new _WGpuCanvasContext{};
  context->surface = wgpuInstanceCreateSurface(GetDawnInstance().Get(), &surfaceDesc);

  return _wgpu_store(kWebGPUCanvasContext, context);
}

WGPU_BOOL wgpu_is_valid_object(WGpuObjectBase obj) {
  return obj != 0 && _dawn_to_webgpu->find(((_WGpuObject*)obj)->dawnObject) != _dawn_to_webgpu->end();
}

void wgpu_object_set_label(WGpuObjectBase objBase, const char* label) {
  _WGpuObject* obj = _wgpu_get(objBase);
  assert(obj);
  switch (obj->type) {
  case kWebGPUDevice:
    wgpuDeviceSetLabel((WGPUDevice)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUBindGroupLayout:
    wgpuBindGroupLayoutSetLabel((WGPUBindGroupLayout)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUBuffer:
    wgpuBufferSetLabel((WGPUBuffer)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUTexture:
    wgpuTextureSetLabel((WGPUTexture)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUTextureView:
    wgpuTextureViewSetLabel((WGPUTextureView)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUExternalTexture:
    wgpuExternalTextureSetLabel((WGPUExternalTexture)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUSampler:
    wgpuSamplerSetLabel((WGPUSampler)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUBindGroup:
    wgpuBindGroupSetLabel((WGPUBindGroup)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUPipelineLayout:
    wgpuPipelineLayoutSetLabel((WGPUPipelineLayout)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUShaderModule:
    wgpuShaderModuleSetLabel((WGPUShaderModule)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUComputePipeline:
    wgpuComputePipelineSetLabel((WGPUComputePipeline)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPURenderPipeline:
    wgpuRenderPipelineSetLabel((WGPURenderPipeline)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUCommandBuffer:
    wgpuCommandBufferSetLabel((WGPUCommandBuffer)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUCommandEncoder:
    wgpuCommandEncoderSetLabel((WGPUCommandEncoder)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUComputePassEncoder:
    wgpuComputePassEncoderSetLabel((WGPUComputePassEncoder)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPURenderPassEncoder:
    wgpuRenderPassEncoderSetLabel((WGPURenderPassEncoder)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUQueue:
    wgpuQueueSetLabel((WGPUQueue)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  case kWebGPUQuerySet:
    wgpuQuerySetSetLabel((WGPUQuerySet)obj->dawnObject, WGPUStringView{label, WGPU_STRLEN });
    break;
  }
}

int wgpu_object_get_label(WGpuObjectBase obj, char* dstLabel, uint32_t dstLabelSize) {
  /* TODO: Dawn has no GetLabel. lib_webgpu.js stores the label in the JS object. */
  return 0;
}

WGPU_BOOL navigator_gpu_request_adapter_async(const WGpuRequestAdapterOptions* options, WGpuRequestAdapterCallback adapterCallback, void* userData) {
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync(options);
  if (adapterCallback)
    adapterCallback(adapter, userData);
  return true;
}

WGpuAdapter navigator_gpu_request_adapter_sync(const WGpuRequestAdapterOptions* options) {
  DawnProcTable procTable = dawn::native::GetProcs();
  dawnProcSetProcs(&procTable);

  WGPURequestAdapterOptions requestOptions{};

  if (options && options->powerPreference == WGPU_POWER_PREFERENCE_LOW_POWER) {
    requestOptions.powerPreference = WGPUPowerPreference_LowPower;
  } else {
    requestOptions.powerPreference = WGPUPowerPreference_HighPerformance;
  }

  // When interacting with dawn the behavior should mimic chromes default selections
  // So instead of specificying a backend, use undefined
  requestOptions.backendType = WGPUBackendType_Undefined;

  std::vector<dawn::native::Adapter> requestedAdapters = GetDawnInstance().EnumerateAdapters(&requestOptions);

  if (!requestedAdapters.empty()) {
    // Adapters supporting requested backend are sorted by power preference
    // ie : when HighPerformance; Discrete_GPU = 0, Integrated = 1, CPU = 2
    auto* finalAdapter = new dawn::native::Adapter(requestedAdapters[0]);
    (*_webgpu_to_dawn)[finalAdapter->Get()] = finalAdapter;
    return _wgpu_store(kWebGPUAdapter, finalAdapter->Get());
  }

  return {0};
}

void navigator_gpu_request_adapter_async_simple(WGpuRequestAdapterCallback adapterCallback) {
  WGpuAdapter adapter = navigator_gpu_request_adapter_sync_simple();
  if (adapterCallback)
    adapterCallback(adapter, nullptr);
}

WGpuAdapter navigator_gpu_request_adapter_sync_simple() {
  WGpuRequestAdapterOptions options = { WGPU_POWER_PREFERENCE_HIGH_PERFORMANCE, false };
  return navigator_gpu_request_adapter_sync(&options);
}

WGPU_TEXTURE_FORMAT navigator_gpu_get_preferred_canvas_format(WGpuAdapter adapter, WGpuCanvasContext canvasContext) {
  assert(wgpu_is_adapter(adapter));
  assert(wgpu_is_canvas_context(canvasContext));

  _WGpuCanvasContext* context = _wgpu_get_dawn<_WGpuCanvasContext*>(canvasContext);
  WGPUSurfaceCapabilities capabilities = WGPU_SURFACE_CAPABILITIES_INIT;
  wgpuSurfaceGetCapabilities(context->surface, _wgpu_get_dawn<WGPUAdapter>(adapter), &capabilities);
  assert(capabilities.formats && capabilities.formatCount > 0);

  return capabilities.formats[0];
}

WGPU_BOOL wgpu_is_adapter(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUAdapter;
}

WGPU_FEATURES_BITFIELD wgpu_adapter_or_device_get_features(WGpuAdapter adapterOrDevice) {
  _WGpuObject* obj = _wgpu_get(adapterOrDevice);
  assert(obj && (obj->type != kWebGPUDevice || obj->type != kWebGPUAdapter));

  bool isDevice = obj->type == kWebGPUDevice;

  WGPU_FEATURES_BITFIELD featuresBitMask = 0;
  for (int i = 0, id = 1; i < _wgpu_num_features; ++i) {
    WGPUFeatureName feature = WGPU_FEATURES_BITFIELD_to_Dawn[i];
    if (isDevice && wgpuDeviceHasFeature((WGPUDevice)obj->dawnObject, feature))
      featuresBitMask |= id;
    else if (!isDevice && wgpuAdapterHasFeature((WGPUAdapter)obj->dawnObject, feature))
      featuresBitMask |= id;
    id *= 2;
  }
  return featuresBitMask;
}

WGPU_BOOL wgpu_adapter_or_device_supports_feature(WGpuAdapter adapterOrDevice, WGPU_FEATURES_BITFIELD feature) {
  _WGpuObject* obj = _wgpu_get(adapterOrDevice);
  assert(obj && (obj->type != kWebGPUDevice || obj->type != kWebGPUAdapter));

  int fi = 32 - clz32(feature);
  if (fi >= _wgpu_num_features)
    return false;
  WGPUFeatureName _feature = WGPU_FEATURES_BITFIELD_to_Dawn[fi];
  if (obj->type == kWebGPUDevice)
    return wgpuDeviceHasFeature((WGPUDevice)obj->dawnObject, _feature);
  return wgpuAdapterHasFeature((WGPUAdapter)obj->dawnObject, _feature);
}

void wgpu_adapter_or_device_get_limits(WGpuAdapter adapterOrDevice, WGpuSupportedLimits* limits) {
  _WGpuObject* obj = _wgpu_get(adapterOrDevice);
  assert(obj && (obj->type != kWebGPUDevice || obj->type != kWebGPUAdapter));

  WGPUSupportedLimits _limits;
  _limits.nextInChain = nullptr;
  if (obj->type == kWebGPUDevice)
    wgpuDeviceGetLimits((WGPUDevice)obj->dawnObject, &_limits);
  else
    wgpuAdapterGetLimits((WGPUAdapter)obj->dawnObject, &_limits);

  limits->maxUniformBufferBindingSize = _limits.limits.maxUniformBufferBindingSize;
  limits->maxStorageBufferBindingSize = _limits.limits.maxStorageBufferBindingSize;
  limits->maxBufferSize = _limits.limits.maxBufferSize;

  limits->maxTextureDimension1D = _limits.limits.maxTextureDimension1D;
  limits->maxTextureDimension2D = _limits.limits.maxTextureDimension2D;
  limits->maxTextureDimension3D = _limits.limits.maxTextureDimension3D;
  limits->maxTextureArrayLayers = _limits.limits.maxTextureArrayLayers;
  limits->maxBindGroups = _limits.limits.maxBindGroups;
  limits->maxBindGroupsPlusVertexBuffers = _limits.limits.maxBindGroupsPlusVertexBuffers;
  limits->maxBindingsPerBindGroup = _limits.limits.maxBindingsPerBindGroup;
  limits->maxDynamicUniformBuffersPerPipelineLayout = _limits.limits.maxDynamicUniformBuffersPerPipelineLayout;
  limits->maxDynamicStorageBuffersPerPipelineLayout = _limits.limits.maxDynamicStorageBuffersPerPipelineLayout;
  limits->maxSampledTexturesPerShaderStage = _limits.limits.maxSampledTexturesPerShaderStage;
  limits->maxSamplersPerShaderStage = _limits.limits.maxSamplersPerShaderStage;
  limits->maxStorageBuffersPerShaderStage = _limits.limits.maxStorageBuffersPerShaderStage;
  limits->maxStorageTexturesPerShaderStage = _limits.limits.maxStorageTexturesPerShaderStage;
  limits->maxUniformBuffersPerShaderStage = _limits.limits.maxUniformBuffersPerShaderStage;
  limits->minUniformBufferOffsetAlignment = _limits.limits.minUniformBufferOffsetAlignment;
  limits->minStorageBufferOffsetAlignment = _limits.limits.minStorageBufferOffsetAlignment;
  limits->maxVertexBuffers = _limits.limits.maxVertexBuffers;
  limits->maxVertexAttributes = _limits.limits.maxVertexAttributes;
  limits->maxVertexBufferArrayStride = _limits.limits.maxVertexBufferArrayStride;
  limits->maxInterStageShaderVariables = _limits.limits.maxInterStageShaderVariables;
  limits->maxColorAttachments = _limits.limits.maxColorAttachments;
  limits->maxColorAttachmentBytesPerSample = _limits.limits.maxColorAttachmentBytesPerSample;
  limits->maxComputeWorkgroupStorageSize = _limits.limits.maxComputeWorkgroupStorageSize;
  limits->maxComputeInvocationsPerWorkgroup = _limits.limits.maxComputeInvocationsPerWorkgroup;
  limits->maxComputeWorkgroupSizeX = _limits.limits.maxComputeWorkgroupSizeX;
  limits->maxComputeWorkgroupSizeY = _limits.limits.maxComputeWorkgroupSizeY;
  limits->maxComputeWorkgroupSizeZ = _limits.limits.maxComputeWorkgroupSizeZ;
  limits->maxComputeWorkgroupsPerDimension = _limits.limits.maxComputeWorkgroupsPerDimension;
}

void wgpu_adapter_get_info(WGpuAdapter adapter, WGpuAdapterInfo *adapterInfo)
{
  assert(wgpu_is_adapter(adapter));
  assert(false); /* TODO */
}

WGPU_BOOL wgpu_adapter_is_fallback_adapter(WGpuAdapter adapter) {
  assert(wgpu_is_adapter(adapter));
  return false; /* TODO */
}

void wgpu_adapter_request_device_async(WGpuAdapter adapter, const WGpuDeviceDescriptor* descriptor,
        WGpuRequestDeviceCallback deviceCallback, void* userData) {
  assert(wgpu_is_adapter(adapter));
  assert(descriptor);
  assert(deviceCallback);
  WGpuDevice device = wgpu_adapter_request_device_sync(adapter, descriptor);
  deviceCallback(device, userData);
}

WGpuDevice wgpu_adapter_request_device_sync(WGpuAdapter adapter, const WGpuDeviceDescriptor* descriptor) {
  assert(wgpu_is_adapter(adapter));
  assert(descriptor);

  std::vector<WGPUFeatureName> features;
  for (int i = 0, id = 1; i < _wgpu_num_features; ++i, id *= 2) {
    if (descriptor->requiredFeatures & id)
      features.push_back(WGPU_FEATURES_BITFIELD_to_Dawn[i]);
  }

  // custom increased limits that we have to specify for the device
  WGPULimits limits = {};
  limits.maxUniformBufferBindingSize = descriptor->requiredLimits.maxUniformBufferBindingSize;
  limits.maxStorageBufferBindingSize = descriptor->requiredLimits.maxStorageBufferBindingSize;
  limits.maxBufferSize = descriptor->requiredLimits.maxBufferSize;
  limits.maxTextureDimension1D = descriptor->requiredLimits.maxTextureDimension1D;
  limits.maxTextureDimension2D = descriptor->requiredLimits.maxTextureDimension2D;
  limits.maxTextureDimension3D = descriptor->requiredLimits.maxTextureDimension3D;
  limits.maxTextureArrayLayers = descriptor->requiredLimits.maxTextureArrayLayers;
  limits.maxBindGroups = descriptor->requiredLimits.maxBindGroups;
  limits.maxBindGroupsPlusVertexBuffers = descriptor->requiredLimits.maxBindGroupsPlusVertexBuffers;
  limits.maxBindingsPerBindGroup = descriptor->requiredLimits.maxBindingsPerBindGroup;
  limits.maxDynamicUniformBuffersPerPipelineLayout = descriptor->requiredLimits.maxDynamicUniformBuffersPerPipelineLayout;
  limits.maxDynamicStorageBuffersPerPipelineLayout = descriptor->requiredLimits.maxDynamicStorageBuffersPerPipelineLayout;
  limits.maxSampledTexturesPerShaderStage = descriptor->requiredLimits.maxSampledTexturesPerShaderStage;
  limits.maxSamplersPerShaderStage = descriptor->requiredLimits.maxSamplersPerShaderStage;
  limits.maxStorageBuffersPerShaderStage = descriptor->requiredLimits.maxStorageBuffersPerShaderStage;
  limits.maxStorageTexturesPerShaderStage = descriptor->requiredLimits.maxStorageTexturesPerShaderStage;
  limits.maxUniformBuffersPerShaderStage = descriptor->requiredLimits.maxUniformBuffersPerShaderStage;
  limits.minUniformBufferOffsetAlignment = descriptor->requiredLimits.minUniformBufferOffsetAlignment;
  limits.minStorageBufferOffsetAlignment = descriptor->requiredLimits.minStorageBufferOffsetAlignment;
  limits.maxVertexBuffers = descriptor->requiredLimits.maxVertexBuffers;
  limits.maxVertexAttributes = descriptor->requiredLimits.maxVertexAttributes;
  limits.maxVertexBufferArrayStride = descriptor->requiredLimits.maxVertexBufferArrayStride;
  limits.maxInterStageShaderVariables = descriptor->requiredLimits.maxInterStageShaderVariables;
  limits.maxColorAttachments = descriptor->requiredLimits.maxColorAttachments;
  limits.maxColorAttachmentBytesPerSample = descriptor->requiredLimits.maxColorAttachmentBytesPerSample;
  limits.maxComputeWorkgroupStorageSize = descriptor->requiredLimits.maxComputeWorkgroupStorageSize;
  limits.maxComputeInvocationsPerWorkgroup = descriptor->requiredLimits.maxComputeInvocationsPerWorkgroup;
  limits.maxComputeWorkgroupSizeX = descriptor->requiredLimits.maxComputeWorkgroupSizeX;
  limits.maxComputeWorkgroupSizeY = descriptor->requiredLimits.maxComputeWorkgroupSizeY;
  limits.maxComputeWorkgroupSizeZ = descriptor->requiredLimits.maxComputeWorkgroupSizeZ;
  limits.maxComputeWorkgroupsPerDimension = descriptor->requiredLimits.maxComputeWorkgroupsPerDimension;

  WGPURequiredLimits requiredLimits = {};
  requiredLimits.nextInChain = nullptr;
  requiredLimits.limits = limits;

  WGPUDeviceDescriptor _desc = {
    nullptr, /* nextInChain */
    WGPU_STRING_VIEW_INIT, /* label */
    features.size(), /* requiredFeaturesCount */
    features.data(), /* requiredFeatures */
    &requiredLimits, /* requiredLimits */
    0 /* defaultQueue */
  };
  WGPUDevice device = wgpuAdapterCreateDevice(_wgpu_get_dawn<WGPUAdapter>(adapter), &_desc);
  return  _wgpu_store_and_set_parent(kWebGPUDevice, device, adapter);
}

void wgpu_adapter_request_device_async_simple(WGpuAdapter adapter, WGpuRequestDeviceCallback deviceCallback) {
  assert(wgpu_is_adapter(adapter));
  wgpu_adapter_request_device_async(adapter, nullptr, deviceCallback, nullptr);
}

WGpuDevice wgpu_adapter_request_device_sync_simple(WGpuAdapter adapter) {
  assert(wgpu_is_adapter(adapter));
  return wgpu_adapter_request_device_sync(adapter, nullptr);
}

WGPU_BOOL wgpu_is_device(WGpuObjectBase object) {
  _WGpuObject* obj = !object ? nullptr : _wgpu_get(object);
  return obj && obj->type == kWebGPUDevice;
}

WGpuQueue wgpu_device_get_queue(WGpuDevice device) {
  assert(wgpu_is_device(device));
  WGPUDevice _device = _wgpu_get_dawn<WGPUDevice>(device);
  WGPUQueue queue = wgpuDeviceGetQueue(_device);
  return _wgpu_store_and_set_parent(kWebGPUQueue, queue, device);
}

void wgpu_device_tick(WGpuDevice device) {
  assert(wgpu_is_device(device));
  WGPUDevice _device = _wgpu_get_dawn<WGPUDevice>(device);
  wgpuDeviceTick(_device);
}

WGpuBuffer wgpu_device_create_buffer(WGpuDevice device, const WGpuBufferDescriptor* bufferDesc) {
  assert(wgpu_is_device(device));
  assert(bufferDesc);

  WGPUBufferDescriptor _desc = {};
  _desc.usage = wgpu_buffer_usage_to_dawn(bufferDesc->usage);
  _desc.size = bufferDesc->size;
  _desc.mappedAtCreation = bufferDesc->mappedAtCreation;
  _desc.label = WGPU_STRING_VIEW_INIT;
  _desc.nextInChain = nullptr;

  WGPUBuffer buffer = wgpuDeviceCreateBuffer(_wgpu_get_dawn<WGPUDevice>(device), &_desc);
  WGpuBuffer id = _wgpu_store_and_set_parent(kWebGPUBuffer, buffer, device);
  if (bufferDesc->mappedAtCreation) {
    _WGpuObjectBuffer* obj = (_WGpuObjectBuffer*)_wgpu_get(id);
    obj->state = kWebGPUBufferMapStateMappedForWriting;
  }
  return id;
}

WGpuTexture wgpu_device_create_texture(WGpuDevice device, const WGpuTextureDescriptor* textureDesc) {
  assert(wgpu_is_device(device));
  assert(textureDesc);

  WGPUTextureDescriptor _desc = {};
  _desc.usage = (WGPUTextureUsage)textureDesc->usage;
  _desc.dimension = wgpu_texture_dimension_to_dawn(textureDesc->dimension);
  _desc.size = WGPUExtent3D{ textureDesc->width, textureDesc->height, textureDesc->depthOrArrayLayers };
  _desc.format = wgpu_texture_format_to_dawn(textureDesc->format);
  _desc.mipLevelCount = textureDesc->mipLevelCount;
  _desc.sampleCount = textureDesc->sampleCount;
  _desc.viewFormatCount = textureDesc->numViewFormats;

  std::vector<WGPUTextureFormat> viewFormats(textureDesc->numViewFormats);
  for (int i = 0; i < textureDesc->numViewFormats; ++i)
    viewFormats[i] = wgpu_texture_format_to_dawn(textureDesc->viewFormats[i]);
  _desc.viewFormats = viewFormats.data();

  WGPUTexture texture = wgpuDeviceCreateTexture(_wgpu_get_dawn<WGPUDevice>(device), &_desc);
  return _wgpu_store_and_set_parent(kWebGPUTexture, texture, device);
}

WGpuSampler wgpu_device_create_sampler(WGpuDevice device, const WGpuSamplerDescriptor* samplerDesc) {
  assert(wgpu_is_device(device));
  // samplerDesc can be a nullptr;

  if (samplerDesc == nullptr) {
    WGPUSampler sampler = wgpuDeviceCreateSampler(_wgpu_get_dawn<WGPUDevice>(device), nullptr);
    return _wgpu_store_and_set_parent(kWebGPUSampler, sampler, device);
  }

  WGPUSamplerDescriptor _desc = {};
  _desc.addressModeU = wgpu_address_mode_to_dawn(samplerDesc->addressModeU);
  _desc.addressModeV = wgpu_address_mode_to_dawn(samplerDesc->addressModeV);
  _desc.addressModeW = wgpu_address_mode_to_dawn(samplerDesc->addressModeW);
  _desc.magFilter = wgpu_filter_mode_to_dawn(samplerDesc->magFilter);
  _desc.minFilter = wgpu_filter_mode_to_dawn(samplerDesc->minFilter);
  _desc.mipmapFilter = wgpu_mipmap_filter_mode_to_dawn(samplerDesc->mipmapFilter);
  _desc.lodMinClamp = samplerDesc->lodMinClamp;
  _desc.lodMaxClamp = samplerDesc->lodMaxClamp;
  _desc.compare = wgpu_compare_function_to_dawn(samplerDesc->compare);
  _desc.maxAnisotropy = (uint16_t)samplerDesc->maxAnisotropy;

  WGPUSampler sampler = wgpuDeviceCreateSampler(_wgpu_get_dawn<WGPUDevice>(device), &_desc);
  return _wgpu_store_and_set_parent(kWebGPUSampler, sampler, device);
}

WGpuExternalTexture wgpu_device_import_external_texture(WGpuDevice device, const WGpuExternalTextureDescriptor* externalTextureDesc) {
  assert(wgpu_is_device(device));
  assert(false); /* TODO */
  return 0;
}

WGpuBindGroupLayout wgpu_device_create_bind_group_layout(WGpuDevice device, const WGpuBindGroupLayoutEntry* bindGroupLayoutEntries, int numEntries) {
  assert(wgpu_is_device(device));
  assert(numEntries == 0 || bindGroupLayoutEntries != nullptr);

  std::vector<WGPUBindGroupLayoutEntry> entries(numEntries);

  for (int i = 0; i < numEntries; ++i) {
    const WGpuBindGroupLayoutEntry& layoutEntry = bindGroupLayoutEntries[i];
    entries[i].binding = layoutEntry.binding;
    entries[i].visibility = layoutEntry.visibility;
    entries[i].nextInChain = nullptr;
    entries[i].sampler = {};
    entries[i].texture = {};
    entries[i].storageTexture = {};
    entries[i].buffer = {};

    if (layoutEntry.type == WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER) {
      WGPUBufferBindingLayout bufferLayout = {};
      bufferLayout.hasDynamicOffset = layoutEntry.layout.buffer.hasDynamicOffset;
      bufferLayout.type = wgpu_buffer_binding_type_to_dawn(layoutEntry.layout.buffer.type);
      bufferLayout.minBindingSize = layoutEntry.layout.buffer.minBindingSize;
      entries[i].buffer = bufferLayout;
    } else if (layoutEntry.type == WGPU_BIND_GROUP_LAYOUT_TYPE_TEXTURE) {
      WGPUTextureBindingLayout textureLayout = {};
      textureLayout.multisampled = layoutEntry.layout.texture.multisampled;
      textureLayout.sampleType = wgpu_texture_sample_type_to_dawn(layoutEntry.layout.texture.sampleType);
      textureLayout.viewDimension = wgpu_texture_view_dimension_to_dawn(layoutEntry.layout.texture.viewDimension);
      entries[i].texture = textureLayout;
    } else if (layoutEntry.type == WGPU_BIND_GROUP_LAYOUT_TYPE_SAMPLER) {
      WGPUSamplerBindingLayout samplerLayout = {};
      samplerLayout.type = wgpu_sampler_binding_type_to_dawn(layoutEntry.layout.sampler.type);
      entries[i].sampler = samplerLayout;
    } else if (layoutEntry.type == WGPU_BIND_GROUP_LAYOUT_TYPE_STORAGE_TEXTURE) {
      WGPUStorageTextureBindingLayout storageTextureLayout = {};
      storageTextureLayout.access = wgpu_storage_texture_access_to_dawn(layoutEntry.layout.storageTexture.access);
      storageTextureLayout.format = wgpu_texture_format_to_dawn(layoutEntry.layout.storageTexture.format);
      storageTextureLayout.viewDimension = wgpu_texture_view_dimension_to_dawn(layoutEntry.layout.storageTexture.viewDimension);
      entries[i].storageTexture = storageTextureLayout;
    } else if (layoutEntry.type == WGPU_BIND_GROUP_LAYOUT_TYPE_EXTERNAL_TEXTURE) {
      assert(false); // TODO
    }
  }

  WGPUBindGroupLayoutDescriptor _desc = {};
  _desc.entryCount = numEntries;
  _desc.entries = entries.data();

  WGPUBindGroupLayout layout = wgpuDeviceCreateBindGroupLayout(_wgpu_get_dawn<WGPUDevice>(device), &_desc);
  return _wgpu_store_and_set_parent(kWebGPUBindGroupLayout, layout, device);
}

WGpuPipelineLayout wgpu_device_create_pipeline_layout(WGpuDevice device, const WGpuBindGroupLayout* bindGroupLayouts, int numLayouts) {
  assert(wgpu_is_device(device));
  assert(numLayouts == 0 || bindGroupLayouts != nullptr);

  std::vector<WGPUBindGroupLayout> layouts(numLayouts);
  for (int i = 0; i < numLayouts; ++i)
    layouts[i] = _wgpu_get_dawn<WGPUBindGroupLayout>(bindGroupLayouts[i]);

  WGPUPipelineLayoutDescriptor _desc = {};
  _desc.bindGroupLayoutCount = numLayouts;
  _desc.bindGroupLayouts = layouts.data();

  WGPUPipelineLayout pipelineLayout = wgpuDeviceCreatePipelineLayout(_wgpu_get_dawn<WGPUDevice>(device), &_desc);
  return _wgpu_store_and_set_parent(kWebGPUPipelineLayout, pipelineLayout, device);
}

WGpuBindGroup wgpu_device_create_bind_group(WGpuDevice device, WGpuBindGroupLayout bindGroupLayout, const WGpuBindGroupEntry* entries, int numEntries) {
  assert(wgpu_is_device(device));
  assert(wgpu_is_bind_group_layout(bindGroupLayout));
  assert(numEntries == 0 || entries != nullptr);

  WGPUBindGroupDescriptor _desc = {};
  _desc.layout = _wgpu_get_dawn<WGPUBindGroupLayout>(bindGroupLayout);

  std::vector<WGPUBindGroupEntry> _entries(numEntries);
  for (int i = 0; i < numEntries; ++i) {
    _entries[i] = WGPUBindGroupEntry{};
    _entries[i].binding = entries[i].binding;
    if (wgpu_is_buffer(entries[i].resource)) {
      _entries[i].buffer = _wgpu_get_dawn<WGPUBuffer>(entries[i].resource);
      _entries[i].offset = entries[i].bufferBindOffset;
      _entries[i].size = entries[i].bufferBindSize;
      if (_entries[i].size == 0)
        _entries[i].size = wgpuBufferGetSize(_entries[i].buffer);
    } else if (wgpu_is_sampler(entries[i].resource))
      _entries[i].sampler = _wgpu_get_dawn<WGPUSampler>(entries[i].resource);
    else if (wgpu_is_texture_view(entries[i].resource))
      _entries[i].textureView = _wgpu_get_dawn<WGPUTextureView>(entries[i].resource);
  }

  _desc.entryCount = numEntries;
  _desc.entries = _entries.data();

  WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(_wgpu_get_dawn<WGPUDevice>(device), &_desc);
  return _wgpu_store_and_set_parent(kWebGPUBindGroup, bindGroup, device);
}

WGpuShaderModule wgpu_device_create_shader_module(WGpuDevice device, const WGpuShaderModuleDescriptor* shaderModuleDesc) {
  assert(wgpu_is_device(device));
  assert(shaderModuleDesc != nullptr);

  WGPUShaderSourceWGSL wgslDescriptor = {};
  wgslDescriptor.code = { shaderModuleDesc->code, WGPU_STRLEN } ;
  wgslDescriptor.chain = { nullptr, WGPUSType_ShaderSourceWGSL };

  WGPUShaderModuleDescriptor _desc = {};
  _desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&wgslDescriptor);

  WGPUShaderModule module = wgpuDeviceCreateShaderModule(_wgpu_get_dawn<WGPUDevice>(device), &_desc);
  return _wgpu_store_and_set_parent(kWebGPUShaderModule, module, device);
}

static void fillConstantsArray(const WGpuPipelineConstant* constants, uint32_t numConstants, std::vector<WGPUConstantEntry>& output) {
  for (int i = 0; i < numConstants; ++i) {
    output[i].key = WGPUStringView{ constants[i].name, WGPU_STRLEN } ;
    output[i].value = constants[i].value;
    output[i].nextInChain = nullptr;
  }
}

WGpuComputePipeline wgpu_device_create_compute_pipeline(WGpuDevice device, WGpuShaderModule computeModule, const char* entryPoint,
    WGpuPipelineLayout layout, const WGpuPipelineConstant* constants, int numConstants) {
  assert(wgpu_is_device(device));
  assert(wgpu_is_shader_module(computeModule));
  assert(wgpu_is_pipeline_layout(layout));
  assert(numConstants == 0 || constants != nullptr);
  assert(entryPoint == nullptr || strlen(entryPoint) > 0);

  WGPUComputePipelineDescriptor _desc = {};
  _desc.layout = _wgpu_get_dawn<WGPUPipelineLayout>(layout);
  _desc.compute.module = _wgpu_get_dawn<WGPUShaderModule>(computeModule);
  _desc.compute.entryPoint = WGPUStringView{ entryPoint, WGPU_STRLEN };
  _desc.compute.constantCount = numConstants;
  std::vector<WGPUConstantEntry> _constants(numConstants);
  _desc.compute.constants = numConstants > 0 ? _constants.data() : nullptr;
  fillConstantsArray(constants, numConstants, _constants);

  WGPUComputePipeline computePipeline = wgpuDeviceCreateComputePipeline(_wgpu_get_dawn<WGPUDevice>(device), &_desc);
  return _wgpu_store_and_set_parent(kWebGPUComputePipeline, computePipeline, device);
}

void wgpu_device_create_compute_pipeline_async(WGpuDevice device, WGpuShaderModule computeModule, const char* entryPoint, WGpuPipelineLayout layout,
    const WGpuPipelineConstant* constants, int numConstants, WGpuCreatePipelineCallback callback, void* userData) {
  assert(wgpu_is_device(device));
  assert(wgpu_is_shader_module(computeModule));
  assert(wgpu_is_pipeline_layout(layout));
  assert(numConstants == 0 || constants != nullptr);
  assert(entryPoint == nullptr || strlen(entryPoint) > 0);
  assert(callback != nullptr);

  WGPUComputePipelineDescriptor _desc = {};
  _desc.layout = _wgpu_get_dawn<WGPUPipelineLayout>(layout);
  _desc.compute.module = _wgpu_get_dawn<WGPUShaderModule>(computeModule);
  _desc.compute.entryPoint = WGPUStringView{ entryPoint, WGPU_STRLEN };
  _desc.compute.constantCount = numConstants;
  std::vector<WGPUConstantEntry> _constants(numConstants);
  _desc.compute.constants = numConstants > 0 ? _constants.data() : nullptr;
  fillConstantsArray(constants, numConstants, _constants);

  struct Data {
    WGpuDevice device;
    WGpuCreatePipelineCallback callback;
    void* userData;
  };
  Data* data = new Data{device, callback, userData};

  wgpuDeviceCreateComputePipelineAsync(_wgpu_get_dawn<WGPUDevice>(device), &_desc,
    [](WGPUCreatePipelineAsyncStatus status, WGPUComputePipeline pipeline, WGPUStringView message, void* userdata) {
      Data* data = (Data*)userdata;
      WGpuRenderPipeline _pipeline = _wgpu_store_and_set_parent(kWebGPUComputePipeline, pipeline, data->device);
      data->callback(data->device, nullptr, _pipeline, data->userData);
      delete data;
    }, data);
}

static WGPUStencilFaceState fillStencilFaceState(const WGpuStencilFaceState& state) {
  return WGPUStencilFaceState{
    wgpu_compare_function_to_dawn(state.compare),
    wgpu_stencil_operation_to_dawn(state.failOp),
    wgpu_stencil_operation_to_dawn(state.depthFailOp),
    wgpu_stencil_operation_to_dawn(state.passOp)
  };
}

static WGPUBlendComponent fillBlendComponent(const WGpuBlendComponent& blendComp) {
  return WGPUBlendComponent{
    wgpu_blend_operation_to_dawn(blendComp.operation),
    wgpu_blend_factor_to_dawn(blendComp.srcFactor),
    wgpu_blend_factor_to_dawn(blendComp.dstFactor)
  };
}

WGpuRenderPipeline wgpu_device_create_render_pipeline(WGpuDevice device, const WGpuRenderPipelineDescriptor* renderPipelineDesc) {
  assert(wgpu_is_device(device));
  assert(renderPipelineDesc != nullptr);

  WGPURenderPipelineDescriptor _desc = {};
  _desc.layout = _wgpu_get_dawn<WGPUPipelineLayout>(renderPipelineDesc->layout);

  {
    WGPUPrimitiveState _primitive = {};
    _primitive.topology = wgpu_primitive_topology_to_dawn(renderPipelineDesc->primitive.topology);
    _primitive.stripIndexFormat = wgpu_index_format_to_dawn(renderPipelineDesc->primitive.stripIndexFormat);
    _primitive.frontFace = wgpu_front_face_to_dawn(renderPipelineDesc->primitive.frontFace);
    _primitive.cullMode = wgpu_cull_mode_to_dawn(renderPipelineDesc->primitive.cullMode);
    _desc.primitive = _primitive;
  }

  WGPUDepthStencilState depthState = {};
  if (renderPipelineDesc->depthStencil.format != WGPU_TEXTURE_FORMAT_INVALID) {
    depthState.depthBias = renderPipelineDesc->depthStencil.depthBias;
    depthState.depthBiasClamp = renderPipelineDesc->depthStencil.depthBiasClamp;
    depthState.depthBiasSlopeScale = renderPipelineDesc->depthStencil.depthBiasSlopeScale;
    depthState.depthCompare = wgpu_compare_function_to_dawn(renderPipelineDesc->depthStencil.depthCompare);
    depthState.depthWriteEnabled = static_cast<WGPUOptionalBool>(renderPipelineDesc->depthStencil.depthWriteEnabled);
    depthState.format = wgpu_texture_format_to_dawn(renderPipelineDesc->depthStencil.format);
    depthState.stencilBack = fillStencilFaceState(renderPipelineDesc->depthStencil.stencilBack);
    depthState.stencilFront = fillStencilFaceState(renderPipelineDesc->depthStencil.stencilFront);
    depthState.stencilReadMask = renderPipelineDesc->depthStencil.stencilReadMask;
    depthState.stencilWriteMask = renderPipelineDesc->depthStencil.stencilWriteMask;
    _desc.depthStencil = &depthState;
  } else
    _desc.depthStencil = nullptr;

  WGPUMultisampleState multisample = {};
  multisample.alphaToCoverageEnabled = renderPipelineDesc->multisample.alphaToCoverageEnabled;
  multisample.count = renderPipelineDesc->multisample.count;
  multisample.mask = renderPipelineDesc->multisample.mask;
  _desc.multisample = multisample;

  WGPUVertexState _vertex = {};

  // vertex state
  const auto& vertex = renderPipelineDesc->vertex;
  _vertex.nextInChain = nullptr;
  _vertex.module = _wgpu_get_dawn<WGPUShaderModule>(vertex.module);
  _vertex.entryPoint = WGPUStringView{ vertex.entryPoint, WGPU_STRLEN };
  _vertex.constantCount = (uint32_t)vertex.numConstants;

  std::vector<WGPUConstantEntry> _vertexConstants(vertex.numConstants);
  fillConstantsArray(vertex.constants, vertex.numConstants, _vertexConstants);
  _vertex.constants = _vertexConstants.data();

  _vertex.bufferCount = (uint32_t)vertex.numBuffers;
  std::vector<WGPUVertexBufferLayout> _vertexBuffers(vertex.numBuffers);
  for (int i = 0; i < vertex.numBuffers; ++i) {
    const auto& vBuffer = vertex.buffers[i];
    _vertexBuffers[i].arrayStride = vBuffer.arrayStride;
    _vertexBuffers[i].attributeCount = vBuffer.numAttributes;
    _vertexBuffers[i].stepMode = wgpu_vertex_step_mode_to_dawn(vBuffer.stepMode);

    WGPUVertexAttribute* attributes = new WGPUVertexAttribute[_vertexBuffers[i].attributeCount];
    for (int j = 0; j < _vertexBuffers[i].attributeCount; ++j) {
      WGPU_VERTEX_FORMAT format = vBuffer.attributes[j].format;
      attributes[j].format = wgpu_vertex_format_to_dawn(format);
      attributes[j].offset = vBuffer.attributes[j].offset;
      attributes[j].shaderLocation = vBuffer.attributes[j].shaderLocation;
    }
    _vertexBuffers[i].attributes = attributes;
  }
  _vertex.buffers = _vertexBuffers.data();
  _desc.vertex = _vertex;

  const auto& fragment = renderPipelineDesc->fragment;
  WGPUFragmentState fragmentState = {};
  std::vector<WGPUColorTargetState> targets(fragment.numTargets);
  std::vector<WGPUConstantEntry> outputConstants(fragment.numConstants);
  
  if (fragment.module == 0) {
    _desc.fragment = nullptr;
  } else {
    fragmentState.constantCount = fragment.numConstants;
    fragmentState.entryPoint = WGPUStringView{ fragment.entryPoint, WGPU_STRLEN };
    fragmentState.nextInChain = nullptr;
    fragmentState.module = _wgpu_get_dawn<WGPUShaderModule>(fragment.module);
    fragmentState.targetCount = fragment.numTargets;

    for (int i = 0; i < targets.size(); ++i) {
      targets[i].nextInChain = nullptr;

      if (fragment.targets[i].blend.color.operation != WGPU_BLEND_OPERATION_DISABLED) {
        WGPUBlendState* blend = new WGPUBlendState{};
        blend->color = fillBlendComponent(fragment.targets[i].blend.color);
        blend->alpha = fillBlendComponent(fragment.targets[i].blend.alpha);
        targets[i].blend = blend;
      } else {
        targets[i].blend = nullptr;
      }

      targets[i].format = wgpu_texture_format_to_dawn(fragment.targets[i].format);
      targets[i].writeMask = fragment.targets[i].writeMask;
    }

    fragmentState.targets = targets.data();

    fillConstantsArray(fragment.constants, fragment.numConstants, outputConstants);
    fragmentState.constants = outputConstants.data();
    _desc.fragment = &fragmentState;
  }
  
  WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(_wgpu_get_dawn<WGPUDevice>(device), &_desc);

  if (_desc.fragment != nullptr) {
    for (int i = 0; i < _desc.fragment->targetCount; ++i)
      delete _desc.fragment->targets[i].blend;
  }

  for (int i = 0; i < _desc.vertex.bufferCount; ++i)
    delete[] _desc.vertex.buffers[i].attributes;

  return _wgpu_store_and_set_parent(kWebGPURenderPipeline, pipeline, device);
}

void wgpu_device_create_render_pipeline_async(WGpuDevice device, const WGpuRenderPipelineDescriptor *renderPipelineDesc,
    WGpuCreatePipelineCallback callback, void *userData) {
  assert(wgpu_is_device(device));
  assert(renderPipelineDesc != nullptr);
  assert(callback);

  WGPURenderPipelineDescriptor _desc = {};
  _desc.layout = renderPipelineDesc->layout > 0 ? _wgpu_get_dawn<WGPUPipelineLayout>(renderPipelineDesc->layout) : 0;
  _desc.nextInChain = nullptr;
  _desc.label = WGPU_STRING_VIEW_INIT;

  {
    WGPUPrimitiveState _primitive = {};
    _primitive.nextInChain = nullptr;
    _primitive.topology = wgpu_primitive_topology_to_dawn(renderPipelineDesc->primitive.topology);
    _primitive.stripIndexFormat = wgpu_index_format_to_dawn(renderPipelineDesc->primitive.stripIndexFormat);
    _primitive.frontFace = wgpu_front_face_to_dawn(renderPipelineDesc->primitive.frontFace);
    _primitive.cullMode = wgpu_cull_mode_to_dawn(renderPipelineDesc->primitive.cullMode);
    _desc.primitive = _primitive;
  }

  WGPUDepthStencilState depthState = {};
  if (renderPipelineDesc->depthStencil.format != WGPU_TEXTURE_FORMAT_INVALID) {
    depthState.depthBias = renderPipelineDesc->depthStencil.depthBias;
    depthState.depthBiasClamp = renderPipelineDesc->depthStencil.depthBiasClamp;
    depthState.depthBiasSlopeScale = renderPipelineDesc->depthStencil.depthBiasSlopeScale;
    depthState.depthCompare = wgpu_compare_function_to_dawn(renderPipelineDesc->depthStencil.depthCompare);
    depthState.depthWriteEnabled = static_cast<WGPUOptionalBool>(renderPipelineDesc->depthStencil.depthWriteEnabled);
    depthState.format = wgpu_texture_format_to_dawn(renderPipelineDesc->depthStencil.format);
    depthState.nextInChain = nullptr;
    depthState.stencilBack = fillStencilFaceState(renderPipelineDesc->depthStencil.stencilBack);
    depthState.stencilFront = fillStencilFaceState(renderPipelineDesc->depthStencil.stencilFront);
    depthState.stencilReadMask = renderPipelineDesc->depthStencil.stencilReadMask;
    depthState.stencilWriteMask = renderPipelineDesc->depthStencil.stencilWriteMask;
    _desc.depthStencil = &depthState;
  } else
    _desc.depthStencil = nullptr;

  WGPUMultisampleState multisample = {};
  multisample.alphaToCoverageEnabled = renderPipelineDesc->multisample.alphaToCoverageEnabled;
  multisample.count = renderPipelineDesc->multisample.count;
  multisample.mask = renderPipelineDesc->multisample.mask;
  multisample.nextInChain = nullptr;
  _desc.multisample = multisample;

  WGPUVertexState _vertex = {};

  // vertex state
  const auto& vertex = renderPipelineDesc->vertex;
  _vertex.nextInChain = nullptr;
  _vertex.module = _wgpu_get_dawn<WGPUShaderModule>(vertex.module);
  _vertex.entryPoint = WGPUStringView{ vertex.entryPoint, WGPU_STRLEN };
  _vertex.constantCount = (uint32_t)vertex.numConstants;

  std::vector<WGPUConstantEntry> _vertexConstants(vertex.numConstants);
  fillConstantsArray(vertex.constants, vertex.numConstants, _vertexConstants);
  _vertex.constants = _vertexConstants.data();

  _vertex.bufferCount = (uint32_t)vertex.numBuffers;
  std::vector<WGPUVertexBufferLayout> _vertexBuffers(vertex.numBuffers);
  for (int i = 0; i < vertex.numBuffers; ++i) {
    const auto& vBuffer = vertex.buffers[i];
    _vertexBuffers[i].arrayStride = vBuffer.arrayStride;
    _vertexBuffers[i].attributeCount = vBuffer.numAttributes;
    _vertexBuffers[i].stepMode = wgpu_vertex_step_mode_to_dawn(vBuffer.stepMode);

    WGPUVertexAttribute* attributes = new WGPUVertexAttribute[_vertexBuffers[i].attributeCount];
    for (int j = 0; j < _vertexBuffers[i].attributeCount; ++j) {
      WGPU_VERTEX_FORMAT format = vBuffer.attributes[j].format;
      attributes[j].format = wgpu_vertex_format_to_dawn(format);
      attributes[j].offset = vBuffer.attributes[j].offset;
      attributes[j].shaderLocation = vBuffer.attributes[j].shaderLocation;
    }
    _vertexBuffers[i].attributes = attributes;
  }
  _vertex.buffers = _vertexBuffers.data();
  _desc.vertex = _vertex;

  WGPUFragmentState fragmentState = {};

  const auto& fragment = renderPipelineDesc->fragment;
  std::vector<WGPUColorTargetState> targets(fragment.numTargets);
  std::vector<WGPUConstantEntry> outputConstants(fragment.numConstants);

  if (fragment.module == 0) {
    _desc.fragment = nullptr;
  } else {
    fragmentState.constantCount = fragment.numConstants;
    fragmentState.entryPoint = WGPUStringView{ fragment.entryPoint, WGPU_STRLEN };
    fragmentState.nextInChain = nullptr;
    fragmentState.module = fragment.module > 0 ? _wgpu_get_dawn<WGPUShaderModule>(fragment.module) : 0;
    fragmentState.targetCount = fragment.numTargets;

    for (int i = 0; i < targets.size(); ++i) {
      targets[i].nextInChain = nullptr;

      if (fragment.targets[i].blend.color.operation != WGPU_BLEND_OPERATION_DISABLED) {
        WGPUBlendState* blend = new WGPUBlendState;
        blend->color = fillBlendComponent(fragment.targets[i].blend.color);
        blend->alpha = fillBlendComponent(fragment.targets[i].blend.alpha);
        targets[i].blend = blend;
      } else
        targets[i].blend = nullptr;

      targets[i].format = wgpu_texture_format_to_dawn(fragment.targets[i].format);
      targets[i].writeMask = fragment.targets[i].writeMask;
    }
      
    fragmentState.targets = targets.data();
    
    fillConstantsArray(fragment.constants, fragment.numConstants, outputConstants);
    fragmentState.constants = outputConstants.data();
    _desc.fragment = &fragmentState;
  }

  WGPUDevice _device = _wgpu_get_dawn<WGPUDevice>(device);

  struct _Data {
    WGpuDevice device;
    WGpuCreatePipelineCallback callback;
    void* userdata;
  };
  _Data* data = new _Data{ device, callback, userData };

  wgpuDeviceCreateRenderPipelineAsync(_device, &_desc,
        [](WGPUCreatePipelineAsyncStatus status, WGPURenderPipeline pipeline, WGPUStringView message, void *userdata) {
    _Data* data = (_Data*)userdata;
    WGpuRenderPipeline _pipeline = _wgpu_store_and_set_parent(kWebGPURenderPipeline, pipeline, data->device);
    data->callback(data->device, nullptr, _pipeline, data->userdata);
    delete data;
  }, data);

  if (_desc.fragment != nullptr) {
    for (int i = 0; i < _desc.fragment->targetCount; ++i)
      delete _desc.fragment->targets[i].blend;
  }

  for (int i = 0; i < _desc.vertex.bufferCount; ++i)
    delete[] _desc.vertex.buffers[i].attributes;
}

WGpuCommandEncoder wgpu_device_create_command_encoder(WGpuDevice device, const WGpuCommandEncoderDescriptor *commandEncoderDesc) {
  assert(wgpu_is_device(device));

  WGPUCommandEncoderDescriptor desc = {};
  WGPUCommandEncoder commandEncoder = wgpuDeviceCreateCommandEncoder(_wgpu_get_dawn<WGPUDevice>(device), nullptr);

  return _wgpu_store_and_set_parent(kWebGPUCommandEncoder, commandEncoder, device);
}

WGpuCommandEncoder wgpu_device_create_command_encoder_simple(WGpuDevice device) {
  assert(wgpu_is_device(device));
  WGPUDevice _device = _wgpu_get_dawn<WGPUDevice>(device);
  WGPUCommandEncoderDescriptor desc = {};
  WGPUCommandEncoder commandEncoder = wgpuDeviceCreateCommandEncoder(_wgpu_get_dawn<WGPUDevice>(device), &desc);

  return _wgpu_store_and_set_parent(kWebGPUCommandEncoder, commandEncoder, device);
}

WGpuRenderBundleEncoder wgpu_device_create_render_bundle_encoder(WGpuDevice device, const WGpuRenderBundleEncoderDescriptor *renderBundleEncoderDesc) {
  assert(wgpu_is_device(device));
  assert(renderBundleEncoderDesc != nullptr);

  std::vector<WGPUTextureFormat> colorFormats(renderBundleEncoderDesc->numColorFormats);
  for (int i = 0; i < renderBundleEncoderDesc->numColorFormats; ++i)
    colorFormats[i] = wgpu_texture_format_to_dawn(renderBundleEncoderDesc->colorFormats[i]);

  WGPURenderBundleEncoderDescriptor _desc = {};
  _desc.colorFormatCount = (uint32_t)renderBundleEncoderDesc->numColorFormats;
  _desc.colorFormats = colorFormats.data();
  _desc.depthStencilFormat = wgpu_texture_format_to_dawn(renderBundleEncoderDesc->depthStencilFormat);
  _desc.sampleCount = renderBundleEncoderDesc->sampleCount;
  _desc.depthReadOnly = renderBundleEncoderDesc->depthReadOnly;
  _desc.stencilReadOnly = renderBundleEncoderDesc->stencilReadOnly;

  WGPURenderBundleEncoder encoder = wgpuDeviceCreateRenderBundleEncoder(_wgpu_get_dawn<WGPUDevice>(device), &_desc);
  return _wgpu_store_and_set_parent(kWebGPURenderBundleEncoder, encoder, device);
}

WGpuQuerySet wgpu_device_create_query_set(WGpuDevice device, const WGpuQuerySetDescriptor *querySetDesc) {
  assert(wgpu_is_device(device));
  assert(querySetDesc != nullptr);

  WGPUQuerySetDescriptor _desc = {};
  _desc.type = wgpu_query_type_to_dawn(querySetDesc->type);
  _desc.count = (uint32_t)querySetDesc->count;

  WGPUQuerySet query = wgpuDeviceCreateQuerySet(_wgpu_get_dawn<WGPUDevice>(device), &_desc);
  return _wgpu_store_and_set_parent(kWebGPUQuerySet, query, device);
}

WGPU_BOOL wgpu_is_buffer(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj != nullptr && obj->type == kWebGPUBuffer;
}

void wgpu_buffer_map_async(WGpuBuffer buffer, WGpuBufferMapCallback callback, void *userData, WGPU_MAP_MODE_FLAGS mode,
        double_int53_t offset, double_int53_t size) {
  assert(wgpu_is_buffer(buffer));
  assert(callback);

  _WGpuObjectBuffer* obj = (_WGpuObjectBuffer*)_wgpu_get(buffer);
  if (obj->state != kWebGPUBufferMapStateUnmapped)
    return;

  struct _Data {
    WGpuBuffer buffer;
    WGPU_MAP_MODE_FLAGS mode;
    double_int53_t offset;
    double_int53_t size;
    WGpuBufferMapCallback callback;
    void* userData;
  };
  
  obj->state = kWebGPUBufferMapStatePending;
  _Data* data = new _Data{ buffer, mode, offset, size, callback, userData };
  wgpuBufferMapAsync(_wgpu_get_dawn<WGPUBuffer>(buffer), (WGPUMapMode)mode, (size_t)offset, (size_t)size,
          [](WGPUBufferMapAsyncStatus status, void* userdata) {
    _Data* data = (_Data*)userdata;
    _WGpuObjectBuffer* obj = (_WGpuObjectBuffer*)_wgpu_get(data->buffer);
    obj->state = (data->mode & WGPUMapMode_Write) ? kWebGPUBufferMapStateMappedForWriting : kWebGPUBufferMapStateMappedForReading;
    data->callback(data->buffer, data->userData, data->mode, data->offset, data->size);
    delete data;
  }, data);
}

void wgpu_buffer_map_sync(WGpuBuffer buffer, WGPU_MAP_MODE_FLAGS mode, double_int53_t offset, double_int53_t size) {
    assert(wgpu_is_buffer(buffer));

  _WGpuObjectBuffer* obj = (_WGpuObjectBuffer*)_wgpu_get(buffer);
  if (obj->state != kWebGPUBufferMapStateUnmapped)
  	return;

  obj->state = kWebGPUBufferMapStatePending;

  struct _Data {
  	WGpuBuffer buffer;
  	WGPU_MAP_MODE_FLAGS mode;
  	bool done = false;
  };

  auto callback = [](WGPUBufferMapAsyncStatus status, void* rawData) {
  	auto* userdata = static_cast<_Data*>(rawData);
  	userdata->done = true;
  	_Data* data = (_Data*)userdata;
  	_WGpuObjectBuffer* obj = (_WGpuObjectBuffer*)_wgpu_get(data->buffer);
  	obj->state = (data->mode & WGPUMapMode_Write) ? kWebGPUBufferMapStateMappedForWriting : kWebGPUBufferMapStateMappedForReading;
  };

  _Data data{ buffer, mode };
  wgpuBufferMapAsync(_wgpu_get_dawn<WGPUBuffer>(buffer), (WGPUMapMode)mode, (size_t)offset, (size_t)size, callback, &data);

  while (!data.done) {
  	wgpuInstanceProcessEvents(GetDawnInstance().Get());
  }

  return;
}

double_int53_t wgpu_buffer_get_mapped_range(WGpuBuffer buffer, double_int53_t startOffset, double_int53_t size) {
  assert(wgpu_is_buffer(buffer));
  _WGpuObjectBuffer* obj = (_WGpuObjectBuffer*)_wgpu_get(buffer);
  assert(obj->state == kWebGPUBufferMapStateMappedForWriting || obj->state == kWebGPUBufferMapStateMappedForReading);
  if (size == (double_int53_t)-1)
    size = wgpu_buffer_size(buffer) - startOffset;
  void* offset;
  if (obj->state == kWebGPUBufferMapStateMappedForWriting)
    offset = wgpuBufferGetMappedRange(_wgpu_get_dawn<WGPUBuffer>(buffer), (size_t)startOffset, (size_t)size);
  else
    offset = (void*)wgpuBufferGetConstMappedRange(_wgpu_get_dawn<WGPUBuffer>(buffer), (size_t)startOffset, (size_t)size);
  return (double_int53_t)offset;
}

void wgpu_buffer_read_mapped_range(WGpuBuffer buffer, double_int53_t startOffset, double_int53_t subOffset, void* dst, double_int53_t size) {
  assert(wgpu_is_buffer(buffer));
  _WGpuObjectBuffer* obj = (_WGpuObjectBuffer*)_wgpu_get(buffer);
  assert(obj->state == kWebGPUBufferMapStateMappedForWriting || obj->state == kWebGPUBufferMapStateMappedForReading);
  memcpy(dst, (uint8_t*)startOffset + subOffset, (size_t)size);
}

void wgpu_buffer_write_mapped_range(WGpuBuffer buffer, double_int53_t startOffset, double_int53_t subOffset, const void* src, double_int53_t size) {
  assert(wgpu_is_buffer(buffer));
  _WGpuObjectBuffer* obj = (_WGpuObjectBuffer*)_wgpu_get(buffer);
  assert(obj->state == kWebGPUBufferMapStateMappedForWriting);
  memcpy((uint8_t*)startOffset + subOffset, src, (size_t)size);
}

void wgpu_buffer_unmap(WGpuBuffer buffer) {
  assert(wgpu_is_buffer(buffer));
  _WGpuObjectBuffer* obj = (_WGpuObjectBuffer*)_wgpu_get(buffer);
  assert(obj->state == kWebGPUBufferMapStateMappedForWriting || obj->state == kWebGPUBufferMapStateMappedForReading);
  obj->state = kWebGPUBufferMapStateUnmapped;
  wgpuBufferUnmap(_wgpu_get_dawn<WGPUBuffer>(buffer));
}

double_int53_t wgpu_buffer_size(WGpuBuffer buffer) {
  assert(wgpu_is_buffer(buffer));
  return wgpuBufferGetSize(_wgpu_get_dawn<WGPUBuffer>(buffer));
}

WGPU_BUFFER_USAGE_FLAGS wgpu_buffer_usage(WGpuBuffer buffer) {
  assert(wgpu_is_buffer(buffer));
  return (WGPU_BUFFER_USAGE_FLAGS)wgpuBufferGetUsage(_wgpu_get_dawn<WGPUBuffer>(buffer));
}

WGPU_BUFFER_MAP_STATE wgpu_buffer_map_state(WGpuBuffer buffer) {
  assert(wgpu_is_buffer(buffer));
  _WGpuObjectBuffer* obj = (_WGpuObjectBuffer*)_wgpu_get(buffer);
  return Dawn_to_WGPU_BUFFER_MAP_STATE[obj->state];
}

WGPU_BOOL wgpu_is_texture(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUTexture;
}

WGpuTextureView wgpu_texture_create_view(WGpuTexture texture, const WGpuTextureViewDescriptor *textureViewDesc) {
  assert(wgpu_is_texture(texture));
  assert(textureViewDesc);

  WGPUTextureViewDescriptor _desc = {};
  _desc.format = wgpu_texture_format_to_dawn(textureViewDesc->format);
  _desc.dimension = wgpu_texture_view_dimension_to_dawn(textureViewDesc->dimension);
  _desc.baseMipLevel = textureViewDesc->baseMipLevel;
  _desc.mipLevelCount = textureViewDesc->mipLevelCount;
  _desc.baseArrayLayer = textureViewDesc->baseArrayLayer;
  _desc.arrayLayerCount = textureViewDesc->arrayLayerCount;
  _desc.aspect = wgpu_texture_aspect_to_dawn(textureViewDesc->aspect);

  WGPUTextureView textureView = wgpuTextureCreateView(_wgpu_get_dawn<WGPUTexture>(texture), &_desc);
  return _wgpu_store_and_set_parent(kWebGPUTextureView, textureView, texture);
}

WGpuTextureView wgpu_texture_create_view_simple(WGpuTexture texture) {
  assert(wgpu_is_texture(texture));

  WGPUTextureView textureView = wgpuTextureCreateView(_wgpu_get_dawn<WGPUTexture>(texture), nullptr);
  return _wgpu_store_and_set_parent(kWebGPUTextureView, textureView, texture);
}

uint32_t wgpu_texture_width(WGpuTexture texture) {
  assert(wgpu_is_texture(texture));
  return wgpuTextureGetWidth(_wgpu_get_dawn<WGPUTexture>(texture));
}

uint32_t wgpu_texture_height(WGpuTexture texture) {
  assert(wgpu_is_texture(texture));
  return wgpuTextureGetHeight(_wgpu_get_dawn<WGPUTexture>(texture));
}

uint32_t wgpu_texture_depth_or_array_layers(WGpuTexture texture) {
  assert(wgpu_is_texture(texture));
  return wgpuTextureGetDepthOrArrayLayers(_wgpu_get_dawn<WGPUTexture>(texture));
}

uint32_t wgpu_texture_mip_level_count(WGpuTexture texture) {
  assert(wgpu_is_texture(texture));
  return wgpuTextureGetMipLevelCount(_wgpu_get_dawn<WGPUTexture>(texture));
}

uint32_t wgpu_texture_sample_count(WGpuTexture texture) {
  assert(wgpu_is_texture(texture));
  return wgpuTextureGetSampleCount(_wgpu_get_dawn<WGPUTexture>(texture));
}

WGPU_TEXTURE_DIMENSION wgpu_texture_dimension(WGpuTexture texture) {
  assert(wgpu_is_texture(texture));
  WGPUTextureDimension dimension = wgpuTextureGetDimension(_wgpu_get_dawn<WGPUTexture>(texture));
  return dawn_to_wgpu_texture_dimension(dimension);
}

WGPU_TEXTURE_FORMAT wgpu_texture_format(WGpuTexture texture) {
  assert(wgpu_is_texture(texture));
  WGPUTextureFormat format = wgpuTextureGetFormat(_wgpu_get_dawn<WGPUTexture>(texture));
  return dawn_to_wgpu_texture_format(format);
}

WGPU_TEXTURE_USAGE_FLAGS wgpu_texture_usage(WGpuTexture texture) {
  assert(wgpu_is_texture(texture));
  WGPUTextureUsage usage = wgpuTextureGetUsage(_wgpu_get_dawn<WGPUTexture>(texture));
  return (WGPU_TEXTURE_USAGE_FLAGS)usage;
}

WGPU_BOOL wgpu_is_texture_view(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUTextureView;
}

WGPU_BOOL wgpu_is_external_texture(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUExternalTexture;
}

WGPU_BOOL wgpu_is_sampler(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUSampler;
}

WGPU_BOOL wgpu_is_bind_group_layout(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUBindGroupLayout;
}

WGPU_BOOL wgpu_is_bind_group(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUBindGroup;
}

WGPU_BOOL wgpu_is_pipeline_layout(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUPipelineLayout;
}

WGPU_BOOL wgpu_is_shader_module(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUShaderModule;
}

void wgpu_shader_module_get_compilation_info_async(WGpuShaderModule shaderModule, WGpuGetCompilationInfoCallback callback, void *userData) {
  assert(false); /* TODO */
}

WGpuBindGroupLayout wgpu_pipeline_get_bind_group_layout(WGpuObjectBase pipelineBase, uint32_t index) {
  assert(wgpu_is_compute_pipeline(pipelineBase) || wgpu_is_render_pipeline(pipelineBase));

  WGPUBindGroupLayout bindGroupLayout = wgpuRenderPipelineGetBindGroupLayout(_wgpu_get_dawn<WGPURenderPipeline>(pipelineBase), index);
  return _wgpu_store(kWebGPUBindGroupLayout, bindGroupLayout);
}

WGPU_BOOL wgpu_is_compute_pipeline(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUComputePipeline;
}

WGPU_BOOL wgpu_is_render_pipeline(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPURenderPipeline;
}

WGPU_BOOL wgpu_is_command_buffer(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUCommandBuffer;
}

WGPU_BOOL wgpu_is_debug_commands_mixin(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && (obj->type == kWebGPUComputePassEncoder || obj->type == kWebGPURenderPassEncoder ||
      obj->type == kWebGPURenderBundleEncoder || obj->type == kWebGPUCommandEncoder);
}

void wgpu_encoder_push_debug_group(WGpuDebugCommandsMixin encoder, const char *groupLabel) {
  assert(wgpu_is_debug_commands_mixin(encoder));

  if (wgpu_is_render_pass_encoder(encoder))
    wgpuRenderPassEncoderPushDebugGroup(_wgpu_get_dawn<WGPURenderPassEncoder>(encoder), WGPUStringView{ groupLabel, WGPU_STRLEN });
  else if (wgpu_is_compute_pass_encoder(encoder))
    wgpuComputePassEncoderPushDebugGroup(_wgpu_get_dawn<WGPUComputePassEncoder>(encoder), WGPUStringView{ groupLabel, WGPU_STRLEN });
  else
    wgpuCommandEncoderPushDebugGroup(_wgpu_get_dawn<WGPUCommandEncoder>(encoder), WGPUStringView{ groupLabel, WGPU_STRLEN });
}

void wgpu_encoder_pop_debug_group(WGpuDebugCommandsMixin encoder) {
  assert(wgpu_is_debug_commands_mixin(encoder));

  if (wgpu_is_render_pass_encoder(encoder))
    wgpuRenderPassEncoderPopDebugGroup(_wgpu_get_dawn<WGPURenderPassEncoder>(encoder));
  else if (wgpu_is_compute_pass_encoder(encoder))
    wgpuComputePassEncoderPopDebugGroup(_wgpu_get_dawn<WGPUComputePassEncoder>(encoder));
  else
    wgpuCommandEncoderPopDebugGroup(_wgpu_get_dawn<WGPUCommandEncoder>(encoder));
}

void wgpu_encoder_insert_debug_marker(WGpuDebugCommandsMixin encoder, const char *markerLabel) {
  assert(wgpu_is_debug_commands_mixin(encoder));

  if (wgpu_is_render_pass_encoder(encoder))
    wgpuRenderPassEncoderInsertDebugMarker(_wgpu_get_dawn<WGPURenderPassEncoder>(encoder), WGPUStringView{ markerLabel, WGPU_STRLEN });
  else if (wgpu_is_compute_pass_encoder(encoder))
    wgpuComputePassEncoderInsertDebugMarker(_wgpu_get_dawn<WGPUComputePassEncoder>(encoder), WGPUStringView{ markerLabel, WGPU_STRLEN });
  else
    wgpuCommandEncoderInsertDebugMarker(_wgpu_get_dawn<WGPUCommandEncoder>(encoder), WGPUStringView{ markerLabel, WGPU_STRLEN });
}

WGPU_BOOL wgpu_is_command_encoder(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUCommandEncoder;
}

static WGPURenderPassColorAttachment getColorAttachInfo(const WGpuRenderPassColorAttachment& colorAttachment) {
  assert(wgpu_is_texture_view(colorAttachment.view));

  WGPURenderPassColorAttachment _attachment = {};
  _attachment.view = _wgpu_get_dawn<WGPUTextureView>(colorAttachment.view);
  _attachment.depthSlice = colorAttachment.depthSlice < 0 ? wgpu::kDepthSliceUndefined : colorAttachment.depthSlice;
  _attachment.resolveTarget = _wgpu_get_dawn<WGPUTextureView>(colorAttachment.resolveTarget);
  _attachment.loadOp = wgpu_load_op_to_dawn(colorAttachment.loadOp);
  _attachment.storeOp = wgpu_store_op_to_dawn(colorAttachment.storeOp);
  _attachment.clearValue = WGPUColor{ colorAttachment.clearValue.r, colorAttachment.clearValue.g, colorAttachment.clearValue.b, colorAttachment.clearValue.a };
  return _attachment;
}

WGpuRenderPassEncoder wgpu_command_encoder_begin_render_pass(WGpuCommandEncoder commandEncoder, const WGpuRenderPassDescriptor *renderPassDesc) {
  assert(wgpu_is_command_encoder(commandEncoder));
  assert(renderPassDesc);

  WGPURenderPassDescriptor _desc = {};
  _desc.colorAttachmentCount = (uint32_t)renderPassDesc->numColorAttachments;

  std::vector<WGPURenderPassColorAttachment> colorAttachments(renderPassDesc->numColorAttachments);
  for (int i = 0; i < renderPassDesc->numColorAttachments; ++i)
    colorAttachments[i] = getColorAttachInfo(renderPassDesc->colorAttachments[i]);
  _desc.colorAttachments = colorAttachments.data();

  WGPURenderPassDepthStencilAttachment depthStencil = {};

  const auto& _depthStencil = renderPassDesc->depthStencilAttachment;
  if (_depthStencil.view <= 0) {
    _desc.depthStencilAttachment = nullptr;
  } else {
    depthStencil.view = _wgpu_get_dawn<WGPUTextureView>(_depthStencil.view);
    depthStencil.depthLoadOp  = wgpu_load_op_to_dawn(_depthStencil.depthLoadOp);
    depthStencil.depthStoreOp = wgpu_store_op_to_dawn(_depthStencil.depthStoreOp);
    depthStencil.depthReadOnly = _depthStencil.depthReadOnly;
    depthStencil.depthClearValue = _depthStencil.depthClearValue;
    depthStencil.stencilLoadOp = wgpu_load_op_to_dawn(_depthStencil.stencilLoadOp);
    depthStencil.stencilStoreOp = wgpu_store_op_to_dawn(_depthStencil.stencilStoreOp);
    depthStencil.stencilClearValue = _depthStencil.stencilClearValue;
    depthStencil.stencilReadOnly = _depthStencil.stencilReadOnly;
    _desc.depthStencilAttachment = &depthStencil;
  }

  _desc.occlusionQuerySet = _wgpu_get_dawn<WGPUQuerySet>(renderPassDesc->occlusionQuerySet);
  _desc.label = WGPU_STRING_VIEW_INIT;

  /* TODO add timestampWrite support*/
  _desc.timestampWrites = nullptr;
  
  WGPURenderPassMaxDrawCount chainedDesc;
  if (renderPassDesc->maxDrawCount > 0) {
    chainedDesc.maxDrawCount = renderPassDesc->maxDrawCount;
    chainedDesc.chain = { nullptr, WGPUSType_RenderPassMaxDrawCount };
    _desc.nextInChain = reinterpret_cast<WGPUChainedStruct*>(&chainedDesc);
  } else
    _desc.nextInChain = nullptr;

  WGPURenderPassEncoder renderPassEncoder = wgpuCommandEncoderBeginRenderPass(_wgpu_get_dawn<WGPUCommandEncoder>(commandEncoder), &_desc);
  return _wgpu_store(kWebGPURenderPassEncoder, renderPassEncoder);
}

WGpuComputePassEncoder wgpu_command_encoder_begin_compute_pass(WGpuCommandEncoder commandEncoder, const WGpuComputePassDescriptor *computePassDesc) {
  assert(wgpu_is_command_encoder(commandEncoder));

  WGPUComputePassDescriptor _desc = {};

  WGPUComputePassEncoder encoder = wgpuCommandEncoderBeginComputePass(_wgpu_get_dawn<WGPUCommandEncoder>(commandEncoder), &_desc);

  return _wgpu_store(kWebGPUComputePassEncoder, encoder);
}

void wgpu_command_encoder_copy_buffer_to_buffer(WGpuCommandEncoder commandEncoder, WGpuBuffer source, double_int53_t sourceOffset, WGpuBuffer destination,
    double_int53_t destinationOffset, double_int53_t size) {
  assert(wgpu_is_command_encoder(commandEncoder));
  assert(wgpu_is_buffer(source));
  assert(wgpu_is_buffer(destination));

  WGPUCommandEncoder _commandEncoder = _wgpu_get_dawn<WGPUCommandEncoder>(commandEncoder);
  WGPUBuffer _source  = _wgpu_get_dawn<WGPUBuffer>(source);
  WGPUBuffer _destination = _wgpu_get_dawn<WGPUBuffer>(destination);

  wgpuCommandEncoderCopyBufferToBuffer(_commandEncoder, _source, (uint64_t)sourceOffset, _destination, (uint64_t)destinationOffset, (uint64_t)size);
}

// Helper Function for reading in WGpuImageCopyTexture-> dawn WGPUImageCopyTexture
static void wgpuReadGpuImageCopyTexture(const WGpuImageCopyTexture* source, WGPUImageCopyTexture& output) {
  output.texture = _wgpu_get_dawn<WGPUTexture>(source->texture);
  output.mipLevel = source->mipLevel;
  output.origin = { (uint32_t)source->origin.x, (uint32_t)source->origin.y, (uint32_t)source->origin.z };
  output.aspect = wgpu_texture_aspect_to_dawn(source->aspect);
}

// Helper Function for reading in WGpuImageCopyBuffer-> dawn WGPUImageCopyBuffer
static void wgpuReadGpuImageCopyBuffer(const WGpuImageCopyBuffer* source, WGPUImageCopyBuffer& output) {
  output.layout.offset = source->offset;
  output.layout.bytesPerRow = source->bytesPerRow;
  output.layout.rowsPerImage = source->rowsPerImage;
  output.buffer = _wgpu_get_dawn<WGPUBuffer>(source->buffer);
}

void wgpu_command_encoder_copy_buffer_to_texture(WGpuCommandEncoder commandEncoder, const WGpuImageCopyBuffer *source,
    const WGpuImageCopyTexture *destination, uint32_t copyWidth, uint32_t copyHeight, uint32_t copyDepthOrArrayLayers) {
  assert(wgpu_is_command_encoder(commandEncoder));
  assert(source);
  assert(destination);

  WGPUCommandEncoder _commandEncoder = _wgpu_get_dawn<WGPUCommandEncoder>(commandEncoder);

  WGPUImageCopyBuffer _source;
  wgpuReadGpuImageCopyBuffer(source, _source);

  WGPUImageCopyTexture _destination;
  wgpuReadGpuImageCopyTexture(destination, _destination);

  WGPUExtent3D copySize { copyWidth, copyHeight, copyDepthOrArrayLayers };
  wgpuCommandEncoderCopyBufferToTexture(_commandEncoder, &_source, &_destination, &copySize);
}

void wgpu_command_encoder_copy_texture_to_buffer(WGpuCommandEncoder commandEncoder, const WGpuImageCopyTexture *source,
    const WGpuImageCopyBuffer *destination, uint32_t copyWidth, uint32_t copyHeight, uint32_t copyDepthOrArrayLayers) {
  assert(wgpu_is_command_encoder(commandEncoder));
  assert(source);
  assert(destination);

  WGPUCommandEncoder _commandEncoder = _wgpu_get_dawn<WGPUCommandEncoder>(commandEncoder);

  WGPUImageCopyTexture _source;
  wgpuReadGpuImageCopyTexture(source, _source);

  WGPUImageCopyBuffer _destination;
  wgpuReadGpuImageCopyBuffer(destination, _destination);

  WGPUExtent3D copySize {copyWidth, copyHeight, copyDepthOrArrayLayers};
  wgpuCommandEncoderCopyTextureToBuffer(_commandEncoder, &_source, &_destination, &copySize);
}

void wgpu_command_encoder_copy_texture_to_texture(WGpuCommandEncoder commandEncoder, const WGpuImageCopyTexture *source,
    const WGpuImageCopyTexture *destination, uint32_t copyWidth, uint32_t copyHeight, uint32_t copyDepthOrArrayLayers) {
  assert(wgpu_is_command_encoder(commandEncoder));
  assert(source);
  assert(destination);

  WGPUCommandEncoder _commandEncoder = _wgpu_get_dawn<WGPUCommandEncoder>(commandEncoder);

  WGPUImageCopyTexture _source;
  wgpuReadGpuImageCopyTexture(source, _source);

  WGPUImageCopyTexture _destination;
  wgpuReadGpuImageCopyTexture(destination, _destination);

  WGPUExtent3D copySize {copyWidth, copyHeight, copyDepthOrArrayLayers};
  wgpuCommandEncoderCopyTextureToTexture(_commandEncoder, &_source, &_destination, &copySize);
}

void wgpu_command_encoder_clear_buffer(WGpuCommandEncoder commandEncoder, WGpuBuffer buffer, double_int53_t offset, double_int53_t size) {
  assert(wgpu_is_command_encoder(commandEncoder));
  assert(wgpu_is_buffer(buffer));

  if (size < 0) size = wgpu_buffer_size(buffer) - offset;

  wgpuCommandEncoderClearBuffer(_wgpu_get_dawn<WGPUCommandEncoder>(commandEncoder), _wgpu_get_dawn<WGPUBuffer>(buffer),
      (uint64_t)offset, (uint64_t)size);
}

void wgpu_command_encoder_resolve_query_set(WGpuCommandEncoder commandEncoder, WGpuQuerySet querySet, uint32_t firstQuery, uint32_t queryCount,
    WGpuBuffer destination, double_int53_t destinationOffset) {
  assert(wgpu_is_command_encoder(commandEncoder));
  assert(wgpu_is_query_set(querySet));
  assert(wgpu_is_buffer(destination));

  WGPUCommandEncoder _commandEncoder = _wgpu_get_dawn<WGPUCommandEncoder>(commandEncoder);
  WGPUBuffer _destination = _wgpu_get_dawn<WGPUBuffer>(destination);

  wgpuCommandEncoderResolveQuerySet(_commandEncoder, _wgpu_get_dawn<WGPUQuerySet>(querySet), firstQuery, queryCount,
      _destination, (uint64_t)destinationOffset);
}

WGpuCommandBuffer wgpu_encoder_finish(WGpuObjectBase commandOrRenderBundleEncoder) {
  _WGpuObject* obj = _wgpu_get(commandOrRenderBundleEncoder);
  assert(obj && (obj->type == kWebGPUCommandEncoder || obj->type == kWebGPURenderBundleEncoder));

  if (obj->type == kWebGPUCommandEncoder) {
    WGPUCommandEncoder encoder = (WGPUCommandEncoder)obj->dawnObject;
  
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, nullptr);
    wgpu_object_destroy(commandOrRenderBundleEncoder);

    return _wgpu_store(kWebGPUCommandBuffer, commandBuffer);
  }

  WGPURenderBundleEncoder encoder = (WGPURenderBundleEncoder)obj->dawnObject;
  WGPURenderBundle bundle = wgpuRenderBundleEncoderFinish(encoder, nullptr);
  wgpu_object_destroy(commandOrRenderBundleEncoder);

  return _wgpu_store(kWebGPURenderBundle, bundle);
}

WGPU_BOOL wgpu_is_binding_commands_mixin(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && (obj->type == kWebGPUComputePassEncoder || obj->type == kWebGPURenderPassEncoder ||
      obj->type == kWebGPURenderBundleEncoder);
}

void wgpu_encoder_set_bind_group(WGpuBindingCommandsMixin encoder, uint32_t index, WGpuBindGroup bindGroup,
    const uint32_t *dynamicOffsets, uint32_t numDynamicOffsets) {
  assert(wgpu_is_binding_commands_mixin(encoder));

  if (wgpu_is_render_pass_encoder(encoder))
    wgpuRenderPassEncoderSetBindGroup(_wgpu_get_dawn<WGPURenderPassEncoder>(encoder), index, _wgpu_get_dawn<WGPUBindGroup>(bindGroup),
      numDynamicOffsets, dynamicOffsets);
  else if (wgpu_is_compute_pass_encoder(encoder))
    wgpuComputePassEncoderSetBindGroup(_wgpu_get_dawn<WGPUComputePassEncoder>(encoder), index, _wgpu_get_dawn<WGPUBindGroup>(bindGroup),
      numDynamicOffsets, dynamicOffsets);
}

void wgpu_encoder_set_pipeline(WGpuBindingCommandsMixin encoder, WGpuObjectBase pipeline) {
  assert(wgpu_is_binding_commands_mixin(encoder));
  assert(wgpu_is_render_pipeline(pipeline) || wgpu_is_compute_pipeline(pipeline));
  if (wgpu_is_render_pass_encoder(encoder))
    wgpuRenderPassEncoderSetPipeline(_wgpu_get_dawn<WGPURenderPassEncoder>(encoder), _wgpu_get_dawn<WGPURenderPipeline>(pipeline));
  else if (wgpu_is_compute_pass_encoder(encoder))
    wgpuComputePassEncoderSetPipeline(_wgpu_get_dawn<WGPUComputePassEncoder>(encoder), _wgpu_get_dawn<WGPUComputePipeline>(pipeline));
}

void wgpu_encoder_end(WGpuBindingCommandsMixin encoder) {
  assert(wgpu_is_binding_commands_mixin(encoder));
  if (wgpu_is_render_pass_encoder(encoder))
    wgpuRenderPassEncoderEnd(_wgpu_get_dawn<WGPURenderPassEncoder>(encoder));
  else if (wgpu_is_compute_pass_encoder(encoder))
    wgpuComputePassEncoderEnd(_wgpu_get_dawn<WGPUComputePassEncoder>(encoder));
  wgpu_object_destroy(encoder);
}

WGPU_BOOL wgpu_is_compute_pass_encoder(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUComputePassEncoder;
}

void wgpu_compute_pass_encoder_dispatch_workgroups(WGpuComputePassEncoder encoder, uint32_t workgroupCountX, uint32_t workgroupCountY, uint32_t workgroupCountZ) {
  assert(wgpu_is_compute_pass_encoder(encoder));
  WGPUComputePassEncoder _encoder = _wgpu_get_dawn<WGPUComputePassEncoder>(encoder);
  wgpuComputePassEncoderDispatchWorkgroups(_encoder, workgroupCountX, workgroupCountY, workgroupCountZ);
}

void wgpu_compute_pass_encoder_dispatch_workgroups_indirect(WGpuComputePassEncoder encoder, WGpuBuffer indirectBuffer, double_int53_t indirectOffset) {
  assert(wgpu_is_compute_pass_encoder(encoder));
  WGPUComputePassEncoder _encoder = _wgpu_get_dawn<WGPUComputePassEncoder>(encoder);
  wgpuComputePassEncoderDispatchWorkgroupsIndirect(_encoder, _wgpu_get_dawn<WGPUBuffer>(indirectBuffer), indirectOffset);
}

WGPU_BOOL wgpu_is_render_commands_mixin(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && (obj->type == kWebGPURenderPassEncoder || obj->type == kWebGPURenderBundleEncoder);
}

WGPU_BOOL wgpu_is_render_pass_encoder(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPURenderPassEncoder;
}

WGPU_BOOL wgpu_is_render_bundle_encoder(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj != nullptr && obj->type == kWebGPURenderBundleEncoder;
}

void wgpu_render_commands_mixin_set_index_buffer(WGpuRenderCommandsMixin renderCommandsMixin, WGpuBuffer buffer, WGPU_INDEX_FORMAT indexFormat,
    double_int53_t offset, double_int53_t size) {
  assert(wgpu_is_render_commands_mixin(renderCommandsMixin));

  if (size == (double_int53_t)-1)
    size = wgpuBufferGetSize(_wgpu_get_dawn<WGPUBuffer>(buffer));

  if (wgpu_is_render_pass_encoder(renderCommandsMixin)) {
    wgpuRenderPassEncoderSetIndexBuffer(_wgpu_get_dawn<WGPURenderPassEncoder>(renderCommandsMixin), _wgpu_get_dawn<WGPUBuffer>(buffer),
        (WGPUIndexFormat)indexFormat, (uint64_t)offset, (uint64_t)(size - offset));
  } else if (wgpu_is_render_bundle_encoder(renderCommandsMixin)) {
    wgpuRenderBundleEncoderSetIndexBuffer(_wgpu_get_dawn<WGPURenderBundleEncoder>(renderCommandsMixin), _wgpu_get_dawn<WGPUBuffer>(buffer),
        (WGPUIndexFormat) indexFormat, (uint64_t) offset, (uint64_t)(size - offset));
  }
}

void wgpu_render_commands_mixin_set_vertex_buffer(WGpuRenderCommandsMixin renderCommandsMixin, int32_t slot, WGpuBuffer buffer, double_int53_t offset, double_int53_t size) {
  assert(wgpu_is_render_commands_mixin(renderCommandsMixin));

  if (size == (double_int53_t)-1)
    size = wgpuBufferGetSize(_wgpu_get_dawn<WGPUBuffer>(buffer));

  if (wgpu_is_render_pass_encoder(renderCommandsMixin)) {
    WGPURenderPassEncoder _encoder =  _wgpu_get_dawn<WGPURenderPassEncoder>(renderCommandsMixin);
    wgpuRenderPassEncoderSetVertexBuffer(_encoder, (uint32_t)slot, _wgpu_get_dawn<WGPUBuffer>(buffer), (uint64_t)offset, (uint64_t)(size - offset));
  } else if (wgpu_is_render_bundle_encoder(renderCommandsMixin)) {
    WGPURenderBundleEncoder _encoder = _wgpu_get_dawn<WGPURenderBundleEncoder>(renderCommandsMixin);
    wgpuRenderBundleEncoderSetVertexBuffer(_encoder, (uint32_t) slot, _wgpu_get_dawn<WGPUBuffer>(buffer), (uint64_t) offset, (uint64_t)(size - offset));
  }
}

void wgpu_render_commands_mixin_draw(WGpuRenderCommandsMixin renderCommandsMixin, uint32_t vertexCount, uint32_t instanceCount, uint32_t firstVertex, uint32_t firstInstance) {
  assert(wgpu_is_render_commands_mixin(renderCommandsMixin));

  if (wgpu_is_render_pass_encoder(renderCommandsMixin)) {
    WGPURenderPassEncoder _encoder =  _wgpu_get_dawn<WGPURenderPassEncoder>(renderCommandsMixin);
    wgpuRenderPassEncoderDraw(_encoder, vertexCount, instanceCount, firstVertex, firstInstance);
  } else if (wgpu_is_render_bundle_encoder(renderCommandsMixin)) {
    WGPURenderBundleEncoder _encoder = _wgpu_get_dawn<WGPURenderBundleEncoder>(renderCommandsMixin);
    wgpuRenderBundleEncoderDraw(_encoder, vertexCount, instanceCount, firstVertex, firstInstance);
  }
}

void wgpu_render_commands_mixin_draw_indexed(WGpuRenderCommandsMixin renderCommandsMixin, uint32_t indexCount, uint32_t instanceCount,
    uint32_t firstVertex, int32_t baseVertex, uint32_t firstInstance) {
  assert(wgpu_is_render_commands_mixin(renderCommandsMixin));

  if (wgpu_is_render_pass_encoder(renderCommandsMixin)) {
    WGPURenderPassEncoder _encoder =  _wgpu_get_dawn<WGPURenderPassEncoder>(renderCommandsMixin);
    wgpuRenderPassEncoderDrawIndexed(_encoder, indexCount, instanceCount, firstVertex, baseVertex, firstInstance);
  } else if (wgpu_is_render_bundle_encoder(renderCommandsMixin)) {
    WGPURenderBundleEncoder _encoder = _wgpu_get_dawn<WGPURenderBundleEncoder>(renderCommandsMixin);
    wgpuRenderBundleEncoderDrawIndexed(_encoder, indexCount, instanceCount, firstVertex, baseVertex, firstInstance);
  }
}

void wgpu_render_commands_mixin_draw_indirect(WGpuRenderCommandsMixin renderCommandsMixin, WGpuBuffer indirectBuffer, double_int53_t indirectOffset) {
  assert(wgpu_is_render_commands_mixin(renderCommandsMixin));

  if (wgpu_is_render_pass_encoder(renderCommandsMixin)) {
    WGPURenderPassEncoder _encoder =  _wgpu_get_dawn<WGPURenderPassEncoder>(renderCommandsMixin);
    wgpuRenderPassEncoderDrawIndirect(_encoder, _wgpu_get_dawn<WGPUBuffer>(indirectBuffer), (uint64_t) indirectOffset);
  } else if (wgpu_is_render_bundle_encoder(renderCommandsMixin)) {
    WGPURenderBundleEncoder _encoder = _wgpu_get_dawn<WGPURenderBundleEncoder>(renderCommandsMixin);
    wgpuRenderBundleEncoderDrawIndirect(_encoder, _wgpu_get_dawn<WGPUBuffer>(indirectBuffer), (uint64_t) indirectOffset);
  }
}

void wgpu_render_commands_mixin_draw_indexed_indirect(WGpuRenderCommandsMixin renderCommandsMixin, WGpuBuffer indirectBuffer, double_int53_t indirectOffset) {
  assert(wgpu_is_render_commands_mixin(renderCommandsMixin));

  if (wgpu_is_render_pass_encoder(renderCommandsMixin)) {
    WGPURenderPassEncoder _encoder = _wgpu_get_dawn<WGPURenderPassEncoder>(renderCommandsMixin);
    wgpuRenderPassEncoderDrawIndexedIndirect(_encoder, _wgpu_get_dawn<WGPUBuffer>(indirectBuffer), (uint64_t)indirectOffset);
  } else if (wgpu_is_render_bundle_encoder(renderCommandsMixin)) {
    WGPURenderBundleEncoder _encoder = _wgpu_get_dawn<WGPURenderBundleEncoder>(renderCommandsMixin);
    wgpuRenderBundleEncoderDrawIndexedIndirect(_encoder, _wgpu_get_dawn<WGPUBuffer>(indirectBuffer), (uint64_t)indirectOffset);
  }
}

void wgpu_render_pass_encoder_set_viewport(WGpuRenderPassEncoder encoder, float x, float y, float width, float height, float minDepth, float maxDepth) {
  assert(wgpu_is_render_pass_encoder(encoder));
  wgpuRenderPassEncoderSetViewport(_wgpu_get_dawn<WGPURenderPassEncoder>(encoder), x, y, width, height, minDepth, maxDepth);
}

void wgpu_render_pass_encoder_set_scissor_rect(WGpuRenderPassEncoder encoder, uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
  assert(wgpu_is_render_pass_encoder(encoder));
  WGPURenderPassEncoder _encoder = _wgpu_get_dawn<WGPURenderPassEncoder>(encoder);
  wgpuRenderPassEncoderSetScissorRect(_encoder, x, y, width, height);
}

void wgpu_render_pass_encoder_set_blend_constant(WGpuRenderPassEncoder encoder, double r, double g, double b, double a) {
  assert(wgpu_is_render_pass_encoder(encoder));
  WGPURenderPassEncoder _encoder = _wgpu_get_dawn<WGPURenderPassEncoder>(encoder);
  WGPUColor color{r, g, b, a};
  wgpuRenderPassEncoderSetBlendConstant(_encoder, &color);
}

void wgpu_render_pass_encoder_set_stencil_reference(WGpuRenderPassEncoder encoder, uint32_t stencilValue) {
  assert(wgpu_is_render_pass_encoder(encoder));
  WGPURenderPassEncoder _encoder = _wgpu_get_dawn<WGPURenderPassEncoder>(encoder);
  wgpuRenderPassEncoderSetStencilReference(_encoder, stencilValue);
}

void wgpu_render_pass_encoder_begin_occlusion_query(WGpuRenderPassEncoder encoder, int32_t queryIndex) {
  assert(wgpu_is_render_pass_encoder(encoder));
  WGPURenderPassEncoder _encoder = _wgpu_get_dawn<WGPURenderPassEncoder>(encoder);
  wgpuRenderPassEncoderBeginOcclusionQuery(_encoder, (uint32_t) queryIndex);
}

void wgpu_render_pass_encoder_end_occlusion_query(WGpuRenderPassEncoder encoder) {
  assert(wgpu_is_render_pass_encoder(encoder));
  WGPURenderPassEncoder _encoder = _wgpu_get_dawn<WGPURenderPassEncoder>(encoder);
  wgpuRenderPassEncoderEndOcclusionQuery(_encoder);
}

void wgpu_render_pass_encoder_execute_bundles(WGpuRenderPassEncoder encoder, const WGpuRenderBundle *bundles, int numBundles) {
  assert(wgpu_is_render_pass_encoder(encoder));
  WGPURenderPassEncoder _encoder = _wgpu_get_dawn<WGPURenderPassEncoder>(encoder);
  std::vector<WGPURenderBundle> _bundles(numBundles);
  for (int i = 0; i < numBundles; ++i)
    _bundles[i] = _wgpu_get_dawn<WGPURenderBundle>(bundles[i]);
  wgpuRenderPassEncoderExecuteBundles(_encoder, numBundles, _bundles.data());
}

WGPU_BOOL wgpu_is_render_bundle(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj != nullptr && obj->type == kWebGPURenderBundle;
}

WGPU_BOOL wgpu_is_queue(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUQueue;
}

void wgpu_queue_submit_one_and_destroy(WGpuQueue queue, WGpuCommandBuffer commandBuffer) {
  assert(wgpu_is_queue(queue));
  assert(wgpu_is_command_buffer(commandBuffer));
  WGPUQueue _queue = _wgpu_get_dawn<WGPUQueue>(queue);
  WGPUCommandBuffer _commandBuffer = _wgpu_get_dawn<WGPUCommandBuffer>(commandBuffer);
  wgpuQueueSubmit(_queue, 1, &_commandBuffer);
  wgpu_object_destroy(commandBuffer);
}

void wgpu_queue_submit_multiple_and_destroy(WGpuQueue queue, const WGpuCommandBuffer *commandBuffers, int numCommandBuffers) {
  assert(wgpu_is_queue(queue));

  WGPUQueue _queue = _wgpu_get_dawn<WGPUQueue>(queue);

  if (commandBuffers == nullptr || numCommandBuffers == 0) {
    wgpuQueueSubmit(_queue, 0, nullptr);
    return;
  }
  
  std::vector<WGPUCommandBuffer> _commandBuffer(numCommandBuffers);
  for (int i = 0; i < numCommandBuffers; ++i)
    _commandBuffer[i] = _wgpu_get_dawn<WGPUCommandBuffer>(commandBuffers[i]);

  wgpuQueueSubmit(_queue, (uint32_t)numCommandBuffers, _commandBuffer.data());
  for (int i = 0; i < numCommandBuffers; ++i) 
    wgpu_object_destroy(commandBuffers[i]);
}

void wgpu_queue_set_on_submitted_work_done_callback(WGpuQueue queue, WGpuOnSubmittedWorkDoneCallback callback, void* userData) {
  assert(wgpu_is_queue(queue));

  struct _Data {
    WGpuQueue queue;
    WGpuOnSubmittedWorkDoneCallback callback;
    void* userData;
  };
  _Data* data = new _Data{queue, callback, userData};
  wgpuQueueOnSubmittedWorkDone(_wgpu_get_dawn<WGPUQueue>(queue), [](WGPUQueueWorkDoneStatus status, void* userdata) {
    _Data* data = (_Data*)userdata;
    if (data->callback)
      data->callback(data->queue, data->userData);
    delete data;
  }, data);
}

void wgpu_queue_write_buffer(WGpuQueue queue, WGpuBuffer buffer, double_int53_t bufferOffset, const void* data, double_int53_t size) {
  assert(wgpu_is_queue(queue));
  assert(wgpu_is_buffer(buffer));
  WGPUQueue _queue = _wgpu_get_dawn<WGPUQueue>(queue);
  WGPUBuffer _buffer = _wgpu_get_dawn<WGPUBuffer>(buffer);
  wgpuQueueWriteBuffer(_queue, _buffer, (uint64_t)bufferOffset, data, (size_t)size);
}

void wgpu_queue_write_texture(WGpuQueue queue, const WGpuImageCopyTexture *destination, const void *data, uint32_t bytesPerBlockRow,
    uint32_t blockRowsPerImage, uint32_t writeWidth, uint32_t writeHeight, uint32_t writeDepthOrArrayLayers) {
  assert(wgpu_is_queue(queue));
  assert(destination != nullptr);
  assert(data != nullptr);

  WGPUQueue _queue = _wgpu_get_dawn<WGPUQueue>(queue);
  WGPUImageCopyTexture _destination = {};
  wgpuReadGpuImageCopyTexture(destination, _destination);

  WGPUTextureDataLayout dataLayout{ nullptr, 0, bytesPerBlockRow, blockRowsPerImage};
  WGPUExtent3D extents {writeWidth, writeHeight, writeDepthOrArrayLayers};

  wgpuQueueWriteTexture(_queue, &_destination, data, bytesPerBlockRow * blockRowsPerImage, &dataLayout, &extents);
}

void wgpu_queue_copy_external_image_to_texture(WGpuQueue queue, const WGpuImageCopyExternalImage *source, const WGpuImageCopyTextureTagged *destination,
    uint32_t copyWidth, uint32_t copyHeight, uint32_t copyDepthOrArrayLayers) {
  assert(wgpu_is_queue(queue));
  assert(source != nullptr);
  assert(destination != nullptr);
  assert(false); /* TODO */
}

WGPU_BOOL wgpu_is_query_set(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUQuerySet;
}

WGPU_QUERY_TYPE wgpu_query_set_type(WGpuQuerySet querySet) {
  assert(wgpu_is_query_set(querySet));

  WGPUQuerySet _querySet = _wgpu_get_dawn<WGPUQuerySet>(querySet);
  return dawn_to_wgpu_query_type(wgpuQuerySetGetType(_querySet));
}

uint32_t wgpu_query_set_count(WGpuQuerySet querySet) {
  assert(wgpu_is_query_set(querySet));

  WGPUQuerySet _querySet = _wgpu_get_dawn<WGPUQuerySet>(querySet);
  return wgpuQuerySetGetCount(_querySet);
}

WGPU_BOOL wgpu_is_canvas_context(WGpuObjectBase object) {
  _WGpuObject* obj = _wgpu_get(object);
  return obj && obj->type == kWebGPUCanvasContext;
}

void wgpu_canvas_context_configure(WGpuCanvasContext canvasContext, const WGpuCanvasConfiguration *config, int width, int height) {
  assert(wgpu_is_canvas_context(canvasContext));
  assert(config != nullptr);
  assert(wgpu_is_device(config->device));
  _WGpuCanvasContext* context = _wgpu_get_dawn<_WGpuCanvasContext*>(canvasContext);
  if (!context->surface)
    return;

  std::vector<WGPUTextureFormat> viewFormats(config->numViewFormats);
  for (int i = 0; i < config->numViewFormats; i++)
  {
    viewFormats[i] = wgpu_texture_format_to_dawn(config->viewFormats[i]);
  }

  WGPUSurfaceConfiguration configuration = WGPU_SURFACE_CONFIGURATION_INIT;
  configuration.device = _wgpu_get_dawn<WGPUDevice>(config->device); 
  configuration.format = wgpu_texture_format_to_dawn(config->format);
  configuration.usage = wgpu_texture_usage_flags_to_dawn(config->usage);
  configuration.viewFormatCount = config->numViewFormats;
  configuration.viewFormats = viewFormats.data();
  configuration.alphaMode = wgpu_canvas_alpha_mode_to_dawn(config->alphaMode);
  configuration.width = width;
  configuration.height = height;
  configuration.presentMode = WGPUPresentMode_Fifo;

  wgpuSurfaceConfigure(context->surface, &configuration);
}

void wgpu_canvas_context_unconfigure(WGpuCanvasContext canvasContext) {
  assert(wgpu_is_canvas_context(canvasContext));
  _WGpuCanvasContext* context = _wgpu_get_dawn<_WGpuCanvasContext*>(canvasContext);

  if (context->surface) {
    wgpuSurfaceUnconfigure(context->surface);
  }
}

WGpuTexture wgpu_canvas_context_get_current_texture(WGpuCanvasContext canvasContext) {
  assert(wgpu_is_canvas_context(canvasContext));
  // Dawn swapchain only returns a TextureView, gotten from wgpu_canvas_context_get_current_texture_view
  return 0;
}

WGpuTextureView wgpu_canvas_context_get_current_texture_view(WGpuCanvasContext canvasContext) {
  assert(wgpu_is_canvas_context(canvasContext));
  _WGpuCanvasContext* context = _wgpu_get_dawn<_WGpuCanvasContext*>(canvasContext);
  if (!context->surface)
    return 0;

  WGPUSurfaceTexture surfaceTexture = WGPU_SURFACE_TEXTURE_INIT;
  wgpuSurfaceGetCurrentTexture(context->surface, &surfaceTexture);

  if (surfaceTexture.status == WGPUSurfaceGetCurrentTextureStatus_Success) {
    WGPUTextureView textureView = wgpuTextureCreateView(surfaceTexture.texture, nullptr);
    return _wgpu_store(kWebGPUTextureView, textureView);
  } else {
    return 0;
  }
}

void wgpu_canvas_context_present(WGpuCanvasContext canvasContext) {
  assert(wgpu_is_canvas_context(canvasContext));
  _WGpuCanvasContext* context = _wgpu_get_dawn<_WGpuCanvasContext*>(canvasContext);
  if (!context->surface)
    return;

  wgpuSurfacePresent(context->surface);
}

void wgpu_device_set_lost_callback(WGpuDevice device, WGpuDeviceLostCallback callback, void* userData) {
  assert(wgpu_is_device(device));
  assert(callback);
  WGPUDevice _device = _wgpu_get_dawn<WGPUDevice>(device);
  if (callback == nullptr) {
    wgpuDeviceSetDeviceLostCallback(_device, nullptr, nullptr);
    return;
  }
  struct _Data {
    WGpuDevice device;
    WGpuDeviceLostCallback callback;
    void* userData;
  };
  _Data* data = new _Data{device, callback, userData};
  wgpuDeviceSetDeviceLostCallback(_device, [](WGPUDeviceLostReason reason, WGPUStringView message, void* userdata) {
    _Data* data = (_Data*)userdata;
    data->callback(data->device, dawn_to_wgpu_device_lost_reason(reason), message.data, data->userData);
    delete data;
  }, data);
}

void wgpu_device_push_error_scope(WGpuDevice device, WGPU_ERROR_FILTER filter) {
  assert(wgpu_is_device(device));
  WGPUDevice _device = _wgpu_get_dawn<WGPUDevice>(device);
  wgpuDevicePushErrorScope(_device, wgpu_error_filter_to_dawn(filter));
}

void wgpu_device_pop_error_scope_async(WGpuDevice device, WGpuDeviceErrorCallback callback, void *userData) {
  assert(wgpu_is_device(device));
  assert(callback);
  WGPUDevice _device = _wgpu_get_dawn<WGPUDevice>(device);
  struct _Data {
    WGpuDevice device;
    WGpuDeviceErrorCallback callback;
    void* userData;
  };
  _Data* data = new _Data{device, callback, userData};
  wgpuDevicePopErrorScope(_device, [](WGPUErrorType type, WGPUStringView message, void* userdata) {
    _Data* data = (_Data*)userdata;
    if (message.data && message.data[0] != 0)
      data->callback(data->device, type, message.data, data->userData);
    delete data;
  }, data);
}

void wgpu_device_set_uncapturederror_callback(WGpuDevice device, WGpuDeviceErrorCallback callback, void *userData) {
  assert(wgpu_is_device(device));
  assert(callback);
  WGPUDevice _device = _wgpu_get_dawn<WGPUDevice>(device);
  struct _Data {
    WGpuDevice device;
    WGpuDeviceErrorCallback callback;
    void* userData;
  };
  _Data* data = new _Data{device, callback, userData};
  wgpuDeviceSetUncapturedErrorCallback(_device, [](WGPUErrorType type, WGPUStringView message, void* userdata) {
    _Data* data = (_Data*)userdata;
    if (message.data && message.data[0] != 0)
      data->callback(data->device, type, message.data, data->userData);
  }, data);
}

void wgpu_load_image_bitmap_from_url_async(const char *url, WGPU_BOOL flipY, WGpuLoadImageBitmapCallback callback, void *userData) {
  assert(false); /* TODO */
}

} // extern "C"
#endif // !__EMSCRIPTEN__
