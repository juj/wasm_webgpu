{{{ (function() { global.wassert = function(condition) { return ASSERTIONS ? `assert(${condition});` : ''; }; return null; })(); }}}
{{{ (function() { global.wdebuglog = function(condition) { return ASSERTIONS ? `console.log(${condition});` : ''; }; return null; })(); }}}
{{{ (function() { global.wdebugwarn = function(condition) { return ASSERTIONS ? `console.warn(${condition});` : ''; }; return null; })(); }}}
{{{ (function() { global.wdebugerror = function(condition) { return ASSERTIONS ? `console.error(${condition});` : ''; }; return null; })(); }}}
{{{ (function() { global.wdebugdir = function(condition) { return ASSERTIONS ? `console.dir(${condition});` : ''; }; return null; })(); }}}
{{{ (function() { global.werror = function(condition) { return ASSERTIONS ? `console.error(${condition});` : ''; }; return null; })(); }}}

mergeInto(LibraryManager.library, {
  // Stores a ID->WebGPU object mapping registry of global top-level WebGPU objects.
  $wgpu: {},

  // Free ID counter generation number
  // 0: reserved for invalid object, 1: reserved for special GPUTexture that GPUSwapChain.getCurrentTexture() returns.
  $wgpuIdCounter: 1,

  // Generates a new free WebGPU object ID.
  $wgpuAllocId__deps: ['$wgpu', '$wgpuIdCounter'],
  $wgpuAllocId: function() {
    // WebGPU renderer usage can burn through a lot of object IDs each rendered frame (a number of GPUCommandEncoder, GPUTexture, GPUTextureView, GPURenderPassEncoder, GPUCommandBuffer objects are created each application frame)
    // If we assume an upper bound of 1000 object IDs created per rendered frame, and a new mobile device with 120hz display, a signed int32 state space is exhausted in
    // 2147483647 / 1000 / 120 / 60 / 60 = 4.97 hours, which is realistic for a page to stay open for that long. Therefore handle wraparound of the ID counter generation, and find free gaps in the object IDs for new objects.
    do {
      if (++wgpuIdCounter > 2147483647) wgpuIdCounter = 1; // Wraparound signed int32.
    } while(wgpu[wgpuIdCounter]);
    return wgpuIdCounter;
  },

  // Global constant string table for all WebGPU strings. Contains 187 entries, using 2396 bytes.
  wgpuStrings: [,'srgb','low-power','high-performance','depth-clamping','depth24unorm-stencil8','depth32float-stencil8','pipeline-statistics-query','texture-compression-bc','timestamp-query','1d','2d','3d','2d-array','cube','cube-array','all','stencil-only','depth-only','r8unorm','r8snorm','r8uint','r8sint','r16uint','r16sint','r16float','rg8unorm','rg8snorm','rg8uint','rg8sint','r32uint','r32sint','r32float','rg16uint','rg16sint','rg16float','rgba8unorm','rgba8unorm-srgb','rgba8snorm','rgba8uint','rgba8sint','bgra8unorm','bgra8unorm-srgb','rgb9e5ufloat','rgb10a2unorm','rg11b10ufloat','rg32uint','rg32sint','rg32float','rgba16uint','rgba16sint','rgba16float','rgba32uint','rgba32sint','rgba32float','stencil8','depth16unorm','depth24plus','depth24plus-stencil8','depth32float','bc1-rgba-unorm','bc1-rgba-unorm-srgb','bc2-rgba-unorm','bc2-rgba-unorm-srgb','bc3-rgba-unorm','bc3-rgba-unorm-srgb','bc4-r-unorm','bc4-r-snorm','bc5-rg-unorm','bc5-rg-snorm','bc6h-rgb-ufloat','bc6h-rgb-float','bc7-rgba-unorm','bc7-rgba-unorm-srgb','clamp-to-edge','repeat','mirror-repeat','nearest','linear','never','less','equal','less-equal','greater','not-equal','greater-equal','always','uniform','storage','read-only-storage','filtering','non-filtering','comparison','float','unfilterable-float','depth','sint','uint','read-only','write-only','error','warning','info','point-list','line-list','line-strip','triangle-list','triangle-strip','ccw','cw','none','front','back','zero','one','src','one-minus-src','src-alpha','one-minus-src-alpha','dst','one-minus-dst','dst-alpha','one-minus-dst-alpha','src-alpha-saturated','constant','one-minus-constant','add','subtract','reverse-subtract','min','max','keep','replace','invert','increment-clamp','decrement-clamp','increment-wrap','decrement-wrap','uint16','uint32','uint8x2','uint8x4','sint8x2','sint8x4','unorm8x2','unorm8x4','snorm8x2','snorm8x4','uint16x2','uint16x4','sint16x2','sint16x4','unorm16x2','unorm16x4','snorm16x2','snorm16x4','float16x2','float16x4','float32','float32x2','float32x3','float32x4','uint32x2','uint32x3','uint32x4','sint32','sint32x2','sint32x3','sint32x4','vertex','instance','load','store','clear','occlusion','pipeline-statistics','timestamp','vertex-shader-invocations','clipper-invocations','clipper-primitives-out','fragment-shader-invocations','compute-shader-invocations','opaque','premultiplied','destroyed','out-of-memory','validation'],

  // Stores a JS reference for newly allocated WebGPU object.
  $wgpuStore__deps: ['$wgpu', '$wgpuAllocId'],
  $wgpuStore: function(object) {
    if (object) {
      var id = wgpuAllocId();
      wgpu[id] = object;
      object.wid = id;
      return id;
    }
    {{{ werror('`WebGPU object creation failed!`'); }}}
  },

  // Marks the given 'object' to be a child/derived object of 'parent',
  // and stores a reference to the object in the WebGPU table,
  // returning the ID.
  $wgpuStoreAndSetParent__deps: ['$wgpuStore'],
  $wgpuStoreAndSetParent: function(object, parent) {
    var id = wgpuStore(object);
    parent.derivedObjects.push(id);
    return id;
  },

  wgpu_get_num_live_objects__deps: ['$wgpu'],
  wgpu_get_num_live_objects: function() {
    return Object.keys(wgpu).length;
  },

  // Calls .destroy() on the given WebGPU object, and releases the reference to it.
  wgpu_object_destroy__deps: ['$wgpu'],
  wgpu_object_destroy: function(object) {
    var o = wgpu[object];
    if (o) {
      // WebGPU objects of type GPUDevice, GPUBuffer, GPUTexture and GPUQuerySet have an explicit .destroy() function. Call that if applicable.
      if (o['destroy']) o['destroy']();
      // If the given object has derived objects (GPUTexture -> GPUTextureViews), delete those in a hierarchy as well.
      if (o.derivedObjects) {
        for(var d of o.derivedObjects) {
          _wgpu_object_destroy(d);
        }
      }
      // Finally erase reference to this object.
      delete wgpu[object];
    }
    {{{ wassert(`!(object in wgpu), 'wgpu dictionary should not be storing nulls/undefineds/zeroes!'`); }}}
  },

  wgpu_is_valid_object: function(o) { return !!wgpu[o]; }, // Tests if this ID references anything (not just a GPUObjectBase)
  wgpu_is_adapter: function(o) { return wgpu[o] instanceof GPUAdapter; },
  wgpu_is_device: function(o) { return wgpu[o] instanceof GPUDevice; },
  wgpu_is_buffer: function(o) { return wgpu[o] instanceof GPUBuffer; },
  wgpu_is_texture: function(o) { return wgpu[o] instanceof GPUTexture; },
  wgpu_is_texture_view: function(o) { return wgpu[o] instanceof GPUTextureView; },
  wgpu_is_external_texture: function(o) { return wgpu[o] instanceof GPUExternalTexture; },
  wgpu_is_sampler: function(o) { return wgpu[o] instanceof GPUSampler; },
  wgpu_is_bind_group_layout: function(o) { return wgpu[o] instanceof GPUBindGroupLayout; },
  wgpu_is_bind_group: function(o) { return wgpu[o] instanceof GPUBindGroup; },
  wgpu_is_pipeline_layout: function(o) { return wgpu[o] instanceof GPUBindPipelineLayout; },
  wgpu_is_shader_module: function(o) { return wgpu[o] instanceof GPUShaderModule; },
  wgpu_is_compute_pipeline: function(o) { return wgpu[o] instanceof GPUComputePipeline; },
  wgpu_is_render_pipeline: function(o) { return wgpu[o] instanceof GPURenderPipeline; },
  wgpu_is_command_buffer: function(o) { return wgpu[o] instanceof GPUCommandBuffer; },
  wgpu_is_command_encoder: function(o) { return wgpu[o] instanceof GPUCommandEncoder; },
  wgpu_is_programmable_pass_encoder: function(o) { return wgpu[o] instanceof GPUProgrammablePassEncoder; },
  wgpu_is_render_encoder_base: function(o) { return wgpu[o] instanceof GPURenderEncoderBase; },
  wgpu_is_render_pass_encoder: function(o) { return wgpu[o] instanceof GPURenderPassEncoder; },
  wgpu_is_render_bundle: function(o) { return wgpu[o] instanceof GPURenderBundle; },
  wgpu_is_render_bundle_encoder: function(o) { return wgpu[o] instanceof GPURenderBundleEncoder; },
  wgpu_is_queue: function(o) { return wgpu[o] instanceof GPUQueue; },
  wgpu_is_query_set: function(o) { return wgpu[o] instanceof GPUQuerySet; },
  wgpu_is_canvas_context: function(o) { return wgpu[o] instanceof GPUCanvasContext; },
  wgpu_is_swap_chain: function(o) { return wgpu[o] instanceof GPUSwapChain; },
  wgpu_is_device_lost_info: function(o) { return wgpu[o] instanceof GPUDeviceLostInfo; },
  wgpu_is_error: function(o) { return wgpu[o] instanceof GPUError; },

  wgpu_canvas_get_canvas_context__deps: ['$wgpuAllocId', '$wgpuStore'],
  wgpu_canvas_get_canvas_context: function(canvasSelector) {
    {{{ wdebuglog('`wgpu_canvas_get_canvas_context(canvasSelector=${UTF8ToString(canvasSelector)})`'); }}}
    var canvas = document.querySelector(UTF8ToString(canvasSelector));
    {{{ wdebuglog('`canvas:`'); }}}
    {{{ wdebugdir('canvas'); }}}
    var context = canvas.getContext('gpupresent');
    {{{ wdebuglog('`canvas.getContext("gpupresent")`'); }}}
    {{{ wdebugdir('context'); }}}
    return wgpuStore(context);
  },

  wgpu_canvas_context_get_swap_chain_preferred_format__deps: ['wgpuStrings'],
  wgpu_canvas_context_get_swap_chain_preferred_format: function(canvasContext, adapter) {
    {{{ wdebuglog('`wgpu_canvas_context_get_swap_chain_preferred_format: canvasContext: ${canvasContext}, adapter: ${adapter}`'); }}}
    {{{ wassert('wgpu[canvasContext]'); }}}
    {{{ wassert('wgpu[canvasContext].wid == canvasContext', "GPUCanvasContext has lost its wid member field!"); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter].wid == adapter', "GPUAdapter has lost its wid member field!"); }}}

    // TODO: Remove this when browsers implement updated spec.
    if (!wgpu[canvasContext].getSwapChainPreferredFormat) {
      console.error('GPUCanvasContext.getSwapChainPreferredFormat() is not implemented! Assuming bgra8unorm');
      return _wgpuStrings.indexOf('bgra8unorm');
    }

    {{{ wassert('_wgpuStrings.includes(wgpu[canvasContext].getSwapChainPreferredFormat(wgpu[adapter]))'); }}}
    return _wgpuStrings.indexOf(wgpu[canvasContext].getSwapChainPreferredFormat(wgpu[adapter]));
  },

  wgpu_canvas_context_configure_swap_chain: function(canvasContext, descriptor) {
    {{{ wdebuglog('`wgpu_canvascontext_configure_swap_chain(canvasContext=${canvasContext}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[canvasContext]'); }}}
    {{{ wassert('wgpu[canvasContext].wid == canvasContext', "GPUCanvasContext has lost its wid member field!"); }}}
    canvasContext = wgpu[canvasContext];

    var idx = descriptor >> 2;
    var device = HEAPU32[idx];
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device].wid == device', "GPUDevice has lost its wid member field!"); }}}
    var format = HEAPU32[idx+1];
    var usage = HEAPU32[idx+2];

    var gpuSwapChainDescriptor = {
      'device': wgpu[device],
      'format': _wgpuStrings[format],
      'usage': usage
    };
    {{{ wdebuglog('`calling canvasContext.configureSwapChain() with descriptor:`'); }}}
    {{{ wdebugdir('gpuSwapChainDescriptor'); }}}
    var swapChain = canvasContext['configureSwapChain'](gpuSwapChainDescriptor);
    {{{ wdebuglog('`canvasContext.configureSwapChain() produced a swap chain:`'); }}}
    {{{ wdebugdir('swapChain'); }}}

    // TODO: What happens on .configureSwapChain() failure?
    return wgpuStore(swapChain);
  },

  wgpu_swap_chain_get_current_texture__deps: ['wgpu_object_destroy'],
  wgpu_swap_chain_get_current_texture: function(swapChain) {
    {{{ wdebuglog('`wgpu_swap_chain_get_current_texture(swapChain=${swapChain})`'); }}}
    {{{ wassert('wgpu[swapChain]'); }}}
    {{{ wassert('wgpu[swapChain].wid == swapChain', "GPUSwapChain has lost its wid member field!"); }}}
    swapChain = wgpu[swapChain];

    // The Swap Chain Texture is a special texture that automatically invalidates itself after the current rAF()
    // callback if over. Therefore when a new swap chain texture is produced, we need to delete the old one to avoid
    // accumulating references to stale Swap Chain Textures from each frame.

    // Acquire the new Swap Chain Texture ...
    var texture = swapChain['getCurrentTexture']();
    if (texture != wgpu[1]) {
      // ... and destroy previous special Swap Chain Texture, if it was an old one.
      _wgpu_object_destroy(1);
      wgpu[1] = texture;
      texture.wid = 1;
      texture.derivedObjects = []; // GPUTextureViews are derived off of GPUTextures
    }
    return 1;
  },

  navigator_gpu_request_adapter_async__deps: ['$wgpuAllocId', '$dynCall', 'wgpuStrings'],
  navigator_gpu_request_adapter_async: function(options, adapterCallback, userData) {
    {{{ wdebuglog('`navigator_gpu_request_adapter_async: options: ${options}, adapterCallback: ${adapterCallback}, userData: ${userData}`'); }}}
    {{{ wassert('adapterCallback, "must pass a callback function to navigator_gpu_request_adapter_async!"'); }}}
    if (navigator['gpu']) {
      var id = wgpuAllocId();
      var powerPreference = options && _wgpuStrings[HEAPU32[options>>2]];
      var opts = powerPreference ? { 'powerPreference': powerPreference } : {};
      {{{ wdebuglog('`navigator.gpu.requestAdapter(options=${JSON.stringify(opts)})`'); }}}
      var adapterPromise = navigator['gpu']['requestAdapter'](opts);
      adapterPromise.then((adapter) => {
        {{{ wdebuglog('`navigator.gpu.requestAdapter resolved with following adapter:`'); }}}
        {{{ wdebugdir('adapter'); }}}
        wgpu[id] = adapter;
        adapter.wid = id;
        {{{ makeDynCall('vii', 'adapterCallback') }}}(id, userData);
      });
      return id;
    }
    {{{ werror('`WebGPU is not supported by the current browser!`'); }}}
    // implicit return 0, WebGPU is not supported.
  },

  wgpuFeatures: ['depth-clamping', 'depth24unorm-stencil8', 'depth32float-stencil8', 'pipeline-statistics-query', 'texture-compression-bc', 'timestamp-query'],

  wgpu_adapter_get_name: function(adapter, dstName, dstNameSize) {
    {{{ wdebuglog('`wgpu_adapter_get_limits(adapter: ${adapter}, dstName: ${dstName}, dstNameSize: ${dstNameSize})`'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter].wid == adapter', "GPUAdapter has lost its wid member field!"); }}}
    {{{ wassert('dstName != 0 || dstNameSize == 0, "passed a null dstName pointer, but with a non-zero dstNameSize length"'); }}}
    return stringToUTF8(wgpu[adapter].name, dstName, dstNameSize);
  },

  wgpu_adapter_get_features__deps: ['wgpuFeatures'],
  wgpu_adapter_get_features: function(adapter) {
    {{{ wdebuglog('`wgpu_adapter_get_features(adapter: ${adapter})`'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter].wid == adapter', "GPUAdapter has lost its wid member field!"); }}}
    var id = 1;
    var featuresBitMask = 0;
    for(var feature of _wgpuFeatures) {
      if (wgpu[adapter]['features'].includes(feature)) featuresBitMask |= id;
      id *= 2;
    }
    return featuresBitMask;
  },

  wgpu_adapter_supports_feature__deps: ['wgpuFeatures'],
  wgpu_adapter_supports_feature: function(adapter, feature) {
    {{{ wdebuglog('`wgpu_adapter_supports_feature(adapter: ${adapter}, feature: ${feature})`'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter].wid == adapter', "GPUAdapter has lost its wid member field!"); }}}
    return wgpu[adapter]['features'].includes(_wgpuFeatures[32 - Match.clz32(feature)])
  },

  wgpu_adapter_get_limits: function(adapter, limits) {
    {{{ wdebuglog('`wgpu_adapter_get_limits(adapter: ${adapter}, limits: ${limits})`'); }}}
    {{{ wassert('limits != 0, "passed a null limits struct pointer"'); }}}
    {{{ wassert('limits % 4 == 0, "passed an unaligned limits struct pointer"'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter].wid == adapter', "GPUAdapter has lost its wid member field!"); }}}

    var l = wgpu[adapter]['limits'];
    limits >>= 2;
    HEAPU32[limits++] = l['maxTextureDimension1D'];
    HEAPU32[limits++] = l['maxTextureDimension2D'];
    HEAPU32[limits++] = l['maxTextureDimension3D'];
    HEAPU32[limits++] = l['maxTextureArrayLayers'];
    HEAPU32[limits++] = l['maxBindGroups'];
    HEAPU32[limits++] = l['maxDynamicUniformBuffersPerPipelineLayout'];
    HEAPU32[limits++] = l['maxDynamicStorageBuffersPerPipelineLayout'];
    HEAPU32[limits++] = l['maxSampledTexturesPerShaderStage'];
    HEAPU32[limits++] = l['maxSamplersPerShaderStage'];
    HEAPU32[limits++] = l['maxStorageBuffersPerShaderStage'];
    HEAPU32[limits++] = l['maxStorageTexturesPerShaderStage'];
    HEAPU32[limits++] = l['maxUniformBuffersPerShaderStage'];
    HEAPU32[limits++] = l['maxUniformBufferBindingSize'];
    HEAPU32[limits++] = l['maxStorageBufferBindingSize'];
    HEAPU32[limits++] = l['maxVertexBuffers'];
    HEAPU32[limits++] = l['maxVertexAttributes'];
    HEAPU32[limits]   = l['maxVertexBufferArrayStride'];

    // Ensure that the GPUAdapter offers at least the limits that the spec guarantees:
    // https://gpuweb.github.io/gpuweb/#limits
#if ASSERTIONS
    if (adapter['limits']) {
      assert(adapter['limits']['maxTextureDimension1D'] >= 8192);
      assert(adapter['limits']['maxTextureDimension2D'] >= 8192);
      assert(adapter['limits']['maxTextureDimension3D'] >= 2048);
      assert(adapter['limits']['maxTextureArrayLayers'] >= 2048);
      assert(adapter['limits']['maxBindGroups'] >= 4);
      assert(adapter['limits']['maxDynamicUniformBuffersPerPipelineLayout'] >= 8);
      assert(adapter['limits']['maxDynamicStorageBuffersPerPipelineLayout'] >= 4);
      assert(adapter['limits']['maxSampledTexturesPerShaderStage'] >= 16);
      assert(adapter['limits']['maxSamplersPerShaderStage'] >= 16);
      assert(adapter['limits']['maxStorageBuffersPerShaderStage'] >= 4);
      assert(adapter['limits']['maxStorageTexturesPerShaderStage'] >= 4);
      assert(adapter['limits']['maxUniformBuffersPerShaderStage'] >= 12);
      assert(adapter['limits']['maxUniformBufferBindingSize'] >= 16384);
      assert(adapter['limits']['maxStorageBufferBindingSize'] >= 134217728);
      assert(adapter['limits']['maxVertexBuffers'] >= 8);
      assert(adapter['limits']['maxVertexAttributes'] >= 16);
      assert(adapter['limits']['maxVertexBufferArrayStride'] >= 2048);
    }
#endif
  },

  wgpu_adapter_request_device_async: function(adapter, descriptor, deviceCallback, userData) {
    {{{ wdebuglog('`wgpu_adapter_request_device_async(adapter: ${adapter}, deviceCallback: ${deviceCallback}, userData: ${userData})`'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter].wid == adapter', "GPUAdapter has lost its wid member field!"); }}}
    adapter = wgpu[adapter];
    descriptor = {
      // TODO: read descriptor
    };
    var id = wgpuAllocId();
    adapter['requestDevice'](descriptor).then((device) => {
      {{{ wdebuglog('`adapter.requestDevice resolved with following device:`'); }}}
      {{{ wdebugdir('device'); }}}
      // TODO Restore this one FF has implemented GPUDevice.adapter
      //{{{ wassert('device["adapter"].wid == adapter.wid', "Obtained GPUDevice.adapter member has lost its wid member field!"); }}}
      wgpu[id] = device;
      device.wid = id;
      device.derivedObjects = []; // A large number of objects are derived from GPUDevice (GPUBuffers, GPUTextures, GPUSamplers, ....)

      // Register an ID for the queue of this newly created device
      wgpuStore(device['queue']);

      {{{ makeDynCall('vii', 'deviceCallback') }}}(id, userData);
    });
  },

  wgpu_device_get_adapter: function(device) {
    {{{ wdebuglog('`wgpu_device_get_adapter(device=${device})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device].wid == device', "GPUDevice has lost its wid member field!"); }}}

    // TODO: Update this code once FF implements GPUDevice.adapter
//    {{{ wassert('wgpu[device]["adapter"]'); }}}
    if (!wgpu[device]['adapter']) { console.error('GPUDevice.adapter field is not implemented!'); return Object.values(wgpu).find((x) => x instanceof GPUAdapter).wid; }

    return wgpu[device]['adapter'].wid;
  },

  wgpu_device_get_queue: function(device) {
    {{{ wdebuglog('`wgpu_device_get_queue(device=${device})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device].wid == device', "GPUDevice has lost its wid member field!"); }}}
    {{{ wassert('wgpu[device]["queue"].wid', "GPUDevice.queue must have been assigned an ID in function wgpu_adapter_request_device!"); }}}
    return wgpu[device]['queue'].wid;
  },

  wgpu_device_create_shader_module__deps: ['$wgpuStoreAndSetParent'],
  wgpu_device_create_shader_module: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_shader_module(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device].wid == device', "GPUDevice has lost its wid member field!"); }}}
    {{{ wassert('descriptor'); }}}
    device = wgpu[device];

    var code = UTF8ToString(HEAPU32[descriptor>>2]);
    {{{ wdebuglog("`device.createShaderModule({code: ${code} }):`"); }}}
    var shaderModule = device['createShaderModule']({ code: code });
    {{{ wdebugdir('shaderModule'); }}}

    return wgpuStoreAndSetParent(shaderModule, device);
  },

  wgpu_device_create_buffer: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_shader_module(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device].wid == device', "GPUDevice has lost its wid member field!"); }}}
    {{{ wassert('descriptor'); }}}
    device = wgpu[device];

    var idx = descriptor>>2;
    var desc = {
      'size': readI53FromU64HeapIdx(idx),
      'usage': HEAPU32[idx+2],
      'mappedAtCreation': !!HEAP32[idx+3]
    };

    {{{ wdebuglog('`GPUDevice.createBuffer() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    var buffer = device['createBuffer'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('buffer'); }}}
    // Add tracking space for mapped ranges
    buffer.mappedRanges = {};

    return wgpuStoreAndSetParent(buffer, device);
  },

  wgpu_buffer_get_mapped_range: function(gpuBuffer, offset, size) {
    {{{ wdebuglog('`wgpu_buffer_get_mapped_range(gpuBuffer=${gpuBuffer}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('wgpu[gpuBuffer]'); }}}
    {{{ wassert('wgpu[gpuBuffer].wid == gpuBuffer', "GPUBuffer has lost its wid member field!"); }}}
    {{{ wassert('offset >= 0'); }}}

    {{{ wdebuglog("`device.getMappedRange(offset=${offset}, size=${size}):`"); }}}
    gpuBuffer = wgpu[gpuBuffer];
    gpuBuffer.mappedRanges[offset] = gpuBuffer['getMappedRange'](offset, size >= 0 ? size : (void 0));
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('gpuBuffer.mappedRanges[offset]'); }}}
    return offset;
  },

  wgpu_buffer_read_mapped_range: function(gpuBuffer, startOffset, subOffset, dst, size) {
    {{{ wdebuglog('`wgpu_buffer_read_mapped_range(gpuBuffer=${gpuBuffer}, startOffset=${startOffset}, subOffset=${subOffset}, dst=${dst}, size=${size})`'); }}}
    {{{ wassert('wgpu[gpuBuffer]'); }}}
    {{{ wassert('wgpu[gpuBuffer].wid == gpuBuffer', "GPUBuffer has lost its wid member field!"); }}}
    {{{ wassert('wgpu[gpuBuffer].mappedRanges[startOffset]', "wgpu_buffer_read_mapped_range: No such mapped range with specified startOffset!"); }}}
    {{{ wassert('dst || size == 0'); }}}
    {{{ wassert('subOffset >= 0'); }}}
    {{{ wassert('size >= 0'); }}}

    // N.b. this generates garbage because JavaScript does not allow ArrayBufferView.set(ArrayBuffer, offset, size), dst)
    // but must create a dummy view.
    HEAPU8.set(new Uint8Array(wgpu[wgpuBuffer].mappedRanges[startOffset], subOffset, size), dst);
  },

  wgpu_buffer_write_mapped_range: function(gpuBuffer, startOffset, subOffset, src, size) {
    {{{ wdebuglog('`wgpu_buffer_write_mapped_range(gpuBuffer=${gpuBuffer}, startOffset=${startOffset}, subOffset=${subOffset}, src=${src}, size=${size})`'); }}}
    {{{ wassert('wgpu[gpuBuffer]'); }}}
    {{{ wassert('wgpu[gpuBuffer].wid == gpuBuffer', "GPUBuffer has lost its wid member field!"); }}}
    {{{ wassert('wgpu[gpuBuffer].mappedRanges[startOffset]', "wgpu_buffer_write_mapped_range: No such mapped range with specified startOffset!"); }}}
    {{{ wassert('src || size == 0'); }}}
    {{{ wassert('subOffset >= 0'); }}}
    {{{ wassert('size >= 0'); }}}

    // Here 'buffer' refers to the global Wasm memory buffer.
    // N.b. generates garbage.
    new Uint8Array(wgpu[gpuBuffer].mappedRanges[startOffset]).set(new Uint8Array(buffer, src, size), subOffset);
  },

  wgpu_buffer_unmap: function(gpuBuffer) {
    {{{ wdebuglog('`wgpu_buffer_unmap(gpuBuffer=${gpuBuffer})`'); }}}
    {{{ wassert('wgpu[gpuBuffer]'); }}}
    {{{ wassert('wgpu[gpuBuffer].wid == gpuBuffer', "GPUBuffer has lost its wid member field!"); }}}
    wgpu[gpuBuffer]['unmap']();

    // Let GC reclaim all previous getMappedRange()s for this buffer.
    gpuBuffer.mappedRanges = {};
  },

  wgpuReadGpuStencilFaceState: function(idx) {
    return {
      'compare': _wgpuStrings[HEAP32[idx]],
      'failOp': _wgpuStrings[HEAP32[idx+1]],
      'depthFailOp': _wgpuStrings[HEAP32[idx+2]],
      'passOp': _wgpuStrings[HEAP32[idx+3]]
    };
  },

  wgpuReadGpuBlendState: function(idx) {
    return {
      'srcFactor': _wgpuStrings[HEAP32[idx]],
      'dstFactor': _wgpuStrings[HEAP32[idx+1]],
      'operation': _wgpuStrings[HEAP32[idx+2]]
    };
  },

  $readI53FromU64HeapIdx: function(idx) {
    return HEAPU32[idx] + HEAPU32[idx+1] * 4294967296;
  },

  wgpu_device_create_render_pipeline__deps: ['$readI53FromU64', 'wgpuReadGpuStencilFaceState', 'wgpuReadGpuBlendState', '$readI53FromU64HeapIdx', '$wgpuStoreAndSetParent'],
  wgpu_device_create_render_pipeline: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_render_pipeline(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device].wid == device', "GPUDevice has lost its wid member field!"); }}}
    {{{ wassert('descriptor'); }}}
    device = wgpu[device];

    var vertexBuffers = [];
    var vertexIdx = descriptor>>2;
    var numVertexBuffers = HEAP32[vertexIdx+2];
    var vertexBuffersIdx = HEAPU32[vertexIdx+3]>>2;
    while(numVertexBuffers--) {
      var attributes = [];
      var numAttributes = HEAP32[vertexBuffersIdx++];
      var attributesIdx = HEAPU32[vertexBuffersIdx++]>>2;
      while(numAttributes--) {
        attributes.push({
          'offset': readI53FromU64HeapIdx(attributesIdx),
          'shaderLocation': HEAP32[attributesIdx+2],
          'format': _wgpuStrings[HEAP32[attributesIdx+3]]
        });
        attributesIdx += 4;
      }
      vertexBuffers.push({
        'arrayStride': readI53FromU64HeapIdx(vertexBuffersIdx),
        'stepMode': _wgpuStrings[HEAP32[vertexBuffersIdx+2]],
        'attributes': attributes
      });
#if ASSERTIONS
      if (!(vertexBuffers[vertexBuffers.length-1].arrayStride > 0)) {
        console.error('wgpu_device_create_render_pipeline: arrayStride must be > 0! (in particular, arrayStride=0 does not mean "tight-packed")');
      }
#endif
    }
    var primitiveIdx = vertexIdx + 4;
    var depthStencilIdx = primitiveIdx + 4;
    var multisampleIdx = depthStencilIdx + 17;
    var fragmentIdx = multisampleIdx + 3;

    var targets = [];
    var numTargets = HEAP32[fragmentIdx+2];
    var targetsIdx = HEAPU32[fragmentIdx+3] >> 2;
    while(numTargets--) {
      targets.push({
        'format': _wgpuStrings[HEAP32[targetsIdx]],
        'blend': {
          'color': _wgpuReadGpuBlendState(targetsIdx+1),
          'alpha': _wgpuReadGpuBlendState(targetsIdx+4)
        },
        'writeMask': HEAP32[targetsIdx+7]
      });
      targetsIdx += 8;
    }

    var desc = {
      'vertex': {
        'module': wgpu[HEAP32[vertexIdx]],
        'entryPoint': UTF8ToString(HEAPU32[vertexIdx+1]),
        'buffers': vertexBuffers
      },
      'primitive': {
        'topology': _wgpuStrings[HEAP32[primitiveIdx]],
        'stripIndexFormat': _wgpuStrings[HEAP32[primitiveIdx+1]],
        'frontFace': _wgpuStrings[HEAP32[primitiveIdx+2]],
        'cullMode': _wgpuStrings[HEAP32[primitiveIdx+3]],
      }
    };
    // Chrome bug: we cannot statically write desc['depthStencil'] = null above.
    // but Chrome will complain that null.format member does not exist.
    var depthStencilFormat = HEAP32[depthStencilIdx++];
    if (depthStencilFormat) desc['depthStencil'] = {
        'format': _wgpuStrings[depthStencilFormat],
        'depthWriteEnabled': !!HEAP32[depthStencilIdx++],
        'depthCompare': _wgpuStrings[HEAP32[depthStencilIdx++]],
        'stencilReadMask': HEAPU32[depthStencilIdx++],
        'stencilWriteMask': HEAPU32[depthStencilIdx++],
        'depthBias': HEAP32[depthStencilIdx++],
        'depthBiasSlopeScale': HEAPF32[depthStencilIdx++],
        'depthBiasClamp': HEAPF32[depthStencilIdx++],
        'stencilFront': _wgpuReadGpuStencilFaceState(depthStencilIdx),
        'stencilBack': _wgpuReadGpuStencilFaceState(depthStencilIdx+4),
        'clampDepth': !!HEAP32[depthStencilIdx+8],
      };

    // Chrome bug: we cannot statically write desc['multisample'] with count=0,
    // but must dynamically populate the multisample object only if doing MSAA.
    var multisampleCount = HEAPU32[multisampleIdx];
    if (multisampleCount) desc['multisample'] = {
        'count': multisampleCount,
        'mask': HEAPU32[multisampleIdx+1],
        'alphaToCoverageEnabled': !!HEAPU32[multisampleIdx+2]
      };

    // Chrome bug: we cannot statically write desc['fragment']['module'] = null,
    // but must omit 'fragment' object altogether if no fragment module is to be used.
    var fragmentModule = HEAP32[fragmentIdx];
    if (fragmentModule) desc['fragment'] = {
        'module': wgpu[fragmentModule],
        'entryPoint': UTF8ToString(HEAPU32[fragmentIdx+1]),
        'targets': targets
      };


    {{{ wdebuglog('`GPUDevice.createRenderPipeline() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    var pipeline = device['createRenderPipeline'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('pipeline'); }}}

    return wgpuStoreAndSetParent(pipeline, device);
  },

  wgpu_device_create_command_encoder__deps: ['$wgpuStoreAndSetParent'],
  wgpu_device_create_command_encoder: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_command_encoder(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device].wid == device', "GPUDevice has lost its wid member field!"); }}}
    device = wgpu[device];

    var desc = descriptor ? {
      'measureExecutionTime': !!HEAP32[descriptor>>2]
    } : void 0;

    {{{ wdebuglog('`GPUDevice.createCommandEncoder() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    var commandEncoder = device['createCommandEncoder'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('commandEncoder'); }}}

    return wgpuStoreAndSetParent(commandEncoder, device);
  },

  wgpu_texture_create_view__deps: ['$wgpuStoreAndSetParent'],
  wgpu_texture_create_view: function(texture, descriptor) {
    {{{ wdebuglog('`wgpu_texture_create_view(texture=${texture}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[texture]'); }}}
    {{{ wassert('wgpu[texture].wid == texture', "GPUTexture has lost its wid member field!"); }}}
    var t = wgpu[texture];
    descriptor >>= 2;

    var desc = descriptor ? {
      'format': _wgpuStrings[HEAP32[descriptor]],
      'dimension': _wgpuStrings[HEAP32[descriptor+1]],
      'aspect': _wgpuStrings[HEAP32[descriptor+2]],
      'baseMipLevel': HEAPU32[descriptor+3],
      'mipLevelCount': HEAPU32[descriptor+4],
      'baseArrayLayer': HEAPU32[descriptor+5],
      'arrayLayerCount': HEAPU32[descriptor+6],
    } : void 0;

    {{{ wdebuglog('`GPUTexture.createView() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    var view = t['createView'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('view'); }}}

    return wgpuStoreAndSetParent(view, t);
  },

  wgpu_command_encoder_begin_render_pass: function(commandEncoder, descriptor) {
    {{{ wdebuglog('`wgpu_command_encoder_begin_render_pass(commandEncoder=${commandEncoder}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder].wid == commandEncoder', "GPUCommandEncoder has lost its wid member field!"); }}}
    commandEncoder = wgpu[commandEncoder];
    descriptor >>= 2;

    var colorAttachments = [];
    var numColorAttachments = HEAP32[descriptor++];
    var colorAttachmentsIdx = HEAPU32[descriptor++] >> 2;
    {{{ wassert('colorAttachmentsIdx % 2 == 0'); }}} // Must be aligned at double boundary
    while(numColorAttachments--) {
      colorAttachments.push({
        'view': wgpu[HEAP32[colorAttachmentsIdx]],
        'resolveTarget': wgpu[HEAP32[colorAttachmentsIdx+1]],
        'storeOp': _wgpuStrings[HEAP32[colorAttachmentsIdx+2]],
        'loadValue': _wgpuStrings[HEAP32[colorAttachmentsIdx+3]] || {
          'r': HEAPF64[colorAttachmentsIdx+4>>1],
          'g': HEAPF64[colorAttachmentsIdx+6>>1],
          'b': HEAPF64[colorAttachmentsIdx+8>>1],
          'a': HEAPF64[colorAttachmentsIdx+10>>1],
        }
      });

      colorAttachmentsIdx += 12;
    }

    var depthStencilView = wgpu[HEAP32[descriptor]];
    var desc = {
      'colorAttachments': colorAttachments,
      'depthStencilAttachment': depthStencilView ? {
        'view': depthStencilView,
        'depthLoadValue': _wgpuStrings[HEAP32[descriptor+1]] || HEAPF32[descriptor+2],
        'depthStoreOp': _wgpuStrings[HEAP32[descriptor+3]],
        'depthReadOnly': !!HEAP32[descriptor+4],
        'stencilLoadValue': _wgpuStrings[HEAP32[descriptor+5]] || HEAPU32[descriptor+6],
        'stencilStoreOp': _wgpuStrings[HEAP32[descriptor+7]],
        'stencilReadOnly': !!HEAP32[descriptor+8],
      } : void 0,
      'occlusionQuerySet': wgpu[HEAP32[descriptor+9]]
    };

    {{{ wdebuglog('`GPUCommandEncoder.beginRenderPass() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    {{{ wdebuglog('JSON.stringify(desc)'); }}}
    var renderPassEncoder = commandEncoder['beginRenderPass'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('renderPassEncoder'); }}}

    return wgpuStore(renderPassEncoder);
  },

  wgpu_render_encoder_base_set_pipeline: function(passEncoder, renderPipeline) {
    {{{ wdebuglog('`wgpu_render_encoder_base_set_pipeline(passEncoder=${passEncoder}, renderPipeline=${renderPipeline})`'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder].wid == passEncoder', "GPUPassEncoder has lost its wid member field!"); }}}
    {{{ wassert('wgpu[renderPipeline]'); }}}
    {{{ wassert('wgpu[renderPipeline].wid == renderPipeline', "GPURenderPipeline has lost its wid member field!"); }}}

    wgpu[passEncoder]['setPipeline'](wgpu[renderPipeline]);
  },

  wgpu_render_encoder_base_set_vertex_buffer: function(passEncoder, slot, buffer, offset, size) {
    {{{ wdebuglog('`wgpu_render_encoder_base_set_vertex_buffer(passEncoder=${passEncoder}, slot=${slot}, buffer=${buffer}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder].wid == passEncoder', "GPUPassEncoder has lost its wid member field!"); }}}
    {{{ wassert('wgpu[buffer]'); }}}
    {{{ wassert('wgpu[buffer].wid == buffer', "GPUBuffer has lost its wid member field!"); }}}

    wgpu[passEncoder]['setVertexBuffer'](slot, wgpu[buffer], offset, size);
  },

  wgpu_render_encoder_base_draw: function(passEncoder, vertexCount, instanceCount, firstVertex, firstInstance) {
    {{{ wdebuglog('`wgpu_render_encoder_base_draw(passEncoder=${passEncoder}, vertexCount=${vertexCount}, instanceCount=${instanceCount}, firstVertex=${firstVertex}, firstInstance=${firstInstance})`'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder].wid == passEncoder', "GPUPassEncoder has lost its wid member field!"); }}}

    wgpu[passEncoder]['draw'](vertexCount, instanceCount, firstVertex, firstInstance);
  },

  wgpu_render_pass_encoder_end_pass: function(passEncoder) {
    {{{ wdebuglog('`wgpu_render_pass_encoder_end_pass(passEncoder=${passEncoder})`'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder].wid == passEncoder', "GPUPassEncoder has lost its wid member field!"); }}}

    wgpu[passEncoder]['endPass']();

    // https://gpuweb.github.io/gpuweb/#render-pass-encoder-finalization:
    // "The render pass encoder can be ended by calling endPass() once the user has finished recording commands for the pass. Once endPass() has been called the render pass encoder can no longer be used."
    // Hence to help make C/C++ side code read easier, immediately release all references to the pass encoder after endPass() occurs, so that C/C++ side code does not need to release references manually.
    _wgpu_object_destroy(passEncoder);
  },

  wgpu_command_encoder_finish: function(commandEncoder) {
    {{{ wdebuglog('`wgpu_command_encoder_finish(commandEncoder=${commandEncoder})`'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder].wid == commandEncoder', "GPUCommandEncoder has lost its wid member field!"); }}}

    {{{ wdebuglog('`GPUCommandEncoder.finish()`'); }}}
    var commandBuffer = wgpu[commandEncoder]['finish']();
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('commandBuffer'); }}}

    // https://gpuweb.github.io/gpuweb/#command-encoder-finalization:
    // "A GPUCommandBuffer containing the commands recorded by the GPUCommandEncoder can be created by calling finish(). Once finish() has been called the command encoder can no longer be used."
    // Hence to help make C/C++ side code read easier, immediately release all references to the command encoder after finish() occurs, so that C/C++ side code does not need to release references manually.
    _wgpu_object_destroy(commandEncoder);

    return wgpuStore(commandBuffer);
  },

  wgpu_queue_submit_one: function(queue, commandBuffer) {
    {{{ wdebuglog('`wgpu_queue_submit(queue=${queue}, commandBuffer=${commandBuffer})`'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue].wid == queue', "GPUQueue has lost its wid member field!"); }}}
    {{{ wassert('wgpu[commandBuffer]'); }}}
    {{{ wassert('wgpu[commandBuffer].wid == commandBuffer', "GPUCommandBuffer has lost its wid member field!"); }}}

    wgpu[queue]['submit']([wgpu[commandBuffer]]);
  },

  wgpu_queue_submit_one_and_destroy__deps: ['wgpu_queue_submit_one', 'wgpu_object_destroy'],
  wgpu_queue_submit_one_and_destroy: function(queue, commandBuffer) {
    _wgpu_queue_submit_one(queue, commandBuffer);
    _wgpu_object_destroy(commandBuffer);
  },

  wgpu_queue_submit_multiple: function(queue, numCommandBuffers, commandBuffers) {
    {{{ wdebuglog('`wgpu_queue_submit_multiple(queue=${queue}, numCommandBuffers=${numCommandBuffers}, commandBuffers=${commandBuffers})`'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue].wid == queue', "GPUQueue has lost its wid member field!"); }}}
    {{{ wassert('commandBuffers'); }}}
    {{{ wassert('numCommandBuffers > 0'); }}}

    commandBuffers >>= 2;
    var buffers = [];
    while(numCommandBuffers--) {
      buffers.push(wgpu[HEAP32[commandBuffers++]]);
      {{{ wassert('buffers[buffers.length-1]'); }}}
    }

    wgpu[queue]['submit'](buffers);
  },

  wgpu_queue_submit_multiple_and_destroy: function(queue, numCommandBuffers, commandBuffers) {
    {{{ wdebuglog('`wgpu_queue_submit_multiple_and_destroy(queue=${queue}, numCommandBuffers=${numCommandBuffers}, commandBuffers=${commandBuffers})`'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue].wid == queue', "GPUQueue has lost its wid member field!"); }}}
    {{{ wassert('commandBuffers'); }}}
    {{{ wassert('numCommandBuffers > 0'); }}}

    commandBuffers >>= 2;
    var buffers = [];
    while(numCommandBuffers--) {
      buffers.push(wgpu[HEAP32[commandBuffers++]]);
      {{{ wassert('buffers[buffers.length-1]'); }}}
    }

    wgpu[queue]['submit'](buffers);

    for(var id of buffers) {
      _wgpu_object_destroy(id);
    }
  }
});
