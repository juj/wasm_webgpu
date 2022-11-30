#pragma once

#ifdef __EMSCRIPTEN__
#include <emscripten/html5.h>
#endif

#ifdef __clang__
// The internal struct member offset layout is extremely important when marshalling structs to JS,
// so never let the compiler add any padding (we manually & explicitly make the fields the right size)
#pragma clang diagnostic push
#elif defined(_MSC_VER)
#pragma warning(push)
#endif

#ifdef __clang__
#pragma clang diagnostic error "-Wpadded"
#endif

#ifdef _MSC_VER
#pragma warning(disable : 4200) // Disable MSVC complaining about zero-sized arrays for copy/move assignment in WGpuCompilationInfo
#endif

#include "lib_webgpu_fwd.h"

// Some WebGPU JS API functions have default parameters so that the user can omit passing them.
// These defaults are carried through to these headers. However C does not support default parameters to
// functions, so enable the default parameters only when called from C++ code.
#ifdef __cplusplus
#define _WGPU_DEFAULT_VALUE(x) = x
#else
#define _WGPU_DEFAULT_VALUE(x)
#endif

#ifdef __GNUC__
#define WGPU_INFINITY __builtin_inf()
#else
#include <math.h>
#define WGPU_INFINITY ((double)INFINITY)
#endif

#ifndef __EMSCRIPTEN__
#define EM_BOOL int
#define EM_TRUE 1
#define EM_FALSE 0
#endif

