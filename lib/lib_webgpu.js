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

  // Global constant string table for all WebGPU strings. 2KB+ in size :(
  wgpuStrings: [void 0,"low-power","high-performance","1d","2d","3d","2d-array","cube","cube-array","all","stencil-only","depth-only","r8unorm","r8snorm","r8uint","r8sint","r16uint","r16sint","r16float","rg8unorm","rg8snorm","rg8uint","rg8sint","r32uint","r32sint","r32float","rg16uint","rg16sint","rg16float","rgba8unorm","rgba8unorm-srgb","rgba8snorm","rgba8uint","rgba8sint","bgra8unorm","bgra8unorm-srgb","rgb9e5ufloat","rgb10a2unorm","rg11b10ufloat","rg32uint","rg32sint","rg32float","rgba16uint","rgba16sint","rgba16float","rgba32uint","rgba32sint","rgba32float","stencil8","depth16unorm","depth24plus","depth24plus-stencil8","depth32float","bc1-rgba-unorm","bc1-rgba-unorm-srgb","bc2-rgba-unorm","bc2-rgba-unorm-srgb","bc3-rgba-unorm","bc3-rgba-unorm-srgb","bc4-r-unorm","bc4-r-snorm","bc5-rg-unorm","bc5-rg-snorm","bc6h-rgb-ufloat","bc6h-rgb-float","bc7-rgba-unorm","bc7-rgba-unorm-srgb","depth24unorm-stencil8","depth32float-stencil8","clamp-to-edge","repeat","mirror-repeat","nearest","linear","never","less","equal","less-equal","greater","not-equal","greater-equal","always","uniform","storage","read-only-storage","filtering","non-filtering","comparison","float","unfilterable-float","depth","sint","uint","read-only","write-only","error","warning","info","point-list","line-list","line-strip","triangle-list","triangle-strip","ccw","cw","none","front","back","zero","one","src-color","one-minus-src-color","src-alpha","one-minus-src-alpha","dst-color","one-minus-dst-color","dst-alpha","one-minus-dst-alpha","src-alpha-saturated","blend-color","one-minus-blend-color","add","subtract","reverse-subtract","min","max","keep","replace","invert","increment-clamp","decrement-clamp","increment-wrap","decrement-wrap","uint16","uint32","uchar2","uchar4","char2","char4","uchar2norm","uchar4norm","char2norm","char4norm","ushort2","ushort4","short2","short4","ushort2norm","ushort4norm","short2norm","short4norm","half2","half4","float2","float3","float4","uint2","uint3","uint4","int","int2","int3","int4","vertex","instance","load","store","clear","occlusion","pipeline-statistics","timestamp","vertex-shader-invocations","clipper-invocations","clipper-primitives-out","fragment-shader-invocations","compute-shader-invocations","destroyed","out-of-memory","validation"],

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

  wgpu_is_canvas_context: function(context) {
    return wgpu[context] instanceof GPUCanvasContext;
  },

  wgpu_is_swap_chain: function(swapChain) {
    return wgpu[swapChain] instanceof GPUSwapChain;
  },

  wgpu_is_adapter: function(adapter) {
    return wgpu[adapter] instanceof GPUAdapter;
  },

  wgpu_is_device: function(device) {
    return wgpu[device] instanceof GPUDevice;
  },

  wgpu_is_command_encoder: function(commandEncoder) {
    return wgpu[commandEncoder] instanceof GPUCommandEncoder;
  },

  wgpu_is_texture: function(texture) {
    return wgpu[texture] instanceof GPUTexture;
  },

  wgpu_is_texture_view: function(textureView) {
    return wgpu[textureView] instanceof GPUTextureView;
  },

  wgpu_is_render_pass_encoder: function(renderPassEncoder) {
    return wgpu[renderPassEncoder] instanceof GPURenderPassEncoder;
  },

  wgpu_is_command_buffer: function(commandBuffer) {
    return wgpu[commandBuffer] instanceof GPUCommandBuffer;
  },

  wgpu_is_queue: function(queue) {
    return wgpu[queue] instanceof GPUQueue;
  },

  wgpu_is_shader_module: function(shaderModule) {
    return wgpu[shaderModule] instanceof GPUShaderModule;
  },

  wgpu_is_render_pipeline: function(renderPipeline) {
    return wgpu[renderPipeline] instanceof GPURenderPipeline;
  },

  wgpu_canvas_context_get_swap_chain_preferred_format__deps: ['wgpuStrings'],
  wgpu_canvas_context_get_swap_chain_preferred_format: function(canvasContext, adapter) {
    {{{ wdebuglog('`wgpu_canvas_context_get_swap_chain_preferred_format: canvasContext: ${canvasContext}, adapter: ${adapter}`'); }}}
    {{{ wassert('wgpu[canvasContext]'); }}}
    {{{ wassert('wgpu[canvasContext].wid == canvasContext', "GPUCanvasContext has lost its wid member field!"); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter].wid == adapter', "GPUAdapter has lost its wid member field!"); }}}

    if (!wgpu[canvasContext].getSwapChainPreferredFormat) {
      console.error('GPUCanvasContext.getSwapChainPreferredFormat() is not implemented! Assuming bgra8unorm');
      return _wgpuStrings.indexOf('bgra8unorm');
    }

    {{{ wassert('_wgpuStrings.includes(wgpu[canvasContext].getSwapChainPreferredFormat(wgpu[adapter]))'); }}}
    return _wgpuStrings.indexOf(wgpu[canvasContext].getSwapChainPreferredFormat(wgpu[adapter]));
  },

  wgpu_canvascontext_configure_swap_chain: function(canvasContext, descriptor) {
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
    var t = swapChain['getCurrentTexture']();
    if (t != wgpu[1]) {
      // ... and destroy previous special Swap Chain Texture, if it was an old one.
      _wgpu_object_destroy(1);
      wgpu[1] = t;
      t.wid = 1;
      t.derivedObjects = []; // GPUTextureViews are derived off of GPUTextures
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
        dynCall('vii', adapterCallback, [id, userData]);
      });
      return id;
    }
    {{{ werror('`WebGPU is not supported by the current browser!`'); }}}
    // implicit return 0, WebGPU is not supported.
  },

  wgpuFeatures: ['depth-clamping', 'depth24unorm-stencil8', 'depth32float-stencil8', 'pipeline-statistics-query', 'texture-compression-bc', 'timestamp-query'],

  wgpu_adapter_get_properties__deps: ['wgpuFeatures'],
  wgpu_adapter_get_properties: function(adapter, properties) {
    {{{ wdebuglog('`wgpu_adapter_get_properties();`'); }}}
    {{{ wassert('properties % 4 == 0, "passed an invalid aligned properties struct pointer"'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter].wid == adapter', "GPUAdapter has lost its wid member field!"); }}}
    adapter = wgpu[adapter];
    stringToUTF8(adapter['name'], properties, 256);
    {{{ wassert('!adapter.features, "GPUAdapter.features has now been implemented! Update this code!"'); }}}
    var features = adapter['features'] || adapter['extensions'] || []; // change to adapter.features
    var id = 1;
    var featuresBitMask = 0;
    for(var feature of _wgpuFeatures) {
      if (features.includes(feature)) featuresBitMask |= id;
      id *= 2;
    }
    properties = (properties >> 2) + 64;
    HEAPU32[properties++] = featuresBitMask;

    {{{ wassert('!adapter.limits, "GPUAdapter.limits has now been implemented! Update this code!"'); }}}
    var l = adapter['limits'] || {}; // TODO Remove "|| {};" part.
    HEAPU32[properties++] = l['maxTextureDimension1D'];
    HEAPU32[properties++] = l['maxTextureDimension2D'];
    HEAPU32[properties++] = l['maxTextureDimension3D'];
    HEAPU32[properties++] = l['maxTextureArrayLayers'];
    HEAPU32[properties++] = l['maxBindGroups'];
    HEAPU32[properties++] = l['maxDynamicUniformBuffersPerPipelineLayout'];
    HEAPU32[properties++] = l['maxDynamicStorageBuffersPerPipelineLayout'];
    HEAPU32[properties++] = l['maxSampledTexturesPerShaderStage'];
    HEAPU32[properties++] = l['maxSamplersPerShaderStage'];
    HEAPU32[properties++] = l['maxStorageBuffersPerShaderStage'];
    HEAPU32[properties++] = l['maxStorageTexturesPerShaderStage'];
    HEAPU32[properties++] = l['maxUniformBuffersPerShaderStage'];
    HEAPU32[properties++] = l['maxUniformBufferBindingSize'];
    HEAPU32[properties++] = l['maxStorageBufferBindingSize'];
    HEAPU32[properties++] = l['maxVertexBuffers'];
    HEAPU32[properties++] = l['maxVertexAttributes'];
    HEAPU32[properties]   = l['maxVertexBufferArrayStride'];

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
    {{{ wdebuglog('`wgpu_adapter_request_device_async: adapter: ${adapter}, deviceCallback: ${deviceCallback}, userData: ${userData}`'); }}}
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

      // Register an ID for the defaultQueue of this newly created device
      wgpuStore(device['defaultQueue']);

      dynCall('vii', deviceCallback, [id, userData]);
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

  wgpu_device_get_default_queue: function(device) {
    {{{ wdebuglog('`wgpu_device_get_default_queue(device=${device})`'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device].wid == device', "GPUDevice has lost its wid member field!"); }}}
    {{{ wassert('wgpu[device]["defaultQueue"].wid', "GPUDevice.defaultQueue must have been assigned an ID in function wgpu_adapter_request_device!"); }}}
    return wgpu[device]['defaultQueue'].wid;
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
          'format': _wgpuStrings[HEAP32[attributesIdx++]],
          'offset': readI53FromU64HeapIdx(attributesIdx++),
          'shaderLocation': HEAP32[attributesIdx++]
        });
      }
      vertexBuffers.push({
        'arrayStride': readI53FromU64HeapIdx(vertexBuffersIdx++),
        'stepMode': _wgpuStrings[HEAP32[vertexBuffersIdx++]],
        'attributes': attributes
      });
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
      },
      'depthStencil': {
        'format': _wgpuStrings[HEAP32[depthStencilIdx++]],
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
      },
      'multisample': {
        'count': HEAPU32[multisampleIdx],
        'mask': HEAPU32[multisampleIdx+1],
        'alphaToCoverageEnabled': !!HEAPU32[multisampleIdx+2]
      },
      'fragment': {
        'module': wgpu[HEAP32[fragmentIdx]],
        'entryPoint': UTF8ToString(HEAPU32[fragmentIdx+1]),
        'targets': targets
      }
    };

    {{{ wdebugerror('`Adapting new spec to old Chrome impl`'); }}}
    desc['colorStates'] = desc['fragment']['targets'];
    desc['primitiveTopology'] = desc['primitive']['topology'];
    desc['vertexStage'] = desc['vertex'];
    desc['fragmentStage'] = desc['fragment'];

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

#if ASSERTIONS
      // TODO: remove this
      warnOnce('Adapting for old Chrome syntax (it uses GPURenderPassColorAttachment.attachment instead of GPURenderPassColorAttachment.view)');
#endif
      colorAttachments[colorAttachments.length-1]['attachment'] = colorAttachments[colorAttachments.length-1]['view'];

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

#if ASSERTIONS
    // TODO: remove this
    warnOnce('Adapting for old Chrome syntax (it uses GPURenderPassDepthStencilAttachment.attachment instead of GPURenderPassDepthStencilAttachment.view)');
#endif
    if (desc['depthStencilAttachment']) desc['depthStencilAttachment']['attachment'] = desc['depthStencilAttachment']['view'];

    {{{ wdebuglog('`GPUCommandEncoder.beginRenderPass() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    {{{ wdebuglog('JSON.stringify(desc)'); }}}
    var renderPassEncoder = commandEncoder['beginRenderPass'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('renderPassEncoder'); }}}

    return wgpuStore(renderPassEncoder);
  },

  wgpu_render_pass_encoder_set_pipeline: function(passEncoder, renderPipeline) {
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_pipeline(passEncoder=${passEncoder}, renderPipeline=${renderPipeline})`'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder].wid == passEncoder', "GPUPassEncoder has lost its wid member field!"); }}}
    {{{ wassert('wgpu[renderPipeline]'); }}}
    {{{ wassert('wgpu[renderPipeline].wid == renderPipeline', "GPURenderPipeline has lost its wid member field!"); }}}

    wgpu[passEncoder]['setPipeline'](wgpu[renderPipeline]);
  },

  wgpu_render_pass_encoder_draw: function(passEncoder, vertexCount, instanceCount, firstVertex, firstInstance) {
    {{{ wdebuglog('`wgpu_render_pass_encoder_draw(passEncoder=${passEncoder}, vertexCount=${vertexCount}, instanceCount=${instanceCount}, firstVertex=${firstVertex}, firstInstance=${firstInstance})`'); }}}
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
