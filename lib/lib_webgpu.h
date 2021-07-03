#pragma once

#include <emscripten/html5.h>
#include <stdint.h>

#include "lib_webgpu_fwd.h"

// Some WebGPU JS API functions have default parameters so that the user can omit passing them.
// These defaults are carried through to these headers. However C does not support default parameters to
// functions, so enable the default parameters only when called from C++ code.
#ifdef __cplusplus
#define _WGPU_DEFAULT_VALUE(x) = x
#else
#define _WGPU_DEFAULT_VALUE(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Use double to represent a JavaScript number that can
// address 2^53 == 9007199254740992 = ~9.0 petabytes.
typedef double double_int53_t;

typedef double_int53_t WGpuBufferMappedRangeStartOffset;

const char *wgpu_enum_to_string(int enumValue);

// Returns the number of WebGPU objects referenced by the WebGPU JS library.
uint32_t wgpu_get_num_live_objects(void);

// Calls .destroy() on the given WebGPU object (if it has such a member function) and releases the JS side reference to it. Use this function
// to release memory for all types of WebGPU objects after you are done with them.
// Note that deleting a GPUTexture will also delete all GPUTextureViews that have been created from it.
// Similar to free(), calling wgpu_object_destroy() on null, or an object that has already been destroyed before is safe, and no-op. (so no need to
// do excess "if (wgpuObject) wgpu_object_destroy(wgpuObject);")
void wgpu_object_destroy(WGpuObjectBase wgpuObject);

// Acquires a canvas context from a canvas by calling canvas.getCanvasContext().
WGpuCanvasContext wgpu_canvas_get_canvas_context(const char *canvasSelector);

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The ordering and structure of this remainder of this file follows the official WebGPU WebIDL definitions at https://www.w3.org/TR/webgpu/#idl-index
// This is so that when the official IDL is modified, the modifications can be easily diffed here for updates.

/*
interface mixin GPUObjectBase {
    attribute USVString? label;
};
*/
typedef int WGpuObjectBase;
// Returns true if the given handle references a valid WebGPU object
EM_BOOL wgpu_is_valid_object(WGpuObjectBase obj);
void wgpu_object_set_label(WGpuObjectBase obj, const char *label);
int wgpu_object_get_label(WGpuObjectBase obj, char *dstLabel, uint32_t dstLabelSize);

/*
dictionary GPUObjectDescriptorBase {
    USVString label;
};
*/
#define WGPU_OBJECT_LABEL_MAX_LENGTH 256
typedef struct WGpuObjectDescriptorBase // TODO: Currently unused. Actually use this, or remove
{
  char label[WGPU_OBJECT_LABEL_MAX_LENGTH];
} WGpuObjectDescriptorBase;

/*
[Exposed=Window]
interface GPUSupportedLimits {
    readonly attribute unsigned long maxTextureDimension1D;
    readonly attribute unsigned long maxTextureDimension2D;
    readonly attribute unsigned long maxTextureDimension3D;
    readonly attribute unsigned long maxTextureArrayLayers;
    readonly attribute unsigned long maxBindGroups;
    readonly attribute unsigned long maxDynamicUniformBuffersPerPipelineLayout;
    readonly attribute unsigned long maxDynamicStorageBuffersPerPipelineLayout;
    readonly attribute unsigned long maxSampledTexturesPerShaderStage;
    readonly attribute unsigned long maxSamplersPerShaderStage;
    readonly attribute unsigned long maxStorageBuffersPerShaderStage;
    readonly attribute unsigned long maxStorageTexturesPerShaderStage;
    readonly attribute unsigned long maxUniformBuffersPerShaderStage;
    readonly attribute unsigned long maxUniformBufferBindingSize;
    readonly attribute unsigned long maxStorageBufferBindingSize;
    readonly attribute unsigned long maxVertexBuffers;
    readonly attribute unsigned long maxVertexAttributes;
    readonly attribute unsigned long maxVertexBufferArrayStride;
};
*/
typedef struct WGpuSupportedLimits
{
  uint32_t maxTextureDimension1D; // required >= 8192
  uint32_t maxTextureDimension2D; // required >= 8192
  uint32_t maxTextureDimension3D; // required >= 2048
  uint32_t maxTextureArrayLayers; // required >= 2048
  uint32_t maxBindGroups; // required >= 4
  uint32_t maxDynamicUniformBuffersPerPipelineLayout; // required >= 8
  uint32_t maxDynamicStorageBuffersPerPipelineLayout; // required >= 4
  uint32_t maxSampledTexturesPerShaderStage; // required >= 16
  uint32_t maxSamplersPerShaderStage; // required >= 16
  uint32_t maxStorageBuffersPerShaderStage; // required >= 8
  uint32_t maxStorageTexturesPerShaderStage; // required >= 8
  uint32_t maxUniformBuffersPerShaderStage; // required >= 12
  uint32_t maxUniformBufferBindingSize; // required >= 16384
  uint32_t maxStorageBufferBindingSize; // required >= 128*1024*1024 (128MB)
  uint32_t maxVertexBuffers; // required >= 8
  uint32_t maxVertexAttributes; // required >= 16
  uint32_t maxVertexBufferArrayStride; // required >= 2048
} WGpuSupportedLimits;

/*
[Exposed=Window]
interface GPUSupportedFeatures {
    readonly setlike<DOMString>;
};
*/
typedef int WGPU_FEATURES_BITFIELD;
#define WGPU_FEATURE_DEPTH_CLAMPING            0x01
#define WGPU_FEATURE_DEPTH24UNORM_STENCIL8     0x02
#define WGPU_FEATURE_DEPTH32FLOAT_STENCIL8     0x04
#define WGPU_FEATURE_PIPELINE_STATISTICS_QUERY 0x08
#define WGPU_FEATURE_TEXTURE_COMPRESSION_BC    0x10
#define WGPU_FEATURE_TIMESTAMP_QUERY           0x20

/*
enum GPUPredefinedColorSpace {
    "srgb",
};
*/
typedef int WGPU_PREDEFINED_COLOR_SPACE;
#define WGPU_PREDEFINED_COLOR_SPACE_INVALID 0
#define WGPU_PREDEFINED_COLOR_SPACE_SRGB 1

/*
interface mixin NavigatorGPU {
    [SameObject] readonly attribute GPU gpu;
};
Navigator includes NavigatorGPU;
WorkerNavigator includes NavigatorGPU;

[Exposed=(Window, DedicatedWorker)]
interface GPU {
    Promise<GPUAdapter?> requestAdapter(optional GPURequestAdapterOptions options = {});
};
*/
typedef void (*WGpuRequestAdapterCallback)(WGpuAdapter adapter, void *userData);
// Requests an adapter from the user agent. The user agent chooses whether to return an adapter, and, if so, chooses according to the provided options.
// If WebGPU is not supported by the browser, returns 0. Otherwise returns an ID for a WebGPU adapter.
WGpuAdapter navigator_gpu_request_adapter_async(const WGpuRequestAdapterOptions *options, WGpuRequestAdapterCallback adapterCallback, void *userData);

/*
dictionary GPURequestAdapterOptions {
    GPUPowerPreference powerPreference;
    boolean forceSoftware = false;
};
*/
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
  EM_BOOL forceSoftware;
} WGpuRequestAdapterOptions;
extern const WGpuRequestAdapterOptions WGPU_REQUEST_ADAPTER_OPTIONS_DEFAULT_INITIALIZER;

/*
enum GPUPowerPreference {
    "low-power",
    "high-performance"
};
*/
typedef int WGPU_POWER_PREFERENCE;
#define WGPU_POWER_PREFERENCE_INVALID 0
#define WGPU_POWER_PREFERENCE_LOW_POWER 2
#define WGPU_POWER_PREFERENCE_HIGH_PERFORMANCE 3

/*
[Exposed=Window]
interface GPUAdapter {
    readonly attribute DOMString name;
    [SameObject] readonly attribute GPUSupportedFeatures features;
    [SameObject] readonly attribute WGpuSupportedLimits limits;
    readonly attribute boolean isSoftware;

    Promise<GPUDevice> requestDevice(optional GPUDeviceDescriptor descriptor = {});
};
*/
typedef int WGpuAdapter;
// Returns true if the given handle references a valid GPUAdapter.
EM_BOOL wgpu_is_adapter(WGpuObjectBase object);

// Writes the name of the adapter to the provided string pointer. If the length of the adapter name would not fit in dstNameSize, then it will be truncated.
// Returns the number of bytes written. (if return value == dstNameSize, truncation likely occurred)
int wgpu_adapter_get_name(WGpuAdapter adapter, char *dstName, int dstNameSize);

// Returns a bitfield of all the supported features on this adapter.
WGPU_FEATURES_BITFIELD wgpu_adapter_or_device_get_features(WGpuAdapter adapter);
#define wgpu_adapter_get_features wgpu_adapter_or_device_get_features

// Returns true if the given feature is supported by this adapter.
EM_BOOL wgpu_adapter_or_device_supports_feature(WGpuAdapter adapter, WGPU_FEATURES_BITFIELD feature);
#define wgpu_adapter_supports_feature wgpu_adapter_or_device_supports_feature

// Populates the adapter.limits field of the given adapter to the provided structure.
void wgpu_adapter_or_device_get_limits(WGpuAdapter adapter, WGpuSupportedLimits *limits);
#define wgpu_adapter_get_limits wgpu_adapter_or_device_get_limits

EM_BOOL wgpu_adapter_is_software(WGpuAdapter adapter);

typedef void (*WGpuRequestDeviceCallback)(WGpuDevice device, void *userData);

void wgpu_adapter_request_device_async(WGpuAdapter adapter, const WGpuDeviceDescriptor *descriptor, WGpuRequestDeviceCallback deviceCallback, void *userData);

/*
dictionary GPUDeviceDescriptor : GPUObjectDescriptorBase {
    sequence<GPUFeatureName> requiredFeatures = [];
    record<DOMString, GPUSize32> requiredLimits = {};
};
*/
typedef struct WGpuDeviceDescriptor
{
  WGPU_FEATURES_BITFIELD requiredFeatures;
  WGpuSupportedLimits requiredLimits;
} WGpuDeviceDescriptor;
extern const WGpuDeviceDescriptor WGPU_DEVICE_DESCRIPTOR_DEFAULT_INITIALIZER;

/*
enum GPUFeatureName {
    "depth-clamping",
    "depth24unorm-stencil8",
    "depth32float-stencil8",
    "pipeline-statistics-query",
    "texture-compression-bc",
    "timestamp-query",
};
*/
typedef int WGPU_FEATURE_NAME;
#define WGPU_FEATURE_NAME_INVALID 0
#define WGPU_FEATURE_NAME_DEPTH_CLAMPING 4
#define WGPU_FEATURE_NAME_DEPTH24UNORM_STENCIL8 5
#define WGPU_FEATURE_NAME_DEPTH32FLOAT_STENCIL8 6
#define WGPU_FEATURE_NAME_PIPELINE_STATISTICS_QUERY 7
#define WGPU_FEATURE_NAME_TEXTURE_COMPRESSION_BC 8
#define WGPU_FEATURE_NAME_TIMESTAMP_QUERY 9

/*
[Exposed=(Window, DedicatedWorker), Serializable]
interface GPUDevice : EventTarget {
    [SameObject] readonly attribute GPUSupportedFeatures features;
    [SameObject] readonly attribute GPUSupportedLimits limits;

    [SameObject] readonly attribute GPUQueue queue;

    undefined destroy();

    GPUBuffer createBuffer(GPUBufferDescriptor descriptor);
    GPUTexture createTexture(GPUTextureDescriptor descriptor);
    GPUSampler createSampler(optional GPUSamplerDescriptor descriptor = {});
    GPUExternalTexture importExternalTexture(GPUExternalTextureDescriptor descriptor);

    GPUBindGroupLayout createBindGroupLayout(GPUBindGroupLayoutDescriptor descriptor);
    GPUPipelineLayout createPipelineLayout(GPUPipelineLayoutDescriptor descriptor);
    GPUBindGroup createBindGroup(GPUBindGroupDescriptor descriptor);

    GPUShaderModule createShaderModule(GPUShaderModuleDescriptor descriptor);
    GPUComputePipeline createComputePipeline(GPUComputePipelineDescriptor descriptor);
    GPURenderPipeline createRenderPipeline(GPURenderPipelineDescriptor descriptor);
    Promise<GPUComputePipeline> createComputePipelineAsync(GPUComputePipelineDescriptor descriptor);
    Promise<GPURenderPipeline> createRenderPipelineAsync(GPURenderPipelineDescriptor descriptor);

    GPUCommandEncoder createCommandEncoder(optional GPUCommandEncoderDescriptor descriptor = {});
    GPURenderBundleEncoder createRenderBundleEncoder(GPURenderBundleEncoderDescriptor descriptor);

    GPUQuerySet createQuerySet(GPUQuerySetDescriptor descriptor);
};
GPUDevice includes GPUObjectBase;
*/
typedef int WGpuDevice;
// Returns true if the given handle references a valid GPUDevice.
EM_BOOL wgpu_is_device(WGpuObjectBase object);

#define wgpu_device_get_features wgpu_adapter_or_device_get_features
#define wgpu_device_supports_feature wgpu_adapter_or_device_supports_feature
#define wgpu_device_get_limits wgpu_adapter_or_device_get_limits

WGpuAdapter wgpu_device_get_adapter(WGpuDevice device);

WGpuQueue wgpu_device_get_queue(WGpuDevice device);

WGpuBuffer wgpu_device_create_buffer(WGpuDevice device, const WGpuBufferDescriptor *bufferDesc);
WGpuTexture wgpu_device_create_texture(WGpuDevice device, const WGpuTextureDescriptor *textureDesc);
WGpuSampler wgpu_device_create_sampler(WGpuDevice device, const WGpuSamplerDescriptor *samplerDesc);
WGpuExternalTexture wgpu_device_import_external_texture(WGpuDevice device, const WGpuExternalTextureDescriptor *externalTextureDesc); // TODO implement

// N.b. not currently using signature WGpuBindGroupLayout wgpu_device_create_bind_group_layout(WGpuDevice device, const WGpuBindGroupLayoutDescriptor *bindGroupLayoutDesc);
// since WGpuBindGroupLayoutDescriptor is a single element struct consisting only of a single array. (if it is expanded in the future, switch to using that signature)
WGpuBindGroupLayout wgpu_device_create_bind_group_layout(WGpuDevice device, const WGpuBindGroupLayoutEntry *bindGroupLayoutEntries, int numEntries);

// N.b. not currently using signature WGpuPipelineLayout wgpu_device_create_pipeline_layout(WGpuDevice device, const WGpuPipelineLayoutDescriptor *pipelineLayoutDesc);
// since WGpuPipelineLayoutDescriptor is a single element struct consisting only of a single array. (if it is expanded in the future, switch to using that signature)
WGpuPipelineLayout wgpu_device_create_pipeline_layout(WGpuDevice device, const WGpuBindGroupLayout *bindGroupLayouts, int numLayouts);

// N.b. not currently using signature WGpuBindGroup wgpu_device_create_bind_group(WGpuDevice device, const WGpuBindGroupDescriptor *bindGroupDesc);
// since WGpuBindGroupDescriptor is a such a light struct. (if it is expanded in the future, switch to using that signature)
WGpuBindGroup wgpu_device_create_bind_group(WGpuDevice device, WGpuBindGroupLayout bindGroupLayout, const WGpuBindGroupEntry *entries, int numEntries);

WGpuShaderModule wgpu_device_create_shader_module(WGpuDevice device, const WGpuShaderModuleDescriptor *shaderModuleDesc);

// N.b. not currently using signature WGpuComputePipeline wgpu_device_create_compute_pipeline(WGpuDevice device, const WGpuComputePipelineDescriptor *computePipelineDesc);
// since WGpuComputePipelineDescriptor is a such a light struct. (if it is expanded in the future, switch to using that signature)
WGpuComputePipeline wgpu_device_create_compute_pipeline(WGpuDevice device, const WGpuShaderModule computeModule, const char *entryPoint);

WGpuRenderPipeline wgpu_device_create_render_pipeline(WGpuDevice device, const WGpuRenderPipelineDescriptor *renderPipelineDesc);

typedef void (*WGpuCreatePipelineCallback)(WGpuDevice device, WGpuPipelineBase pipeline, void *userData);
void wgpu_device_create_compute_pipeline_async(WGpuDevice device, const WGpuComputePipelineDescriptor *computePipelineDesc, WGpuCreatePipelineCallback callback, void *userData); // TODO implement
void wgpu_device_create_render_pipeline_async(WGpuDevice device, const WGpuRenderPipelineDescriptor *renderPipelineDesc, WGpuCreatePipelineCallback callback, void *userData);

WGpuCommandEncoder wgpu_device_create_command_encoder(WGpuDevice device, const WGpuCommandEncoderDescriptor *commandEncoderDesc);
WGpuRenderBundleEncoder wgpu_device_create_render_bundle_encoder(WGpuDevice device, const WGpuRenderBundleEncoderDescriptor *renderBundleEncoderDesc);

WGpuQuerySet wgpu_device_create_query_set(WGpuDevice device, const WGpuQuerySetDescriptor *querySetDesc);

/*
[Exposed=Window, Serializable]
interface GPUBuffer {
    Promise<undefined> mapAsync(GPUMapModeFlags mode, optional GPUSize64 offset = 0, optional GPUSize64 size);
    ArrayBuffer getMappedRange(optional GPUSize64 offset = 0, optional GPUSize64 size);
    undefined unmap();

    undefined destroy();
};
GPUBuffer includes GPUObjectBase;
*/
typedef int WGpuBuffer;
// Returns true if the given handle references a valid GPUBuffer.
EM_BOOL wgpu_is_buffer(WGpuObjectBase object);

// TODO: Add error status to map callback for when mapAsync() promise rejects.
typedef void (*WGpuBufferMapCallback)(WGpuBuffer buffer, void *userData, WGPU_MAP_MODE_FLAGS mode, double_int53_t offset, double_int53_t size);
#define WGPU_MAP_MAX_LENGTH -1
void wgpu_buffer_map_async(WGpuBuffer buffer, WGpuBufferMapCallback callback, void *userData, WGPU_MAP_MODE_FLAGS mode, double_int53_t offset _WGPU_DEFAULT_VALUE(0), double_int53_t size _WGPU_DEFAULT_VALUE(WGPU_MAP_MAX_LENGTH));

#define WGPU_BUFFER_GET_MAPPED_RANGE_FAILED ((WGpuBufferMappedRangeStartOffset)-1)

// Calls buffer.getMappedRange(). Returns `startOffset`, which is used as an ID token to wgpu_buffer_read/write_mapped_range().
// If .getMappedRange() fails, the value WGPU_BUFFER_GET_MAPPED_RANGE_FAILED (-1) will be returned.
WGpuBufferMappedRangeStartOffset wgpu_buffer_get_mapped_range(WGpuBuffer buffer, double_int53_t startOffset, double_int53_t size _WGPU_DEFAULT_VALUE(WGPU_MAP_MAX_LENGTH));
void wgpu_buffer_read_mapped_range(WGpuBuffer buffer, WGpuBufferMappedRangeStartOffset startOffset, double_int53_t subOffset, void *dst, double_int53_t size);
void wgpu_buffer_write_mapped_range(WGpuBuffer buffer, WGpuBufferMappedRangeStartOffset startOffset, double_int53_t subOffset, const void *src, double_int53_t size);
void wgpu_buffer_unmap(WGpuBuffer buffer);

/*
dictionary GPUBufferDescriptor : GPUObjectDescriptorBase {
    required GPUSize64 size;
    required GPUBufferUsageFlags usage;
    boolean mappedAtCreation = false;
};
*/
typedef struct WGpuBufferDescriptor
{
  uint64_t size;
  WGPU_BUFFER_USAGE_FLAGS usage;
  EM_BOOL mappedAtCreation; // Note: it is valid to set mappedAtCreation to true without MAP_READ or MAP_WRITE in usage. This can be used to set the buffer’s initial data.
} WGpuBufferDescriptor;

/*
typedef [EnforceRange] unsigned long GPUBufferUsageFlags;
[Exposed=Window]
interface GPUBufferUsage {
    const GPUFlagsConstant MAP_READ      = 0x0001;
    const GPUFlagsConstant MAP_WRITE     = 0x0002;
    const GPUFlagsConstant COPY_SRC      = 0x0004;
    const GPUFlagsConstant COPY_DST      = 0x0008;
    const GPUFlagsConstant INDEX         = 0x0010;
    const GPUFlagsConstant VERTEX        = 0x0020;
    const GPUFlagsConstant UNIFORM       = 0x0040;
    const GPUFlagsConstant STORAGE       = 0x0080;
    const GPUFlagsConstant INDIRECT      = 0x0100;
    const GPUFlagsConstant QUERY_RESOLVE = 0x0200;
};
*/
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

/*
typedef [EnforceRange] unsigned long GPUMapModeFlags;
[Exposed=Window]
interface GPUMapMode {
    const GPUFlagsConstant READ  = 0x0001;
    const GPUFlagsConstant WRITE = 0x0002;
};
*/
typedef int WGPU_MAP_MODE_FLAGS;
#define WGPU_MAP_MODE_READ   0x1
#define WGPU_MAP_MODE_WRITE  0x2

/*
[Exposed=Window, Serializable]
interface GPUTexture {
    GPUTextureView createView(optional GPUTextureViewDescriptor descriptor = {});

    undefined destroy();
};
GPUTexture includes GPUObjectBase;
*/
typedef int WGpuTexture;
// Returns true if the given handle references a valid GPUTexture.
EM_BOOL wgpu_is_texture(WGpuObjectBase object);
WGpuTextureView wgpu_texture_create_view(WGpuTexture texture, const WGpuTextureViewDescriptor *textureViewDesc);

/*
dictionary GPUTextureDescriptor : GPUObjectDescriptorBase {
    required GPUExtent3D size;
    GPUIntegerCoordinate mipLevelCount = 1;
    GPUSize32 sampleCount = 1;
    GPUTextureDimension dimension = "2d";
    required GPUTextureFormat format;
    required GPUTextureUsageFlags usage;
};
*/
typedef struct WGpuTextureDescriptor
{
  uint32_t width;
  uint32_t height; // default = 1;
  uint32_t depthOrArrayLayers; // default = 1;
  uint32_t mipLevelCount; // default = 1;
  uint32_t sampleCount; // default = 1;
  WGPU_TEXTURE_DIMENSION dimension; // default = WGPU_TEXTURE_DIMENSION_2D
  WGPU_TEXTURE_FORMAT format;
  WGPU_TEXTURE_USAGE_FLAGS usage;
} WGpuTextureDescriptor;
extern const WGpuTextureDescriptor WGPU_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;

/*
enum GPUTextureDimension {
    "1d",
    "2d",
    "3d",
};
*/
typedef int WGPU_TEXTURE_DIMENSION;
#define WGPU_TEXTURE_DIMENSION_INVALID 0
#define WGPU_TEXTURE_DIMENSION_1D 10
#define WGPU_TEXTURE_DIMENSION_2D 11
#define WGPU_TEXTURE_DIMENSION_3D 12

/*
typedef [EnforceRange] unsigned long GPUTextureUsageFlags;
[Exposed=Window]
interface GPUTextureUsage {
    const GPUFlagsConstant COPY_SRC          = 0x01;
    const GPUFlagsConstant COPY_DST          = 0x02;
    const GPUFlagsConstant SAMPLED           = 0x04;
    const GPUFlagsConstant STORAGE           = 0x08;
    const GPUFlagsConstant RENDER_ATTACHMENT = 0x10;
};
*/
typedef int WGPU_TEXTURE_USAGE_FLAGS;
#define WGPU_TEXTURE_USAGE_COPY_SRC 0x01
#define WGPU_TEXTURE_USAGE_COPY_DST 0x02
#define WGPU_TEXTURE_USAGE_SAMPLED  0x04
#define WGPU_TEXTURE_USAGE_STORAGE  0x08
#define WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT  0x10

/*
[Exposed=Window]
interface GPUTextureView {
};
GPUTextureView includes GPUObjectBase;
*/
typedef int WGpuTextureView;
// Returns true if the given handle references a valid GPUTextureView.
EM_BOOL wgpu_is_texture_view(WGpuObjectBase object);


/*
dictionary GPUTextureViewDescriptor : GPUObjectDescriptorBase {
    GPUTextureFormat format;
    GPUTextureViewDimension dimension;
    GPUTextureAspect aspect = "all";
    GPUIntegerCoordinate baseMipLevel = 0;
    GPUIntegerCoordinate mipLevelCount;
    GPUIntegerCoordinate baseArrayLayer = 0;
    GPUIntegerCoordinate arrayLayerCount;
};
*/
typedef struct WGpuTextureViewDescriptor
{
  WGPU_TEXTURE_FORMAT format;
  WGPU_TEXTURE_VIEW_DIMENSION dimension;
  WGPU_TEXTURE_ASPECT aspect; // default = WGPU_TEXTURE_ASPECT_ALL
  uint32_t baseMipLevel; // default = 0
  uint32_t mipLevelCount;
  uint32_t baseArrayLayer; // default = 0
  uint32_t arrayLayerCount;
} WGpuTextureViewDescriptor;
extern const WGpuTextureViewDescriptor WGPU_TEXTURE_VIEW_DESCRIPTOR_DEFAULT_INITIALIZER;