#ifdef __cplusplus
extern "C" {
#endif


// Returns the number of WebGPU objects referenced by the WebGPU JS library.
uint32_t wgpu_get_num_live_objects(void);

// Calls .destroy() on the given WebGPU object (if it has such a member function) and releases the JS side reference to it. Use this function
// to release memory for all types of WebGPU objects after you are done with them.
// Note that deleting a GPUTexture will also delete all GPUTextureViews that have been created from it.
// Similar to free(), calling wgpu_object_destroy() on null, or an object that has already been destroyed before is safe, and no-op. (so no need to
// do excess "if (wgpuObject) wgpu_object_destroy(wgpuObject);")
void wgpu_object_destroy(WGpuObjectBase wgpuObject);

// Deinitializes all initialized WebGPU objects.
void wgpu_destroy_all_objects(void);

// Acquires a canvas context from a canvas by calling canvas.getCanvasContext().
#ifdef __EMSCRIPTEN__
WGpuCanvasContext wgpu_canvas_get_webgpu_context(const char *canvasSelector NOTNULL);
#elif defined (_WIN32)
WGpuCanvasContext wgpu_canvas_get_webgpu_context(HWND hwnd);
#else
#error Targeting currently unsupported platform! (no declaration for wgpu_canvas_get_webgpu_context())
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The ordering and structure of this remainder of this file follows the official WebGPU WebIDL definitions at https://www.w3.org/TR/webgpu/#idl-index
// This is so that when the official IDL is modified, the modifications can be easily diffed here for updates.

/*
interface mixin GPUObjectBase {
    attribute USVString label;
};
*/
// Returns true if the given handle references a valid WebGPU object
EM_BOOL wgpu_is_valid_object(WGpuObjectBase obj);
// Set a human-readable label for the given WebGPU object. Pass an empty string "" to clear a label.
void wgpu_object_set_label(WGpuObjectBase obj, const char *label NOTNULL);
// Gets the human-readable label of a WebGPU object. If dstLabelSize is too short to
// contain the label string, then the label is truncated.
// dstLabelSize: length of dstLabel array in bytes.
// Returns the number of bytes written (excluding null byte at end).
int wgpu_object_get_label(WGpuObjectBase obj, char *dstLabel NOTNULL, uint32_t dstLabelSize);

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

/*
[Exposed=(Window, DedicatedWorker)]
interface GPUSupportedLimits {
    readonly attribute unsigned long maxTextureDimension1D;
    readonly attribute unsigned long maxTextureDimension2D;
    readonly attribute unsigned long maxTextureDimension3D;
    readonly attribute unsigned long maxTextureArrayLayers;
    readonly attribute unsigned long maxBindGroups;
    readonly attribute unsigned long maxBindingsPerBindGroup;
    readonly attribute unsigned long maxDynamicUniformBuffersPerPipelineLayout;
    readonly attribute unsigned long maxDynamicStorageBuffersPerPipelineLayout;
    readonly attribute unsigned long maxSampledTexturesPerShaderStage;
    readonly attribute unsigned long maxSamplersPerShaderStage;
    readonly attribute unsigned long maxStorageBuffersPerShaderStage;
    readonly attribute unsigned long maxStorageTexturesPerShaderStage;
    readonly attribute unsigned long maxUniformBuffersPerShaderStage;
    readonly attribute unsigned long long maxUniformBufferBindingSize;
    readonly attribute unsigned long long maxStorageBufferBindingSize;
    readonly attribute unsigned long minUniformBufferOffsetAlignment;
    readonly attribute unsigned long minStorageBufferOffsetAlignment;
    readonly attribute unsigned long maxVertexBuffers;
    readonly attribute unsigned long long maxBufferSize;
    readonly attribute unsigned long maxVertexAttributes;
    readonly attribute unsigned long maxVertexBufferArrayStride;
    readonly attribute unsigned long maxInterStageShaderComponents;
    readonly attribute unsigned long maxInterStageShaderVariables;
    readonly attribute unsigned long maxColorAttachments;
    readonly attribute unsigned long maxColorAttachmentBytesPerSample;
    readonly attribute unsigned long maxComputeWorkgroupStorageSize;
    readonly attribute unsigned long maxComputeInvocationsPerWorkgroup;
    readonly attribute unsigned long maxComputeWorkgroupSizeX;
    readonly attribute unsigned long maxComputeWorkgroupSizeY;
    readonly attribute unsigned long maxComputeWorkgroupSizeZ;
    readonly attribute unsigned long maxComputeWorkgroupsPerDimension;
};
*/
typedef struct WGpuSupportedLimits
{
  // See the table in https://www.w3.org/TR/webgpu/#limits for the minimum/maximum
  // default values for these limits.

  // 64-bit fields must be present first before the 32-bit fields in this struct.
  uint64_t maxUniformBufferBindingSize; // required >= 16384
  uint64_t maxStorageBufferBindingSize; // required >= 128*1024*1024 (128MB)
  uint64_t maxBufferSize;               // required >= 256*1024*1024 (256MB)

  uint32_t maxTextureDimension1D; // required >= 8192
  uint32_t maxTextureDimension2D; // required >= 8192
  uint32_t maxTextureDimension3D; // required >= 2048
  uint32_t maxTextureArrayLayers; // required >= 2048
  uint32_t maxBindGroups; // required >= 4
  uint32_t maxBindingsPerBindGroup; // required >= 640
  uint32_t maxDynamicUniformBuffersPerPipelineLayout; // required >= 8
  uint32_t maxDynamicStorageBuffersPerPipelineLayout; // required >= 4
  uint32_t maxSampledTexturesPerShaderStage; // required >= 16
  uint32_t maxSamplersPerShaderStage; // required >= 16
  uint32_t maxStorageBuffersPerShaderStage; // required >= 8
  uint32_t maxStorageTexturesPerShaderStage; // required >= 8
  uint32_t maxUniformBuffersPerShaderStage; // required >= 12
  uint32_t minUniformBufferOffsetAlignment; // required >= 256 bytes
  uint32_t minStorageBufferOffsetAlignment; // required >= 256 bytes
  uint32_t maxVertexBuffers; // required >= 8
  uint32_t maxVertexAttributes; // required >= 16
  uint32_t maxVertexBufferArrayStride; // required >= 2048
  uint32_t maxInterStageShaderComponents; // required >= 60
  uint32_t maxInterStageShaderVariables; // required >= 16
  uint32_t maxColorAttachments; // required >= 8
  uint32_t maxColorAttachmentBytesPerSample; // required >= 32
  uint32_t maxComputeWorkgroupStorageSize; // required >= 16384 bytes
  uint32_t maxComputeInvocationsPerWorkgroup; // required >= 256
  uint32_t maxComputeWorkgroupSizeX; // required >= 256
  uint32_t maxComputeWorkgroupSizeY; // required >= 256
  uint32_t maxComputeWorkgroupSizeZ; // required >= 64
  uint32_t _explicitPaddingFor8BytesAlignedSize;
} WGpuSupportedLimits;

/*
[Exposed=(Window, DedicatedWorker)]
interface GPUSupportedFeatures {
    readonly setlike<DOMString>;
};
*/
/*
enum GPUFeatureName {
    "depth-clip-control",
    "depth32float-stencil8",
    "texture-compression-bc",
    "texture-compression-etc2",
    "texture-compression-astc",
    "timestamp-query",
    "indirect-first-instance",
    "shader-f16",
    "bgra8unorm-storage",
    "rg11b10ufloat-renderable"
};
*/
typedef int WGPU_FEATURES_BITFIELD;
#define WGPU_FEATURE_DEPTH_CLIP_CONTROL         0x01
#define WGPU_FEATURE_DEPTH32FLOAT_STENCIL8      0x02
#define WGPU_FEATURE_TEXTURE_COMPRESSION_BC     0x04
#define WGPU_FEATURE_TEXTURE_COMPRESSION_ETC2   0x08
#define WGPU_FEATURE_TEXTURE_COMPRESSION_ASTC   0x10
#define WGPU_FEATURE_TIMESTAMP_QUERY            0x20
#define WGPU_FEATURE_INDIRECT_FIRST_INSTANCE    0x40
#define WGPU_FEATURE_SHADER_F16                 0x80
#define WGPU_FEATURE_BGRA8UNORM_STORAGE        0x100
#define WGPU_FEATURE_RG11B10UFLOAT_RENDERABLE  0x200

/*
// WebGPU reuses the color space enum from the HTML Canvas specification:
   https://html.spec.whatwg.org/multipage/canvas.html#predefinedcolorspace
   Because of that reason, it is prefixed here with HTML_ as opposed to WGPU_.
enum PredefinedColorSpace {
    "srgb",
    "display-p3"
};
*/
typedef int HTML_PREDEFINED_COLOR_SPACE;
#define HTML_PREDEFINED_COLOR_SPACE_INVALID 0
#define HTML_PREDEFINED_COLOR_SPACE_SRGB 1
#define HTML_PREDEFINED_COLOR_SPACE_DISPLAY_P3 2

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUAdapterInfo {
    readonly attribute DOMString vendor;
    readonly attribute DOMString architecture;
    readonly attribute DOMString device;
    readonly attribute DOMString description;
};
*/
typedef struct WGpuAdapterInfo
{
  char vendor[512];
  char architecture[512];
  char device[512];
  char description[512];
} WGpuAdapterInfo;

/*
interface mixin NavigatorGPU {
    [SameObject, SecureContext] readonly attribute GPU gpu;
};
Navigator includes NavigatorGPU;
WorkerNavigator includes NavigatorGPU;

[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPU {
    Promise<GPUAdapter?> requestAdapter(optional GPURequestAdapterOptions options = {});
    GPUTextureFormat getPreferredCanvasFormat();
};
*/
typedef void (*WGpuRequestAdapterCallback)(WGpuAdapter adapter, void *userData);
// Requests an adapter from the user agent. The user agent chooses whether to return an adapter, and, if so, chooses according to the provided options.
// If WebGPU is not supported by the browser, returns EM_FALSE.
// Otherwise returns EM_TRUE, and the callback will resolve later with an ID handle to the adapter.
// The callback will also be resolved in the event of an initialization failure, but the ID handle
// passed to the callback will then be zero.
// options: may be null to request an adapter without specific options.
EM_BOOL navigator_gpu_request_adapter_async(const WGpuRequestAdapterOptions *options, WGpuRequestAdapterCallback adapterCallback, void *userData);
// Requests a WebGPU adapter synchronously. Requires building with -sASYNCIFY=1 linker flag to work.
// options: may be null to request an adapter without specific options.
WGpuAdapter navigator_gpu_request_adapter_sync(const WGpuRequestAdapterOptions *options);

// Like above, but tiny code size without options.
void navigator_gpu_request_adapter_async_simple(WGpuRequestAdapterCallback adapterCallback);
WGpuAdapter navigator_gpu_request_adapter_sync_simple(void);

WGPU_TEXTURE_FORMAT navigator_gpu_get_preferred_canvas_format(void);

/*
dictionary GPURequestAdapterOptions {
    GPUPowerPreference powerPreference;
    boolean forceFallbackAdapter = false;
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
  EM_BOOL forceFallbackAdapter;
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
#define WGPU_POWER_PREFERENCE_LOW_POWER 1
#define WGPU_POWER_PREFERENCE_HIGH_PERFORMANCE 2

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUAdapter {
    [SameObject] readonly attribute GPUSupportedFeatures features;
    [SameObject] readonly attribute WGpuSupportedLimits limits;
    readonly attribute boolean isFallbackAdapter;

    Promise<GPUDevice> requestDevice(optional GPUDeviceDescriptor descriptor = {});
    Promise<GPUAdapterInfo> requestAdapterInfo(optional sequence<DOMString> unmaskHints = []);
};
*/
typedef WGpuObjectBase WGpuAdapter;
// Returns true if the given handle references a valid GPUAdapter.
EM_BOOL wgpu_is_adapter(WGpuObjectBase object);

// Returns a bitfield of all the supported features on this adapter.
WGPU_FEATURES_BITFIELD wgpu_adapter_or_device_get_features(WGpuAdapter adapter);
#define wgpu_adapter_get_features wgpu_adapter_or_device_get_features

// Returns true if the given feature is supported by this adapter.
EM_BOOL wgpu_adapter_or_device_supports_feature(WGpuAdapter adapter, WGPU_FEATURES_BITFIELD feature);
#define wgpu_adapter_supports_feature wgpu_adapter_or_device_supports_feature

// Populates the adapter.limits field of the given adapter to the provided structure.
void wgpu_adapter_or_device_get_limits(WGpuAdapter adapter, WGpuSupportedLimits *limits NOTNULL);
#define wgpu_adapter_get_limits wgpu_adapter_or_device_get_limits

EM_BOOL wgpu_adapter_is_fallback_adapter(WGpuAdapter adapter);

typedef void (*WGpuRequestDeviceCallback)(WGpuDevice device, void *userData);

void wgpu_adapter_request_device_async(WGpuAdapter adapter, const WGpuDeviceDescriptor *descriptor NOTNULL, WGpuRequestDeviceCallback deviceCallback, void *userData);
// Requests a WebGPU device synchronously. Requires building with -sASYNCIFY=1 linker flag to work.
WGpuDevice wgpu_adapter_request_device_sync(WGpuAdapter adapter, const WGpuDeviceDescriptor *descriptor NOTNULL);

// Like above, but tiny code size without options.
void wgpu_adapter_request_device_async_simple(WGpuAdapter adapter, WGpuRequestDeviceCallback deviceCallback);
WGpuDevice wgpu_adapter_request_device_sync_simple(WGpuAdapter adapter);

// Callback function type that is called when GPUAdapter information has been obtained. The information will be reported in a struct of
// type WGpuAdapterInfo. Do not hold on to this struct pointer after the duration of this call (but make a copy of the contents if desirable)
typedef void (*WGpuRequestAdapterInfoCallback)(WGpuAdapter adapter, const WGpuAdapterInfo *adapterInfo NOTNULL, void *userData);

// Begins a process to asynchronously request GPUAdapter information. 'unmaskHints' should be a null-terminated array of null-terminated strings
// of which information to retrieve, e.g. { "vendor", "architecture", "device", "description", 0 }.
void wgpu_adapter_request_adapter_info_async(WGpuAdapter adapter, const char **unmaskHints, WGpuRequestAdapterInfoCallback callback, void *userData);
// TODO: Create asyncified wgpu_adapter_request_adapter_info_sync() function.

/*
dictionary GPUQueueDescriptor : GPUObjectDescriptorBase {
};
*/
typedef struct WGpuQueueDescriptor
{
  const char *label;
} WGpuQueueDescriptor;

/*
dictionary GPUDeviceDescriptor : GPUObjectDescriptorBase {
    sequence<GPUFeatureName> requiredFeatures = [];
    record<DOMString, GPUSize64> requiredLimits = {};
    GPUQueueDescriptor defaultQueue = {};
};
*/
typedef struct WGpuDeviceDescriptor
{
  WGPU_FEATURES_BITFIELD requiredFeatures;
  uint32_t _explicitPaddingFor8BytesAlignedSize; // alignof(WGpuSupportedLimits) is 64-bit, hence explicitly show a padding here.
  WGpuSupportedLimits requiredLimits;
  WGpuQueueDescriptor defaultQueue;
  uint32_t _explicitPaddingFor8BytesAlignedSize2;
} WGpuDeviceDescriptor;
extern const WGpuDeviceDescriptor WGPU_DEVICE_DESCRIPTOR_DEFAULT_INITIALIZER;

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
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
typedef WGpuObjectBase WGpuDevice;
// Returns true if the given handle references a valid GPUDevice.
EM_BOOL wgpu_is_device(WGpuObjectBase object);

#define wgpu_device_get_features wgpu_adapter_or_device_get_features
#define wgpu_device_supports_feature wgpu_adapter_or_device_supports_feature
#define wgpu_device_get_limits wgpu_adapter_or_device_get_limits

WGpuQueue wgpu_device_get_queue(WGpuDevice device);

WGpuBuffer wgpu_device_create_buffer(WGpuDevice device, const WGpuBufferDescriptor *bufferDesc NOTNULL);
WGpuTexture wgpu_device_create_texture(WGpuDevice device, const WGpuTextureDescriptor *textureDesc NOTNULL);
WGpuSampler wgpu_device_create_sampler(WGpuDevice device, const WGpuSamplerDescriptor *samplerDesc NOTNULL);
WGpuExternalTexture wgpu_device_import_external_texture(WGpuDevice device, const WGpuExternalTextureDescriptor *externalTextureDesc NOTNULL);

// N.b. not currently using signature WGpuBindGroupLayout wgpu_device_create_bind_group_layout(WGpuDevice device, const WGpuBindGroupLayoutDescriptor *bindGroupLayoutDesc);
// since WGpuBindGroupLayoutDescriptor is a single element struct consisting only of a single array. (if it is expanded in the future, switch to using that signature)
WGpuBindGroupLayout wgpu_device_create_bind_group_layout(WGpuDevice device, const WGpuBindGroupLayoutEntry *bindGroupLayoutEntries, int numEntries);

// N.b. not currently using signature WGpuPipelineLayout wgpu_device_create_pipeline_layout(WGpuDevice device, const WGpuPipelineLayoutDescriptor *pipelineLayoutDesc);
// since WGpuPipelineLayoutDescriptor is a single element struct consisting only of a single array. (if it is expanded in the future, switch to using that signature)
WGpuPipelineLayout wgpu_device_create_pipeline_layout(WGpuDevice device, const WGpuBindGroupLayout *bindGroupLayouts, int numLayouts);

// N.b. not currently using signature WGpuBindGroup wgpu_device_create_bind_group(WGpuDevice device, const WGpuBindGroupDescriptor *bindGroupDesc);
// since WGpuBindGroupDescriptor is a such a light struct. (if it is expanded in the future, switch to using that signature)
WGpuBindGroup wgpu_device_create_bind_group(WGpuDevice device, WGpuBindGroupLayout bindGroupLayout, const WGpuBindGroupEntry *entries, int numEntries);

WGpuShaderModule wgpu_device_create_shader_module(WGpuDevice device, const WGpuShaderModuleDescriptor *shaderModuleDesc NOTNULL);

typedef void (*WGpuCreatePipelineCallback)(WGpuDevice device, WGpuPipelineBase pipeline, void *userData);

// N.b. not currently using signature WGpuComputePipeline wgpu_device_create_compute_pipeline(WGpuDevice device, const WGpuComputePipelineDescriptor *computePipelineDesc);
// since WGpuComputePipelineDescriptor is a such a light struct. (if it is expanded in the future, switch to using that signature)
WGpuComputePipeline wgpu_device_create_compute_pipeline(WGpuDevice device, WGpuShaderModule computeModule, const char *entryPoint NOTNULL, WGpuPipelineLayout layout, const WGpuPipelineConstant *constants, int numConstants);
void wgpu_device_create_compute_pipeline_async(WGpuDevice device, WGpuShaderModule computeModule, const char *entryPoint NOTNULL, WGpuPipelineLayout layout, const WGpuPipelineConstant *constants, int numConstants, WGpuCreatePipelineCallback callback, void *userData);

WGpuRenderPipeline wgpu_device_create_render_pipeline(WGpuDevice device, const WGpuRenderPipelineDescriptor *renderPipelineDesc NOTNULL);
void wgpu_device_create_render_pipeline_async(WGpuDevice device, const WGpuRenderPipelineDescriptor *renderPipelineDesc NOTNULL, WGpuCreatePipelineCallback callback, void *userData);

WGpuCommandEncoder wgpu_device_create_command_encoder(WGpuDevice device, const WGpuCommandEncoderDescriptor *commandEncoderDesc);
// Same as above, but without any descriptor args.
WGpuCommandEncoder wgpu_device_create_command_encoder_simple(WGpuDevice device);

WGpuRenderBundleEncoder wgpu_device_create_render_bundle_encoder(WGpuDevice device, const WGpuRenderBundleEncoderDescriptor *renderBundleEncoderDesc NOTNULL);

WGpuQuerySet wgpu_device_create_query_set(WGpuDevice device, const WGpuQuerySetDescriptor *querySetDesc NOTNULL);

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUBuffer {
    readonly attribute GPUSize64 size;
    readonly attribute GPUBufferUsageFlags usage;

    readonly attribute GPUBufferMapState mapState;

    Promise<undefined> mapAsync(GPUMapModeFlags mode, optional GPUSize64 offset = 0, optional GPUSize64 size);
    ArrayBuffer getMappedRange(optional GPUSize64 offset = 0, optional GPUSize64 size);
    undefined unmap();

    undefined destroy();
};
GPUBuffer includes GPUObjectBase;
*/
typedef WGpuObjectBase WGpuBuffer;
// Returns true if the given handle references a valid GPUBuffer.
EM_BOOL wgpu_is_buffer(WGpuObjectBase object);

// TODO: Add error status to map callback for when mapAsync() promise rejects.
typedef void (*WGpuBufferMapCallback)(WGpuBuffer buffer, void *userData, WGPU_MAP_MODE_FLAGS mode, double_int53_t offset, double_int53_t size);
#define WGPU_MAP_MAX_LENGTH -1
void wgpu_buffer_map_async(WGpuBuffer buffer, WGpuBufferMapCallback callback, void *userData, WGPU_MAP_MODE_FLAGS mode, double_int53_t offset _WGPU_DEFAULT_VALUE(0), double_int53_t size _WGPU_DEFAULT_VALUE(WGPU_MAP_MAX_LENGTH));

// Maps the given WGpuBuffer synchronously. Requires building with -sASYNCIFY=1 linker flag to work.
void wgpu_buffer_map_sync(WGpuBuffer buffer, WGPU_MAP_MODE_FLAGS mode, double_int53_t offset _WGPU_DEFAULT_VALUE(0), double_int53_t size _WGPU_DEFAULT_VALUE(WGPU_MAP_MAX_LENGTH));

#define WGPU_BUFFER_GET_MAPPED_RANGE_FAILED ((double_int53_t)-1)

// Calls buffer.getMappedRange(). Returns `startOffset`, which is used as an ID token to wgpu_buffer_read/write_mapped_range().
// If .getMappedRange() fails, the value WGPU_BUFFER_GET_MAPPED_RANGE_FAILED (-1) will be returned.
double_int53_t wgpu_buffer_get_mapped_range(WGpuBuffer buffer, double_int53_t startOffset, double_int53_t size _WGPU_DEFAULT_VALUE(WGPU_MAP_MAX_LENGTH));
void wgpu_buffer_read_mapped_range(WGpuBuffer buffer, double_int53_t startOffset, double_int53_t subOffset, void *dst NOTNULL, double_int53_t size);
void wgpu_buffer_write_mapped_range(WGpuBuffer buffer, double_int53_t startOffset, double_int53_t subOffset, const void *src NOTNULL, double_int53_t size);
void wgpu_buffer_unmap(WGpuBuffer buffer);

// Getters for retrieving buffer properties:
double_int53_t wgpu_buffer_size(WGpuBuffer buffer);
WGPU_BUFFER_USAGE_FLAGS wgpu_buffer_usage(WGpuBuffer buffer);
WGPU_BUFFER_MAP_STATE wgpu_buffer_map_state(WGpuBuffer buffer);

/*
enum GPUBufferMapState {
    "unmapped",
    "pending",
    "mapped"
};
*/
typedef int WGPU_BUFFER_MAP_STATE;
#define WGPU_BUFFER_MAP_STATE_INVALID  0
#define WGPU_BUFFER_MAP_STATE_UNMAPPED 1
#define WGPU_BUFFER_MAP_STATE_PENDING  2
#define WGPU_BUFFER_MAP_STATE_MAPPED   3

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
[Exposed=(Window, DedicatedWorker)]
namespace GPUBufferUsage {
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
[Exposed=(Window, DedicatedWorker)]
namespace GPUMapMode {
    const GPUFlagsConstant READ  = 0x0001;
    const GPUFlagsConstant WRITE = 0x0002;
};
*/
typedef int WGPU_MAP_MODE_FLAGS;
#define WGPU_MAP_MODE_READ   0x1
#define WGPU_MAP_MODE_WRITE  0x2

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUTexture {
    GPUTextureView createView(optional GPUTextureViewDescriptor descriptor = {});

    undefined destroy();

    readonly attribute GPUIntegerCoordinate width;
    readonly attribute GPUIntegerCoordinate height;
    readonly attribute GPUIntegerCoordinate depthOrArrayLayers;
    readonly attribute GPUIntegerCoordinate mipLevelCount;
    readonly attribute GPUSize32 sampleCount;
    readonly attribute GPUTextureDimension dimension;
    readonly attribute GPUTextureFormat format;
    readonly attribute GPUTextureUsageFlags usage;
};
GPUTexture includes GPUObjectBase;
*/
typedef WGpuObjectBase WGpuTexture;
// Returns true if the given handle references a valid GPUTexture.
EM_BOOL wgpu_is_texture(WGpuObjectBase object);
// textureViewDesc: Can be null, in which case a default view is created.
WGpuTextureView wgpu_texture_create_view(WGpuTexture texture, const WGpuTextureViewDescriptor *textureViewDesc _WGPU_DEFAULT_VALUE(0));
// Same as above, but does not take any descriptor args.
WGpuTextureView wgpu_texture_create_view_simple(WGpuTexture texture);

// Getters for retrieving texture properties:
uint32_t wgpu_texture_width(WGpuTexture texture);
uint32_t wgpu_texture_height(WGpuTexture texture);
uint32_t wgpu_texture_depth_or_array_layers(WGpuTexture texture);
uint32_t wgpu_texture_mip_level_count(WGpuTexture texture);
uint32_t wgpu_texture_sample_count(WGpuTexture texture);
WGPU_TEXTURE_DIMENSION wgpu_texture_dimension(WGpuTexture texture);
WGPU_TEXTURE_FORMAT wgpu_texture_format(WGpuTexture texture);
WGPU_TEXTURE_USAGE_FLAGS wgpu_texture_usage(WGpuTexture texture);
/*
dictionary GPUTextureDescriptor : GPUObjectDescriptorBase {
    required GPUExtent3D size;
    GPUIntegerCoordinate mipLevelCount = 1;
    GPUSize32 sampleCount = 1;
    GPUTextureDimension dimension = "2d";
    required GPUTextureFormat format;
    required GPUTextureUsageFlags usage;
    sequence<GPUTextureFormat> viewFormats = [];
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
  int numViewFormats;
  WGPU_TEXTURE_FORMAT *viewFormats;
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
#define WGPU_TEXTURE_DIMENSION_1D 1
#define WGPU_TEXTURE_DIMENSION_2D 2
#define WGPU_TEXTURE_DIMENSION_3D 3

/*
typedef [EnforceRange] unsigned long GPUTextureUsageFlags;
[Exposed=(Window, DedicatedWorker)]
namespace GPUTextureUsage {
    const GPUFlagsConstant COPY_SRC          = 0x01;
    const GPUFlagsConstant COPY_DST          = 0x02;
    const GPUFlagsConstant TEXTURE_BINDING   = 0x04;
    const GPUFlagsConstant STORAGE_BINDING   = 0x08;
    const GPUFlagsConstant RENDER_ATTACHMENT = 0x10;
};
*/
typedef int WGPU_TEXTURE_USAGE_FLAGS;
#define WGPU_TEXTURE_USAGE_COPY_SRC            0x01
#define WGPU_TEXTURE_USAGE_COPY_DST            0x02
#define WGPU_TEXTURE_USAGE_TEXTURE_BINDING     0x04
#define WGPU_TEXTURE_USAGE_STORAGE_BINDING     0x08
#define WGPU_TEXTURE_USAGE_RENDER_ATTACHMENT   0x10

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUTextureView {
};
GPUTextureView includes GPUObjectBase;
*/
typedef WGpuObjectBase WGpuTextureView;
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
#define WGPU_TEXTURE_VIEW_DIMENSION_1D 1
#define WGPU_TEXTURE_VIEW_DIMENSION_2D 2
#define WGPU_TEXTURE_VIEW_DIMENSION_2D_ARRAY 3
#define WGPU_TEXTURE_VIEW_DIMENSION_CUBE 4
#define WGPU_TEXTURE_VIEW_DIMENSION_CUBE_ARRAY 5
#define WGPU_TEXTURE_VIEW_DIMENSION_3D 6

/*
enum GPUTextureAspect {
    "all",
    "stencil-only",
    "depth-only"
};
*/
typedef int WGPU_TEXTURE_ASPECT;
#define WGPU_TEXTURE_ASPECT_INVALID 0
#define WGPU_TEXTURE_ASPECT_ALL 1
#define WGPU_TEXTURE_ASPECT_STENCIL_ONLY 2
#define WGPU_TEXTURE_ASPECT_DEPTH_ONLY 3

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

    // Depth/stencil formats
    "stencil8",
    "depth16unorm",
    "depth24plus",
    "depth24plus-stencil8",
    "depth32float",

    // "depth32float-stencil8" feature
    "depth32float-stencil8",

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

    // ETC2 compressed formats usable if "texture-compression-etc2" is both
    // supported by the device/user agent and enabled in requestDevice.
    "etc2-rgb8unorm",
    "etc2-rgb8unorm-srgb",
    "etc2-rgb8a1unorm",
    "etc2-rgb8a1unorm-srgb",
    "etc2-rgba8unorm",
    "etc2-rgba8unorm-srgb",
    "eac-r11unorm",
    "eac-r11snorm",
    "eac-rg11unorm",
    "eac-rg11snorm",

    // ASTC compressed formats usable if "texture-compression-astc" is both
    // supported by the device/user agent and enabled in requestDevice.
    "astc-4x4-unorm",
    "astc-4x4-unorm-srgb",
    "astc-5x4-unorm",
    "astc-5x4-unorm-srgb",
    "astc-5x5-unorm",
    "astc-5x5-unorm-srgb",
    "astc-6x5-unorm",
    "astc-6x5-unorm-srgb",
    "astc-6x6-unorm",
    "astc-6x6-unorm-srgb",
    "astc-8x5-unorm",
    "astc-8x5-unorm-srgb",
    "astc-8x6-unorm",
    "astc-8x6-unorm-srgb",
    "astc-8x8-unorm",
    "astc-8x8-unorm-srgb",
    "astc-10x5-unorm",
    "astc-10x5-unorm-srgb",
    "astc-10x6-unorm",
    "astc-10x6-unorm-srgb",
    "astc-10x8-unorm",
    "astc-10x8-unorm-srgb",
    "astc-10x10-unorm",
    "astc-10x10-unorm-srgb",
    "astc-12x10-unorm",
    "astc-12x10-unorm-srgb",
    "astc-12x12-unorm",
    "astc-12x12-unorm-srgb"
};
*/
typedef int WGPU_TEXTURE_FORMAT;
#define WGPU_TEXTURE_FORMAT_INVALID 0
    // 8-bit formats
#define WGPU_TEXTURE_FORMAT_R8UNORM 1
#define WGPU_TEXTURE_FORMAT_R8SNORM 2
#define WGPU_TEXTURE_FORMAT_R8UINT  3
#define WGPU_TEXTURE_FORMAT_R8SINT  4

    // 16-bit formats
#define WGPU_TEXTURE_FORMAT_R16UINT  5
#define WGPU_TEXTURE_FORMAT_R16SINT  6
#define WGPU_TEXTURE_FORMAT_R16FLOAT 7
#define WGPU_TEXTURE_FORMAT_RG8UNORM 8
#define WGPU_TEXTURE_FORMAT_RG8SNORM 9
#define WGPU_TEXTURE_FORMAT_RG8UINT  10
#define WGPU_TEXTURE_FORMAT_RG8SINT  11

    // 32-bit formats
#define WGPU_TEXTURE_FORMAT_R32UINT         12
#define WGPU_TEXTURE_FORMAT_R32SINT         13
#define WGPU_TEXTURE_FORMAT_R32FLOAT        14
#define WGPU_TEXTURE_FORMAT_RG16UINT        15
#define WGPU_TEXTURE_FORMAT_RG16SINT        16
#define WGPU_TEXTURE_FORMAT_RG16FLOAT       17
#define WGPU_TEXTURE_FORMAT_RGBA8UNORM      18
#define WGPU_TEXTURE_FORMAT_RGBA8UNORM_SRGB 19
#define WGPU_TEXTURE_FORMAT_RGBA8SNORM      20
#define WGPU_TEXTURE_FORMAT_RGBA8UINT       21
#define WGPU_TEXTURE_FORMAT_RGBA8SINT       22
#define WGPU_TEXTURE_FORMAT_BGRA8UNORM      23
#define WGPU_TEXTURE_FORMAT_BGRA8UNORM_SRGB 24
    // Packed 32-bit formats
#define WGPU_TEXTURE_FORMAT_RGB9E5UFLOAT  25
#define WGPU_TEXTURE_FORMAT_RGB10A2UNORM  26
#define WGPU_TEXTURE_FORMAT_RG11B10UFLOAT 27

    // 64-bit formats
#define WGPU_TEXTURE_FORMAT_RG32UINT    28
#define WGPU_TEXTURE_FORMAT_RG32SINT    29
#define WGPU_TEXTURE_FORMAT_RG32FLOAT   30
#define WGPU_TEXTURE_FORMAT_RGBA16UINT  31
#define WGPU_TEXTURE_FORMAT_RGBA16SINT  32
#define WGPU_TEXTURE_FORMAT_RGBA16FLOAT 33

    // 128-bit formats
#define WGPU_TEXTURE_FORMAT_RGBA32UINT  34
#define WGPU_TEXTURE_FORMAT_RGBA32SINT  35
#define WGPU_TEXTURE_FORMAT_RGBA32FLOAT 36

    // Depth/stencil formats
#define WGPU_TEXTURE_FORMAT_STENCIL8              37
#define WGPU_TEXTURE_FORMAT_DEPTH16UNORM          38
#define WGPU_TEXTURE_FORMAT_DEPTH24PLUS           39
#define WGPU_TEXTURE_FORMAT_DEPTH24PLUS_STENCIL8  40
#define WGPU_TEXTURE_FORMAT_DEPTH32FLOAT          41
#define WGPU_TEXTURE_FORMAT_DEPTH32FLOAT_STENCIL8 42

    // BC compressed formats usable if "texture-compression-bc" is both
    // supported by the device/user agent and enabled in requestDevice.
#define WGPU_TEXTURE_FORMAT_BC1_RGBA_UNORM      43
#define WGPU_TEXTURE_FORMAT_BC1_RGBA_UNORM_SRGB 44
#define WGPU_TEXTURE_FORMAT_BC2_RGBA_UNORM      45
#define WGPU_TEXTURE_FORMAT_BC2_RGBA_UNORM_SRGB 46
#define WGPU_TEXTURE_FORMAT_BC3_RGBA_UNORM      47
#define WGPU_TEXTURE_FORMAT_BC3_RGBA_UNORM_SRGB 48
#define WGPU_TEXTURE_FORMAT_BC4_R_UNORM         49
#define WGPU_TEXTURE_FORMAT_BC4_R_SNORM         50
#define WGPU_TEXTURE_FORMAT_BC5_RG_UNORM        51
#define WGPU_TEXTURE_FORMAT_BC5_RG_SNORM        52
#define WGPU_TEXTURE_FORMAT_BC6H_RGB_UFLOAT     53
#define WGPU_TEXTURE_FORMAT_BC6H_RGB_FLOAT      54
#define WGPU_TEXTURE_FORMAT_BC7_RGBA_UNORM      55
#define WGPU_TEXTURE_FORMAT_BC7_RGBA_UNORM_SRGB 56

    // ETC2 compressed formats usable if "texture-compression-etc2" is both
    // supported by the device/user agent and enabled in requestDevice.
#define WGPU_TEXTURE_FORMAT_ETC2_RGB8UNORM        57
#define WGPU_TEXTURE_FORMAT_ETC2_RGB8UNORM_SRGB   58
#define WGPU_TEXTURE_FORMAT_ETC2_RGB8A1UNORM      59
#define WGPU_TEXTURE_FORMAT_ETC2_RGB8A1UNORM_SRGB 60
#define WGPU_TEXTURE_FORMAT_ETC2_RGBA8UNORM       61
#define WGPU_TEXTURE_FORMAT_ETC2_RGBA8UNORM_SRGB  62
#define WGPU_TEXTURE_FORMAT_EAC_R11UNORM          63
#define WGPU_TEXTURE_FORMAT_EAC_R11SNORM          64
#define WGPU_TEXTURE_FORMAT_EAC_RG11UNORM         65
#define WGPU_TEXTURE_FORMAT_EAC_RG11SNORM         66

    // ASTC compressed formats usable if "texture-compression-astc" is both
    // supported by the device/user agent and enabled in requestDevice.
#define WGPU_TEXTURE_FORMAT_ASTC_4X4_UNORM        67
#define WGPU_TEXTURE_FORMAT_ASTC_4X4_UNORM_SRGB   68
#define WGPU_TEXTURE_FORMAT_ASTC_5X4_UNORM        69
#define WGPU_TEXTURE_FORMAT_ASTC_5X4_UNORM_SRGB   70
#define WGPU_TEXTURE_FORMAT_ASTC_5X5_UNORM        71
#define WGPU_TEXTURE_FORMAT_ASTC_5X5_UNORM_SRGB   72
#define WGPU_TEXTURE_FORMAT_ASTC_6X5_UNORM        73
#define WGPU_TEXTURE_FORMAT_ASTC_6X5_UNORM_SRGB   74
#define WGPU_TEXTURE_FORMAT_ASTC_6X6_UNORM        75
#define WGPU_TEXTURE_FORMAT_ASTC_6X6_UNORM_SRGB   76
#define WGPU_TEXTURE_FORMAT_ASTC_8X5_UNORM        77
#define WGPU_TEXTURE_FORMAT_ASTC_8X5_UNORM_SRGB   78
#define WGPU_TEXTURE_FORMAT_ASTC_8X6_UNORM        79
#define WGPU_TEXTURE_FORMAT_ASTC_8X6_UNORM_SRGB   80
#define WGPU_TEXTURE_FORMAT_ASTC_8X8_UNORM        81
#define WGPU_TEXTURE_FORMAT_ASTC_8X8_UNORM_SRGB   82
#define WGPU_TEXTURE_FORMAT_ASTC_10X5_UNORM       83
#define WGPU_TEXTURE_FORMAT_ASTC_10X5_UNORM_SRGB  84
#define WGPU_TEXTURE_FORMAT_ASTC_10X6_UNORM       85
#define WGPU_TEXTURE_FORMAT_ASTC_10X6_UNORM_SRGB  86
#define WGPU_TEXTURE_FORMAT_ASTC_10X8_UNORM       87
#define WGPU_TEXTURE_FORMAT_ASTC_10X8_UNORM_SRGB  88
#define WGPU_TEXTURE_FORMAT_ASTC_10X10_UNORM      89
#define WGPU_TEXTURE_FORMAT_ASTC_10X10_UNORM_SRGB 90
#define WGPU_TEXTURE_FORMAT_ASTC_12X10_UNORM      91
#define WGPU_TEXTURE_FORMAT_ASTC_12X10_UNORM_SRGB 92
#define WGPU_TEXTURE_FORMAT_ASTC_12X12_UNORM      93
#define WGPU_TEXTURE_FORMAT_ASTC_12X12_UNORM_SRGB 94

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUExternalTexture {
    readonly attribute boolean expired;
};
GPUExternalTexture includes GPUObjectBase;
*/
typedef WGpuObjectBase WGpuExternalTexture;
// Returns true if the given handle references a valid GPUExternalTexture.
EM_BOOL wgpu_is_external_texture(WGpuObjectBase object);
// Returns true if the given GPUExternalTexture object has expired.
EM_BOOL wgpu_external_texture_is_expired(WGpuExternalTexture externalTexture);

/*
dictionary GPUExternalTextureDescriptor : GPUObjectDescriptorBase {
    required HTMLVideoElement source;
    PredefinedColorSpace colorSpace = "srgb";
};
*/
typedef struct WGpuExternalTextureDescriptor
{
  // An object ID pointing to instance of type HTMLVideoElement. To obtain this id, you must call
  // either wgpuStore() or wgpuStoreAndSetParent() on JavaScript side on a HTMLVideoElement object
  // to pin/register the video element to a Wasm referenceable object ID.
  WGpuObjectBase source;
  HTML_PREDEFINED_COLOR_SPACE colorSpace;
} WGpuExternalTextureDescriptor;
extern const WGpuExternalTextureDescriptor WGPU_EXTERNAL_TEXTURE_DESCRIPTOR_DEFAULT_INITIALIZER;

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUSampler {
};
GPUSampler includes GPUObjectBase;
*/
typedef WGpuObjectBase WGpuSampler;
// Returns true if the given handle references a valid GPUSampler.
EM_BOOL wgpu_is_sampler(WGpuObjectBase object);

/*
dictionary GPUSamplerDescriptor : GPUObjectDescriptorBase {
    GPUAddressMode addressModeU = "clamp-to-edge";
    GPUAddressMode addressModeV = "clamp-to-edge";
    GPUAddressMode addressModeW = "clamp-to-edge";
    GPUFilterMode magFilter = "nearest";
    GPUFilterMode minFilter = "nearest";
    GPUMipmapFilterMode mipmapFilter = "nearest";
    float lodMinClamp = 0;
    float lodMaxClamp = 32;
    GPUCompareFunction compare;
    [Clamp] unsigned short maxAnisotropy = 1;
};
*/
typedef struct WGpuSamplerDescriptor
{
  WGPU_ADDRESS_MODE addressModeU; // default = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE
  WGPU_ADDRESS_MODE addressModeV; // default = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE
  WGPU_ADDRESS_MODE addressModeW; // default = WGPU_ADDRESS_MODE_CLAMP_TO_EDGE
  WGPU_FILTER_MODE magFilter;     // default = WGPU_FILTER_MODE_NEAREST
  WGPU_FILTER_MODE minFilter;     // default = WGPU_FILTER_MODE_NEAREST
  WGPU_MIPMAP_FILTER_MODE mipmapFilter; // default = WGPU_MIPMAP_FILTER_MODE_NEAREST
  float lodMinClamp;              // default = 0
  float lodMaxClamp;              // default = 32
  WGPU_COMPARE_FUNCTION compare;  // default = WGPU_COMPARE_FUNCTION_INVALID (not used)
  uint32_t maxAnisotropy;         // default = 1. N.b. this is 32-bit wide in the bindings implementation for simplicity, unlike in the IDL which specifies a unsigned short.
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
#define WGPU_ADDRESS_MODE_CLAMP_TO_EDGE 1
#define WGPU_ADDRESS_MODE_REPEAT 2
#define WGPU_ADDRESS_MODE_MIRROR_REPEAT 3

/*
enum GPUFilterMode {
    "nearest",
    "linear"
};
*/
typedef int WGPU_FILTER_MODE;
#define WGPU_FILTER_MODE_INVALID 0
#define WGPU_FILTER_MODE_NEAREST 1
#define WGPU_FILTER_MODE_LINEAR 2

/*
enum GPUMipmapFilterMode {
    "nearest",
    "linear"
};
*/
typedef int WGPU_MIPMAP_FILTER_MODE;
#define WGPU_MIPMAP_FILTER_MODE_INVALID 0
#define WGPU_MIPMAP_FILTER_MODE_NEAREST 1
#define WGPU_MIPMAP_FILTER_MODE_LINEAR 2

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
#define WGPU_COMPARE_FUNCTION_NEVER 1
#define WGPU_COMPARE_FUNCTION_LESS 2
#define WGPU_COMPARE_FUNCTION_EQUAL 3
#define WGPU_COMPARE_FUNCTION_LESS_EQUAL 4
#define WGPU_COMPARE_FUNCTION_GREATER 5
#define WGPU_COMPARE_FUNCTION_NOT_EQUAL 6
#define WGPU_COMPARE_FUNCTION_GREATER_EQUAL 7
#define WGPU_COMPARE_FUNCTION_ALWAYS 8

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUBindGroupLayout {
};
GPUBindGroupLayout includes GPUObjectBase;
*/
typedef WGpuObjectBase WGpuBindGroupLayout;
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
[Exposed=(Window, DedicatedWorker)]
namespace GPUShaderStage {
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
#define WGPU_BUFFER_BINDING_TYPE_UNIFORM 1
#define WGPU_BUFFER_BINDING_TYPE_STORAGE 2
#define WGPU_BUFFER_BINDING_TYPE_READ_ONLY_STORAGE 3

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
#define WGPU_SAMPLER_BINDING_TYPE_FILTERING 1
#define WGPU_SAMPLER_BINDING_TYPE_NON_FILTERING 2
#define WGPU_SAMPLER_BINDING_TYPE_COMPARISON 3

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
#define WGPU_TEXTURE_SAMPLE_TYPE_FLOAT 1
#define WGPU_TEXTURE_SAMPLE_TYPE_UNFILTERABLE_FLOAT 2
#define WGPU_TEXTURE_SAMPLE_TYPE_DEPTH 3
#define WGPU_TEXTURE_SAMPLE_TYPE_SINT 4
#define WGPU_TEXTURE_SAMPLE_TYPE_UINT 5

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
    "write-only",
};
*/
typedef int WGPU_STORAGE_TEXTURE_ACCESS;
#define WGPU_STORAGE_TEXTURE_ACCESS_INVALID 0
#define WGPU_STORAGE_TEXTURE_ACCESS_WRITE_ONLY 1

/*
dictionary GPUStorageTextureBindingLayout {
    GPUStorageTextureAccess access = "write-only";
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
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUBindGroup {
};
GPUBindGroup includes GPUObjectBase;
*/
typedef WGpuObjectBase WGpuBindGroup;
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
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUPipelineLayout {
};
GPUPipelineLayout includes GPUObjectBase;
*/
typedef WGpuObjectBase WGpuPipelineLayout;
// Returns true if the given handle references a valid GPUPipelineLayout.
EM_BOOL wgpu_is_pipeline_layout(WGpuObjectBase object);

/*
dictionary GPUPipelineLayoutDescriptor : GPUObjectDescriptorBase {
    required sequence<GPUBindGroupLayout> bindGroupLayouts;
};
*/
// Currently unused.

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUShaderModule {
    Promise<GPUCompilationInfo> compilationInfo();
};
GPUShaderModule includes GPUObjectBase;
*/
typedef WGpuObjectBase WGpuShaderModule;
// Returns true if the given handle references a valid GPUShaderModule.
EM_BOOL wgpu_is_shader_module(WGpuObjectBase object);

typedef void (*WGpuGetCompilationInfoCallback)(WGpuShaderModule shaderModule, WGpuCompilationInfo *compilationInfo NOTNULL, void *userData);

// Asynchronously obtains information about WebGPU shader module compilation to the given callback.
// !! Remember to call wgpu_free_compilation_info() in the callback function after being done with the data.
void wgpu_shader_module_get_compilation_info_async(WGpuShaderModule shaderModule, WGpuGetCompilationInfoCallback callback, void *userData);

/*
dictionary GPUShaderModuleCompilationHint {
    (GPUPipelineLayout or GPUAutoLayoutMode) layout;
};
*/
typedef struct WGpuShaderModuleCompilationHint
{
  const char *entryPointName;
  // WGPU_AUTO_LAYOUT_MODE
  WGpuPipelineLayout layout;  // Assign the special value WGPU_AUTO_LAYOUT_MODE_AUTO (default) to hint an automatically created pipeline object.
} WGpuShaderModuleCompilationHint;
extern const WGpuShaderModuleCompilationHint WGPU_SHADER_MODULE_COMPILATION_HINT_DEFAULT_INITIALIZER;

/*
dictionary GPUShaderModuleDescriptor : GPUObjectDescriptorBase {
    required USVString code;
    object sourceMap;
    record<USVString, GPUShaderModuleCompilationHint> hints;
};
*/
typedef struct WGpuShaderModuleDescriptor
{
  const char *code;
  // TODO: add sourceMap support
  int numHints;
  const WGpuShaderModuleCompilationHint *hints;
} WGpuShaderModuleDescriptor;

/*
enum GPUCompilationMessageType {
    "error",
    "warning",
    "info"
};
*/
typedef int WGPU_COMPILATION_MESSAGE_TYPE;
#define WGPU_COMPILATION_MESSAGE_TYPE_ERROR 0
#define WGPU_COMPILATION_MESSAGE_TYPE_WARNING 1
#define WGPU_COMPILATION_MESSAGE_TYPE_INFO 2

const char *wgpu_compilation_message_type_to_string(WGPU_COMPILATION_MESSAGE_TYPE type);

/*
[Exposed=(Window, DedicatedWorker), Serializable, SecureContext]
interface GPUCompilationMessage {
    readonly attribute DOMString message;
    readonly attribute GPUCompilationMessageType type;
    readonly attribute unsigned long long lineNum;
    readonly attribute unsigned long long linePos;
    readonly attribute unsigned long long offset;
    readonly attribute unsigned long long length;
};
*/
typedef struct WGpuCompilationMessage
{
  // A human-readable string containing the message generated during the shader compilation.
  char *message;

  // The severity level of the message.
  WGPU_COMPILATION_MESSAGE_TYPE type;

  // The line number in the shader code the message corresponds to. Value is one-based, such
  // that a lineNum of 1 indicates the first line of the shader code.
  // If the message corresponds to a substring this points to the line on which the substring
  // begins. Must be 0 if the message does not correspond to any specific point in the shader code.
  uint32_t lineNum;

  // The offset, in UTF-16 code units, from the beginning of line lineNum of the shader code
  // to the point or beginning of the substring that the message corresponds to. Value is
  // one-based, such that a linePos of 1 indicates the first character of the line.
  // If message corresponds to a substring this points to the first UTF-16 code unit of the
  // substring. Must be 0 if the message does not correspond to any specific point in the shader code.
  uint32_t linePos;

  // The offset from the beginning of the shader code in UTF-16 code units to the point or
  // beginning of the substring that message corresponds to. Must reference the same position as
  // lineNum and linePos. Must be 0 if the message does not correspond to any specific point in
  // the shader code.
  uint32_t offset;

  // The number of UTF-16 code units in the substring that message corresponds to. If the message
  // does not correspond with a substring then length must be 0.
  uint32_t length;
} WGpuCompilationMessage;

/*
[Exposed=(Window, DedicatedWorker), Serializable, SecureContext]
interface GPUCompilationInfo {
    readonly attribute FrozenArray<GPUCompilationMessage> messages;
};
*/
typedef struct WGpuCompilationInfo
{
  int numMessages;
  WGpuCompilationMessage messages[];
} WGpuCompilationInfo;
// Deallocates a WGpuCompilationInfo object produced by a call to wgpu_free_compilation_info()
#define wgpu_free_compilation_info(info) free((info))

/*
enum GPUAutoLayoutMode {
    "auto",
};
*/
typedef int WGPU_AUTO_LAYOUT_MODE;
#define WGPU_AUTO_LAYOUT_MODE_NO_HINT 0 // In shader compilation, specifies that no hint is to be passed. Invalid to be used in pipeline creation.
#define WGPU_AUTO_LAYOUT_MODE_AUTO    1 // In shader compilation, specifies that the hint { layout: 'auto' } is to be passed. In pipeline creation, uses automatic layout creation.

/*
dictionary GPUPipelineDescriptorBase : GPUObjectDescriptorBase {
    required (GPUPipelineLayout or GPUAutoLayoutMode) layout;
};
*/
// Not used since it contains only one member. Will be implemented if # of members increases.

/*
interface mixin GPUPipelineBase {
     [NewObject] GPUBindGroupLayout getBindGroupLayout(unsigned long index);
};
*/
// Returns the bind group layout at the given index of the pipeline. Important: this function allocates a
// new WebGPU object, so in order not to leak WebGPU handles, call wgpu_object_destroy() on the returned value
// when done with it.
WGpuBindGroupLayout wgpu_pipeline_get_bind_group_layout(WGpuObjectBase pipelineBase, uint32_t index);
#define wgpu_render_pipeline_get_bind_group_layout wgpu_pipeline_get_bind_group_layout
#define wgpu_compute_pipeline_get_bind_group_layout wgpu_pipeline_get_bind_group_layout

/*
dictionary GPUProgrammableStage {
    required GPUShaderModule module;
    required USVString entryPoint;
    record<USVString, GPUPipelineConstantValue> constants;
};
typedef double GPUPipelineConstantValue; // May represent WGSL’s bool, f32, i32, u32, and f16 if enabled.
*/

typedef struct WGpuPipelineConstant
{
  const char *name;
  uint32_t _dummyPadding; // (would be automatically inserted by the compiler, but present here for explicity)
  double value;
} WGpuPipelineConstant;

/*

[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUComputePipeline {
};
GPUComputePipeline includes GPUObjectBase;
GPUComputePipeline includes GPUPipelineBase;
*/
typedef WGpuObjectBase WGpuComputePipeline;
// Returns true if the given handle references a valid GPUComputePipeline.
EM_BOOL wgpu_is_compute_pipeline(WGpuObjectBase object);

/*
dictionary GPUComputePipelineDescriptor : GPUPipelineDescriptorBase {
    required GPUProgrammableStage compute;
};
*/
// Currently unused.

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPURenderPipeline {
};
GPURenderPipeline includes GPUObjectBase;
GPURenderPipeline includes GPUPipelineBase;
*/
typedef WGpuObjectBase WGpuRenderPipeline;
typedef WGpuObjectBase WGpuPipelineBase;
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
#define WGPU_PRIMITIVE_TOPOLOGY_POINT_LIST 1
#define WGPU_PRIMITIVE_TOPOLOGY_LINE_LIST 2
#define WGPU_PRIMITIVE_TOPOLOGY_LINE_STRIP 3
#define WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST 4
#define WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP 5

/*
dictionary GPUPrimitiveState {
    GPUPrimitiveTopology topology = "triangle-list";
    GPUIndexFormat stripIndexFormat;
    GPUFrontFace frontFace = "ccw";
    GPUCullMode cullMode = "none";

    // Requires "depth-clip-control" feature.
    boolean unclippedDepth = false;
};
*/
typedef struct WGpuPrimitiveState
{
  WGPU_PRIMITIVE_TOPOLOGY topology; // Defaults to WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST ('triangle-list')
  WGPU_INDEX_FORMAT stripIndexFormat; // Defaults to undefined, must be explicitly specified if WGPU_PRIMITIVE_TOPOLOGY_LINE_STRIP ('line-strip') or WGPU_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP ('triangle-strip') is used.
  WGPU_FRONT_FACE frontFace; // Defaults to WGPU_FRONT_FACE_CCW ('ccw')
  WGPU_CULL_MODE cullMode; // Defaults to WGPU_CULL_MODE_NONE ('none')

  EM_BOOL unclippedDepth; // defaults to EM_FALSE.
} WGpuPrimitiveState;

/*
enum GPUFrontFace {
    "ccw",
    "cw"
};
*/
typedef int WGPU_FRONT_FACE;
#define WGPU_FRONT_FACE_INVALID 0
#define WGPU_FRONT_FACE_CCW 1
#define WGPU_FRONT_FACE_CW 2

/*
enum GPUCullMode {
    "none",
    "front",
    "back"
};
*/
typedef int WGPU_CULL_MODE;
#define WGPU_CULL_MODE_INVALID 0
#define WGPU_CULL_MODE_NONE 1
#define WGPU_CULL_MODE_FRONT 2
#define WGPU_CULL_MODE_BACK 3

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
    required sequence<GPUColorTargetState?> targets;
};
*/
typedef struct WGpuFragmentState
{
  WGpuShaderModule module;
  const char *entryPoint;
  int numTargets;
  const WGpuColorTargetState *targets;
  int numConstants;
  const WGpuPipelineConstant *constants;
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
[Exposed=(Window, DedicatedWorker)]
namespace GPUColorWrite {
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
    GPUBlendOperation operation = "add";
    GPUBlendFactor srcFactor = "one";
    GPUBlendFactor dstFactor = "zero";
};
*/
typedef struct WGpuBlendComponent
{
  WGPU_BLEND_OPERATION operation;
  WGPU_BLEND_FACTOR srcFactor;
  WGPU_BLEND_FACTOR dstFactor;
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
#define WGPU_BLEND_FACTOR_ZERO 1
#define WGPU_BLEND_FACTOR_ONE 2
#define WGPU_BLEND_FACTOR_SRC 3
#define WGPU_BLEND_FACTOR_ONE_MINUS_SRC 4
#define WGPU_BLEND_FACTOR_SRC_ALPHA 5
#define WGPU_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA 6
#define WGPU_BLEND_FACTOR_DST 7
#define WGPU_BLEND_FACTOR_ONE_MINUS_DST 8
#define WGPU_BLEND_FACTOR_DST_ALPHA 9
#define WGPU_BLEND_FACTOR_ONE_MINUS_DST_ALPHA 10
#define WGPU_BLEND_FACTOR_SRC_ALPHA_SATURATED 11
#define WGPU_BLEND_FACTOR_CONSTANT 12
#define WGPU_BLEND_FACTOR_ONE_MINUS_CONSTANT 13

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
#define WGPU_BLEND_OPERATION_DISABLED 0 // Alias to 'WGPU_BLEND_OPERATION_INVALID'. Used to denote alpha blending being disabled in a more readable way.
#define WGPU_BLEND_OPERATION_ADD 1
#define WGPU_BLEND_OPERATION_SUBTRACT 2
#define WGPU_BLEND_OPERATION_REVERSE_SUBTRACT 3
#define WGPU_BLEND_OPERATION_MIN 4
#define WGPU_BLEND_OPERATION_MAX 5

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
#define WGPU_STENCIL_OPERATION_KEEP 1
#define WGPU_STENCIL_OPERATION_ZERO 2
#define WGPU_STENCIL_OPERATION_REPLACE 3
#define WGPU_STENCIL_OPERATION_INVERT 4
#define WGPU_STENCIL_OPERATION_INCREMENT_CLAMP 5
#define WGPU_STENCIL_OPERATION_DECREMENT_CLAMP 6
#define WGPU_STENCIL_OPERATION_INCREMENT_WRAP 7
#define WGPU_STENCIL_OPERATION_DECREMENT_WRAP 8

/*
enum GPUIndexFormat {
    "uint16",
    "uint32"
};
*/
typedef int WGPU_INDEX_FORMAT;
#define WGPU_INDEX_FORMAT_INVALID 0
#define WGPU_INDEX_FORMAT_UINT16 1
#define WGPU_INDEX_FORMAT_UINT32 2

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
    "sint32x4"
};
*/
// The numbering on these types continues at the end of WGPU_TEXTURE_FORMAT
// for optimization reasons.
typedef int WGPU_VERTEX_FORMAT;
#define WGPU_VERTEX_FORMAT_INVALID   0
#define WGPU_VERTEX_FORMAT_UINT8X2   95
#define WGPU_VERTEX_FORMAT_UINT8X4   96
#define WGPU_VERTEX_FORMAT_SINT8X2   97
#define WGPU_VERTEX_FORMAT_SINT8X4   98
#define WGPU_VERTEX_FORMAT_UNORM8X2  99
#define WGPU_VERTEX_FORMAT_UNORM8X4  100
#define WGPU_VERTEX_FORMAT_SNORM8X2  101
#define WGPU_VERTEX_FORMAT_SNORM8X4  102
#define WGPU_VERTEX_FORMAT_UINT16X2  103
#define WGPU_VERTEX_FORMAT_UINT16X4  104
#define WGPU_VERTEX_FORMAT_SINT16X2  105
#define WGPU_VERTEX_FORMAT_SINT16X4  106
#define WGPU_VERTEX_FORMAT_UNORM16X2 107
#define WGPU_VERTEX_FORMAT_UNORM16X4 108
#define WGPU_VERTEX_FORMAT_SNORM16X2 109
#define WGPU_VERTEX_FORMAT_SNORM16X4 110
#define WGPU_VERTEX_FORMAT_FLOAT16X2 111
#define WGPU_VERTEX_FORMAT_FLOAT16X4 112
#define WGPU_VERTEX_FORMAT_FLOAT32   113
#define WGPU_VERTEX_FORMAT_FLOAT32X2 114
#define WGPU_VERTEX_FORMAT_FLOAT32X3 115
#define WGPU_VERTEX_FORMAT_FLOAT32X4 116
#define WGPU_VERTEX_FORMAT_UINT32    117
#define WGPU_VERTEX_FORMAT_UINT32X2  118
#define WGPU_VERTEX_FORMAT_UINT32X3  119
#define WGPU_VERTEX_FORMAT_UINT32X4  120
#define WGPU_VERTEX_FORMAT_SINT32    121
#define WGPU_VERTEX_FORMAT_SINT32X2  122
#define WGPU_VERTEX_FORMAT_SINT32X3  123
#define WGPU_VERTEX_FORMAT_SINT32X4  124

/*
enum GPUVertexStepMode {
    "vertex",
    "instance"
};
*/
typedef int WGPU_VERTEX_STEP_MODE;
#define WGPU_VERTEX_STEP_MODE_INVALID 0
#define WGPU_VERTEX_STEP_MODE_VERTEX 1
#define WGPU_VERTEX_STEP_MODE_INSTANCE 2

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
  int numConstants;
  const WGpuPipelineConstant *constants;
} WGpuVertexState;

/*
dictionary GPUVertexBufferLayout {
    required GPUSize64 arrayStride;
    GPUVertexStepMode stepMode = "vertex";
    required sequence<GPUVertexAttribute> attributes;
};
*/
typedef struct WGpuVertexBufferLayout
{
  int numAttributes;
  const WGpuVertexAttribute *attributes;
  uint64_t arrayStride;
  WGPU_VERTEX_STEP_MODE stepMode;
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
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUCommandBuffer {
};
GPUCommandBuffer includes GPUObjectBase;
*/
typedef WGpuObjectBase WGpuCommandBuffer;
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
interface mixin GPUDebugCommandsMixin {
    undefined pushDebugGroup(USVString groupLabel);
    undefined popDebugGroup();
    undefined insertDebugMarker(USVString markerLabel);
};
*/
typedef WGpuObjectBase WGpuDebugCommandsMixin; // One of GPURenderBundleEncoder, GPURenderPassEncoder, GPUComputePassEncoder or GPUCommandEncoder

void wgpu_encoder_push_debug_group(WGpuDebugCommandsMixin encoder, const char *groupLabel NOTNULL);
void wgpu_encoder_pop_debug_group(WGpuDebugCommandsMixin encoder);
void wgpu_encoder_insert_debug_marker(WGpuDebugCommandsMixin encoder, const char *markerLabel NOTNULL);

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
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

    undefined clearBuffer(
        GPUBuffer buffer,
        optional GPUSize64 offset = 0,
        optional GPUSize64 size);

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
GPUCommandEncoder includes GPUCommandsMixin;
GPUCommandEncoder includes GPUDebugCommandsMixin;
*/
typedef WGpuObjectBase WGpuCommandEncoder;
// Returns true if the given handle references a valid GPUCommandEncoder.
EM_BOOL wgpu_is_command_encoder(WGpuObjectBase object);

WGpuRenderPassEncoder wgpu_command_encoder_begin_render_pass(WGpuCommandEncoder commandEncoder, const WGpuRenderPassDescriptor *renderPassDesc NOTNULL);
// Like above, but tiny code size path for the case when there is exactly one color and zero depth-stencil targets and no occlusion query set specified for the render pass.
WGpuRenderPassEncoder wgpu_command_encoder_begin_render_pass_1color_0depth(WGpuCommandEncoder commandEncoder, const WGpuRenderPassDescriptor *renderPassDesc NOTNULL);
WGpuComputePassEncoder wgpu_command_encoder_begin_compute_pass(WGpuCommandEncoder commandEncoder, const WGpuComputePassDescriptor *computePassDesc _WGPU_DEFAULT_VALUE(0));
void wgpu_command_encoder_copy_buffer_to_buffer(WGpuCommandEncoder commandEncoder, WGpuBuffer source, double_int53_t sourceOffset, WGpuBuffer destination, double_int53_t destinationOffset, double_int53_t size);
void wgpu_command_encoder_copy_buffer_to_texture(WGpuCommandEncoder commandEncoder, const WGpuImageCopyBuffer *source NOTNULL, const WGpuImageCopyTexture *destination NOTNULL, uint32_t copyWidth, uint32_t copyHeight _WGPU_DEFAULT_VALUE(1), uint32_t copyDepthOrArrayLayers _WGPU_DEFAULT_VALUE(1));
void wgpu_command_encoder_copy_texture_to_buffer(WGpuCommandEncoder commandEncoder, const WGpuImageCopyTexture *source NOTNULL, const WGpuImageCopyBuffer *destination NOTNULL, uint32_t copyWidth, uint32_t copyHeight _WGPU_DEFAULT_VALUE(1), uint32_t copyDepthOrArrayLayers _WGPU_DEFAULT_VALUE(1));
void wgpu_command_encoder_copy_texture_to_texture(WGpuCommandEncoder commandEncoder, const WGpuImageCopyTexture *source NOTNULL, const WGpuImageCopyTexture *destination NOTNULL, uint32_t copyWidth, uint32_t copyHeight _WGPU_DEFAULT_VALUE(1), uint32_t copyDepthOrArrayLayers _WGPU_DEFAULT_VALUE(1));
void wgpu_command_encoder_clear_buffer(WGpuCommandEncoder commandEncoder, WGpuBuffer buffer, double_int53_t offset _WGPU_DEFAULT_VALUE(0), double_int53_t size _WGPU_DEFAULT_VALUE(WGPU_INFINITY));
void wgpu_command_encoder_write_timestamp(WGpuCommandEncoder commandEncoder, WGpuQuerySet querySet, uint32_t queryIndex);
void wgpu_command_encoder_resolve_query_set(WGpuCommandEncoder commandEncoder, WGpuQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, WGpuBuffer destination, double_int53_t destinationOffset);

// GPUCommandEncoder and GPURenderBundleEncoder share the same finish() command.
WGpuObjectBase wgpu_encoder_finish(WGpuObjectBase commandOrRenderBundleEncoder);
#define wgpu_command_encoder_finish wgpu_encoder_finish

// Inherited from GPUDebugCommandsMixin
#define wgpu_command_encoder_push_debug_group wgpu_encoder_push_debug_group
#define wgpu_command_encoder_pop_debug_group wgpu_encoder_pop_debug_group
#define wgpu_command_encoder_insert_debug_marker wgpu_encoder_insert_debug_marker

/*
dictionary GPUCommandEncoderDescriptor : GPUObjectDescriptorBase {
};
*/
typedef struct WGpuCommandEncoderDescriptor
{
  uint32_t _dummyPadding; // Appease mixed C and C++ compilation to agree on non-zero struct size.
} WGpuCommandEncoderDescriptor;
extern const WGpuCommandEncoderDescriptor WGPU_COMMAND_ENCODER_DESCRIPTOR_DEFAULT_INITIALIZER;

/*
dictionary GPUImageDataLayout {
    GPUSize64 offset = 0;
    GPUSize32 bytesPerRow;
    GPUSize32 rowsPerImage;
};
// unused: fused to WGpuImageCopyBuffer
*/

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
  uint32_t _explicitPaddingFor8BytesAlignedSize;
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
    PredefinedColorSpace colorSpace = "srgb";
    boolean premultipliedAlpha = false;
};
*/
// Defined at the end of this file

/*
dictionary GPUImageCopyExternalImage {
    required (ImageBitmap or HTMLVideoElement or HTMLCanvasElement or OffscreenCanvas) source;
    GPUOrigin2D origin = {};
    boolean flipY = false;
};
*/
// Defined at the end of this file

/*
interface mixin GPUBindingCommandsMixin {
    undefined setBindGroup(GPUIndex32 index, GPUBindGroup bindGroup,
                      optional sequence<GPUBufferDynamicOffset> dynamicOffsets = []);

    undefined setBindGroup(GPUIndex32 index, GPUBindGroup bindGroup,
                      Uint32Array dynamicOffsetsData,
                      GPUSize64 dynamicOffsetsDataStart,
                      GPUSize32 dynamicOffsetsDataLength);
};
*/
typedef WGpuObjectBase WGpuBindingCommandsMixin;
// Returns true if the given handle references a valid GPUBindingCommandsMixin. (one of: GPUComputePassEncoder, GPURenderPassEncoder, or GPURenderBundleEncoder)
EM_BOOL wgpu_is_binding_commands_mixin(WGpuObjectBase object);
void wgpu_encoder_set_bind_group(WGpuBindingCommandsMixin encoder, uint32_t index, WGpuBindGroup bindGroup, const uint32_t *dynamicOffsets _WGPU_DEFAULT_VALUE(0), uint32_t numDynamicOffsets _WGPU_DEFAULT_VALUE(0));

// Some of the functions in GPURenderBundleEncoder, GPURenderPassEncoder and GPUComputePassEncoder are identical in implementation,
// so group them under a common base class.
void wgpu_encoder_set_pipeline(WGpuBindingCommandsMixin encoder, WGpuObjectBase pipeline);
void wgpu_encoder_end(WGpuBindingCommandsMixin encoder);

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUComputePassEncoder {
    undefined setPipeline(GPUComputePipeline pipeline);
    undefined dispatchWorkgroups(GPUSize32 workgroupCountX, optional GPUSize32 workgroupCountY = 1, optional GPUSize32 workgroupCountZ = 1);
    undefined dispatchWorkgroupsIndirect(GPUBuffer indirectBuffer, GPUSize64 indirectOffset);

    undefined end();
};
GPUComputePassEncoder includes GPUObjectBase;
GPUComputePassEncoder includes GPUCommandsMixin;
GPUComputePassEncoder includes GPUDebugCommandsMixin;
GPUComputePassEncoder includes GPUBindingCommandsMixin;
*/
typedef WGpuObjectBase WGpuComputePassEncoder;
// Returns true if the given handle references a valid GPUComputePassEncoder.
EM_BOOL wgpu_is_compute_pass_encoder(WGpuObjectBase object);

#define wgpu_compute_pass_encoder_set_pipeline wgpu_encoder_set_pipeline
void wgpu_compute_pass_encoder_dispatch_workgroups(WGpuComputePassEncoder encoder, uint32_t workgroupCountX, uint32_t workgroupCountY _WGPU_DEFAULT_VALUE(1), uint32_t workgroupCountZ _WGPU_DEFAULT_VALUE(1));
void wgpu_compute_pass_encoder_dispatch_workgroups_indirect(WGpuComputePassEncoder encoder, WGpuBuffer indirectBuffer, double_int53_t indirectOffset);
#define wgpu_compute_pass_encoder_end wgpu_encoder_end

// Inherited from GPUDebugCommandsMixin:
#define wgpu_compute_pass_encoder_push_debug_group wgpu_encoder_push_debug_group
#define wgpu_compute_pass_encoder_pop_debug_group wgpu_encoder_pop_debug_group
#define wgpu_compute_pass_encoder_insert_debug_marker wgpu_encoder_insert_debug_marker

// Inherited from GPUBindingCommandsMixin:
#define wgpu_compute_pass_encoder_set_bind_group wgpu_encoder_set_bind_group

/*
 enum GPUComputePassTimestampLocation {
    "beginning",
    "end",
};
*/
typedef int WGPU_COMPUTE_PASS_TIMESTAMP_LOCATION;
#define WGPU_COMPUTE_PASS_TIMESTAMP_LOCATION_BEGINNING 0
#define WGPU_COMPUTE_PASS_TIMESTAMP_LOCATION_END       1

/*
dictionary GPUComputePassTimestampWrite {
    required GPUQuerySet querySet;
    required GPUSize32 queryIndex;
    required GPUComputePassTimestampLocation location;
};
*/
typedef struct WGpuComputePassTimestampWrite
{
  WGpuQuerySet querySet;
  uint32_t queryIndex;
  WGPU_COMPUTE_PASS_TIMESTAMP_LOCATION location;
} WGpuComputePassTimestampWrite;

/*
typedef sequence<GPUComputePassTimestampWrite> GPUComputePassTimestampWrites;

dictionary GPUComputePassDescriptor : GPUObjectDescriptorBase {
  GPUComputePassTimestampWrites timestampWrites = [];
};
*/
typedef struct WGpuComputePassDescriptor
{
  uint32_t numTimestampWrites;
  WGpuComputePassTimestampWrite *timestampWrites;
} WGpuComputePassDescriptor;

/*
interface mixin GPURenderCommandsMixin {
    undefined setPipeline(GPURenderPipeline pipeline);

    undefined setIndexBuffer(GPUBuffer buffer, GPUIndexFormat indexFormat, optional GPUSize64 offset = 0, optional GPUSize64 size);
    undefined setVertexBuffer(GPUIndex32 slot, GPUBuffer buffer, optional GPUSize64 offset = 0, optional GPUSize64 size);

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
// Deliberate API naming divergence: in upstream WebGPU API, there are base "mixin" classes
typedef WGpuObjectBase WGpuRenderCommandsMixin;
// Returns true if the given handle references a valid GPURenderCommandsMixin.
EM_BOOL wgpu_is_render_commands_mixin(WGpuObjectBase object);

#define wgpu_render_commands_mixin_set_pipeline wgpu_encoder_set_pipeline
void wgpu_render_commands_mixin_set_index_buffer(WGpuRenderCommandsMixin renderCommandsMixin, WGpuBuffer buffer, WGPU_INDEX_FORMAT indexFormat, double_int53_t offset _WGPU_DEFAULT_VALUE(0), double_int53_t size _WGPU_DEFAULT_VALUE(-1));
void wgpu_render_commands_mixin_set_vertex_buffer(WGpuRenderCommandsMixin renderCommandsMixin, int32_t slot, WGpuBuffer buffer, double_int53_t offset _WGPU_DEFAULT_VALUE(0), double_int53_t size _WGPU_DEFAULT_VALUE(-1));

void wgpu_render_commands_mixin_draw(WGpuRenderCommandsMixin renderCommandsMixin, uint32_t vertexCount, uint32_t instanceCount _WGPU_DEFAULT_VALUE(1), uint32_t firstVertex _WGPU_DEFAULT_VALUE(0), uint32_t firstInstance _WGPU_DEFAULT_VALUE(0));
void wgpu_render_commands_mixin_draw_indexed(WGpuRenderCommandsMixin renderCommandsMixin, uint32_t indexCount, uint32_t instanceCount _WGPU_DEFAULT_VALUE(1), uint32_t firstVertex _WGPU_DEFAULT_VALUE(0), int32_t baseVertex _WGPU_DEFAULT_VALUE(0), uint32_t firstInstance _WGPU_DEFAULT_VALUE(0));

void wgpu_render_commands_mixin_draw_indirect(WGpuRenderCommandsMixin renderCommandsMixin, WGpuBuffer indirectBuffer, double_int53_t indirectOffset);
void wgpu_render_commands_mixin_draw_indexed_indirect(WGpuRenderCommandsMixin renderCommandsMixin, WGpuBuffer indirectBuffer, double_int53_t indirectOffset);

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
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

    undefined executeBundles(sequence<GPURenderBundle> bundles);
    undefined end();
};
GPURenderPassEncoder includes GPUObjectBase;
GPURenderPassEncoder includes GPUCommandsMixin;
GPURenderPassEncoder includes GPUDebugCommandsMixin;
GPURenderPassEncoder includes GPUBindingCommandsMixin;
GPURenderPassEncoder includes GPURenderCommandsMixin;
*/
typedef WGpuObjectBase WGpuRenderPassEncoder;
// Returns true if the given handle references a valid GPURenderPassEncoder.
EM_BOOL wgpu_is_render_pass_encoder(WGpuObjectBase object);

void wgpu_render_pass_encoder_set_viewport(WGpuRenderPassEncoder encoder, float x, float y, float width, float height, float minDepth, float maxDepth);
void wgpu_render_pass_encoder_set_scissor_rect(WGpuRenderPassEncoder encoder, uint32_t x, uint32_t y, uint32_t width, uint32_t height);
void wgpu_render_pass_encoder_set_blend_constant(WGpuRenderPassEncoder encoder, double r, double g, double b, double a);
void wgpu_render_pass_encoder_set_stencil_reference(WGpuRenderPassEncoder encoder, uint32_t stencilValue);
void wgpu_render_pass_encoder_begin_occlusion_query(WGpuRenderPassEncoder encoder, int32_t queryIndex);
void wgpu_render_pass_encoder_end_occlusion_query(WGpuRenderPassEncoder encoder);
void wgpu_render_pass_encoder_execute_bundles(WGpuRenderPassEncoder encoder, const WGpuRenderBundle *bundles, int numBundles);
#define wgpu_render_pass_encoder_end wgpu_encoder_end

// Inherited from GPUDebugCommandsMixin:
#define wgpu_render_pass_encoder_push_debug_group wgpu_programmable_pass_encoder_push_debug_group
#define wgpu_render_pass_encoder_pop_debug_group wgpu_programmable_pass_encoder_pop_debug_group
#define wgpu_render_pass_encoder_insert_debug_marker wgpu_programmable_pass_encoder_insert_debug_marker

// Inherited from GPUBindingCommandsMixin:
#define wgpu_render_pass_encoder_set_bind_group wgpu_encoder_set_bind_group

// Inherited from GPURenderCommandsMixin:
#define wgpu_render_pass_encoder_set_pipeline wgpu_render_commands_mixin_set_pipeline
#define wgpu_render_pass_encoder_set_index_buffer wgpu_render_commands_mixin_set_index_buffer
#define wgpu_render_pass_encoder_set_vertex_buffer wgpu_render_commands_mixin_set_vertex_buffer
#define wgpu_render_pass_encoder_draw wgpu_render_commands_mixin_draw
#define wgpu_render_pass_encoder_draw_indexed wgpu_render_commands_mixin_draw_indexed
#define wgpu_render_pass_encoder_draw_indirect wgpu_render_commands_mixin_draw_indirect
#define wgpu_render_pass_encoder_draw_indexed_indirect wgpu_render_commands_mixin_draw_indexed_indirect

/*
dictionary GPURenderPassColorAttachment {
    required GPUTextureView view;
    GPUTextureView resolveTarget;

    GPUColor clearValue;
    required GPULoadOp loadOp;
    required GPUStoreOp storeOp;
};
*/
// Defined at the end of this file

/*
dictionary GPURenderPassDepthStencilAttachment {
    required GPUTextureView view;

    float depthClearValue = 0;
    GPULoadOp depthLoadOp;
    GPUStoreOp depthStoreOp;
    boolean depthReadOnly = false;

    GPUStencilValue stencilClearValue = 0;
    GPULoadOp stencilLoadOp;
    GPUStoreOp stencilStoreOp;
    boolean stencilReadOnly = false;
};
*/
typedef struct WGpuRenderPassDepthStencilAttachment
{
  WGpuTextureView view;

  WGPU_LOAD_OP depthLoadOp; // Either WGPU_LOAD_OP_LOAD (== default, 0) or WGPU_LOAD_OP_CLEAR
  float depthClearValue;

  WGPU_STORE_OP depthStoreOp;
  EM_BOOL depthReadOnly;

  WGPU_LOAD_OP stencilLoadOp;  // Either WGPU_LOAD_OP_LOAD (== default, 0) or WGPU_LOAD_OP_CLEAR
  uint32_t stencilClearValue;
  WGPU_STORE_OP stencilStoreOp;
  EM_BOOL stencilReadOnly;
} WGpuRenderPassDepthStencilAttachment;

/*
enum GPULoadOp {
    "load",
    "clear",
};
*/
typedef int WGPU_LOAD_OP;
#define WGPU_LOAD_OP_LOAD 0
#define WGPU_LOAD_OP_CLEAR 1

/*
enum GPUStoreOp {
    "store",
    "discard"
};
*/
typedef int WGPU_STORE_OP;
#define WGPU_STORE_OP_STORE 0
#define WGPU_STORE_OP_DISCARD 1

/*
dictionary GPURenderPassLayout : GPUObjectDescriptorBase {
    required sequence<GPUTextureFormat?> colorFormats;
    GPUTextureFormat depthStencilFormat;
    GPUSize32 sampleCount = 1;
};
*/
// Not currently exposed, fused to GPURenderBundleEncoderDescriptor.

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPURenderBundle {
};
GPURenderBundle includes GPUObjectBase;
*/
typedef WGpuObjectBase WGpuRenderBundle;
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
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPURenderBundleEncoder {
    GPURenderBundle finish(optional GPURenderBundleDescriptor descriptor = {});
};
GPURenderBundleEncoder includes GPUObjectBase;
GPURenderBundleEncoder includes GPUCommandsMixin;
GPURenderBundleEncoder includes GPUDebugCommandsMixin;
GPURenderBundleEncoder includes GPUBindingCommandsMixin;
GPURenderBundleEncoder includes GPURenderCommandsMixin;
*/
typedef WGpuObjectBase WGpuRenderBundleEncoder;
// Returns true if the given handle references a valid GPURenderBundleEncoder.
EM_BOOL wgpu_is_render_bundle_encoder(WGpuObjectBase object);
#define wgpu_render_bundle_encoder_finish wgpu_encoder_finish

#define wgpu_render_bundle_encoder_set_bind_group wgpu_encoder_set_bind_group
// Inherited from GPUDebugCommandsMixin:
#define wgpu_render_bundle_encoder_push_debug_group wgpu_encoder_push_debug_group
#define wgpu_render_bundle_encoder_pop_debug_group wgpu_encoder_pop_debug_group
#define wgpu_render_bundle_encoder_insert_debug_marker wgpu_encoder_insert_debug_marker

#define wgpu_render_bundle_encoder_set_pipeline wgpu_encoder_set_pipeline
#define wgpu_render_bundle_encoder_set_index_buffer wgpu_render_commands_mixin_set_index_buffer
#define wgpu_render_bundle_encoder_set_vertex_buffer wgpu_render_commands_mixin_set_vertex_buffer
#define wgpu_render_bundle_encoder_draw wgpu_render_commands_mixin_draw
#define wgpu_render_bundle_encoder_draw_indexed wgpu_render_commands_mixin_draw_indexed
#define wgpu_render_bundle_encoder_draw_indirect wgpu_render_commands_mixin_draw_indirect
#define wgpu_render_bundle_encoder_draw_indexed_indirect wgpu_render_commands_mixin_draw_indexed_indirect

/*
dictionary GPURenderBundleEncoderDescriptor : GPURenderPassLayout {
    boolean depthReadOnly = false;
    boolean stencilReadOnly = false;
};
*/
typedef struct WGpuRenderBundleEncoderDescriptor
{
  int numColorFormats;
  const WGPU_TEXTURE_FORMAT *colorFormats;
  WGPU_TEXTURE_FORMAT depthStencilFormat;
  uint32_t sampleCount;
  EM_BOOL depthReadOnly;
  EM_BOOL stencilReadOnly;
} WGpuRenderBundleEncoderDescriptor;

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
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
typedef WGpuObjectBase WGpuQueue;
// Returns true if the given handle references a valid GPUQueue.
EM_BOOL wgpu_is_queue(WGpuObjectBase object);

// Submits one command buffer to the given queue for rendering. The command buffer is held alive for later resubmission to another queue.
void wgpu_queue_submit_one(WGpuQueue queue, WGpuCommandBuffer commandBuffer);
// Submits one command buffer to the given queue for rendering. The command buffer is destroyed after rendering by calling wgpu_object_destroy() on it.
// (this is a helper function to help remind that wasm side references to WebGPU JS objects need to be destroyed or a reference leak occurs. See
// function wgpu_get_num_live_objects() to help debug the number of live references)
void wgpu_queue_submit_one_and_destroy(WGpuQueue queue, WGpuCommandBuffer commandBuffer);

// Submits multiple command buffers to the given queue for rendering. The command buffers are held alive for later resubmission to another queue.
void wgpu_queue_submit_multiple(WGpuQueue queue, const WGpuCommandBuffer *commandBuffers, int numCommandBuffers);
// Submits multiple command buffers to the given queue for rendering. The command buffers are destroyed after rendering by calling wgpu_object_destroy() on them.
// (this is a helper function to help remind that wasm side references to WebGPU JS objects need to be destroyed or a reference leak occurs. See
// function wgpu_get_num_live_objects() to help debug the number of live references)
void wgpu_queue_submit_multiple_and_destroy(WGpuQueue queue, const WGpuCommandBuffer *commandBuffers, int numCommandBuffers);

typedef void (*WGpuOnSubmittedWorkDoneCallback)(WGpuQueue queue, void *userData);
void wgpu_queue_set_on_submitted_work_done_callback(WGpuQueue queue, WGpuOnSubmittedWorkDoneCallback callback, void *userData);

// Uploads data to the given GPUBuffer. Data is copied from memory in byte addresses data[0], data[1], ... data[size-1], and uploaded
// to the GPU buffer at byte offset bufferOffset, bufferOffset+1, ..., bufferOffset+size-1.
void wgpu_queue_write_buffer(WGpuQueue queue, WGpuBuffer buffer, double_int53_t bufferOffset, const void *data NOTNULL, double_int53_t size);
void wgpu_queue_write_texture(WGpuQueue queue, const WGpuImageCopyTexture *destination NOTNULL, const void *data NOTNULL, uint32_t bytesPerBlockRow, uint32_t blockRowsPerImage, uint32_t writeWidth, uint32_t writeHeight _WGPU_DEFAULT_VALUE(1), uint32_t writeDepthOrArrayLayers _WGPU_DEFAULT_VALUE(1));
void wgpu_queue_copy_external_image_to_texture(WGpuQueue queue, const WGpuImageCopyExternalImage *source NOTNULL, const WGpuImageCopyTextureTagged *destination NOTNULL, uint32_t copyWidth, uint32_t copyHeight _WGPU_DEFAULT_VALUE(1), uint32_t copyDepthOrArrayLayers _WGPU_DEFAULT_VALUE(1));

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUQuerySet {
    undefined destroy();

    readonly attribute GPUQueryType type;
    readonly attribute GPUSize32 count;
};
GPUQuerySet includes GPUObjectBase;
*/
typedef WGpuObjectBase WGpuQuerySet;
// Returns true if the given handle references a valid GPUQuerySet.
EM_BOOL wgpu_is_query_set(WGpuObjectBase object);
// Getters for retrieving query set properties:
WGPU_QUERY_TYPE wgpu_query_set_type(WGpuQuerySet querySet);
uint32_t wgpu_query_set_count(WGpuQuerySet querySet);

/*
dictionary GPUQuerySetDescriptor : GPUObjectDescriptorBase {
    required GPUQueryType type;
    required GPUSize32 count;
};
*/
typedef struct WGpuQuerySetDescriptor
{
  WGPU_QUERY_TYPE type;
  uint32_t count;
} WGpuQuerySetDescriptor;

/*
enum GPUQueryType {
    "occlusion",
    "timestamp"
};
*/
typedef int WGPU_QUERY_TYPE;
#define WGPU_QUERY_TYPE_INVALID 0
#define WGPU_QUERY_TYPE_OCCLUSION 1
#define WGPU_QUERY_TYPE_TIMESTAMP 2

/*
enum GPUPipelineStatisticName {
    "timestamp"
};
*/
typedef int WGPU_PIPELINE_STATISTIC_NAME;
#define WGPU_PIPELINE_STATISTIC_NAME_INVALID 0
#define WGPU_PIPELINE_STATISTIC_NAME_TIMESTAMP 1

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUCanvasContext {
    readonly attribute (HTMLCanvasElement or OffscreenCanvas) canvas;

    undefined configure(GPUCanvasConfiguration configuration);
    undefined unconfigure();

    GPUTexture getCurrentTexture();
};
*/
typedef WGpuObjectBase WGpuCanvasContext;
// Returns true if the given handle references a valid GPUCanvasContext.
EM_BOOL wgpu_is_canvas_context(WGpuObjectBase object);

// TODO: Add char *wgpu_canvas_context_get_canvas_selector_id() for 'canvas' member property, as both CSS ID selector and object ID.

// Configures the swap chain for this context.
#ifdef __EMSCRIPTEN__
void wgpu_canvas_context_configure(WGpuCanvasContext canvasContext, const WGpuCanvasConfiguration *config NOTNULL);
#else
void wgpu_canvas_context_configure(WGpuCanvasContext canvasContext, const WGpuCanvasConfiguration *config NOTNULL, int width _WGPU_DEFAULT_VALUE(0), int height _WGPU_DEFAULT_VALUE(0));
#endif
void wgpu_canvas_context_unconfigure(WGpuCanvasContext canvasContext);

WGpuTexture wgpu_canvas_context_get_current_texture(WGpuCanvasContext canvasContext);

// Native Dawn implementation has a concept of a SwapChain. Web does not have this.
// Dawn SwapChain returns a TextureView, whereas GPUCanvasContext.getCurrentTexture() returns a Texture,
// so provide a convenient function wgpu_canvas_context_get_current_texture_view() to obtain
// a TextureView in a cross-platform manner.
#ifdef __EMSCRIPTEN__
#define wgpu_canvas_context_get_current_texture_view(canvasContext) wgpu_texture_create_view_simple(wgpu_canvas_context_get_current_texture((canvasContext)))
#else
WGpuTextureView wgpu_canvas_context_get_current_texture_view(WGpuCanvasContext canvasContext);
#endif

#ifdef __EMSCRIPTEN__
void wgpu_canvas_context_present(WGpuCanvasContext canvasContext) __attribute__((deprecated("The function wgpu_canvas_context_present() is not available when targeting the web. Presentation always occurs when yielding out from browser event loop. Refactor the code to avoid any blocking render loop and calling wgpu_canvas_context_present() when targeting web browsers.", "Use emscripten_request_animation_frame_loop() instead.")));
#else
void wgpu_canvas_context_present(WGpuCanvasContext canvasContext);
#endif

/*
enum GPUCanvasAlphaMode {
    "opaque",
    "premultiplied",
};
*/
typedef int WGPU_CANVAS_ALPHA_MODE;
#define WGPU_CANVAS_ALPHA_MODE_INVALID 0
#define WGPU_CANVAS_ALPHA_MODE_OPAQUE 1
#define WGPU_CANVAS_ALPHA_MODE_PREMULTIPLIED 2

/*
enum GPUDeviceLostReason {
    "destroyed",
};
*/
typedef int WGPU_DEVICE_LOST_REASON;
#define WGPU_DEVICE_LOST_REASON_INVALID 0
#define WGPU_DEVICE_LOST_REASON_DESTROYED 1

/*
[Exposed=(Window, DedicatedWorker)]
interface GPUDeviceLostInfo {
    readonly attribute (GPUDeviceLostReason or undefined) reason;
    readonly attribute DOMString message;
};

partial interface GPUDevice {
    readonly attribute Promise<GPUDeviceLostInfo> lost;
};
*/
typedef void (*WGpuDeviceLostCallback)(WGpuDevice device, WGPU_DEVICE_LOST_REASON deviceLostReason, const char *message NOTNULL, void *userData);
void wgpu_device_set_lost_callback(WGpuDevice device, WGpuDeviceLostCallback callback, void *userData);

// Specifies the type of an error that occurred.
// N.b. the values of these should be kept in sync with values of WGPU_ERROR_FILTER_*. (except for the unknown error value)
typedef int WGPU_ERROR_TYPE;
#define WGPU_ERROR_TYPE_NO_ERROR      0
#define WGPU_ERROR_TYPE_OUT_OF_MEMORY 1
#define WGPU_ERROR_TYPE_VALIDATION    2
#define WGPU_ERROR_TYPE_UNKNOWN_ERROR 3

/*
[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUError {
    readonly attribute DOMString message;
};

[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUValidationError : GPUError {
    constructor(DOMString message);
};

[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUOutOfMemoryError : GPUError {
    constructor(DOMString message);
};

[Exposed=(Window, DedicatedWorker), SecureContext]
interface GPUInternalError : GPUError {
    constructor(DOMString message);
};

enum GPUErrorFilter {
    "validation",
    "out-of-memory",
    "internal"
};
*/
typedef int WGPU_ERROR_FILTER;
#define WGPU_ERROR_FILTER_INVALID       0
#define WGPU_ERROR_FILTER_OUT_OF_MEMORY 1
#define WGPU_ERROR_FILTER_VALIDATION    2
#define WGPU_ERROR_FILTER_INTERNAL      3

/*
partial interface GPUDevice {
    undefined pushErrorScope(GPUErrorFilter filter);
    Promise<GPUError?> popErrorScope();
};
*/
void wgpu_device_push_error_scope(WGpuDevice device, WGPU_ERROR_FILTER filter);

typedef void (*WGpuDeviceErrorCallback)(WGpuDevice device, WGPU_ERROR_TYPE errorType, const char *errorMessage NOTNULL, void *userData);
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

////////////////////////////////////////////////////////
// Sorted struct definitions for proper C parsing order:

/*
dictionary GPUCanvasConfiguration : GPUObjectDescriptorBase {
    required GPUDevice device;
    required GPUTextureFormat format;
    GPUTextureUsageFlags usage = 0x10;  // GPUTextureUsage.RENDER_ATTACHMENT
    sequence<GPUTextureFormat> viewFormats = [];
    PredefinedColorSpace colorSpace = "srgb";
    GPUCanvasAlphaMode alphaMode = "opaque";
    GPUExtent3D size;
};
*/
typedef struct WGpuCanvasConfiguration
{
  WGpuDevice device;
  WGPU_TEXTURE_FORMAT format;
  WGPU_TEXTURE_USAGE_FLAGS usage;
  int numViewFormats;
  WGPU_TEXTURE_FORMAT *viewFormats;
  HTML_PREDEFINED_COLOR_SPACE colorSpace;
  WGPU_CANVAS_ALPHA_MODE alphaMode;
} WGpuCanvasConfiguration;
extern const WGpuCanvasConfiguration WGPU_CANVAS_CONFIGURATION_DEFAULT_INITIALIZER;

/*
enum GPURenderPassTimestampLocation {
    "beginning",
    "end",
};
*/
typedef int WGPU_RENDER_PASS_TIMESTAMP_LOCATION;
#define WGPU_RENDER_PASS_TIMESTAMP_LOCATION_BEGINNING 0
#define WGPU_RENDER_PASS_TIMESTAMP_LOCATION_END       1

/*
dictionary GPURenderPassTimestampWrite {
    required GPUQuerySet querySet;
    required GPUSize32 queryIndex;
    required GPURenderPassTimestampLocation location;
};
*/
typedef struct WGpuRenderPassTimestampWrite
{
  WGpuQuerySet querySet;
  uint32_t queryIndex;
  WGPU_RENDER_PASS_TIMESTAMP_LOCATION location;
} WGpuRenderPassTimestampWrite;

/*
typedef sequence<GPURenderPassTimestampWrite> GPURenderPassTimestampWrites;

dictionary GPURenderPassDescriptor : GPUObjectDescriptorBase {
    required sequence<GPURenderPassColorAttachment?> colorAttachments;
    GPURenderPassDepthStencilAttachment depthStencilAttachment;
    GPUQuerySet occlusionQuerySet;
    GPURenderPassTimestampWrites timestampWrites = [];
    GPUSize64 maxDrawCount = 50000000;
};
*/
typedef struct WGpuRenderPassDescriptor
{
  int numColorAttachments;
  const WGpuRenderPassColorAttachment *colorAttachments;
  WGpuRenderPassDepthStencilAttachment depthStencilAttachment;
  WGpuQuerySet occlusionQuerySet;
  double_int53_t maxDrawCount; // If set to zero, the default value (50000000) will be used.
  uint32_t numTimestampWrites;
  WGpuRenderPassTimestampWrite *timestampWrites;
} WGpuRenderPassDescriptor;

typedef struct WGpuRenderPassColorAttachment
{
  WGpuTextureView view;
  WGpuTextureView resolveTarget;

  WGPU_STORE_OP storeOp; // Required, be sure to set to WGPU_STORE_OP_STORE (default) or WGPU_STORE_OP_DISCARD
  WGPU_LOAD_OP loadOp; // Either WGPU_LOAD_OP_LOAD (== default, 0) or WGPU_LOAD_OP_CLEAR.
  WGpuColor clearValue; // Used if loadOp == WGPU_LOAD_OP_CLEAR. Default value = { r = 0.0, g = 0.0, b = 0.0, a = 1.0 }
} WGpuRenderPassColorAttachment;
extern const WGpuRenderPassColorAttachment WGPU_RENDER_PASS_COLOR_ATTACHMENT_DEFAULT_INITIALIZER;

typedef struct WGpuImageCopyExternalImage
{
  WGpuObjectBase source; // must point to a WGpuImageBitmap (could also point to a HTMLVideoElement, HTMLCanvasElement or OffscreenCanvas, but those are currently unimplemented)
  WGpuOrigin2D origin;
  EM_BOOL flipY; // defaults to EM_FALSE.
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

  HTML_PREDEFINED_COLOR_SPACE colorSpace; // = "srgb";
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
  float depthBiasSlopeScale;
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

  // The member field blend.operation is default initialized to WGPU_BLEND_OPERATION_DISABLED (integer value 0)
  // to disable alpha blending on this color target. Set blend.operation to e.g. WGPU_BLEND_OPERATION_ADD to enable
  // alpha blending.
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
  WGpuPipelineLayout layout; // Set to special value WGPU_AUTO_LAYOUT_MODE_AUTO to specify that automatic layout should be used.
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

////////////////////////////////////////////////////////////////
// Extensions to the WebGPU specification:

typedef WGpuObjectBase WGpuImageBitmap;

// Called when the ImageBitmap finishes loading. If loading fails, this callback will be called with width==height==0.
typedef void (*WGpuLoadImageBitmapCallback)(WGpuImageBitmap bitmap, int width, int height, void *userData);

void wgpu_load_image_bitmap_from_url_async(const char *url NOTNULL, EM_BOOL flipY, WGpuLoadImageBitmapCallback callback, void *userData);


#ifdef __cplusplus
} // ~extern "C"
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif
