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
  // 0: reserved for invalid object, 1: reserved for special GPUTexture that GPUPresentationContext.getCurrentTexture() returns.
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

  // Global constant string table for all WebGPU strings. Contains 187 entries, using 2398 bytes.
  wgpuStrings: [,'srgb','low-power','high-performance','depth-clamping','depth24unorm-stencil8','depth32float-stencil8','pipeline-statistics-query','texture-compression-bc','timestamp-query','1d','2d','3d','2d-array','cube','cube-array','all','stencil-only','depth-only','r8unorm','r8snorm','r8uint','r8sint','r16uint','r16sint','r16float','rg8unorm','rg8snorm','rg8uint','rg8sint','r32uint','r32sint','r32float','rg16uint','rg16sint','rg16float','rgba8unorm','rgba8unorm-srgb','rgba8snorm','rgba8uint','rgba8sint','bgra8unorm','bgra8unorm-srgb','rgb9e5ufloat','rgb10a2unorm','rg11b10ufloat','rg32uint','rg32sint','rg32float','rgba16uint','rgba16sint','rgba16float','rgba32uint','rgba32sint','rgba32float','stencil8','depth16unorm','depth24plus','depth24plus-stencil8','depth32float','bc1-rgba-unorm','bc1-rgba-unorm-srgb','bc2-rgba-unorm','bc2-rgba-unorm-srgb','bc3-rgba-unorm','bc3-rgba-unorm-srgb','bc4-r-unorm','bc4-r-snorm','bc5-rg-unorm','bc5-rg-snorm','bc6h-rgb-ufloat','bc6h-rgb-float','bc7-rgba-unorm','bc7-rgba-unorm-srgb','clamp-to-edge','repeat','mirror-repeat','nearest','linear','never','less','equal','less-equal','greater','not-equal','greater-equal','always','uniform','storage','read-only-storage','filtering','non-filtering','comparison','float','unfilterable-float','depth','sint','uint','read-only','write-only','error','warning','info','point-list','line-list','line-strip','triangle-list','triangle-strip','ccw','cw','none','front','back','zero','one','src','one-minus-src','src-alpha','one-minus-src-alpha','dst','one-minus-dst','dst-alpha','one-minus-dst-alpha','src-alpha-saturated','constant','one-minus-constant','add','subtract','reverse-subtract','min','max','keep','replace','invert','increment-clamp','decrement-clamp','increment-wrap','decrement-wrap','uint16','uint32','uint8x2','uint8x4','sint8x2','sint8x4','unorm8x2','unorm8x4','snorm8x2','snorm8x4','uint16x2','uint16x4','sint16x2','sint16x4','unorm16x2','unorm16x4','snorm16x2','snorm16x4','float16x2','float16x4','float32','float32x2','float32x3','float32x4','uint32x2','uint32x3','uint32x4','sint32','sint32x2','sint32x3','sint32x4','vertex','instance','load','store','discard','occlusion','pipeline-statistics','timestamp','vertex-shader-invocations','clipper-invocations','clipper-primitives-out','fragment-shader-invocations','compute-shader-invocations','opaque','premultiplied','destroyed','out-of-memory','validation'],

  // Stores a JS reference for newly allocated WebGPU object.
  $wgpuStore__deps: ['$wgpu', '$wgpuAllocId'],
  $wgpuStore: function(object) {
    if (object) {
      var id = wgpuAllocId();
      wgpu[id] = object;
      object.wid = id; // Each persisted objects gets a custom 'wid' field which stores the ID that this object is known by on Wasm side.
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
  wgpu_is_presentation_context: function(o) { return wgpu[o] instanceof GPUPresentationContext; },
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

  wgpu_presentation_context_get_preferred_format__deps: ['wgpuStrings'],
  wgpu_presentation_context_get_preferred_format: function(presentationContext, adapter) {
    {{{ wdebuglog('`wgpu_presentation_context_get_preferred_format: presentationContext: ${presentationContext}, adapter: ${adapter}`'); }}}
    {{{ wassert('wgpu[presentationContext]'); }}}
    {{{ wassert('wgpu[adapter]'); }}}

    {{{ wassert('_wgpuStrings.includes(wgpu[presentationContext].getPreferredFormat(wgpu[adapter]))'); }}}
    return _wgpuStrings.indexOf(wgpu[presentationContext].getPreferredFormat(wgpu[adapter]));
  },

  wgpu_presentation_context_configure: function(presentationContext, config) {
    {{{ wdebuglog('`wgpu_presentation_context_configure(presentationContext=${presentationContext}, config=${config})`'); }}}
    {{{ wassert('wgpu[presentationContext]'); }}}
    presentationContext = wgpu[presentationContext];

    config >>= 2;
    var c = {
      'device': wgpu[HEAPU32[config]],
      'format': _wgpuStrings[HEAPU32[config+1]],
      'usage': HEAPU32[config+2],
      'compositingAlphaMode': _wgpuStrings[HEAPU32[config+3]],
      'size': [HEAP32[config+4], HEAP32[config+5], HEAP32[config+6]]
    };
    {{{ wdebuglog('`calling presentationContext.configure() with config:`'); }}}
    {{{ wdebugdir('c'); }}}
    presentationContext['configure'](c);
  },

  wgpu_presentation_context_get_current_texture__deps: ['wgpu_object_destroy'],
  wgpu_presentation_context_get_current_texture: function(presentationContext) {
    {{{ wdebuglog('`wgpu_presentation_context_get_current_texture(presentationContext=${presentationContext})`'); }}}
    {{{ wassert('wgpu[presentationContext]'); }}}
    presentationContext = wgpu[presentationContext];

    // The presentation context texture is a special texture that automatically invalidates itself after the current rAF()
    // callback if over. Therefore when a new swap chain texture is produced, we need to delete the old one to avoid
    // accumulating references to stale textures from each frame.

    // Acquire the new presentation context texture..
    var texture = presentationContext['getCurrentTexture']();
    if (texture != wgpu[1]) {
      // ... and destroy previous special presentation context texture, if it was an old one.
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
    {{{ wdebuglog('`wgpu_adapter_get_name(adapter: ${adapter}, dstName: ${dstName}, dstNameSize: ${dstNameSize})`'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('dstName != 0 || dstNameSize == 0, "passed a null dstName pointer, but with a non-zero dstNameSize length"'); }}}
    return stringToUTF8(wgpu[adapter].name, dstName, dstNameSize);
  },

  wgpu_adapter_or_device_get_features__deps: ['wgpuFeatures'],
  wgpu_adapter_or_device_get_features: function(adapterOrDevice) {
    {{{ wdebuglog('`wgpu_adapter_or_device_get_features(adapterOrDevice: ${adapterOrDevice})`'); }}}
    {{{ wassert('wgpu[adapterOrDevice]'); }}}
    var id = 1;
    var featuresBitMask = 0;
    for(var feature of _wgpuFeatures) {
      if (wgpu[adapterOrDevice]['features'].has(feature)) featuresBitMask |= id;
      id *= 2;
    }
    return featuresBitMask;
  },

  wgpu_adapter_or_device_supports_feature__deps: ['wgpuFeatures'],
  wgpu_adapter_or_device_supports_feature: function(adapterOrDevice, feature) {
    {{{ wdebuglog('`wgpu_adapter_or_device_supports_feature(adapterOrDevice: ${adapterOrDevice}, feature: ${feature})`'); }}}
    {{{ wassert('wgpu[adapterOrDevice]'); }}}
    return wgpu[adapterOrDevice]['features'].has(_wgpuFeatures[32 - Match.clz32(feature)])
  },

  wgpu_adapter_or_device_get_limits: function(adapterOrDevice, limits) {
    {{{ wdebuglog('`wgpu_adapter_or_device_get_limits(adapterOrDevice: ${adapterOrDevice}, limits: ${limits})`'); }}}
    {{{ wassert('limits != 0, "passed a null limits struct pointer"'); }}}
    {{{ wassert('limits % 4 == 0, "passed an unaligned limits struct pointer"'); }}}
    {{{ wassert('wgpu[adapterOrDevice]'); }}}

    var l = wgpu[adapterOrDevice]['limits'];
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
  },

  wgpu_adapter_is_software: function(adapter) {
    {{{ wdebuglog('`wgpu_adapter_is_software(adapter: ${adapter}`'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    return wgpu[adapter]['isSoftware'];
  },

  wgpu_adapter_request_device_async: function(adapter, descriptor, deviceCallback, userData) {
    {{{ wdebuglog('`wgpu_adapter_request_device_async(adapter: ${adapter}, deviceCallback: ${deviceCallback}, userData: ${userData})`'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    adapter = wgpu[adapter];
    descriptor = {
      // TODO: read descriptor
    };
    var id = wgpuAllocId();
    adapter['requestDevice'](descriptor).then((device) => {
      {{{ wdebuglog('`adapter.requestDevice resolved with following device:`'); }}}
      {{{ wdebugdir('device'); }}}
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
    {{{ wassert('descriptor'); }}}
    device = wgpu[device];

    var code = UTF8ToString(HEAPU32[descriptor>>2]);
    {{{ wdebuglog("`device.createShaderModule({code: ${code} }):`"); }}}
    var shaderModule = device['createShaderModule']({ code: code });
    {{{ wdebugdir('shaderModule'); }}}

    return wgpuStoreAndSetParent(shaderModule, device);
  },

  wgpu_device_create_buffer__deps: ['$wgpuReadI53FromU64HeapIdx'],
  wgpu_device_create_buffer: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_shader_module(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('descriptor'); }}}
    device = wgpu[device];

    var idx = descriptor>>2;
    var desc = {
      'size': wgpuReadI53FromU64HeapIdx(idx),
      'usage': HEAPU32[idx+2],
      'mappedAtCreation': !!HEAPU32[idx+3]
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
    wgpu[gpuBuffer]['unmap']();

    // Let GC reclaim all previous getMappedRange()s for this buffer.
    gpuBuffer.mappedRanges = {};
  },

  $wgpuReadGpuStencilFaceState: function(idx) {
    return {
      'compare': _wgpuStrings[HEAPU32[idx]],
      'failOp': _wgpuStrings[HEAPU32[idx+1]],
      'depthFailOp': _wgpuStrings[HEAPU32[idx+2]],
      'passOp': _wgpuStrings[HEAPU32[idx+3]]
    };
  },

  $wgpuReadGpuBlendState: function(idx) {
    return {
      'srcFactor': _wgpuStrings[HEAPU32[idx]],
      'dstFactor': _wgpuStrings[HEAPU32[idx+1]],
      'operation': _wgpuStrings[HEAPU32[idx+2]]
    };
  },

  $wgpuReadI53FromU64HeapIdx: function(idx) {
    return HEAPU32[idx] + HEAPU32[idx+1] * 4294967296;
  },

  wgpu_device_create_render_pipeline__deps: ['$readI53FromU64', '$wgpuReadGpuStencilFaceState', '$wgpuReadGpuBlendState', '$wgpuReadI53FromU64HeapIdx', '$wgpuStoreAndSetParent'],
  wgpu_device_create_render_pipeline: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_render_pipeline(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('descriptor'); }}}
    device = wgpu[device];

    var vertexBuffers = [];
    var vertexIdx = descriptor>>2;
    var numVertexBuffers = HEAP32[vertexIdx+2];
    var vertexBuffersIdx = HEAPU32[vertexIdx+3]>>2;
    while(numVertexBuffers--) {
      var attributes = [];
      var numAttributes = HEAP32[vertexBuffersIdx];
      var attributesIdx = HEAPU32[vertexBuffersIdx+1]>>2;
      while(numAttributes--) {
        attributes.push({
          'offset': wgpuReadI53FromU64HeapIdx(attributesIdx),
          'shaderLocation': HEAPU32[attributesIdx+2],
          'format': _wgpuStrings[HEAPU32[attributesIdx+3]]
        });
        attributesIdx += 4;
      }
      vertexBuffers.push({
        'arrayStride': wgpuReadI53FromU64HeapIdx(vertexBuffersIdx+2),
        'stepMode': _wgpuStrings[HEAPU32[vertexBuffersIdx+4]],
        'attributes': attributes
      });
      vertexBuffersIdx += 6;
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
        'format': _wgpuStrings[HEAPU32[targetsIdx]],
        'blend': {
          'color': wgpuReadGpuBlendState(targetsIdx+1),
          'alpha': wgpuReadGpuBlendState(targetsIdx+4)
        },
        'writeMask': HEAPU32[targetsIdx+7]
      });
      targetsIdx += 8;
    }

    var desc = {
      'vertex': {
        'module': wgpu[HEAPU32[vertexIdx]],
        'entryPoint': UTF8ToString(HEAPU32[vertexIdx+1]),
        'buffers': vertexBuffers
      },
      'primitive': {
        'topology': _wgpuStrings[HEAPU32[primitiveIdx]],
        'stripIndexFormat': _wgpuStrings[HEAPU32[primitiveIdx+1]],
        'frontFace': _wgpuStrings[HEAPU32[primitiveIdx+2]],
        'cullMode': _wgpuStrings[HEAPU32[primitiveIdx+3]],
      }
    };
    // Chrome bug: we cannot statically write desc['depthStencil'] = null above.
    // but Chrome will complain that null.format member does not exist.
    var depthStencilFormat = HEAPU32[depthStencilIdx++];
    if (depthStencilFormat) desc['depthStencil'] = {
        'format': _wgpuStrings[depthStencilFormat],
        'depthWriteEnabled': !!HEAPU32[depthStencilIdx++],
        'depthCompare': _wgpuStrings[HEAPU32[depthStencilIdx++]],
        'stencilReadMask': HEAPU32[depthStencilIdx++],
        'stencilWriteMask': HEAPU32[depthStencilIdx++],
        'depthBias': HEAP32[depthStencilIdx++],
        'depthBiasSlopeScale': HEAPF32[depthStencilIdx++],
        'depthBiasClamp': HEAPF32[depthStencilIdx++],
        'stencilFront': wgpuReadGpuStencilFaceState(depthStencilIdx),
        'stencilBack': wgpuReadGpuStencilFaceState(depthStencilIdx+4),
        'clampDepth': !!HEAPU32[depthStencilIdx+8],
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
    var fragmentModule = HEAPU32[fragmentIdx];
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
    device = wgpu[device];

    var desc = descriptor ? {
      'measureExecutionTime': !!HEAPU32[descriptor>>2]
    } : void 0;

    {{{ wdebuglog('`GPUDevice.createCommandEncoder() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    var commandEncoder = device['createCommandEncoder'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('commandEncoder'); }}}

    return wgpuStoreAndSetParent(commandEncoder, device);
  },

  wgpu_device_create_texture: function(device, descriptor) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_device_create_texture(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('descriptor != 0'); }}} // Must be non-null
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    device = wgpu[device];

    descriptor >>= 2;
    var desc = {
      'size': [HEAP32[descriptor], HEAP32[descriptor+1], HEAP32[descriptor+2]],
      'mipLevelCount': HEAP32[descriptor+3],
      'sampleCount': HEAP32[descriptor+4],
      'dimension': _wgpuStrings[HEAPU32[descriptor+5]],
      'format': _wgpuStrings[HEAPU32[descriptor+6]],
      'usage': HEAPU32[descriptor+7],
    };
    {{{ wdebuglog('`GPUDevice.createTexture() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    var texture = device['createTexture'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('texture'); }}}

    return wgpuStoreAndSetParent(texture, device);
  },

  wgpu_device_create_sampler: function(device, descriptor) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_device_create_sampler(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    device = wgpu[device];

    descriptor >>= 2;
    var desc = desc ? {
      'addressModeU': _wgpuStrings[HEAPU32[descriptor]],
      'addressModeV': _wgpuStrings[HEAPU32[descriptor+1]],
      'addressModeW': _wgpuStrings[HEAPU32[descriptor+2]],
      'magFilter': _wgpuStrings[HEAPU32[descriptor+3]],
      'minFilter': _wgpuStrings[HEAPU32[descriptor+4]],
      'mipmapFilter': _wgpuStrings[HEAPU32[descriptor+5]],
      'lodMinClamp': HEAPF32[descriptor+6],
      'lodMaxClamp': HEAPF32[descriptor+7],
      'compare': _wgpuStrings[HEAPU32[descriptor+8]],
      'maxAnisotropy': _wgpuStrings[HEAPU32[descriptor+9]],
    } : void 0;
    {{{ wdebuglog('`GPUDevice.createSampler() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    var sampler = device['createSampler'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('sampler'); }}}

    return wgpuStoreAndSetParent(sampler, device);
  },

  wgpu_device_create_bind_group_layout: function(device, entries, numEntries) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_device_create_bind_group_layout(device=${device}, entries=${entries}, numEntries=${numEntries})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('entries != 0 || numEntries <= 0'); }}} // Must be non-null pointer
    {{{ wassert('entries % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    entries >>= 2;
    var e = [];
    while(numEntries--) {
      var entry = {
        'binding': HEAPU32[entries],
        'visibility': HEAPU32[entries+1],
      };
      var type = HEAPU32[entries+2];
      entries += 4;
      {{{ wassert('type >= 0 && type <= 5'); }}}
      if (type == 1/*WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER*/) {
        entry['buffer'] = {
          'type': _wgpuStrings[HEAPU32[entries]],
          'hasDynamicOffset': !!HEAPU32[entries+1],
          'minBindingSize': readI53FromU64(entries+2)
        };
      } else if (type == 2/*WGPU_BIND_GROUP_LAYOUT_TYPE_SAMPLER*/) {
        entry['sampler'] = {
          'type': _wgpuStrings[HEAPU32[entries]]
        };
      } else if (type == 3/*WGPU_BIND_GROUP_LAYOUT_TYPE_TEXTURE*/) {
        entry['texture'] = {
          'sampleType': _wgpuStrings[HEAPU32[entries]],
          'viewDimension': _wgpuStrings[HEAPU32[entries+1]]
        };
      } else if (type == 4/*WGPU_BIND_GROUP_LAYOUT_TYPE_STORAGE_TEXTURE*/) {
        entry['storageTexture'] = {
          'access': _wgpuStrings[HEAPU32[entries]],
          'format': _wgpuStrings[HEAPU32[entries+1]],
          'viewDimension': _wgpuStrings[HEAPU32[entries+2]]
        };
      } else { // type == 5/*WGPU_BIND_GROUP_LAYOUT_TYPE_EXTERNAL_TEXTURE*/ {
        entry['externalTexture'] = {};
      }
      entries += 4;
      e.push(entry);
    }
    var desc = {
      'entries': e
    }
    {{{ wdebuglog('`GPUDevice.createBindGroupLayout() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    var bgl = wgpu[device]['createBindGroupLayout'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('bgl'); }}}

    return wgpuStoreAndSetParent(bgl, device);
  },

  wgpu_device_create_pipeline_layout: function(device, layouts, numLayouts) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_device_create_pipeline_layout(device=${device}, layouts=${layouts}, numLayouts=${numLayouts})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('layouts != 0 || numLayouts <= 0'); }}} // Must be non-null pointer
    {{{ wassert('layouts % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    layouts >>= 2;
    var e = [];
    while(numLayouts--) {
      {{{ wassert('wgpu[HEAPU32[layouts]]'); }}} // Must reference a valid BindGroupLayout
      e.push(wgpu[HEAPU32[layouts++]]);
    }

    var desc = {
      'bindGroupLayouts': e
    }
    {{{ wdebuglog('`GPUDevice.createPipelineLayout() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    var pipelineLayout = wgpu[device]['createPipelineLayout'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('pipelineLayout'); }}}

    return wgpuStoreAndSetParent(pipelineLayout, device);
  },

  wgpu_device_create_bind_group: function(device, layout, entries, numEntries) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_device_create_bind_group(device=${device}, layout=${layout}, entries=${entries}, numEntries=${numEntries})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[layout]'); }}} // Must be a valid BindGroupLayout
    {{{ wassert('entries != 0 || numEntries <= 0'); }}} // Must be non-null pointer
    {{{ wassert('entries % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    entries >>= 2;
    var e = [];
    while(numEntries--) {
      e.push({ 'binding': HEAPU32[entries++],
        'resource': wgpu[HEAPU32[entries++]]
      });
      {{{ wassert('e[e.length-1].resource'); }}} // Must reference a valid BindGroupLayout
    }

    var desc = {
      'layout': wgpu[layout],
      'entries': e
    }
    {{{ wdebuglog('`GPUDevice.createBindGroup() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    var bindGroup = wgpu[device]['createBindGroup'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('bindGroup'); }}}

    return wgpuStoreAndSetParent(bindGroup, device);
  },

  wgpu_device_create_compute_pipeline: function(device, computeModule, entryPoint) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_device_create_compute_pipeline(device=${device}, computeModule=${computeModule}, entryPoint=${entryPoint}`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[computeModule]'); }}} // Must be a valid GPUShaderModule
    {{{ wassert('wgpu[entryPoint]'); }}} // Must be a valid C string
    {{{ wassert('UTF8ToString(wgpu[entryPoint]).length > 0'); }}} // Must be a nonempty JS string

    var desc = {
      'compute': {
        'module': wgpu[computeModule],
        'entryPoint': UTF8ToString(entryPoint)
      }
    };
    {{{ wdebuglog('`GPUDevice.createComputePipeline() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    var computePipeline = wgpu[device]['createComputePipeline'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('computePipeline'); }}}

    return wgpuStoreAndSetParent(computePipeline, device);
  },

  wgpu_texture_create_view__deps: ['$wgpuStoreAndSetParent'],
  wgpu_texture_create_view: function(texture, descriptor) {
    {{{ wdebuglog('`wgpu_texture_create_view(texture=${texture}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[texture]'); }}}
    var t = wgpu[texture];
    descriptor >>= 2;

    var desc = descriptor ? {
      'format': _wgpuStrings[HEAPU32[descriptor]],
      'dimension': _wgpuStrings[HEAPU32[descriptor+1]],
      'aspect': _wgpuStrings[HEAPU32[descriptor+2]],
      'baseMipLevel': HEAP32[descriptor+3],
      'mipLevelCount': HEAP32[descriptor+4],
      'baseArrayLayer': HEAP32[descriptor+5],
      'arrayLayerCount': HEAP32[descriptor+6],
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
    commandEncoder = wgpu[commandEncoder];
    descriptor >>= 2;

    var colorAttachments = [];
    var numColorAttachments = HEAP32[descriptor++];
    var colorAttachmentsIdx = HEAPU32[descriptor++] >> 2;
    {{{ wassert('colorAttachmentsIdx % 2 == 0'); }}} // Must be aligned at double boundary
    while(numColorAttachments--) {
      colorAttachments.push({
        'view': wgpu[HEAPU32[colorAttachmentsIdx]],
        'resolveTarget': wgpu[HEAPU32[colorAttachmentsIdx+1]],
        'storeOp': _wgpuStrings[HEAPU32[colorAttachmentsIdx+2]],
        'loadValue': _wgpuStrings[HEAPU32[colorAttachmentsIdx+3]] || {
          'r': HEAPF64[colorAttachmentsIdx+4>>1],
          'g': HEAPF64[colorAttachmentsIdx+6>>1],
          'b': HEAPF64[colorAttachmentsIdx+8>>1],
          'a': HEAPF64[colorAttachmentsIdx+10>>1],
        }
      });

      colorAttachmentsIdx += 12;
    }

    var depthStencilView = wgpu[HEAPU32[descriptor]];
    var desc = {
      'colorAttachments': colorAttachments,
      'depthStencilAttachment': depthStencilView ? {
        'view': depthStencilView,
        'depthLoadValue': _wgpuStrings[HEAPU32[descriptor+1]] || HEAPF32[descriptor+2],
        'depthStoreOp': _wgpuStrings[HEAPU32[descriptor+3]],
        'depthReadOnly': !!HEAPU32[descriptor+4],
        'stencilLoadValue': _wgpuStrings[HEAPU32[descriptor+5]] || HEAPU32[descriptor+6],
        'stencilStoreOp': _wgpuStrings[HEAPU32[descriptor+7]],
        'stencilReadOnly': !!HEAPU32[descriptor+8],
      } : void 0,
      'occlusionQuerySet': wgpu[HEAPU32[descriptor+9]]
    };

    {{{ wdebuglog('`GPUCommandEncoder.beginRenderPass() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    {{{ wdebuglog('JSON.stringify(desc)'); }}}
    var renderPassEncoder = commandEncoder['beginRenderPass'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('renderPassEncoder'); }}}

    return wgpuStore(renderPassEncoder);
  },

  wgpu_command_encoder_begin_compute_pass: function(commandEncoder, descriptor) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_begin_compute_pass(commandEncoder=${commandEncoder}, descriptor=${descriptor})`'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    commandEncoder = wgpu[commandEncoder];
    descriptor >>= 2;

    var desc = {}; // TODO: Read label
    {{{ wdebuglog('`GPUCommandEncoder.beginComputePass() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    var computePassEncoder = commandEncoder['beginComputePass'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('computePassEncoder'); }}}

    return wgpuStore(computePassEncoder);
  },

  wgpu_command_encoder_copy_buffer_to_buffer: function(commandEncoder, source, sourceOffset, destination, destinationOffset, size) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_copy_buffer_to_buffer(commandEncoder=${commandEncoder}, source=${source}, sourceOffset=${sourceOffset}, destination=${destination}, destinationOffset=${destinationOffset}, size=${size})`'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[source]'); }}}
    wgpu[commandEncoder]['copyBufferToBuffer'](wgpu[source], sourceOffset, destination, destinationOffset, size);
  },

  $wgpuReadGpuImageCopyBuffer__deps: ['$wgpuReadI53FromU64HeapIdx'],
  $wgpuReadGpuImageCopyBuffer: function(ptr) {
    ptr >>= 2;
    return {
      'offset': wgpuReadI53FromU64HeapIdx(ptr),
      'bytesPerRow': HEAP32[ptr+2],
      'rowsPerImage': HEAP32[ptr+3],
      'buffer': wgpu[HEAPU32[ptr+4]]
    };
  },

  $wgpuReadGpuImageCopyTexture: function(ptr) {
    {{{ wassert('ptr'); }}}
    {{{ wassert('ptr % 4 == 0'); }}}
    ptr >>= 2;
    return {
      'texture': wgpu[HEAPU32[ptr]],
      'mipLevel': HEAP32[ptr+1],
      'origin': [HEAP32[ptr+2], HEAP32[ptr+3], HEAP32[ptr+4]],
      'aspect': _wgpuStrings[HEAPU32[ptr+5]]
    };
  },

  wgpu_command_encoder_copy_buffer_to_texture__deps: ['$wgpuReadGpuImageCopyBuffer', '$wgpuReadGpuImageCopyTexture'],
  wgpu_command_encoder_copy_buffer_to_texture: function(commandEncoder, source, destination, copyWidth, copyHeight, copyDepthOrArrayLayers) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_copy_buffer_to_texture(commandEncoder=${commandEncoder}, source=${source}, destination=${destination}, copyWidth=${copyWidth}, copyHeight=${copyHeight}, copyDepthOrArrayLayers=${copyDepthOrArrayLayers})`'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[source]'); }}}
    wgpu[commandEncoder]['copyBufferToTexture'](wgpu[source], source, wgpuReadGpuImageCopyTexture(destination), [copyWidth, copyHeight, copyDepthOrArrayLayers]);
  },

  wgpu_command_encoder_copy_texture_to_buffer__deps: ['$wgpuReadGpuImageCopyTexture', '$wgpuReadGpuImageCopyBuffer'],
  wgpu_command_encoder_copy_texture_to_buffer: function(commandEncoder, source, destination, copyWidth, copyHeight, copyDepthOrArrayLayers) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_copy_texture_to_buffer(commandEncoder=${commandEncoder}, source=${source}, destination=${destination}, copyWidth=${copyWidth}, copyHeight=${copyHeight}, copyDepthOrArrayLayers=${copyDepthOrArrayLayers})`'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[source]'); }}}
    {{{ wassert('wgpu[destination]'); }}}
    wgpu[commandEncoder]['copyTextureToBuffer'](wgpu[source], wgpuReadGpuImageCopyTexture(source), destination, [copyWidth, copyHeight, copyDepthOrArrayLayers]);
  },

  wgpu_command_encoder_copy_texture_to_texture__deps: ['$wgpuReadGpuImageCopyTexture'],
  wgpu_command_encoder_copy_texture_to_texture: function(commandEncoder, source, destination, copyWidth, copyHeight, copyDepthOrArrayLayers) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_copy_texture_to_texture(commandEncoder=${commandEncoder}, source=${source}, destination=${destination}, copyWidth=${copyWidth}, copyHeight=${copyHeight}, copyDepthOrArrayLayers=${copyDepthOrArrayLayers})`'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[source]'); }}}
    {{{ wassert('wgpu[destination]'); }}}
    wgpu[commandEncoder]['copyTextureToTexture'](wgpu[source], wgpuReadGpuImageCopyTexture(source), wgpuReadGpuImageCopyTexture(destination), [copyWidth, copyHeight, copyDepthOrArrayLayers]);
  },

  wgpu_encoder_push_debug_group: function(encoder, groupLabel) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_push_debug_group(encoder=${encoder}, groupLabel=${groupLabel})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[groupLabel]'); }}}
    wgpu[encoder]['pushDebugGroup'](UTF8ToString(groupLabel));
  },

  wgpu_encoder_pop_debug_group: function(encoder) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_pop_debug_group(encoder=${encoder})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    wgpu[encoder]['popDebugGroup']();
  },

  wgpu_encoder_insert_debug_marker: function(encoder, markerLabel) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_insert_debug_marker(encoder=${encoder}, markerLabel=${markerLabel})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[markerLabel]'); }}}
    wgpu[encoder]['insertDebugMarker'](UTF8ToString(markerLabel));
  },

  wgpu_encoder_write_timestamp: function(encoder, querySet, queryIndex) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_encoder_write_timestamp(commandEncoder=${commandEncoder}, querySet=${querySet}, queryIndex=${queryIndex})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[querySet]'); }}}
    wgpu[encoder]['writeTimestamp'](wgpu[querySet], queryIndex);
  },

  wgpu_command_encoder_resolve_query_set: function(commandEncoder, querySet, firstQuery, queryCount, destination, destinationOffset) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_resolve_query_set(commandEncoder=${commandEncoder}, querySet=${querySet}, firstQuery=${firstQuery}, queryCount=${queryCount}, destination=${destination}, destinationOffset=${destinationOffset})`'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[querySet]'); }}}
    {{{ wassert('wgpu[destination]'); }}}
    wgpu[commandEncoder]['resolveQuerySet'](wgpu[querySet], firstQuery, queryCount, wgpu[destination], destinationOffset);
  },

  wgpu_encoder_set_pipeline: function(encoder, pipeline) {
    {{{ wdebuglog('`wgpu_encoder_set_pipeline(encoder=${encoder}, pipeline=${pipeline})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[pipeline]'); }}}

    wgpu[encoder]['setPipeline'](wgpu[pipeline]);
  },

  wgpu_render_encoder_base_set_index_buffer: function(passEncoder, buffer, indexFormat, offset, size) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_encoder_base_set_index_buffer(passEncoder=${passEncoder}, buffer=${buffer}, indexFormat=${indexFormat}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[buffer]'); }}}

    wgpu[passEncoder]['setIndexBuffer'](wgpu[buffer], _wgpuStrings[indexFormat], offset, size);
  },

  wgpu_render_encoder_base_set_vertex_buffer: function(passEncoder, slot, buffer, offset, size) {
    {{{ wdebuglog('`wgpu_render_encoder_base_set_vertex_buffer(passEncoder=${passEncoder}, slot=${slot}, buffer=${buffer}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[buffer]'); }}}

    wgpu[passEncoder]['setVertexBuffer'](slot, wgpu[buffer], offset, size);
  },

  wgpu_render_encoder_base_draw: function(passEncoder, vertexCount, instanceCount, firstVertex, firstInstance) {
    {{{ wdebuglog('`wgpu_render_encoder_base_draw(passEncoder=${passEncoder}, vertexCount=${vertexCount}, instanceCount=${instanceCount}, firstVertex=${firstVertex}, firstInstance=${firstInstance})`'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}

    wgpu[passEncoder]['draw'](vertexCount, instanceCount, firstVertex, firstInstance);
  },

  wgpu_render_encoder_base_draw_indexed: function(passEncoder, indexCount, instanceCount, firstVertex, baseVertex, firstInstance) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_encoder_base_draw_indexed(passEncoder=${passEncoder}, indexCount=${indexCount}, instanceCount=${instanceCount}, firstVertex=${firstVertex}, baseVertex=${baseVertex}, firstInstance=${firstInstance})`'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}

    wgpu[passEncoder]['drawIndexed'](vertexCount, instanceCount, firstVertex, baseVertex, firstInstance);
  },

  wgpu_render_encoder_base_draw_indirect: function(passEncoder, indirectBuffer, indirectOffset) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_encoder_base_draw_indirect(passEncoder=${passEncoder}, indirectBuffer=${indirectBuffer}, indirectOffset=${indirectOffset})`'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[indirectBuffer]'); }}}

    wgpu[passEncoder]['drawIndirect'](wgpu[indirectBuffer], indirectOffset);
  },

  wgpu_render_encoder_base_draw_indexed_indirect: function(passEncoder, indirectBuffer, indirectOffset) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_encoder_base_draw_indexed_indirect(passEncoder=${passEncoder}, indirectBuffer=${indirectBuffer}, indirectOffset=${indirectOffset})`'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[indirectBuffer]'); }}}

    wgpu[passEncoder]['drawIndexedIndirect'](wgpu[indirectBuffer], indirectOffset);
  },

  wgpu_encoder_end_pass: function(encoder) {
    {{{ wdebuglog('`wgpu_encoder_end_pass(encoder=${encoder})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}

    wgpu[encoder]['endPass']();

    // https://gpuweb.github.io/gpuweb/#render-pass-encoder-finalization:
    // "The render pass encoder can be ended by calling endPass() once the user has finished recording commands for the pass. Once endPass() has been called the render pass encoder can no longer be used."
    // Hence to help make C/C++ side code read easier, immediately release all references to the pass encoder after endPass() occurs, so that C/C++ side code does not need to release references manually.
    _wgpu_object_destroy(encoder);
  },

  wgpu_command_encoder_finish: function(commandEncoder) {
    {{{ wdebuglog('`wgpu_command_encoder_finish(commandEncoder=${commandEncoder})`'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}

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

  wgpu_programmable_pass_encoder_set_bind_group: function(encoder, index, bindGroup, dynamicOffsets, numDynamicOffsets) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_programmable_pass_encoder_set_bind_group(encoder=${encoder}, index=${index}, bindGroup=${bindGroup}, dynamicOffsets=${dynamicOffsets}, numDynamicOffsets=${numDynamicOffsets})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[bindGroup]'); }}}
    {{{ wassert('dynamicOffsets != 0 || numDynamicOffsets == 0'); }}}
    {{{ wassert('dynamicOffsets % 4 == 0'); }}}
    wgpu[encoder]['setBindGroup'](index, wgpu[bindGroup], HEAPU32, dynamicOffsets >> 2, numDynamicOffsets);
  },

  wgpu_compute_pass_encoder_dispatch: function(encoder, x, y, z) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_compute_pass_encoder_dispatch(encoder=${encoder}, x=${x}, y=${y}, z=${z})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    wgpu[encoder]['dispatch'](x, y, z);
  },

  wgpu_compute_pass_encoder_dispatch_indirect: function(encoder, indirectBuffer, indirectOffset) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_compute_pass_encoder_dispatch_indirect(encoder=${encoder}, indirectBuffer=${indirectBuffer}, indirectOffset=${indirectOffset})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[indirectBuffer]'); }}}
    wgpu[encoder]['dispatchIndirect'](wgpu[indirectBuffer], indirectOffset);
  },

  wgpu_encoder_begin_pipeline_statistics_query: function(encoder, querySet, queryIndex) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_encoder_begin_pipeline_statistics_query(encoder=${encoder}, querySet=${querySet}, queryIndex=${queryIndex})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[querySet]'); }}}
    wgpu[encoder]['beginPipelineStatisticsQuery'](wgpu[querySet], queryIndex);
  },

  wgpu_encoder_end_pipeline_statistics_query: function(encoder) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_encoder_end_pipeline_statistics_query(encoder=${encoder})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    wgpu[encoder]['endPipelineStatisticsQuery']();
  },

  wgpu_render_pass_encoder_set_viewport: function(encoder, x, y, width, height, minDepth, maxDepth) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_viewport(encoder=${encoder}, x=${x}, y=${y}, width=${width}, height=${height}, minDepth=${minDepth}, maxDepth=${maxDepth})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    wgpu[encoder]['setViewport'](x, y, width, height, minDepth, maxDepth);
  },

  wgpu_render_pass_encoder_set_scissor_rect: function(encoder, x, y, width, height) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_scissor_rect(encoder=${encoder}, x=${x}, y=${y}, width=${width}, height=${height})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    wgpu[encoder]['setScissorRect'](x, y, width, height);
  },

  wgpu_render_pass_encoder_set_blend_constant: function(encoder, r, g, b, a) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_blend_constant(encoder=${encoder}, r=${r}, g=${g}, b=${b}, a=${a})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    wgpu[encoder]['setBlendConstant'](r, g, b, a);
  },

  wgpu_render_pass_encoder_set_stencil_reference: function(encoder, stencilValue) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_stencil_reference(stencilValue=${stencilValue})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    wgpu[encoder]['setStencilReference'](stencilValue);
  },

  wgpu_render_pass_encoder_begin_occlusion_query: function(encoder, queryIndex) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_begin_occlusion_query(queryIndex=${queryIndex})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    wgpu[encoder]['beginOcclusionQuery'](queryIndex);
  },

  wgpu_render_pass_encoder_end_occlusion_query: function(encoder) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_end_occlusion_query()`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    wgpu[encoder]['endOcclusionQuery']();
  },

  wgpu_render_pass_encoder_execute_bundles: function(encoder, bundles, numBundles) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_execute_bundles(encoder=${encoder}, bundles=${bundles}, numBundles=${numBundles})`'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('bundles != 0 || numBundles <= 0'); }}}
    {{{ wassert('bundles % 4 == 0'); }}}

    bundles >>= 2;
    var b = [];
    while(numBundles--) b.push(wgpu[bundles++]);
    wgpu[encoder]['executeBundles'](b);
  },

  wgpu_queue_submit_one: function(queue, commandBuffer) {
    {{{ wdebuglog('`wgpu_queue_submit(queue=${queue}, commandBuffer=${commandBuffer})`'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[commandBuffer]'); }}}

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
    {{{ wassert('commandBuffers'); }}}
    {{{ wassert('numCommandBuffers > 0'); }}}

    commandBuffers >>= 2;
    var buffers = [];
    while(numCommandBuffers--) {
      buffers.push(wgpu[HEAPU32[commandBuffers++]]);
      {{{ wassert('buffers[buffers.length-1]'); }}}
    }

    wgpu[queue]['submit'](buffers);
  },

  wgpu_queue_submit_multiple_and_destroy: function(queue, numCommandBuffers, commandBuffers) {
    {{{ wdebuglog('`wgpu_queue_submit_multiple_and_destroy(queue=${queue}, numCommandBuffers=${numCommandBuffers}, commandBuffers=${commandBuffers})`'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('commandBuffers'); }}}
    {{{ wassert('numCommandBuffers > 0'); }}}

    commandBuffers >>= 2;
    var buffers = [];
    while(numCommandBuffers--) {
      buffers.push(wgpu[HEAPU32[commandBuffers++]]);
      {{{ wassert('buffers[buffers.length-1]'); }}}
    }

    wgpu[queue]['submit'](buffers);

    for(var id of buffers) {
      _wgpu_object_destroy(id);
    }
  },

  wgpu_queue_write_buffer: function(queue, buffer, bufferOffset, data, size) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_queue_write_buffer(queue=${queue}, buffer=${buffer}, bufferOffset=${bufferOffset}, data=${data}, size=${size})`'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[buffer]'); }}}

    wgpu[queue]['writeBuffer'](wgpu[buffer], bufferOffset, HEAPU8, data, size);
  },

  wgpu_queue_write_texture__deps: ['$wgpuReadGpuImageCopyTexture'],
  wgpu_queue_write_texture: function(queue, destination, data, bytesPerBlockRow, blockRowsPerImage, writeWidth, writeHeight, writeDepthOrArrayLayers) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_queue_write_texture(queue=${queue}, destination=${destination}, data=${data}, bytesPerBlockRow=${bytesPerBlockRow}, blockRowsPerImage=${blockRowsPerImage}, writeWidth=${writeWidth}, writeHeight=${writeHeight}, writeDepthOrArrayLayers=${writeDepthOrArrayLayers})`'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('destination'); }}}

    wgpu[queue]['writeTexture'](wgpuReadGpuImageCopyTexture(destination), HEAPU8, { offset: data, bytesPerRow: bytesPerBlockRow, rowsPerImage: blockRowsPerImage }, data, [writeWidth, writeHeight, writeDepthOrArrayLayers]);
  },

  wgpu_queue_copy_external_image_to_texture__deps: ['$wgpuReadGpuImageCopyTexture'],
  wgpu_queue_copy_external_image_to_texture: function(queue, source, destination, copyWidth, copyHeight, copyDepthOrArrayLayers) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_queue_copy_external_image_to_texture(queue=${queue}, source=${source}, destination=${destination}, copyWidth=${copyWidth}, copyHeight=${copyHeight}, copyDepthOrArrayLayers=${copyDepthOrArrayLayers})`'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('source'); }}}
    {{{ wassert('source % 4 == 0'); }}}
    {{{ wassert('destination'); }}}
    {{{ wassert('destination % 4 == 0'); }}}

    source >>= 2;
    var dest = wgpuReadGpuImageCopyTexture(destination);
    destination >>= 2;
    dest['colorSpace'] = wgpuStrings[HEAP32[destination+6]];
    dest['premultipliedAlpha'] = !!HEAP32[destination+7];

    wgpu[queue]['copyExternalImageToTexture']({
      'source': wgpu[HEAPU32[source]],
      'origin': [HEAP32[source+1], HEAP32[source+2]]
      }, dest, [copyWidth, copyHeight, copyDepthOrArrayLayers]);
  },
});