/*
enum GPUTextureViewDimension {
    "1d",
    "2d",
    "2d-array",
    "cube",
    "cube-array",
    "3d"
};
*/
typedef int WGPU_TEXTURE_VIEW_DIMENSION;
#define WGPU_TEXTURE_VIEW_DIMENSION_INVALID 0
#define WGPU_TEXTURE_VIEW_DIMENSION_1D 10
#define WGPU_TEXTURE_VIEW_DIMENSION_2D 11
#define WGPU_TEXTURE_VIEW_DIMENSION_2D_ARRAY 13
#define WGPU_TEXTURE_VIEW_DIMENSION_CUBE 14
#define WGPU_TEXTURE_VIEW_DIMENSION_CUBE_ARRAY 15
#define WGPU_TEXTURE_VIEW_DIMENSION_3D 12

/*
enum GPUTextureAspect {
    "all",
    "stencil-only",
    "depth-only"
};
*/
typedef int WGPU_TEXTURE_ASPECT;
#define WGPU_TEXTURE_ASPECT_INVALID 0
#define WGPU_TEXTURE_ASPECT_ALL 16
#define WGPU_TEXTURE_ASPECT_STENCIL_ONLY 17
#define WGPU_TEXTURE_ASPECT_DEPTH_ONLY 18

/*
enum GPUTextureFormat {
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
};
*/
typedef int WGPU_TEXTURE_FORMAT;
#define WGPU_TEXTURE_FORMAT_INVALID 0
#define WGPU_TEXTURE_FORMAT_R8UNORM 19
#define WGPU_TEXTURE_FORMAT_R8SNORM 20
#define WGPU_TEXTURE_FORMAT_R8UINT 21
#define WGPU_TEXTURE_FORMAT_R8SINT 22
#define WGPU_TEXTURE_FORMAT_R16UINT 23
#define WGPU_TEXTURE_FORMAT_R16SINT 24
#define WGPU_TEXTURE_FORMAT_R16FLOAT 25
#define WGPU_TEXTURE_FORMAT_RG8UNORM 26
#define WGPU_TEXTURE_FORMAT_RG8SNORM 27
#define WGPU_TEXTURE_FORMAT_RG8UINT 28
#define WGPU_TEXTURE_FORMAT_RG8SINT 29
#define WGPU_TEXTURE_FORMAT_R32UINT 30
#define WGPU_TEXTURE_FORMAT_R32SINT 31
#define WGPU_TEXTURE_FORMAT_R32FLOAT 32
#define WGPU_TEXTURE_FORMAT_RG16UINT 33
#define WGPU_TEXTURE_FORMAT_RG16SINT 34
#define WGPU_TEXTURE_FORMAT_RG16FLOAT 35
#define WGPU_TEXTURE_FORMAT_RGBA8UNORM 36
#define WGPU_TEXTURE_FORMAT_RGBA8UNORM_SRGB 37
#define WGPU_TEXTURE_FORMAT_RGBA8SNORM 38
#define WGPU_TEXTURE_FORMAT_RGBA8UINT 39
#define WGPU_TEXTURE_FORMAT_RGBA8SINT 40
#define WGPU_TEXTURE_FORMAT_BGRA8UNORM 41
#define WGPU_TEXTURE_FORMAT_BGRA8UNORM_SRGB 42
#define WGPU_TEXTURE_FORMAT_RGB9E5UFLOAT 43
#define WGPU_TEXTURE_FORMAT_RGB10A2UNORM 44
#define WGPU_TEXTURE_FORMAT_RG11B10UFLOAT 45
#define WGPU_TEXTURE_FORMAT_RG32UINT 46
#define WGPU_TEXTURE_FORMAT_RG32SINT 47
#define WGPU_TEXTURE_FORMAT_RG32FLOAT 48
#define WGPU_TEXTURE_FORMAT_RGBA16UINT 49
#define WGPU_TEXTURE_FORMAT_RGBA16SINT 50
#define WGPU_TEXTURE_FORMAT_RGBA16FLOAT 51
#define WGPU_TEXTURE_FORMAT_RGBA32UINT 52
#define WGPU_TEXTURE_FORMAT_RGBA32SINT 53
#define WGPU_TEXTURE_FORMAT_RGBA32FLOAT 54
#define WGPU_TEXTURE_FORMAT_STENCIL8 55
#define WGPU_TEXTURE_FORMAT_DEPTH16UNORM 56
#define WGPU_TEXTURE_FORMAT_DEPTH24PLUS 57
#define WGPU_TEXTURE_FORMAT_DEPTH24PLUS_STENCIL8 58
#define WGPU_TEXTURE_FORMAT_DEPTH32FLOAT 59
#define WGPU_TEXTURE_FORMAT_BC1_RGBA_UNORM 60
#define WGPU_TEXTURE_FORMAT_BC1_RGBA_UNORM_SRGB 61
#define WGPU_TEXTURE_FORMAT_BC2_RGBA_UNORM 62
#define WGPU_TEXTURE_FORMAT_BC2_RGBA_UNORM_SRGB 63
#define WGPU_TEXTURE_FORMAT_BC3_RGBA_UNORM 64
#define WGPU_TEXTURE_FORMAT_BC3_RGBA_UNORM_SRGB 65
#define WGPU_TEXTURE_FORMAT_BC4_R_UNORM 66
#define WGPU_TEXTURE_FORMAT_BC4_R_SNORM 67
#define WGPU_TEXTURE_FORMAT_BC5_RG_UNORM 68
#define WGPU_TEXTURE_FORMAT_BC5_RG_SNORM 69
#define WGPU_TEXTURE_FORMAT_BC6H_RGB_UFLOAT 70
#define WGPU_TEXTURE_FORMAT_BC6H_RGB_FLOAT 71
#define WGPU_TEXTURE_FORMAT_BC7_RGBA_UNORM 72
#define WGPU_TEXTURE_FORMAT_BC7_RGBA_UNORM_SRGB 73
#define WGPU_TEXTURE_FORMAT_DEPTH24UNORM_STENCIL8 5
#define WGPU_TEXTURE_FORMAT_DEPTH32FLOAT_STENCIL8 6

/*
[Exposed=Window]
interface GPUExternalTexture {
};
GPUExternalTexture includes GPUObjectBase;
*/
typedef int WGpuExternalTexture;
// Returns true if the given handle references a valid GPUExternalTexture.
EM_BOOL wgpu_is_external_texture(WGpuObjectBase object);


/*
dictionary GPUExternalTextureDescriptor : GPUObjectDescriptorBase {
    required HTMLVideoElement source;
    GPUPredefinedColorSpace colorSpace = "srgb";
};
*/
typedef struct WGpuExternalTextureDescriptor
{
  char source[512]; // CSS selector for a source HTMLVideoElement
  WGPU_PREDEFINED_COLOR_SPACE colorSpace;
} WGpuExternalTextureDescriptor;
extern const WGpuExternalTextureDescriptor WGPU_EXTERNAL_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;

/*
[Exposed=Window]
interface GPUSampler {
};
GPUSampler includes GPUObjectBase;
*/
typedef int WGpuSampler;
// Returns true if the given handle references a valid GPUSampler.
EM_BOOL wgpu_is_sampler(WGpuObjectBase object);

/*
dictionary GPUSamplerDescriptor : GPUObjectDescriptorBase {
    GPUAddressMode addressModeU = "clamp-to-edge";
    GPUAddressMode addressModeV = "clamp-to-edge";
    GPUAddressMode addressModeW = "clamp-to-edge";
    GPUFilterMode magFilter = "nearest";
    GPUFilterMode minFilter = "nearest";
    GPUFilterMode mipmapFilter = "nearest";
    float lodMinClamp = 0;
    float lodMaxClamp = 0xffffffff; // TODO: What should this be? Was Number.MAX_VALUE.
    GPUCompareFunction compare;
    [Clamp] unsigned short maxAnisotropy = 1;
};
*/
typedef struct WGpuSamplerDescriptor
{
  WGPU_ADDRESS_MODE addressModeU;
  WGPU_ADDRESS_MODE addressModeV;
  WGPU_ADDRESS_MODE addressModeW;
  WGPU_FILTER_MODE magFilter;
  WGPU_FILTER_MODE minFilter;
  WGPU_FILTER_MODE mipmapFilter;
  float lodMinClamp;
  float lodMaxClamp;
  WGPU_COMPARE_FUNCTION compare;
  uint32_t maxAnisotropy; // N.b. this is 32-bit wide in the bindings implementation for simplicity, unlike in the IDL which specifies a unsigned short.
} WGpuSamplerDescriptor;
extern const WGpuSamplerDescriptor WGPU_SAMPLER_DESCRIPTOR_DEFAULT_INITIALIZER;

/*
enum GPUAddressMode {
    "clamp-to-edge",
    "repeat",
    "mirror-repeat"
};
*/
typedef int WGPU_ADDRESS_MODE;
#define WGPU_ADDRESS_MODE_INVALID 0
#define WGPU_ADDRESS_MODE_CLAMP_TO_EDGE 74
#define WGPU_ADDRESS_MODE_REPEAT 75
#define WGPU_ADDRESS_MODE_MIRROR_REPEAT 76

/*
enum GPUFilterMode {
    "nearest",
    "linear"
};
*/
typedef int WGPU_FILTER_MODE;
#define WGPU_FILTER_MODE_INVALID 0
#define WGPU_FILTER_MODE_NEAREST 77
#define WGPU_FILTER_MODE_LINEAR 78

/*
enum GPUCompareFunction {
    "never",
    "less",
    "equal",
    "less-equal",
    "greater",
    "not-equal",
    "greater-equal",
    "always"
};
*/
typedef int WGPU_COMPARE_FUNCTION;
#define WGPU_COMPARE_FUNCTION_INVALID 0
#define WGPU_COMPARE_FUNCTION_NEVER 79
#define WGPU_COMPARE_FUNCTION_LESS 80
#define WGPU_COMPARE_FUNCTION_EQUAL 81
#define WGPU_COMPARE_FUNCTION_LESS_EQUAL 82
#define WGPU_COMPARE_FUNCTION_GREATER 83
#define WGPU_COMPARE_FUNCTION_NOT_EQUAL 84
#define WGPU_COMPARE_FUNCTION_GREATER_EQUAL 85
#define WGPU_COMPARE_FUNCTION_ALWAYS 86

/*
[Exposed=Window, Serializable]
interface GPUBindGroupLayout {
};
GPUBindGroupLayout includes GPUObjectBase;
*/
typedef int WGpuBindGroupLayout;
// Returns true if the given handle references a valid GPUBindGroupLayout.
EM_BOOL wgpu_is_bind_group_layout(WGpuObjectBase object);

/*
dictionary GPUBindGroupLayoutDescriptor : GPUObjectDescriptorBase {
    required sequence<GPUBindGroupLayoutEntry> entries;
};
*/
// Currently not used

/*
typedef [EnforceRange] unsigned long GPUShaderStageFlags;
[Exposed=Window]
interface GPUShaderStage {
    const GPUFlagsConstant VERTEX   = 0x1;
    const GPUFlagsConstant FRAGMENT = 0x2;
    const GPUFlagsConstant COMPUTE  = 0x4;
};
*/
typedef int WGPU_SHADER_STAGE_FLAGS;
#define WGPU_SHADER_STAGE_VERTEX   0x1
#define WGPU_SHADER_STAGE_FRAGMENT 0x2
#define WGPU_SHADER_STAGE_COMPUTE  0x4

/*
dictionary GPUBindGroupLayoutEntry {
    required GPUIndex32 binding;
    required GPUShaderStageFlags visibility;

    GPUBufferBindingLayout buffer;
    GPUSamplerBindingLayout sampler;
    GPUTextureBindingLayout texture;
    GPUStorageTextureBindingLayout storageTexture;
    GPUExternalTextureBindingLayout externalTexture;
};
*/
typedef int WGPU_BIND_GROUP_LAYOUT_TYPE;
#define WGPU_BIND_GROUP_LAYOUT_TYPE_INVALID 0
#define WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER 1
#define WGPU_BIND_GROUP_LAYOUT_TYPE_SAMPLER 2
#define WGPU_BIND_GROUP_LAYOUT_TYPE_TEXTURE 3
#define WGPU_BIND_GROUP_LAYOUT_TYPE_STORAGE_TEXTURE 4
#define WGPU_BIND_GROUP_LAYOUT_TYPE_EXTERNAL_TEXTURE 5

// typedef struct WGpuBindGroupLayoutEntry at the end of this file.

/*
enum GPUBufferBindingType {
    "uniform",
    "storage",
    "read-only-storage",
};
*/
typedef int WGPU_BUFFER_BINDING_TYPE;
#define WGPU_BUFFER_BINDING_TYPE_INVALID 0
#define WGPU_BUFFER_BINDING_TYPE_UNIFORM 87
#define WGPU_BUFFER_BINDING_TYPE_STORAGE 88
#define WGPU_BUFFER_BINDING_TYPE_READ_ONLY_STORAGE 89

/*
dictionary GPUBufferBindingLayout {
    GPUBufferBindingType type = "uniform";
    boolean hasDynamicOffset = false;
    GPUSize64 minBindingSize = 0;
};
*/
typedef struct WGpuBufferBindingLayout
{
  WGPU_BUFFER_BINDING_TYPE type;
  EM_BOOL hasDynamicOffset;
  uint64_t minBindingSize;
} WGpuBufferBindingLayout;
extern const WGpuBufferBindingLayout WGPU_BUFFER_BINDING_LAYOUT_DEFAULT_INITIALIZER;

/*
enum GPUSamplerBindingType {
    "filtering",
    "non-filtering",
    "comparison",
};
*/
typedef int WGPU_SAMPLER_BINDING_TYPE;
#define WGPU_SAMPLER_BINDING_TYPE_INVALID 0
#define WGPU_SAMPLER_BINDING_TYPE_FILTERING 90
#define WGPU_SAMPLER_BINDING_TYPE_NON_FILTERING 91
#define WGPU_SAMPLER_BINDING_TYPE_COMPARISON 92

/*
dictionary GPUSamplerBindingLayout {
    GPUSamplerBindingType type = "filtering";
};
*/
typedef struct WGpuSamplerBindingLayout
{
  WGPU_SAMPLER_BINDING_TYPE type;
} WGpuSamplerBindingLayout;
extern const WGpuSamplerBindingLayout WGPU_SAMPLER_BINDING_LAYOUT_DEFAULT_INITIALIZER;

/*
enum GPUTextureSampleType {
  "float",
  "unfilterable-float",
  "depth",
  "sint",
  "uint",
};
*/
typedef int WGPU_TEXTURE_SAMPLE_TYPE;
#define WGPU_TEXTURE_SAMPLE_TYPE_INVALID 0
#define WGPU_TEXTURE_SAMPLE_TYPE_FLOAT 93
#define WGPU_TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT 94
#define WGPU_TEXTURE_SAMPLE_TYPE_DEPTH 95
#define WGPU_TEXTURE_SAMPLE_TYPE_SINT 96
#define WGPU_TEXTURE_SAMPLE_TYPE_UINT 97

/*
dictionary GPUTextureBindingLayout {
    GPUTextureSampleType sampleType = "float";
    GPUTextureViewDimension viewDimension = "2d";
    boolean multisampled = false;
};
*/
typedef struct WGpuTextureBindingLayout
{
  WGPU_TEXTURE_SAMPLE_TYPE sampleType;
  WGPU_TEXTURE_VIEW_DIMENSION viewDimension;
} WGpuTextureBindingLayout;
extern const WGpuTextureBindingLayout WGPU_TEXTURE_BINDING_LAYOUT_DEFAULT_INITIALIZER;

/*
enum GPUStorageTextureAccess {
    "read-only",
    "write-only",
};
*/
typedef int WGPU_STORAGE_TEXTURE_ACCESS;
#define WGPU_STORAGE_TEXTURE_ACCESS_INVALID 0
#define WGPU_STORAGE_TEXTURE_ACCESS_READ_ONLY 98
#define WGPU_STORAGE_TEXTURE_ACCESS_WRITE_ONLY 99

/*
dictionary GPUStorageTextureBindingLayout {
    required GPUStorageTextureAccess access;
    required GPUTextureFormat format;
    GPUTextureViewDimension viewDimension = "2d";
};
*/
typedef struct WGpuStorageTextureBindingLayout
{
  WGPU_STORAGE_TEXTURE_ACCESS access;
  WGPU_TEXTURE_FORMAT format;
  WGPU_TEXTURE_VIEW_DIMENSION viewDimension;
} WGpuStorageTextureBindingLayout;
extern const WGpuStorageTextureBindingLayout WGPU_STORAGE_TEXTURE_BINDING_LAYOUT_DEFAULT_INITIALIZER;

/*
dictionary GPUExternalTextureBindingLayout {
};
*/
typedef struct WGpuExternalTextureBindingLayout
{
  uint32_t _dummyPadding; // Appease mixed C and C++ compilation to agree on non-zero struct size.
} WGpuExternalTextureBindingLayout;

/*
[Exposed=Window]
interface GPUBindGroup {
};
GPUBindGroup includes GPUObjectBase;
*/
typedef int WGpuBindGroup;
// Returns true if the given handle references a valid GPUBindGroup.
EM_BOOL wgpu_is_bind_group(WGpuObjectBase object);

/*
dictionary GPUBindGroupDescriptor : GPUObjectDescriptorBase {
    required GPUBindGroupLayout layout;
    required sequence<GPUBindGroupEntry> entries;
};
*/
// Currently unused

/*
typedef (GPUSampler or GPUTextureView or GPUBufferBinding or GPUExternalTexture) GPUBindingResource;

dictionary GPUBindGroupEntry {
    required GPUIndex32 binding;
    required GPUBindingResource resource;
};
*/
typedef struct WGpuBindGroupEntry
{
  uint32_t binding;
  WGpuObjectBase resource;
  // If 'resource' points to a WGpuBuffer, bufferBindOffset and bufferBindSize specify
  // the offset and length of the buffer to bind. If 'resource' does not point to a WGpuBuffer,
  // offset and size are ignored.
  uint64_t bufferBindOffset;
  uint64_t bufferBindSize; // If set to 0 (default), the whole buffer is bound.
} WGpuBindGroupEntry;
extern const WGpuBindGroupEntry WGPU_BIND_GROUP_ENTRY_DEFAULT_INITIALIZER;

/*
dictionary GPUBufferBinding {
    required GPUBuffer buffer;
    GPUSize64 offset = 0;
    GPUSize64 size;
};
*/
// Not exposed. Integrated as part of WGpuBindGroupEntry.

/*
[Exposed=Window, Serializable]
interface GPUPipelineLayout {
};
GPUPipelineLayout includes GPUObjectBase;
*/
typedef int WGpuPipelineLayout;
// Returns true if the given handle references a valid GPUPipelineLayout.
EM_BOOL wgpu_is_pipeline_layout(WGpuObjectBase object);

/*
dictionary GPUPipelineLayoutDescriptor : GPUObjectDescriptorBase {
    required sequence<GPUBindGroupLayout> bindGroupLayouts;
};
*/
// Currently unused.

/*
[Exposed=Window, Serializable]
interface GPUShaderModule {
    Promise<GPUCompilationInfo> compilationInfo();
};
GPUShaderModule includes GPUObjectBase;
*/
typedef int WGpuShaderModule;
// Returns true if the given handle references a valid GPUShaderModule.
EM_BOOL wgpu_is_shader_module(WGpuObjectBase object);

/*
dictionary GPUShaderModuleDescriptor : GPUObjectDescriptorBase {
    required USVString code;
    object sourceMap;
};
*/
typedef struct WGpuShaderModuleDescriptor
{
  const char *code;
  // TODO: add sourceMap
} WGpuShaderModuleDescriptor;

/*
enum GPUCompilationMessageType {
    "error",
    "warning",
    "info"
};
*/
typedef int WGPU_COMPILATION_MESSAGE_TYPE;
#define WGPU_COMPILATION_MESSAGE_TYPE_INVALID 0
#define WGPU_COMPILATION_MESSAGE_TYPE_ERROR 100
#define WGPU_COMPILATION_MESSAGE_TYPE_WARNING 101
#define WGPU_COMPILATION_MESSAGE_TYPE_INFO 102

/*
[Exposed=Window, Serializable]
interface GPUCompilationMessage {
    readonly attribute DOMString message;
    readonly attribute GPUCompilationMessageType type;
    readonly attribute unsigned long long lineNum;
    readonly attribute unsigned long long linePos;
    readonly attribute unsigned long long offset;
    readonly attribute unsigned long long length;
};
*/
// TODO: implement

/*
[Exposed=Window, Serializable]
interface GPUCompilationInfo {
    readonly attribute FrozenArray<GPUCompilationMessage> messages;
};
*/
// TODO: implement

/*
dictionary GPUPipelineDescriptorBase : GPUObjectDescriptorBase {
    GPUPipelineLayout layout;
};
*/
// TODO: implement

/*
interface mixin GPUPipelineBase {
    GPUBindGroupLayout getBindGroupLayout(unsigned long index);
};
*/
// TODO: implement

/*
dictionary GPUProgrammableStage {
    required GPUShaderModule module;
    required USVString entryPoint;
    record<USVString, GPUPipelineConstantValue> constants;
};
*/
// TODO: implement

/*

typedef double GPUPipelineConstantValue; // May represent WGSL’s bool, f32, i32, u32.

[Exposed=Window, Serializable]
interface GPUComputePipeline {
};
GPUComputePipeline includes GPUObjectBase;
GPUComputePipeline includes GPUPipelineBase;
*/
typedef int WGpuComputePipeline;
// Returns true if the given handle references a valid GPUComputePipeline.
EM_BOOL wgpu_is_compute_pipeline(WGpuObjectBase object);

/*
dictionary GPUComputePipelineDescriptor : GPUPipelineDescriptorBase {
    required GPUProgrammableStage compute;
};
*/
// Currently unused.

/*
[Exposed=Window, Serializable]
interface GPURenderPipeline {
};
GPURenderPipeline includes GPUObjectBase;
GPURenderPipeline includes GPUPipelineBase;
*/
typedef int WGpuRenderPipeline;
typedef int WGpuPipelineBase;
// Returns true if the given handle references a valid GPURenderPipeline.
EM_BOOL wgpu_is_render_pipeline(WGpuObjectBase object);

/*
dictionary GPURenderPipelineDescriptor : GPUPipelineDescriptorBase {
    required GPUVertexState vertex;
    GPUPrimitiveState primitive = {};
    GPUDepthStencilState depthStencil;
    GPUMultisampleState multisample = {};
    GPUFragmentState fragment;
};
*/
// Defined at the end of this file

/*
enum GPUPrimitiveTopology {
    "point-list",
    "line-list",
    "line-strip",
    "triangle-list",
    "triangle-strip"
};
*/
typedef int WGPU_PRIMITIVE_TOPOLOGY;
#define WGPU_PRIMITIVE_TOPOLOGY_INVALID 0
#define WGPU_PRIMITIVE_TOPOLOGY_POINT_LIST 103
#define WGPU_PRIMITIVE_TOPOLOGY_LINE_LIST 104
#define WGPU_PRIMITIVE_TOPOLOGY_LINE_STRIP 105
#define WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST 106
#define WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP 107

/*
dictionary GPUPrimitiveState {
    GPUPrimitiveTopology topology = "triangle-list";
    GPUIndexFormat stripIndexFormat;
    GPUFrontFace frontFace = "ccw";
    GPUCullMode cullMode = "none";

    // Enable depth clamping (requires "depth-clamping" feature)
    boolean clampDepth = false;
};
*/
typedef struct WGpuPrimitiveState
{
  WGPU_PRIMITIVE_TOPOLOGY topology; // Defaults to WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ('triangle-list')
  WGPU_INDEX_FORMAT stripIndexFormat; // Defaults to undefined, must be explicitly specified if WGPU_PRIMITIVE_TOPOLOGY_LINE_STRIP ('line-strip') or WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP ('triangle-strip') is used.
  WGPU_FRONT_FACE frontFace; // Defaults to WGPU_FRONT_FACE_CCW ('ccw')
  WGPU_CULL_MODE cullMode; // Defaults to WGPU_CULL_MODE_NONE ('none')
} WGpuPrimitiveState;

/*
enum GPUFrontFace {
    "ccw",
    "cw"
};
*/
typedef int WGPU_FRONT_FACE;
#define WGPU_FRONT_FACE_INVALID 0
#define WGPU_FRONT_FACE_CCW 108
#define WGPU_FRONT_FACE_CW 109

/*
enum GPUCullMode {
    "none",
    "front",
    "back"
};
*/
typedef int WGPU_CULL_MODE;
#define WGPU_CULL_MODE_INVALID 0
#define WGPU_CULL_MODE_NONE 110
#define WGPU_CULL_MODE_FRONT 111
#define WGPU_CULL_MODE_BACK 112

/*
dictionary GPUMultisampleState {
    GPUSize32 count = 1;
    GPUSampleMask mask = 0xFFFFFFFF;
    boolean alphaToCoverageEnabled = false;
};
*/
typedef struct WGpuMultisampleState
{
  uint32_t count;
  uint32_t mask;
  EM_BOOL alphaToCoverageEnabled;
} WGpuMultisampleState;

/*
dictionary GPUFragmentState: GPUProgrammableStage {
    required sequence<GPUColorTargetState> targets;
};
*/
typedef struct WGpuFragmentState
{
  WGpuShaderModule module;
  const char *entryPoint;
  int numTargets;
  const WGpuColorTargetState *targets;
} WGpuFragmentState;

/*
dictionary GPUColorTargetState {
    required GPUTextureFormat format;

    GPUBlendState blend;
    GPUColorWriteFlags writeMask = 0xF;  // GPUColorWrite.ALL
};
*/
// Defined at the end of this file

/*
dictionary GPUBlendState {
    required GPUBlendComponent color;
    required GPUBlendComponent alpha;
};
*/
// Defined at the end of this file

/*
typedef [EnforceRange] unsigned long GPUColorWriteFlags;
[Exposed=Window]
interface GPUColorWrite {
    const GPUFlagsConstant RED   = 0x1;
    const GPUFlagsConstant GREEN = 0x2;
    const GPUFlagsConstant BLUE  = 0x4;
    const GPUFlagsConstant ALPHA = 0x8;
    const GPUFlagsConstant ALL   = 0xF;
};
*/
typedef int WGPU_COLOR_WRITE_FLAGS;
#define WGPU_COLOR_WRITE_RED   0x01
#define WGPU_COLOR_WRITE_GREEN 0x02
#define WGPU_COLOR_WRITE_BLUE  0x04
#define WGPU_COLOR_WRITE_ALPHA 0x08
#define WGPU_COLOR_WRITE_ALL   0x0F

/*
dictionary GPUBlendComponent {
    GPUBlendFactor srcFactor = "one";
    GPUBlendFactor dstFactor = "zero";
    GPUBlendOperation operation = "add";
};
*/
typedef struct WGpuBlendComponent
{
  WGPU_BLEND_FACTOR srcFactor;
  WGPU_BLEND_FACTOR dstFactor;
  WGPU_BLEND_OPERATION operation;
} WGpuBlendComponent;

/*
enum GPUBlendFactor {
    "zero",
    "one",
    "src",
    "one-minus-src",
    "src-alpha",
    "one-minus-src-alpha",
    "dst",
    "one-minus-dst",
    "dst-alpha",
    "one-minus-dst-alpha",
    "src-alpha-saturated",
    "constant",
    "one-minus-constant"
};
*/
typedef int WGPU_BLEND_FACTOR;
#define WGPU_BLEND_FACTOR_INVALID 0
#define WGPU_BLEND_FACTOR_ZERO 113
#define WGPU_BLEND_FACTOR_ONE 114
#define WGPU_BLEND_FACTOR_SRC 115
#define WGPU_BLEND_FACTOR_ONE_MINUS_SRC 116
#define WGPU_BLEND_FACTOR_SRC_ALPHA 117
#define WGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA 118
#define WGPU_BLEND_FACTOR_DST 119
#define WGPU_BLEND_FACTOR_ONE_MINUS_DST 120
#define WGPU_BLEND_FACTOR_DST_ALPHA 121
#define WGPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA 122
#define WGPU_BLEND_FACTOR_SRC_ALPHA_SATURATED 123
#define WGPU_BLEND_FACTOR_CONSTANT 124
#define WGPU_BLEND_FACTOR_ONE_MINUS_CONSTANT 125

/*
enum GPUBlendOperation {
    "add",
    "subtract",
    "reverse-subtract",
    "min",
    "max"
};
*/
typedef int WGPU_BLEND_OPERATION;
#define WGPU_BLEND_OPERATION_INVALID 0
#define WGPU_BLEND_OPERATION_ADD 126
#define WGPU_BLEND_OPERATION_SUBTRACT 127
#define WGPU_BLEND_OPERATION_REVERSE_SUBTRACT 128
#define WGPU_BLEND_OPERATION_MIN 129
#define WGPU_BLEND_OPERATION_MAX 130

/*
dictionary GPUDepthStencilState {
    required GPUTextureFormat format;

    boolean depthWriteEnabled = false;
    GPUCompareFunction depthCompare = "always";

    GPUStencilFaceState stencilFront = {};
    GPUStencilFaceState stencilBack = {};

    GPUStencilValue stencilReadMask = 0xFFFFFFFF;
    GPUStencilValue stencilWriteMask = 0xFFFFFFFF;

    GPUDepthBias depthBias = 0;
    float depthBiasSlopeScale = 0;
    float depthBiasClamp = 0;
};
*/
// Defined at the end of this file

/*
dictionary GPUStencilFaceState {
    GPUCompareFunction compare = "always";
    GPUStencilOperation failOp = "keep";
    GPUStencilOperation depthFailOp = "keep";
    GPUStencilOperation passOp = "keep";
};
*/
typedef struct WGpuStencilFaceState
{
  WGPU_COMPARE_FUNCTION compare;
  WGPU_STENCIL_OPERATION failOp;
  WGPU_STENCIL_OPERATION depthFailOp;
  WGPU_STENCIL_OPERATION passOp;
} WGpuStencilFaceState;

/*
enum GPUStencilOperation {
    "keep",
    "zero",
    "replace",
    "invert",
    "increment-clamp",
    "decrement-clamp",
    "increment-wrap",
    "decrement-wrap"
};
*/
typedef int WGPU_STENCIL_OPERATION;
#define WGPU_STENCIL_OPERATION_INVALID 0
#define WGPU_STENCIL_OPERATION_KEEP 131
#define WGPU_STENCIL_OPERATION_ZERO 113
#define WGPU_STENCIL_OPERATION_REPLACE 132
#define WGPU_STENCIL_OPERATION_INVERT 133
#define WGPU_STENCIL_OPERATION_INCREMENT_CLAMP 134
#define WGPU_STENCIL_OPERATION_DECREMENT_CLAMP 135
#define WGPU_STENCIL_OPERATION_INCREMENT_WRAP 136
#define WGPU_STENCIL_OPERATION_DECREMENT_WRAP 137

/*
enum GPUIndexFormat {
    "uint16",
    "uint32"
};
*/
typedef int WGPU_INDEX_FORMAT;
#define WGPU_INDEX_FORMAT_INVALID 0
#define WGPU_INDEX_FORMAT_UINT16 138
#define WGPU_INDEX_FORMAT_UINT32 139

/*
enum GPUVertexFormat {
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
    "sint32x4",
};
*/
typedef int WGPU_VERTEX_FORMAT;
#define WGPU_VERTEX_FORMAT_INVALID 0
#define WGPU_VERTEX_FORMAT_UINT8X2 140
#define WGPU_VERTEX_FORMAT_UINT8X4 141
#define WGPU_VERTEX_FORMAT_SINT8X2 142
#define WGPU_VERTEX_FORMAT_SINT8X4 143
#define WGPU_VERTEX_FORMAT_UNORM8X2 144
#define WGPU_VERTEX_FORMAT_UNORM8X4 145
#define WGPU_VERTEX_FORMAT_SNORM8X2 146
#define WGPU_VERTEX_FORMAT_SNORM8X4 147
#define WGPU_VERTEX_FORMAT_UINT16X2 148
#define WGPU_VERTEX_FORMAT_UINT16X4 149
#define WGPU_VERTEX_FORMAT_SINT16X2 150
#define WGPU_VERTEX_FORMAT_SINT16X4 151
#define WGPU_VERTEX_FORMAT_UNORM16X2 152
#define WGPU_VERTEX_FORMAT_UNORM16X4 153
#define WGPU_VERTEX_FORMAT_SNORM16X2 154
#define WGPU_VERTEX_FORMAT_SNORM16X4 155
#define WGPU_VERTEX_FORMAT_FLOAT16X2 156
#define WGPU_VERTEX_FORMAT_FLOAT16X4 157
#define WGPU_VERTEX_FORMAT_FLOAT32 158
#define WGPU_VERTEX_FORMAT_FLOAT32X2 159
#define WGPU_VERTEX_FORMAT_FLOAT32X3 160
#define WGPU_VERTEX_FORMAT_FLOAT32X4 161
#define WGPU_VERTEX_FORMAT_UINT32 139
#define WGPU_VERTEX_FORMAT_UINT32X2 162
#define WGPU_VERTEX_FORMAT_UINT32X3 163
#define WGPU_VERTEX_FORMAT_UINT32X4 164
#define WGPU_VERTEX_FORMAT_SINT32 165
#define WGPU_VERTEX_FORMAT_SINT32X2 166
#define WGPU_VERTEX_FORMAT_SINT32X3 167
#define WGPU_VERTEX_FORMAT_SINT32X4 168

/*
enum GPUInputStepMode {
    "vertex",
    "instance"
};
*/
typedef int WGPU_INPUT_STEP_MODE;
#define WGPU_INPUT_STEP_MODE_INVALID 0
#define WGPU_INPUT_STEP_MODE_VERTEX 169
#define WGPU_INPUT_STEP_MODE_INSTANCE 170

/*
dictionary GPUVertexState: GPUProgrammableStage {
    sequence<GPUVertexBufferLayout?> buffers = [];
};
*/
typedef struct WGpuVertexState
{
  WGpuShaderModule module;
  const char *entryPoint;
  int numBuffers;
  const WGpuVertexBufferLayout *buffers;
} WGpuVertexState;

/*
dictionary GPUVertexBufferLayout {
    required GPUSize64 arrayStride;
    GPUInputStepMode stepMode = "vertex";
    required sequence<GPUVertexAttribute> attributes;
};
*/
typedef struct WGpuVertexBufferLayout
{
  int numAttributes;
  const WGpuVertexAttribute *attributes;
  uint64_t arrayStride;
  WGPU_INPUT_STEP_MODE stepMode;
  uint32_t _unused64BitPadding;
} WGpuVertexBufferLayout;

/*
dictionary GPUVertexAttribute {
    required GPUVertexFormat format;
    required GPUSize64 offset;

    required GPUIndex32 shaderLocation;
};
*/
typedef struct WGpuVertexAttribute
{
  uint64_t offset;
  uint32_t shaderLocation;
  WGPU_VERTEX_FORMAT format;
} WGpuVertexAttribute;

/*
[Exposed=Window]
interface GPUCommandBuffer {
    readonly attribute Promise<double> executionTime;
};
GPUCommandBuffer includes GPUObjectBase;
*/
typedef int WGpuCommandBuffer;
// Returns true if the given handle references a valid GPUCommandBuffer.
EM_BOOL wgpu_is_command_buffer(WGpuObjectBase object);

/*
dictionary GPUCommandBufferDescriptor : GPUObjectDescriptorBase {
};
*/
typedef struct WGpuCommandBufferDescriptor
{
  uint32_t _dummyPadding; // Appease mixed C and C++ compilation to agree on non-zero struct size. Remove this once label is added
  // TODO: add label
} WGpuCommandBufferDescriptor;

/*
[Exposed=Window]
interface GPUCommandEncoder {
    GPURenderPassEncoder beginRenderPass(GPURenderPassDescriptor descriptor);
    GPUComputePassEncoder beginComputePass(optional GPUComputePassDescriptor descriptor = {});

    undefined copyBufferToBuffer(
        GPUBuffer source,
        GPUSize64 sourceOffset,
        GPUBuffer destination,
        GPUSize64 destinationOffset,
        GPUSize64 size);

    undefined copyBufferToTexture(
        GPUImageCopyBuffer source,
        GPUImageCopyTexture destination,
        GPUExtent3D copySize);

    undefined copyTextureToBuffer(
        GPUImageCopyTexture source,
        GPUImageCopyBuffer destination,
        GPUExtent3D copySize);

    undefined copyTextureToTexture(
        GPUImageCopyTexture source,
        GPUImageCopyTexture destination,
        GPUExtent3D copySize);

    undefined pushDebugGroup(USVString groupLabel);
    undefined popDebugGroup();
    undefined insertDebugMarker(USVString markerLabel);

    undefined writeTimestamp(GPUQuerySet querySet, GPUSize32 queryIndex);

    undefined resolveQuerySet(
        GPUQuerySet querySet,
        GPUSize32 firstQuery,
        GPUSize32 queryCount,
        GPUBuffer destination,
        GPUSize64 destinationOffset);

    GPUCommandBuffer finish(optional GPUCommandBufferDescriptor descriptor = {});
};
GPUCommandEncoder includes GPUObjectBase;
*/
typedef int WGpuCommandEncoder;
// Returns true if the given handle references a valid GPUCommandEncoder.
EM_BOOL wgpu_is_command_encoder(WGpuObjectBase object);

WGpuRenderPassEncoder wgpu_command_encoder_begin_render_pass(WGpuCommandEncoder commandEncoder, const WGpuRenderPassDescriptor *renderPassDesc);
WGpuComputePassEncoder wgpu_command_encoder_begin_compute_pass(WGpuCommandEncoder commandEncoder, const WGpuComputePassDescriptor *computePassDesc _WGPU_DEFAULT_VALUE(0));
void wgpu_command_encoder_copy_buffer_to_buffer(WGpuCommandEncoder commandEncoder, WGpuBuffer source, double_int53_t sourceOffset, WGpuBuffer destination, double_int53_t destinationOffset, double_int53_t size);
void wgpu_command_encoder_copy_buffer_to_texture(WGpuCommandEncoder commandEncoder, const WGpuImageCopyBuffer *source, const WGpuImageCopyTexture *destination, uint32_t copyWidth, uint32_t copyHeight _WGPU_DEFAULT_VALUE(1), uint32_t copyDepthOrArrayLayers _WGPU_DEFAULT_VALUE(1));
void wgpu_command_encoder_copy_texture_to_buffer(WGpuCommandEncoder commandEncoder, const WGpuImageCopyTexture *source, const WGpuImageCopyBuffer *destination, uint32_t copyWidth, uint32_t copyHeight _WGPU_DEFAULT_VALUE(1), uint32_t copyDepthOrArrayLayers _WGPU_DEFAULT_VALUE(1));
void wgpu_command_encoder_copy_texture_to_texture(WGpuCommandEncoder commandEncoder, const WGpuImageCopyTexture *source, const WGpuImageCopyTexture *destination, uint32_t copyWidth, uint32_t copyHeight _WGPU_DEFAULT_VALUE(1), uint32_t copyDepthOrArrayLayers _WGPU_DEFAULT_VALUE(1));

void wgpu_encoder_push_debug_group(WGpuCommandEncoder commandEncoder, const char *groupLabel);
void wgpu_encoder_pop_debug_group(WGpuCommandEncoder commandEncoder);
void wgpu_encoder_insert_debug_marker(WGpuCommandEncoder commandEncoder, const char *markerLabel);
#define wgpu_command_encoder_push_debug_group wgpu_encoder_push_debug_group
#define wgpu_command_encoder_pop_debug_group wgpu_encoder_pop_debug_group
#define wgpu_command_encoder_insert_debug_marker wgpu_encoder_insert_debug_marker

void wgpu_encoder_write_timestamp(WGpuObjectBase encoder, WGpuQuerySet querySet, uint32_t queryIndex);
#define wgpu_command_encoder_write_timestamp wgpu_encoder_write_timestamp

void wgpu_command_encoder_resolve_query_set(WGpuCommandEncoder commandEncoder, WGpuQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, WGpuBuffer destination, double_int53_t destinationOffset);

WGpuCommandBuffer wgpu_command_encoder_finish(WGpuCommandEncoder commandEncoder);

/*
dictionary GPUCommandEncoderDescriptor : GPUObjectDescriptorBase {
    boolean measureExecutionTime = false;

    // TODO: reusability flag?
};
*/
typedef struct WGpuCommandEncoderDescriptor
{
  EM_BOOL measureExecutionTime;
} WGpuCommandEncoderDescriptor;
extern const WGpuCommandEncoderDescriptor WGPU_COMMAND_ENCODER_DESCRIPTOR_DEFAULT_INITIALIZER;

/*
dictionary GPUImageDataLayout {
    GPUSize64 offset = 0;
    GPUSize32 bytesPerRow;
    GPUSize32 rowsPerImage;
};
*/
typedef struct WGpuImageDataLayout
{
  uint64_t offset;
  uint32_t bytesPerRow;
  uint32_t rowsPerImage;
} WGpuImageDataLayout;
extern const WGpuImageDataLayout WGPU_IMAGE_DATA_LAYOUT_DEFAULT_INITIALIZER;

/*
dictionary GPUImageCopyBuffer : GPUImageDataLayout {
    required GPUBuffer buffer;
};
*/
typedef struct WGpuImageCopyBuffer
{
  uint64_t offset;
  uint32_t bytesPerRow;
  uint32_t rowsPerImage;
  WGpuBuffer buffer;
} WGpuImageCopyBuffer;
extern const WGpuImageCopyBuffer WGPU_IMAGE_COPY_BUFFER_DEFAULT_INITIALIZER;

/*
dictionary GPUImageCopyTexture {
    required GPUTexture texture;
    GPUIntegerCoordinate mipLevel = 0;
    GPUOrigin3D origin = {};
    GPUTextureAspect aspect = "all";
};
*/
// Defined at the end of this file

/*
dictionary GPUImageCopyTextureTagged : GPUImageCopyTexture {
    GPUPredefinedColorSpace colorSpace = "srgb";
    boolean premultipliedAlpha = false;
};
*/
// Defined at the end of this file

/*
dictionary GPUImageCopyExternalImage {
    required (ImageBitmap or HTMLCanvasElement or OffscreenCanvas) source;
    GPUOrigin2D origin = {};
};
*/
// Defined at the end of this file

/*
interface mixin GPUProgrammablePassEncoder {
    undefined setBindGroup(GPUIndex32 index, GPUBindGroup bindGroup,
                      optional sequence<GPUBufferDynamicOffset> dynamicOffsets = []);

    undefined setBindGroup(GPUIndex32 index, GPUBindGroup bindGroup,
                      Uint32Array dynamicOffsetsData,
                      GPUSize64 dynamicOffsetsDataStart,
                      GPUSize32 dynamicOffsetsDataLength);

    undefined pushDebugGroup(USVString groupLabel);
    undefined popDebugGroup();
    undefined insertDebugMarker(USVString markerLabel);
};
*/
typedef int WGpuProgrammablePassEncoder;
// Returns true if the given handle references a valid GPUProgrammablePassEncoder.
EM_BOOL wgpu_is_programmable_pass_encoder(WGpuObjectBase object);
void wgpu_programmable_pass_encoder_set_bind_group(WGpuProgrammablePassEncoder encoder, uint32_t index, WGpuBindGroup bindGroup, uint32_t *dynamicOffsets _WGPU_DEFAULT_VALUE(0), uint32_t numDynamicOffsets _WGPU_DEFAULT_VALUE(0));

#define wgpu_programmable_pass_encoder_push_debug_group wgpu_encoder_push_debug_group
#define wgpu_programmable_pass_encoder_pop_debug_group wgpu_encoder_pop_debug_group
#define wgpu_programmable_pass_encoder_insert_debug_marker wgpu_encoder_insert_debug_marker

/*
[Exposed=Window]
interface GPUComputePassEncoder {
    undefined setPipeline(GPUComputePipeline pipeline);
    undefined dispatch(GPUSize32 x, optional GPUSize32 y = 1, optional GPUSize32 z = 1);
    undefined dispatchIndirect(GPUBuffer indirectBuffer, GPUSize64 indirectOffset);

    undefined beginPipelineStatisticsQuery(GPUQuerySet querySet, GPUSize32 queryIndex);
    undefined endPipelineStatisticsQuery();

    undefined writeTimestamp(GPUQuerySet querySet, GPUSize32 queryIndex);

    undefined endPass();
};
GPUComputePassEncoder includes GPUObjectBase;
GPUComputePassEncoder includes GPUProgrammablePassEncoder;
*/
typedef int WGpuComputePassEncoder;
// Returns true if the given handle references a valid GPUComputePassEncoder.
EM_BOOL wgpu_is_compute_pass_encoder(WGpuObjectBase object);

void wgpu_encoder_set_pipeline(WGpuRenderEncoderBase passEncoder, WGpuRenderPipeline renderPipeline);
#define wgpu_compute_pass_encoder_set_pipeline wgpu_encoder_set_pipeline
void wgpu_compute_pass_encoder_dispatch(WGpuComputePassEncoder encoder, uint32_t x, uint32_t y _WGPU_DEFAULT_VALUE(1), uint32_t z _WGPU_DEFAULT_VALUE(1));
void wgpu_compute_pass_encoder_dispatch_indirect(WGpuComputePassEncoder encoder, WGpuBuffer indirectBuffer, double_int53_t indirectOffset);

void wgpu_encoder_begin_pipeline_statistics_query(WGpuComputePassEncoder encoder, WGpuQuerySet querySet, uint32_t queryIndex);
void wgpu_encoder_end_pipeline_statistics_query(WGpuComputePassEncoder encoder);
#define wgpu_compute_pass_encoder_begin_pipeline_statistics_query wgpu_encoder_begin_pipeline_statistics_query
#define wgpu_compute_pass_encoder_end_pipeline_statistics_query wgpu_encoder_end_pipeline_statistics_query

#define wgpu_compute_pass_encoder_write_timestamp wgpu_encoder_write_timestamp

void wgpu_encoder_end_pass(WGpuRenderPassEncoder encoder);
#define wgpu_compute_pass_encoder_end_pass wgpu_encoder_end_pass

#define wgpu_compute_pass_encoder_set_bind_group wgpu_programmable_pass_encoder_set_bind_group
#define wgpu_compute_pass_encoder_push_debug_group wgpu_encoder_push_debug_group
#define wgpu_compute_pass_encoder_pop_debug_group wgpu_encoder_pop_debug_group
#define wgpu_compute_pass_encoder_insert_debug_marker wgpu_encoder_insert_debug_marker

/*
dictionary GPUComputePassDescriptor : GPUObjectDescriptorBase {
};
*/
typedef struct WGpuComputePassDescriptor
{
  uint32_t _dummyPadding; // Appease mixed C and C++ compilation to agree on non-zero struct size.
} WGpuComputePassDescriptor;

/*
interface mixin GPURenderEncoderBase {
    undefined setPipeline(GPURenderPipeline pipeline);

    undefined setIndexBuffer(GPUBuffer buffer, GPUIndexFormat indexFormat, optional GPUSize64 offset = 0, optional GPUSize64 size = 0);
    undefined setVertexBuffer(GPUIndex32 slot, GPUBuffer buffer, optional GPUSize64 offset = 0, optional GPUSize64 size = 0);

    undefined draw(GPUSize32 vertexCount, optional GPUSize32 instanceCount = 1,
              optional GPUSize32 firstVertex = 0, optional GPUSize32 firstInstance = 0);
    undefined drawIndexed(GPUSize32 indexCount, optional GPUSize32 instanceCount = 1,
                     optional GPUSize32 firstIndex = 0,
                     optional GPUSignedOffset32 baseVertex = 0,
                     optional GPUSize32 firstInstance = 0);

    undefined drawIndirect(GPUBuffer indirectBuffer, GPUSize64 indirectOffset);
    undefined drawIndexedIndirect(GPUBuffer indirectBuffer, GPUSize64 indirectOffset);
};
*/
typedef int WGpuRenderEncoderBase;
// Returns true if the given handle references a valid GPURenderEncoderBase.
EM_BOOL wgpu_is_render_encoder_base(WGpuObjectBase object);

#define wgpu_render_encoder_base_set_pipeline wgpu_encoder_set_pipeline
void wgpu_render_encoder_base_set_index_buffer(WGpuRenderEncoderBase passEncoder, WGpuBuffer buffer, WGPU_INDEX_FORMAT indexFormat, double_int53_t offset _WGPU_DEFAULT_VALUE(0), double_int53_t size _WGPU_DEFAULT_VALUE(0));
void wgpu_render_encoder_base_set_vertex_buffer(WGpuRenderEncoderBase passEncoder, int32_t slot, WGpuBuffer buffer, double_int53_t offset _WGPU_DEFAULT_VALUE(0), double_int53_t size _WGPU_DEFAULT_VALUE(0));

void wgpu_render_encoder_base_draw(WGpuRenderPassEncoder passEncoder, uint32_t vertexCount, uint32_t instanceCount _WGPU_DEFAULT_VALUE(1), uint32_t firstVertex _WGPU_DEFAULT_VALUE(0), uint32_t firstInstance _WGPU_DEFAULT_VALUE(0));
void wgpu_render_encoder_base_draw_indexed(WGpuRenderPassEncoder passEncoder, uint32_t indexCount, uint32_t instanceCount _WGPU_DEFAULT_VALUE(1), uint32_t firstVertex _WGPU_DEFAULT_VALUE(0), int32_t baseVertex _WGPU_DEFAULT_VALUE(0), uint32_t firstInstance _WGPU_DEFAULT_VALUE(0));

void wgpu_render_encoder_base_draw_indirect(WGpuRenderPassEncoder passEncoder, WGpuBuffer indirectBuffer, double_int53_t indirectOffset);
void wgpu_render_encoder_base_draw_indexed_indirect(WGpuRenderPassEncoder passEncoder, WGpuBuffer indirectBuffer, double_int53_t indirectOffset);

/*
[Exposed=Window]
interface GPURenderPassEncoder {
    undefined setViewport(float x, float y,
                     float width, float height,
                     float minDepth, float maxDepth);

    undefined setScissorRect(GPUIntegerCoordinate x, GPUIntegerCoordinate y,
                        GPUIntegerCoordinate width, GPUIntegerCoordinate height);

    undefined setBlendConstant(GPUColor color);
    undefined setStencilReference(GPUStencilValue reference);

    undefined beginOcclusionQuery(GPUSize32 queryIndex);
    undefined endOcclusionQuery();

    undefined beginPipelineStatisticsQuery(GPUQuerySet querySet, GPUSize32 queryIndex);
    undefined endPipelineStatisticsQuery();

    undefined writeTimestamp(GPUQuerySet querySet, GPUSize32 queryIndex);

    undefined executeBundles(sequence<GPURenderBundle> bundles);
    undefined endPass();
};
GPURenderPassEncoder includes GPUObjectBase;
GPURenderPassEncoder includes GPUProgrammablePassEncoder;
GPURenderPassEncoder includes GPURenderEncoderBase;
*/
typedef int WGpuRenderPassEncoder;
// Returns true if the given handle references a valid GPURenderPassEncoder.
EM_BOOL wgpu_is_render_pass_encoder(WGpuObjectBase object);

void wgpu_render_pass_encoder_set_viewport(WGpuRenderPassEncoder encoder, float x, float y, float width, float height, float minDepth, float maxDepth);

void wgpu_render_pass_encoder_set_scissor_rect(WGpuRenderPassEncoder encoder, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

void wgpu_render_pass_encoder_set_blend_constant(WGpuRenderPassEncoder encoder, double r, double g, double b, double a);
void wgpu_render_pass_encoder_set_stencil_reference(WGpuRenderPassEncoder encoder, uint32_t stencilValue);

void wgpu_render_pass_encoder_begin_occlusion_query(WGpuRenderPassEncoder encoder, int32_t queryIndex);
void wgpu_render_pass_encoder_end_occlusion_query(WGpuRenderPassEncoder encoder);

#define wgpu_render_pass_encoder_begin_pipeline_statistics_query wgpu_encoder_begin_pipeline_statistics_query
#define wgpu_render_pass_encoder_end_pipeline_statistics_query wgpu_encoder_end_pipeline_statistics_query

#define wgpu_render_pass_encoder_write_timestamp wgpu_encoder_write_timestamp

void wgpu_render_pass_encoder_execute_bundles(WGpuRenderPassEncoder encoder, WGpuRenderBundle *bundles, int numBundles);

#define wgpu_render_pass_encoder_end_pass wgpu_encoder_end_pass

#define wgpu_render_pass_encoder_set_bind_group wgpu_programmable_pass_encoder_set_bind_group
#define wgpu_render_pass_encoder_push_debug_group wgpu_programmable_pass_encoder_push_debug_group
#define wgpu_render_pass_encoder_pop_debug_group wgpu_programmable_pass_encoder_pop_debug_group
#define wgpu_render_pass_encoder_insert_debug_marker wgpu_programmable_pass_encoder_insert_debug_marker

#define wgpu_render_pass_encoder_set_pipeline wgpu_encoder_set_pipeline
#define wgpu_render_pass_encoder_set_index_buffer wgpu_render_encoder_base_set_index_buffer
#define wgpu_render_pass_encoder_set_vertex_buffer wgpu_render_encoder_base_set_vertex_buffer
#define wgpu_render_pass_encoder_draw wgpu_render_encoder_base_draw
#define wgpu_render_pass_encoder_draw_indexed wgpu_render_encoder_base_draw_indexed
#define wgpu_render_pass_encoder_draw_indirect wgpu_render_encoder_base_draw_indirect
#define wgpu_render_pass_encoder_draw_indexed_indirect wgpu_render_encoder_base_draw_indexed_indirect

/*
dictionary GPURenderPassDescriptor : GPUObjectDescriptorBase {
    required sequence<GPURenderPassColorAttachment> colorAttachments;
    GPURenderPassDepthStencilAttachment depthStencilAttachment;
    GPUQuerySet occlusionQuerySet;
};
*/
// Defined at the end of this file

/*
dictionary GPURenderPassColorAttachment {
    required GPUTextureView view;
    GPUTextureView resolveTarget;

    required (GPULoadOp or GPUColor) loadValue;
    required GPUStoreOp storeOp;
};
*/
// Defined at the end of this file

/*
dictionary GPURenderPassDepthStencilAttachment {
    required GPUTextureView view;

    required (GPULoadOp or float) depthLoadValue;
    required GPUStoreOp depthStoreOp;
    boolean depthReadOnly = false;

    required (GPULoadOp or GPUStencilValue) stencilLoadValue;
    required GPUStoreOp stencilStoreOp;
    boolean stencilReadOnly = false;
};
*/
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

/*
enum GPULoadOp {
    "load"
};
*/
typedef int WGPU_LOAD_OP;
#define WGPU_LOAD_OP_INVALID 0
#define WGPU_LOAD_OP_LOAD 171

/*
enum GPUStoreOp {
    "store",
    "discard"
};
*/
typedef int WGPU_STORE_OP;
#define WGPU_STORE_OP_INVALID 0
#define WGPU_STORE_OP_STORE 172
#define WGPU_STORE_OP_DISCARD 173

/*
[Exposed=Window]
interface GPURenderBundle {
};
GPURenderBundle includes GPUObjectBase;
*/
typedef int WGpuRenderBundle;
// Returns true if the given handle references a valid GPURenderBundle.
EM_BOOL wgpu_is_render_bundle(WGpuObjectBase object);

/*
dictionary GPURenderBundleDescriptor : GPUObjectDescriptorBase {
};
*/
typedef struct WGpuRenderBundleDescriptor
{
  uint32_t _dummyPadding; // Appease mixed C and C++ compilation to agree on non-zero struct size. Remove this once label is added
  // TODO add label
} WGpuRenderBundleDescriptor;

/*
[Exposed=Window]
interface GPURenderBundleEncoder {
    GPURenderBundle finish(optional GPURenderBundleDescriptor descriptor = {});
};
GPURenderBundleEncoder includes GPUObjectBase;
GPURenderBundleEncoder includes GPUProgrammablePassEncoder;
GPURenderBundleEncoder includes GPURenderEncoderBase;
*/
typedef int WGpuRenderBundleEncoder;
// Returns true if the given handle references a valid GPURenderBundleEncoder.
EM_BOOL wgpu_is_render_bundle_encoder(WGpuObjectBase object);
void wgpu_render_bundle_encoder_finish(const WGpuRenderBundleDescriptor *renderBundleDescriptor);

#define wgpu_render_bundle_encoder_set_bind_group wgpu_programmable_pass_encoder_set_bind_group
#define wgpu_render_bundle_encoder_push_debug_group wgpu_programmable_pass_encoder_push_debug_group
#define wgpu_render_bundle_encoder_pop_debug_group wgpu_programmable_pass_encoder_pop_debug_group
#define wgpu_render_bundle_encoder_insert_debug_marker wgpu_programmable_pass_encoder_insert_debug_marker

#define wgpu_render_bundle_encoder_set_pipeline wgpu_encoder_set_pipeline
#define wgpu_render_bundle_encoder_set_index_buffer wgpu_render_encoder_base_set_index_buffer
#define wgpu_render_bundle_encoder_set_vertex_buffer wgpu_render_encoder_base_set_vertex_buffer
#define wgpu_render_bundle_encoder_draw wgpu_render_encoder_base_draw
#define wgpu_render_bundle_encoder_draw_indexed wgpu_render_encoder_base_draw_indexed
#define wgpu_render_bundle_encoder_draw_indirect wgpu_render_encoder_base_draw_indirect
#define wgpu_render_bundle_encoder_draw_indexed_indirect wgpu_render_encoder_base_draw_indexed_indirect

/*
dictionary GPURenderBundleEncoderDescriptor : GPUObjectDescriptorBase {
    required sequence<GPUTextureFormat> colorFormats;
    GPUTextureFormat depthStencilFormat;
    GPUSize32 sampleCount = 1;
};
*/
typedef struct WGpuRenderBundleEncoderDescriptor
{
  int numColorFormats;
  const WGPU_TEXTURE_FORMAT *colorFormats;
  WGPU_TEXTURE_FORMAT depthStencilFormat;
  uint32_t sampleCount;
} WGpuRenderBundleEncoderDescriptor;

/*
[Exposed=Window]
interface GPUQueue {
    undefined submit(sequence<GPUCommandBuffer> commandBuffers);

    Promise<undefined> onSubmittedWorkDone();

    undefined writeBuffer(
        GPUBuffer buffer,
        GPUSize64 bufferOffset,
        [AllowShared] BufferSource data,
        optional GPUSize64 dataOffset = 0,
        optional GPUSize64 size);

    undefined writeTexture(
      GPUImageCopyTexture destination,
      [AllowShared] BufferSource data,
      GPUImageDataLayout dataLayout,
      GPUExtent3D size);

    undefined copyExternalImageToTexture(
        GPUImageCopyExternalImage source,
        GPUImageCopyTextureTagged destination,
        GPUExtent3D copySize);
};
GPUQueue includes GPUObjectBase;
*/
typedef int WGpuQueue;
// Returns true if the given handle references a valid GPUQueue.
EM_BOOL wgpu_is_queue(WGpuObjectBase object);

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

typedef void (*WGpuOnSubmittedWorkDoneCallback)(WGpuQueue queue, void *userData);
void wgpu_queue_set_on_submitted_work_done_callback(WGpuQueue queue, WGpuOnSubmittedWorkDoneCallback callback, void *userData);

void wgpu_queue_write_buffer(WGpuQueue queue, WGpuBuffer buffer, double_int53_t bufferOffset, void *data, double_int53_t size); // TODO other buffer sources?
void wgpu_queue_write_texture(WGpuQueue queue, const WGpuImageCopyTexture *destination, void *data, uint32_t bytesPerBlockRow, uint32_t blockRowsPerImage, uint32_t writeWidth, uint32_t writeHeight _WGPU_DEFAULT_VALUE(1), uint32_t writeDepthOrArrayLayers _WGPU_DEFAULT_VALUE(1)); // TODO other buffer sources?
void wgpu_queue_copy_external_image_to_texture(WGpuQueue queue, const WGpuImageCopyExternalImage *source, const WGpuImageCopyTextureTagged *destination, uint32_t copyWidth, uint32_t copyHeight _WGPU_DEFAULT_VALUE(1), uint32_t copyDepthOrArrayLayers _WGPU_DEFAULT_VALUE(1));

/*
[Exposed=Window]
interface GPUQuerySet {
    undefined destroy();
};
GPUQuerySet includes GPUObjectBase;
*/
typedef int WGpuQuerySet;
// Returns true if the given handle references a valid GPUQuerySet.
EM_BOOL wgpu_is_query_set(WGpuObjectBase object);

/*
dictionary GPUQuerySetDescriptor : GPUObjectDescriptorBase {
    required GPUQueryType type;
    required GPUSize32 count;
    sequence<GPUPipelineStatisticName> pipelineStatistics = [];
};
*/
typedef struct WGpuQuerySetDescriptor
{
  WGPU_QUERY_TYPE type;
  uint32_t count;
  int numPipelineStatistics;
  WGPU_PIPELINE_STATISTIC_NAME *pipelineStatistics;
} WGpuQuerySetDescriptor;

/*
enum GPUQueryType {
    "occlusion",
    "pipeline-statistics",
    "timestamp"
};
*/
typedef int WGPU_QUERY_TYPE;
#define WGPU_QUERY_TYPE_INVALID 0
#define WGPU_QUERY_TYPE_OCCLUSION 174
#define WGPU_QUERY_TYPE_PIPELINE_STATISTICS 175
#define WGPU_QUERY_TYPE_TIMESTAMP 176

/*
enum GPUPipelineStatisticName {
    "vertex-shader-invocations",
    "clipper-invocations",
    "clipper-primitives-out",
    "fragment-shader-invocations",
    "compute-shader-invocations"
};
*/
typedef int WGPU_PIPELINE_STATISTIC_NAME;
#define WGPU_PIPELINE_STATISTIC_NAME_INVALID 0
#define WGPU_PIPELINE_STATISTIC_NAME_VERTEX_SHADER_INVOCATIONS 177
#define WGPU_PIPELINE_STATISTIC_NAME_CLIPPER_INVOCATIONS 178
#define WGPU_PIPELINE_STATISTIC_NAME_CLIPPER_PRIMITIVES_OUT 179
#define WGPU_PIPELINE_STATISTIC_NAME_FRAGMENT_SHADER_INVOCATIONS 180
#define WGPU_PIPELINE_STATISTIC_NAME_COMPUTE_SHADER_INVOCATIONS 181

/*
[Exposed=Window]
interface GPUPresentationContext {
    undefined configure(GPUPresentationConfiguration configuration);
    undefined unconfigure();

    GPUTextureFormat getPreferredFormat(GPUAdapter adapter);
    GPUTexture getCurrentTexture();
};
*/
typedef int WGpuPresentationContext;
// Returns true if the given handle references a valid GPUCanvasContext.
EM_BOOL wgpu_is_presentation_context(WGpuObjectBase object);
// Configures the swap chain for this context.
void wgpu_presentation_context_configure(WGpuPresentationContext presentationContext, const WGpuPresentationConfiguration *config);
void wgpu_presentation_context_unconfigure(WGpuPresentationContext presentationContext);

// Returns an optimal GPUTextureFormat to use for swap chains with this context and the given device.
WGPU_TEXTURE_FORMAT wgpu_presentation_context_get_preferred_format(WGpuPresentationContext presentationContext, WGpuAdapter adapter);

WGpuTexture wgpu_presentation_context_get_current_texture(WGpuPresentationContext presentationContext);

/*
enum GPUCanvasCompositingAlphaMode {
    "opaque",
    "premultiplied",
};
*/
typedef int WGPU_CANVAS_COMPOSITING_ALPHA_MODE;
#define WGPU_CANVAS_COMPOSITING_ALPHA_MODE_INVALID 0
#define WGPU_CANVAS_COMPOSITING_ALPHA_MODE_OPAQUE 182
#define WGPU_CANVAS_COMPOSITING_ALPHA_MODE_PREMULTIPLIED 183

/*
dictionary GPUPresentationConfiguration : GPUObjectDescriptorBase {
    required GPUDevice device;
    required GPUTextureFormat format;
    GPUTextureUsageFlags usage = 0x10;  // GPUTextureUsage.RENDER_ATTACHMENT
    GPUCanvasCompositingAlphaMode compositingAlphaMode = "opaque";
    GPUExtent3D size;
};
*/
// Specified at the end of this file

/*
enum GPUDeviceLostReason {
    "destroyed",
};
*/
typedef int WGPU_DEVICE_LOST_REASON;
#define WGPU_DEVICE_LOST_REASON_INVALID 0
#define WGPU_DEVICE_LOST_REASON_DESTROYED 184

/*
[Exposed=Window]
interface GPUDeviceLostInfo {
    readonly attribute (GPUDeviceLostReason or undefined) reason;
    readonly attribute DOMString message;
};

partial interface GPUDevice {
    readonly attribute Promise<GPUDeviceLostInfo> lost;
};
*/
typedef void (*WGpuDeviceLostCallback)(WGpuDevice device, WGPU_DEVICE_LOST_REASON deviceLostReason, const char *message, void *userData);
void wgpu_device_set_lost_callback(WGpuDevice device, WGpuDeviceLostCallback callback, void *userData);

/*
enum GPUErrorFilter {
    "out-of-memory",
    "validation"
};
*/
typedef int WGPU_ERROR_FILTER;
#define WGPU_ERROR_FILTER_INVALID 0
#define WGPU_ERROR_FILTER_OUT_OF_MEMORY 185
#define WGPU_ERROR_FILTER_VALIDATION 186

/*
[Exposed=Window]
interface GPUOutOfMemoryError {
    constructor();
};

[Exposed=Window]
interface GPUValidationError {
    constructor(DOMString message);
    readonly attribute DOMString message;
};

typedef (GPUOutOfMemoryError or GPUValidationError) GPUError;

partial interface GPUDevice {
    undefined pushErrorScope(GPUErrorFilter filter);
    Promise<GPUError?> popErrorScope();
};
*/
void wgpu_device_push_error_scope(WGpuDevice device, WGPU_ERROR_FILTER filter);

typedef void (*WGpuDeviceErrorCallback)(WGpuDevice device, WGPU_ERROR_FILTER errorType, const char *errorMessage, void *userData);
void wgpu_device_pop_error_scope_async(WGpuDevice device, WGpuDeviceErrorCallback callback, void *userData);

/*
[
    Exposed=(Window, DedicatedWorker)
]
interface GPUUncapturedErrorEvent : Event {
    constructor(
        DOMString type,
        GPUUncapturedErrorEventInit gpuUncapturedErrorEventInitDict
    );
    [SameObject] readonly attribute GPUError error;
};

dictionary GPUUncapturedErrorEventInit : EventInit {
    required GPUError error;
};

partial interface GPUDevice {
    [Exposed=(Window, DedicatedWorker)]
    attribute EventHandler onuncapturederror;
};
*/

// registers a device uncapturederror event callback. Call with 0 pointer to unregister. Only one callback handler is supported, new call overwrites previous
void wgpu_device_set_uncapturederror_callback(WGpuDevice device, WGpuDeviceErrorCallback callback, void *userData);

/*
typedef [EnforceRange] unsigned long GPUBufferDynamicOffset;
typedef [EnforceRange] unsigned long GPUStencilValue;
typedef [EnforceRange] unsigned long GPUSampleMask;
typedef [EnforceRange] long GPUDepthBias;
*/
// These do not get their own typedefs for readability, but use uint32_t in headers.

/*
typedef [EnforceRange] unsigned long long GPUSize64;
*/
// No custom typedef for readability, use double_int53_t

/*
typedef [EnforceRange] unsigned long GPUIntegerCoordinate;
typedef [EnforceRange] unsigned long GPUIndex32;
typedef [EnforceRange] unsigned long GPUSize32;
*/
// These do not get their own typedefs for readability, but use uint32_t in headers.

/*
typedef [EnforceRange] long GPUSignedOffset32;
*/
// No custom typedef for readability, use int32_t.

/*
typedef unsigned long GPUFlagsConstant;
*/
// No custom typedef for readability, use uint32_t.

/*
dictionary GPUColorDict {
    required double r;
    required double g;
    required double b;
    required double a;
};
typedef (sequence<double> or GPUColorDict) GPUColor;
*/
typedef struct WGpuColor
{
  double r, g, b, a;
} WGpuColor;

/*
dictionary GPUOrigin2DDict {
    GPUIntegerCoordinate x = 0;
    GPUIntegerCoordinate y = 0;
};
typedef (sequence<GPUIntegerCoordinate> or GPUOrigin2DDict) GPUOrigin2D;
*/
typedef struct WGpuOrigin2D
{
  int x, y;
} WGpuOrigin2D;

/*
dictionary GPUOrigin3DDict {
    GPUIntegerCoordinate x = 0;
    GPUIntegerCoordinate y = 0;
    GPUIntegerCoordinate z = 0;
};
typedef (sequence<GPUIntegerCoordinate> or GPUOrigin3DDict) GPUOrigin3D;
*/
typedef struct WGpuOrigin3D
{
  int x, y, z;
} WGpuOrigin3D;

/*
dictionary GPUExtent3DDict {
    required GPUIntegerCoordinate width;
    GPUIntegerCoordinate height = 1;
    GPUIntegerCoordinate depthOrArrayLayers = 1;
};
typedef (sequence<GPUIntegerCoordinate> or GPUExtent3DDict) GPUExtent3D;
*/
typedef struct WGpuExtent3D
{
  int width;
  int height; // = 1;
  int depthOrArrayLayers; // = 1;
} WGpuExtent3D;
extern const WGpuExtent3D WGPU_EXTENT_3D_DEFAULT_INITIALIZER;

////////////////////////////////////////////////////////
// Sorted struct definitions for proper C parsing order:

typedef struct WGpuPresentationConfiguration
{
  WGpuDevice device;
  WGPU_TEXTURE_FORMAT format;
  WGPU_TEXTURE_USAGE_FLAGS usage;
  WGPU_CANVAS_COMPOSITING_ALPHA_MODE compositingAlphaMode;
  WGpuExtent3D size; // If size.width == 0 (as default initialized via WGPU_PRESENTATION_CONFIGURATION_DEFAULT_INITIALIZER), then full screen size is used.
} WGpuPresentationConfiguration;
extern const WGpuPresentationConfiguration WGPU_PRESENTATION_CONFIGURATION_DEFAULT_INITIALIZER;

typedef struct WGpuRenderPassDescriptor
{
  int numColorAttachments;
  const WGpuRenderPassColorAttachment *colorAttachments;
  WGpuRenderPassDepthStencilAttachment depthStencilAttachment;
  WGpuQuerySet occlusionQuerySet;
} WGpuRenderPassDescriptor;

typedef struct WGpuRenderPassColorAttachment
{
  WGpuTextureView view;
  WGpuTextureView resolveTarget;

  WGPU_STORE_OP storeOp;

  WGPU_LOAD_OP loadOp; // Either WGPU_LOAD_OP_CONSTANT_VALUE (== default, 0) or WGPU_LOAD_OP_LOAD
  WGpuColor loadColor;
} WGpuRenderPassColorAttachment;

typedef struct WGpuImageCopyExternalImage
{
  WGpuObjectBase source; // TODO add wgpu_acquire_dom_element_using_css_selector(const char *cssSelector); and wgpu_release_dom_element_using_css_selector(WGpuObjectBase object);
  WGpuOrigin2D origin;
} WGpuImageCopyExternalImage;
extern const WGpuImageCopyExternalImage WGPU_IMAGE_COPY_EXTERNAL_IMAGE_DEFAULT_INITIALIZER;

typedef struct WGpuImageCopyTexture
{
  WGpuTexture texture;
  uint32_t mipLevel;
  WGpuOrigin3D origin;
  WGPU_TEXTURE_ASPECT aspect;
} WGpuImageCopyTexture;
extern const WGpuImageCopyTexture WGPU_IMAGE_COPY_TEXTURE_DEFAULT_INITIALIZER;

typedef struct WGpuImageCopyTextureTagged
{
  // WGpuImageCopyTexture part:
  WGpuTexture texture;
  uint32_t mipLevel;
  WGpuOrigin3D origin;
  WGPU_TEXTURE_ASPECT aspect;

  WGPU_PREDEFINED_COLOR_SPACE colorSpace; // = "srgb";
  EM_BOOL premultipliedAlpha; // = false;
} WGpuImageCopyTextureTagged;
extern const WGpuImageCopyTextureTagged WGPU_IMAGE_COPY_TEXTURE_TAGGED_DEFAULT_INITIALIZER;

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

typedef struct WGpuRenderPipelineDescriptor
{
  WGpuVertexState vertex;
  WGpuPrimitiveState primitive;
  WGpuDepthStencilState depthStencil;
  WGpuMultisampleState multisample;
  WGpuFragmentState fragment;
} WGpuRenderPipelineDescriptor;
extern const WGpuRenderPipelineDescriptor WGPU_RENDER_PIPELINE_DESCRIPTOR_DEFAULT_INITIALIZER;

typedef struct WGpuBindGroupLayoutEntry
{
  uint32_t binding;
  WGPU_SHADER_STAGE_FLAGS visibility;
  WGPU_BIND_GROUP_LAYOUT_TYPE type;
  uint32_t _dummyPadding64Bits; // Explicitly present to pad 'buffer' to 64-bit alignment

  union {
    WGpuBufferBindingLayout buffer;
    WGpuSamplerBindingLayout sampler;
    WGpuTextureBindingLayout texture;
    WGpuStorageTextureBindingLayout storageTexture;
    WGpuExternalTextureBindingLayout externalTexture;
  } layout;
} WGpuBindGroupLayoutEntry;
extern const WGpuBindGroupLayoutEntry WGPU_BUFFER_BINDING_LAYOUT_ENTRY_DEFAULT_INITIALIZER;

#ifdef __cplusplus
} // ~extern "C"
#endif
