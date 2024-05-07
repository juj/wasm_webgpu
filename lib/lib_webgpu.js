// N.b. Link this file with Emscripten linker flag -jsDWEBGPU_NO_BW_COMPAT=1 to remove all backwards compatibility polyfills
// for smaller code size.

#if EMSCRIPTEN_VERSION.split('.').map(parseInt) < [3,1,35]
// Emscripten 3.1.35 or newer is needed because of https://github.com/emscripten-core/emscripten/issues/19469
#error "wasm_webgpu requires building with Emscripten 3.1.35 or newer! Please update"
#endif

{{{
  global.wassert = function(condition) {
    if (ASSERTIONS || global.WEBGPU_DEBUG) return `assert(${condition}, "assert(${condition.replace(/"/g, "'")}) failed!");`;
    else return '';
  };
  global.wdebuglog = function(condition) {
    return global.WEBGPU_DEBUG ? `console.log(${condition});` : '';
  }
  global.wdebugwarn = function(condition) {
    return global.WEBGPU_DEBUG ? `console.warn(${condition});` : '';
  }
  global.wdebugerror = function(condition) {
    return global.WEBGPU_DEBUG ? `console.error(${condition});` : '';
  }
  global.wdebugdir = function(condition) {
    return global.WEBGPU_DEBUG ? `console.dir(${condition});` : '';
  }
  global.werror = function(condition) {
    return global.WEBGPU_DEBUG ? `console.error(${condition});` : '';
  }
  // Implement safe heap accesses for 2GB, 4GB and Wasm64 build modes.
  // (TODO: lib_webgpu is not yet really Wasm64-capable except for the two functions below)
  global.ptrToIdx = function(ptr, accessWidth) {
    if (MEMORY64) return `${ptr} >>= ${accessWidth}n`;
    if (MAXIMUM_MEMORY > 2*1024*1024*1024) return `${ptr} >>>= ${accessWidth}`;
    return `${ptr} >>= ${accessWidth}`;
  }
  global.shiftPtr = function(ptr, accessWidth) {
    if (MEMORY64) return `${ptr} >> ${accessWidth}n`;
    if (MAXIMUM_MEMORY > 2*1024*1024*1024) return `${ptr} >>> ${accessWidth}`;
    return `${ptr} >> ${accessWidth}`;
  }
  null;
}}}

let api = {
  $debugDir: function(x, desc) {
#if global.WEBGPU_DEBUG
    if (desc) console.log(`${desc}:`);
    console.dir(x);
#endif
    return x;
  },

  // Stores a ID->WebGPU object mapping registry of global top-level WebGPU objects.
  $wgpu: {},

  // Free ID counter generation number
  // 0: reserved for invalid object, 1: reserved for special GPUTexture that GPUCanvasContext.getCurrentTexture() returns.
  $wgpuIdCounter: 1,

  // Stores the given WebGPU object under a new free WebGPU object ID.
  // Returns the new ID. Can be called with a null/undefined, in which
  // case no object/ID is persisted.
  $wgpuStore__deps: ['$wgpu', '$wgpuIdCounter'],
  $wgpuStore: function(object) {
    if (object) {
      // WebGPU renderer usage can burn through a lot of object IDs each rendered frame
      // (a number of GPUCommandEncoder, GPUTexture, GPUTextureView, GPURenderPassEncoder,
      // GPUCommandBuffer objects are created each application frame)
      // If we assume an upper bound of 1000 object IDs created per rendered frame, and a
      // new mobile device with 120hz display, a signed int32 state space is exhausted in
      // 2147483646 / 1000 / 120 / 60 / 60 = 4.97 hours, which is realistic for a page to
      // stay open for that long. Therefore handle wraparound of the ID counter generation,
      // and find free gaps in the object IDs for new objects.
      while(wgpu[++wgpuIdCounter]) if (wgpuIdCounter > 2147483646) wgpuIdCounter = 1;

      wgpu[wgpuIdCounter] = object;

      // Each persisted objects gets a custom 'wid' field (wasm ID) which stores the ID that
      // this object is known by on Wasm side.
      object.wid = wgpuIdCounter;

#if global.WEBGPU_DEBUG
      debugDir(object, `Stored WebGPU object with ID ${wgpuIdCounter}`);
#endif

      return wgpuIdCounter;
    }
    // Implicit return undefined to marshal ID 0 over to Wasm.
  },

  // Marks the given 'object' to be a child/derived object of 'parent',
  // and stores a reference to the object in the WebGPU table,
  // returning the ID.
  $wgpuStoreAndSetParent__deps: ['$wgpuStore'],
  $wgpuStoreAndSetParent: function(object, parent) {
    object = wgpuStore(object);
    object && parent.derivedObjects.push(object);
    return object;
  },

  $wgpuReadArrayOfWgpuObjects: function(ptr, numObjects) {
    {{{ wassert('numObjects >= 0'); }}}
    {{{ wassert('ptr != 0 || numObjects == 0'); }}} // Must be non-null pointer
    {{{ wassert('ptr % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    {{{ ptrToIdx('ptr', 2); }}}

    let arrayOfObjects = [];
    while(numObjects--) {
      {{{ wassert('HEAPU32[ptr]'); }}} // Must reference a nonzero WebGPU object handle
      {{{ wassert('wgpu[HEAPU32[ptr]]'); }}} // Must reference a valid WebGPU object
      arrayOfObjects.push(wgpu[HEAPU32[ptr++]]);
    }
    return arrayOfObjects;
  },

  $wgpuReadI53FromU64HeapIdx: function(heap32Idx) {
    {{{ wassert('heap32Idx != 0'); }}}
#if WASM_BIGINT
    {{{ wassert('heap32Idx % 2 == 0'); }}}
    return Number(HEAPU64[heap32Idx>>1]);
#else
    return HEAPU32[heap32Idx] + HEAPU32[heap32Idx+1] * 4294967296;
#endif
  },

  $wgpuWriteI53ToU64HeapIdx: function(heap32Idx, number) {
    {{{ wassert('heap32Idx != 0'); }}}
#if WASM_BIGINT
    {{{ wassert('heap32Idx % 2 == 0'); }}}
    HEAPU64[heap32Idx>>1] = BigInt(number);
#else
    HEAPU32[heap32Idx] = number;
    HEAPU32[heap32Idx+1] = number / 4294967296;
#endif
  },

  wgpu_get_num_live_objects__deps: ['$wgpu'],
  wgpu_get_num_live_objects: function() {
    return Object.keys(wgpu).length;
  },

  // Calls .destroy() on the given WebGPU object, and releases the reference to it.
  wgpu_object_destroy__deps: ['$wgpu'],
  wgpu_object_destroy: function(object) {
    let o = wgpu[object];
    {{{ wassert(`o || !wgpu.hasOwnProperty(object), 'wgpu dictionary should never be storing key-values with null/undefined value in it'`); }}}
    if (o) {
      // WebGPU objects of type GPUDevice, GPUBuffer, GPUTexture and GPUQuerySet have an explicit .destroy() function. Call that if applicable.
      if (o['destroy']) o['destroy']();
      // If the given object has derived objects (GPUTexture -> GPUTextureViews), delete those in a hierarchy as well.
      if (o.derivedObjects) o.derivedObjects.forEach(_wgpu_object_destroy);
      // Finally erase reference to this object.
      delete wgpu[object];
    }
    {{{ wassert(`!wgpu.hasOwnProperty(object), 'object should have gotten deleted'`); }}}
  },

  wgpu_destroy_all_objects__deps: ['$wgpu'],
  wgpu_destroy_all_objects: function() {
    wgpu.forEach((o) => { if (o['destroy']) o['destroy'](); });
    wgpu = {};
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
  wgpu_is_pipeline_layout: function(o) { return wgpu[o] instanceof GPUPipelineLayout; },
  wgpu_is_shader_module: function(o) { return wgpu[o] instanceof GPUShaderModule; },
  wgpu_is_compute_pipeline: function(o) { return wgpu[o] instanceof GPUComputePipeline; },
  wgpu_is_render_pipeline: function(o) { return wgpu[o] instanceof GPURenderPipeline; },
  wgpu_is_command_buffer: function(o) { return wgpu[o] instanceof GPUCommandBuffer; },
  wgpu_is_command_encoder: function(o) { return wgpu[o] instanceof GPUCommandEncoder; },
  wgpu_is_binding_commands_mixin: function(o) { return wgpu[o] instanceof GPUComputePassEncoder || wgpu[o] instanceof GPURenderPassEncoder || wgpu[o] instanceof GPURenderBundleEncoder; },
  wgpu_is_render_commands_mixin: function(o) { return wgpu[o] instanceof GPURenderPassEncoder || wgpu[o] instanceof GPURenderBundleEncoder; },
  wgpu_is_render_pass_encoder: function(o) { return wgpu[o] instanceof GPURenderPassEncoder; },
  wgpu_is_render_bundle: function(o) { return wgpu[o] instanceof GPURenderBundle; },
  wgpu_is_render_bundle_encoder: function(o) { return wgpu[o] instanceof GPURenderBundleEncoder; },
  wgpu_is_queue: function(o) { return wgpu[o] instanceof GPUQueue; },
  wgpu_is_query_set: function(o) { return wgpu[o] instanceof GPUQuerySet; },
  wgpu_is_canvas_context: function(o) { return wgpu[o] instanceof GPUCanvasContext; },
  wgpu_is_device_lost_info: function(o) { return wgpu[o] instanceof GPUDeviceLostInfo; },
  wgpu_is_error: function(o) { return wgpu[o] instanceof GPUError; },

  wgpu_object_set_label: function(o, label) {
    {{{ wassert('wgpu[o]'); }}}
    wgpu[o]['label'] = UTF8ToString(label);
  },

  wgpu_object_get_label__deps: ['$stringToUTF8'],
  wgpu_object_get_label: function(o, dstLabel, dstLabelSize) {
    {{{ wassert('wgpu[o]'); }}}
    stringToUTF8(wgpu[o]['label'], dstLabel, dstLabelSize);
  },

  wgpu_canvas_get_webgpu_context__deps: ['$wgpuStore', '$debugDir'],
  wgpu_canvas_get_webgpu_context: function(canvasSelector) {
    {{{ wdebuglog('`wgpu_canvas_get_webgpu_context(canvasSelector=${UTF8ToString(canvasSelector)})`'); }}}
    return wgpuStore(
      debugDir(
        debugDir(
          document.querySelector(UTF8ToString(canvasSelector)),
          'canvas'
        )
        .getContext('webgpu'),
        'canvas.getContext("webgpu")'
      )
    );
  },

////////////////////////////////////////////////////////////
// Automatically generated with scripts/compress_strings.js:

#if !global.WEBGPU_NO_BW_COMPAT
  $replaceAll_polyfill: ";if (!String.prototype.replaceAll) String.prototype.replaceAll = function(str, newStr) { if (str === '?') str = '\\\\?'; return this.replace(str instanceof RegExp ? str : new RegExp(str, 'g'), newStr); }",
  $wgpuDecodeStrings__deps: ['$replaceAll_polyfill'],
#endif

  $wgpuDecodeStrings__docs: '/** @param {number=} ch */',
  $wgpuDecodeStrings: function(s, c, ch) {
    ch = ch || 65;
    for(c = c.split('|'); c[0];) s = s['replaceAll'](String.fromCharCode(ch++), c.pop());
    return [,].concat(s.split(' '));
  },

  $GPUTextureAndVertexFormats__deps: ['$wgpuDecodeStrings'],
//$GPUTextureAndVertexFormats: [undefined (0), 'r8unorm' (1), 'r8snorm' (2), 'r8uint' (3), 'r8sint' (4), 'r16uint' (5), 'r16sint' (6), 'r16float' (7), 'rg8unorm' (8), 'rg8snorm' (9), 'rg8uint' (10), 'rg8sint' (11), 'r32uint' (12), 'r32sint' (13), 'r32float' (14), 'rg16uint' (15), 'rg16sint' (16), 'rg16float' (17), 'rgba8unorm' (18), 'rgba8unorm-srgb' (19), 'rgba8snorm' (20), 'rgba8uint' (21), 'rgba8sint' (22), 'bgra8unorm' (23), 'bgra8unorm-srgb' (24), 'rgb9e5ufloat' (25), 'rgb10a2uint' (26), 'rgb10a2unorm' (27), 'rg11b10ufloat' (28), 'rg32uint' (29), 'rg32sint' (30), 'rg32float' (31), 'rgba16uint' (32), 'rgba16sint' (33), 'rgba16float' (34), 'rgba32uint' (35), 'rgba32sint' (36), 'rgba32float' (37), 'stencil8' (38), 'depth16unorm' (39), 'depth24plus' (40), 'depth24plus-stencil8' (41), 'depth32float' (42), 'depth32float-stencil8' (43), 'bc1-rgba-unorm' (44), 'bc1-rgba-unorm-srgb' (45), 'bc2-rgba-unorm' (46), 'bc2-rgba-unorm-srgb' (47), 'bc3-rgba-unorm' (48), 'bc3-rgba-unorm-srgb' (49), 'bc4-r-unorm' (50), 'bc4-r-snorm' (51), 'bc5-rg-unorm' (52), 'bc5-rg-snorm' (53), 'bc6h-rgb-ufloat' (54), 'bc6h-rgb-float' (55), 'bc7-rgba-unorm' (56), 'bc7-rgba-unorm-srgb' (57), 'etc2-rgb8unorm' (58), 'etc2-rgb8unorm-srgb' (59), 'etc2-rgb8a1unorm' (60), 'etc2-rgb8a1unorm-srgb' (61), 'etc2-rgba8unorm' (62), 'etc2-rgba8unorm-srgb' (63), 'eac-r11unorm' (64), 'eac-r11snorm' (65), 'eac-rg11unorm' (66), 'eac-rg11snorm' (67), 'astc-4x4-unorm' (68), 'astc-4x4-unorm-srgb' (69), 'astc-5x4-unorm' (70), 'astc-5x4-unorm-srgb' (71), 'astc-5x5-unorm' (72), 'astc-5x5-unorm-srgb' (73), 'astc-6x5-unorm' (74), 'astc-6x5-unorm-srgb' (75), 'astc-6x6-unorm' (76), 'astc-6x6-unorm-srgb' (77), 'astc-8x5-unorm' (78), 'astc-8x5-unorm-srgb' (79), 'astc-8x6-unorm' (80), 'astc-8x6-unorm-srgb' (81), 'astc-8x8-unorm' (82), 'astc-8x8-unorm-srgb' (83), 'astc-10x5-unorm' (84), 'astc-10x5-unorm-srgb' (85), 'astc-10x6-unorm' (86), 'astc-10x6-unorm-srgb' (87), 'astc-10x8-unorm' (88), 'astc-10x8-unorm-srgb' (89), 'astc-10x10-unorm' (90), 'astc-10x10-unorm-srgb' (91), 'astc-12x10-unorm' (92), 'astc-12x10-unorm-srgb' (93), 'astc-12x12-unorm' (94), 'astc-12x12-unorm-srgb' (95), 'uint8x2' (96), 'uint8x4' (97), 'sint8x2' (98), 'sint8x4' (99), 'unorm8x2' (100), 'unorm8x4' (101), 'snorm8x2' (102), 'snorm8x4' (103), 'uint16x2' (104), 'uint16x4' (105), 'sint16x2' (106), 'sint16x4' (107), 'unorm16x2' (108), 'unorm16x4' (109), 'snorm16x2' (110), 'snorm16x4' (111), 'float16x2' (112), 'float16x4' (113), 'float32' (114), 'float32x2' (115), 'float32x3' (116), 'float32x4' (117), 'uint32' (118), 'uint32x2' (119), 'uint32x3' (120), 'uint32x4' (121), 'sint32' (122), 'sint32x2' (123), 'sint32x3' (124), 'sint32x4' (125), 'unorm10-10-10-2' (126)],
  $GPUTextureAndVertexFormats: "wgpuDecodeStrings('r8YA8TA8SA8UALSALUALWR8YR8TR8SR8UANSANUANWRLSRLURLW V8Y V8Z V8T V8S V8U bgra8Y bgra8ZRb9e5uWRbJa2SRbJa2YR11bJuWRNSRNURNW VLS VLU VLW VNS VNU VNWB8ILYI24plusI24plus-E8INWINW-E8Q1-V-YQ1-V-ZQ2-V-YQ2-V-ZQ3-V-YQ3-V-ZQ4-r-YQ4-r-TQ5-rg-YQ5-rg-TQ6h-rgb-uWQ6h-rgb-WQ7-V-YQ7-V-ZPYPZPa1YPa1Z etc2-V8Y etc2-V8ZFr11YFr11TFrg11YFrg11TX4x4-YX4x4-ZX5x4-YX5x4-ZX5x5-YX5x5-ZX6x5-YX6x5-ZX6x6-YX6x6-ZX8x5-YX8x5-ZX8x6-YX8x6-ZX8x8-YX8x8-ZXJx5-YXJx5-ZXJx6-YXJx6-ZXJx8-YXJx8-ZXJxJ-YXJxJ-ZX12xJ-YX12xJ-ZX12x12-YX12x12-Z S8MS8KU8MU8KY8MY8KT8MT8KSLMSLKULMULKYLMYLKTLMTLKWLMWLKWN WNMWNx3 WNKSN SNMSNx3 SNKUN UNMUNx3 UNKYJ-J-J-2', 'unorm-srgb|unorm| astc-|float|rgba|sint|snorm|uint| rg| bc| etc2-rgb8|-AC|32|x2 |16|x4 |10| depth|-B|SC| eac-|stencil|-ESJ|-E-A| E| r')",

  wgpu32BitLimitNames__deps: ['$wgpuDecodeStrings'],
//wgpu32BitLimitNames: ['maxTextureDimension1D' (0), 'maxTextureDimension2D' (1), 'maxTextureDimension3D' (2), 'maxTextureArrayLayers' (3), 'maxBindGroups' (4), 'maxBindGroupsPlusVertexBuffers' (5), 'maxBindingsPerBindGroup' (6), 'maxDynamicUniformBuffersPerPipelineLayout' (7), 'maxDynamicStorageBuffersPerPipelineLayout' (8), 'maxSampledTexturesPerShaderStage' (9), 'maxSamplersPerShaderStage' (10), 'maxStorageBuffersPerShaderStage' (11), 'maxStorageTexturesPerShaderStage' (12), 'maxUniformBuffersPerShaderStage' (13), 'minUniformBufferOffsetAlignment' (14), 'minStorageBufferOffsetAlignment' (15), 'maxVertexBuffers' (16), 'maxVertexAttributes' (17), 'maxVertexBufferArrayStride' (18), 'maxInterStageShaderComponents' (19), 'maxInterStageShaderVariables' (20), 'maxColorAttachments' (21), 'maxColorAttachmentBytesPerSample' (22), 'maxComputeWorkgroupStorageSize' (23), 'maxComputeInvocationsPerWorkgroup' (24), 'maxComputeWorkgroupSizeX' (25), 'maxComputeWorkgroupSizeY' (26), 'maxComputeWorkgroupSizeZ' (27)],
  wgpu32BitLimitNames: "wgpuDecodeStrings('>1D >2D >3D maxTextureArrayLayer<6<6sPlus7=<BindingsPer6 maxDynamicUniform=;DynamicS:=;SampledTexture@maxSampler@maxS:=@maxS:Texture@maxUniform=@minUniform=9inS:=9ax7=<7Attribute<7=ArrayStride max8Component<8Variable<ColorAttachment<ColorAttachmentBytesPerSample?pS:Size maxComputeInvocationsPerWorkgroup?pSizeX?pSizeY?pSizeZ', 'sPerShaderStage | maxComputeWorkgrou|maxTextureDimension|Buffer|s max|sPerPipelineLayout max|torage|OffsetAlignment m|InterStageShader|Vertex|BindGroup|Uniform7|8ColorAttachmen', 52).slice(1)",

  wgpu64BitLimitNames__deps: ['$wgpuDecodeStrings'],
//wgpu64BitLimitNames: ['maxUniformBufferBindingSize' (0), 'maxStorageBufferBindingSize' (1), 'maxBufferSize' (2)],
  wgpu64BitLimitNames: "wgpuDecodeStrings('maxUniform4Storage4BufferSize', 'BufferBindingSize max', 52).slice(1)",

  wgpuFeatures__deps: ['$wgpuDecodeStrings'],
//wgpuFeatures: ['depth-clip-control' (0), 'depth32float-stencil8' (1), 'texture-compression-bc' (2), 'texture-compression-etc2' (3), 'texture-compression-astc' (4), 'timestamp-query' (5), 'indirect-first-instance' (6), 'shader-f16' (7), 'rg11b10ufloat-renderable' (8), 'bgra8unorm-storage' (9), 'float32-filterable' (10)],
  wgpuFeatures: "wgpuDecodeStrings('B-clip-control B32D-Aencil8EbcEetc2EaAc timeAamp-query indirect-firA-inAance shader-f16 rg11b10uD-rendC bgra8unorm-Aorage D32-filtC', ' texture-compression-|float|erable|depth|st').slice(1)",

  $GPUBlendFactors__deps: ['$wgpuDecodeStrings'],
//$GPUBlendFactors: [undefined (0), 'zero' (1), 'one' (2), 'src' (3), 'one-minus-src' (4), 'src-alpha' (5), 'one-minus-src-alpha' (6), 'dst' (7), 'one-minus-dst' (8), 'dst-alpha' (9), 'one-minus-dst-alpha' (10), 'src-alpha-saturated' (11), 'constant' (12), 'one-minus-constant' (13)],
  $GPUBlendFactors: "wgpuDecodeStrings('zero one BEB BDEBD AEA ADEAD BD-saturated CEC', ' one-minus-|-alpha|constant|src|dst')",

  $GPUStencilOperations__deps: ['$wgpuDecodeStrings'],
//$GPUStencilOperations: [undefined (0), 'keep' (1), 'zero' (2), 'replace' (3), 'invert' (4), 'increment-clamp' (5), 'decrement-clamp' (6), 'increment-wrap' (7), 'decrement-wrap' (8)],
  $GPUStencilOperations: "wgpuDecodeStrings('keep zero replace invert inCBdeCBinCA deCA', 'crement-|clamp |wrap')",

  $GPUCompareFunctions__deps: ['$wgpuDecodeStrings'],
//$GPUCompareFunctions: [undefined (0), 'never' (1), 'less' (2), 'equal' (3), 'less-equal' (4), 'greater' (5), 'not-equal' (6), 'greater-equal' (7), 'always' (8)],
  $GPUCompareFunctions: "wgpuDecodeStrings('neverA equalACB notCBCalways', '-equal |greater| less')",

  $GPUBlendOperations__deps: ['$wgpuDecodeStrings'],
//$GPUBlendOperations: [undefined (0), 'add' (1), 'subtract' (2), 'reverse-subtract' (3), 'min' (4), 'max' (5)],
  $GPUBlendOperations: "wgpuDecodeStrings('add Areverse-Amin max', 'subtract ')",

  $GPUIndexFormats__deps: ['$wgpuDecodeStrings'],
//$GPUIndexFormats: [undefined (0), 'uint16' (1), 'uint32' (2)],
  $GPUIndexFormats: "wgpuDecodeStrings('A16 A32', 'uint')",

  $GPUBufferMapStates__deps: ['$wgpuDecodeStrings'],
//$GPUBufferMapStates: [undefined (0), 'unmapped' (1), 'pending' (2), 'mapped' (3)],
  $GPUBufferMapStates: "wgpuDecodeStrings('unA pending A', 'mapped')",

  $GPUTextureDimensions: [, '1d', '2d', '3d'],

  $GPUTextureViewDimensions__deps: ['$wgpuDecodeStrings'],
//$GPUTextureViewDimensions: [undefined (0), '1d' (1), '2d' (2), '2d-array' (3), 'cube' (4), 'cube-array' (5), '3d' (6)],
  $GPUTextureViewDimensions: "wgpuDecodeStrings('1B 2dCA AC3d', '-array |d 2d|cube')",

  $GPUStorageTextureSampleTypes__deps: ['$wgpuDecodeStrings'],
//$GPUStorageTextureSampleTypes: [undefined (0), 'write-only' (1), 'read-only' (2), 'read-write' (3)],
  $GPUStorageTextureSampleTypes: "wgpuDecodeStrings('A-BBA', 'only read-|write')",

  $GPUAddressModes__deps: ['$wgpuDecodeStrings'],
//$GPUAddressModes: [undefined (0), 'clamp-to-edge' (1), 'repeat' (2), 'mirror-repeat' (3)],
  $GPUAddressModes: "wgpuDecodeStrings('clamp-to-edge A mirror-A', 'repeat')",

  $GPUTextureAspects__deps: ['$wgpuDecodeStrings'],
//$GPUTextureAspects: [undefined (0), 'all' (1), 'stencil-only' (2), 'depth-only' (3)],
  $GPUTextureAspects: "wgpuDecodeStrings('all stencilA depthA', '-only')",

  $GPUPipelineStatisticNames: [, 'timestamp'],

  $GPUPrimitiveTopologys__deps: ['$wgpuDecodeStrings'],
//$GPUPrimitiveTopologys: [undefined (0), 'point-list' (1), 'line-list' (2), 'line-strip' (3), 'triangle-list' (4), 'triangle-strip' (5)],
  $GPUPrimitiveTopologys: "wgpuDecodeStrings('pointDADAB CDCB', '-list |triangle|-strip|line')",

  $GPUBufferBindingTypes__deps: ['$wgpuDecodeStrings'],
//$GPUBufferBindingTypes: [undefined (0), 'uniform' (1), 'storage' (2), 'read-only-storage' (3)],
  $GPUBufferBindingTypes: "wgpuDecodeStrings('uniform A read-only-A', 'storage')",

  $GPUSamplerBindingTypes__deps: ['$wgpuDecodeStrings'],
//$GPUSamplerBindingTypes: [undefined (0), 'filtering' (1), 'non-filtering' (2), 'comparison' (3)],
  $GPUSamplerBindingTypes: "wgpuDecodeStrings('Anon-Acomparison', 'filtering ')",

  $GPUTextureSampleTypes__deps: ['$wgpuDecodeStrings'],
//$GPUTextureSampleTypes: [undefined (0), 'float' (1), 'unfilterable-float' (2), 'depth' (3), 'sint' (4), 'uint' (5)],
  $GPUTextureSampleTypes: "wgpuDecodeStrings('Aunfilterable-Adepth sint uint', 'float ')",

  $GPUQueryTypes: [, 'occlusion', 'timestamp'],

  $HTMLPredefinedColorSpaces: [, 'srgb', 'display-p3'],

  $GPUFilterModes__deps: ['$wgpuDecodeStrings'],
//$GPUFilterModes: [undefined (0), 'nearest' (1), 'linear' (2)],
  $GPUFilterModes: "wgpuDecodeStrings('Aest liA', 'near')",

  $GPUMipmapFilterModes__deps: ['$wgpuDecodeStrings'],
//$GPUMipmapFilterModes: [undefined (0), 'nearest' (1), 'linear' (2)],
  $GPUMipmapFilterModes: "wgpuDecodeStrings('Aest liA', 'near')",

  $GPULoadOps: [, 'load', 'clear'],

  $GPUStoreOps: [, 'store', 'discard'],

  $GPUAutoLayoutMode: '="auto"',

// End of automatically generated with scripts/compress_strings.js
//////////////////////////////////////////////////////////////////

  wgpu_canvas_context_configure__deps: ['$GPUTextureAndVertexFormats', '$HTMLPredefinedColorSpaces', '$wgpuReadArrayOfWgpuObjects'],
  wgpu_canvas_context_configure: function(canvasContext, config) {
    {{{ wdebuglog('`wgpu_canvas_context_configure(canvasContext=${canvasContext}, config=${config})`'); }}}
    {{{ wassert('canvasContext != 0'); }}}
    {{{ wassert('wgpu[canvasContext]'); }}}
    {{{ wassert('wgpu[canvasContext] instanceof GPUCanvasContext'); }}}
    {{{ wassert('config != 0'); }}} // Must be non-null
    {{{ wassert('config % 4 == 0'); }}} // Must be aligned at uint32_t boundary

    {{{ ptrToIdx('config', 2); }}}
    wgpu[canvasContext]['configure'](
      debugDir(
        {
          'device': wgpu[HEAPU32[config]],
          'format': GPUTextureAndVertexFormats[HEAPU32[config+1]],
          'usage': HEAPU32[config+2],
          'viewFormats': wgpuReadArrayOfWgpuObjects(HEAPU32[config+4], HEAPU32[config+3]),
          'colorSpace': HTMLPredefinedColorSpaces[HEAPU32[config+5]],
          'alphaMode': [, 'opaque', 'premultiplied'][HEAPU32[config+6]]
        },
        'canvasContext.configure() with config'
      )
    );
  },

  wgpu_canvas_context_get_current_texture__deps: ['wgpu_object_destroy'],
  wgpu_canvas_context_get_current_texture: function(canvasContext) {
    {{{ wdebuglog('`wgpu_canvas_context_get_current_texture(canvasContext=${canvasContext})`'); }}}
    {{{ wassert('canvasContext != 0'); }}}
    {{{ wassert('wgpu[canvasContext]'); }}}
    {{{ wassert('wgpu[canvasContext] instanceof GPUCanvasContext'); }}}

    // The canvas context texture is a special texture that automatically invalidates itself after the current rAF()
    // callback if over. Therefore when a new swap chain texture is produced, we need to delete the old one to avoid
    // accumulating references to stale textures from each frame.

    // Acquire the new canvas context texture..
    canvasContext = wgpu[canvasContext]['getCurrentTexture']();
    {{{ wassert('canvasContext'); }}}
    // The browser implementation for getCurrentTexture() should always return a new texture for each frame, so
    // derivedObjects array should not have a chance to pile up (a lot of) derived views. It is observed that
    // Chrome at least will temporarily return the same texture when it is not compositing, in which case
    // derivedObjects will have ~20-50(?) old derived views simultaneously. These will clear up when Chrome
    // actually starts compositing.
    {{{ wassert('!canvasContext.derivedObjects || canvasContext.derivedObjects.length < 1000'); }}}
    if (canvasContext != wgpu[1]) {
      // ... and destroy previous special canvas context texture, if it was an old one.
      _wgpu_object_destroy(1);
      wgpu[1] = canvasContext;
      canvasContext.wid = 1;
      canvasContext.derivedObjects = []; // GPUTextureViews are derived off of GPUTextures
    }
    // The canvas context texture is hardcoded the special ID 1. Return that to caller.
    return 1;
  },

  wgpuReportErrorCodeAndMessage__deps: ['$lengthBytesUTF8', '$stringToUTF8'],
  wgpuReportErrorCodeAndMessage: function(device, callback, errorCode, stringMessage, userData) {
    if (stringMessage) {
      // n.b. these variables deliberately rely on 'var' scope.
      var stackTop = stackSave(),
        len = lengthBytesUTF8(stringMessage)+1,
        errorMessage = stackAlloc(len);
      stringToUTF8(stringMessage, errorMessage, len);
    }
    {{{ makeDynCall('viiii', 'callback') }}}(device, errorCode, errorMessage, userData);
    if (stackTop) stackRestore(stackTop);
  },

  wgpu_device_set_lost_callback__deps: ['wgpuReportErrorCodeAndMessage'],
  wgpu_device_set_lost_callback: function(device, callback, userData) {
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    wgpu[device]['lost'].then((deviceLostInfo) => {
      _wgpuReportErrorCodeAndMessage(device, callback,
        deviceLostInfo['reason'] == 'destroyed' ? 1/*WGPU_DEVICE_LOST_REASON_DESTROYED*/ : 0/*WGPU_DEVICE_LOST_REASON_UNKNOWN*/,
        deviceLostInfo['message'], userData);
    });
  },

  wgpu_device_push_error_scope: function(device, filter) {
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    wgpu[device]['pushErrorScope']([, 'out-of-memory', 'validation', 'internal'][filter]);
  },

  wgpuDispatchWebGpuErrorEvent__deps: ['wgpuReportErrorCodeAndMessage'],
  wgpuDispatchWebGpuErrorEvent: function(device, callback, error, userData) {
    // Awkward WebGPU spec: errors do not contain a data-driven error code that
    // could be used to identify the error type in a general forward compatible
    // fashion, but must do an 'instanceof' check to look at the types of the
    // errors. If new error types are introduced in the future, their types won't
    // be recognized! (and code size creeps by having to do an 'instanceof' on every
    // error type)
    _wgpuReportErrorCodeAndMessage(device,
      callback,
      error
        ? (error instanceof GPUInternalError    ? 3/*WGPU_ERROR_TYPE_INTERNAL*/
        : (error instanceof GPUValidationError  ? 2/*WGPU_ERROR_TYPE_VALIDATION*/
        : (error instanceof GPUOutOfMemoryError ? 1/*WGPU_ERROR_TYPE_OUT_OF_MEMORY*/
        : 3/*WGPU_ERROR_TYPE_UNKNOWN_ERROR*/)))
        : 0/*WGPU_ERROR_TYPE_NO_ERROR*/,
      error && error['message'],
      userData);
  },

  // wgpuMuteJsExceptions(fn) returns a new function that can be called to invoke
  // the passed function in a way that all exceptions coming out from that function
  // are guarded inside a try-catch.
  wgpuMuteJsExceptions: function(fn) {
    return (p) => { // only support one argument to function fn (we could do ...params, but we only ever need one arg so that's fine)
      try {
        return fn(p);
      } catch(e) {
        {{{ werror('`Exception thrown when handling a WebGPU callback: ${e}`'); }}}
      }
    }
  },

  wgpu_device_pop_error_scope_async__deps: ['wgpuDispatchWebGpuErrorEvent', 'wgpuMuteJsExceptions'],
  wgpu_device_pop_error_scope_async: function(device, callback, userData) {
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('callback'); }}}

    function dispatchErrorCallback(error) {
      _wgpuDispatchWebGpuErrorEvent(device, callback, error, userData);
    }

    wgpu[device]['popErrorScope']().then(_wgpuMuteJsExceptions(dispatchErrorCallback)).catch(dispatchErrorCallback);
  },

  wgpu_device_set_uncapturederror_callback__deps: ['wgpuDispatchWebGpuErrorEvent'],
  wgpu_device_set_uncapturederror_callback: function(device, callback, userData) {
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    wgpu[device]['onuncapturederror'] = callback ? function(uncapturedError) {
      {{{ wdebugdir('uncapturedError'); }}}
      _wgpuDispatchWebGpuErrorEvent(device, callback, uncapturedError['error'], userData);
    } : null;
  },

  navigator_gpu_available: function() {
    return !!navigator['gpu'];
  },

  navigator_delete_webgpu_api_access: function() {
    // N.b. removing access to WebGPU is done via the prototype chain of Navigator,
    // and not the instance object navigator.
    delete Navigator.prototype.gpu;
  },

  navigator_gpu_request_adapter_async__deps: ['$wgpuStore', '$debugDir', 'wgpuMuteJsExceptions'],
  navigator_gpu_request_adapter_async__docs: '/** @suppress{checkTypes} */', // This function intentionally calls cb() without args.
  navigator_gpu_request_adapter_async: function(options, adapterCallback, userData) {
    {{{ wdebuglog('`navigator_gpu_request_adapter_async: options: ${options}, adapterCallback: ${adapterCallback}, userData: ${userData}`'); }}}
    {{{ wassert('adapterCallback, "must pass a callback function to navigator_gpu_request_adapter_async!"'); }}}
    {{{ wassert('navigator["gpu"], "Your browser does not support WebGPU!"'); }}}
    {{{ wassert('options != 0'); }}}
    {{{ wassert('options % 4 == 0'); }}} // Must be aligned at uint32_t boundary

    {{{ ptrToIdx('options', 2); }}}

    let gpu = navigator['gpu'],
      powerPreference = [, 'low-power', 'high-performance'][HEAPU32[options]],
      opts = {};

    if (gpu) {
      if (options) {
        opts['forceFallbackAdapter'] = !!HEAPU32[options+1];
        if (powerPreference) opts['powerPreference'] = powerPreference;
      }

      {{{ wdebuglog('`navigator.gpu.requestAdapter(options=${JSON.stringify(opts)})`'); }}}
      function cb(adapter) {
        {{{ wdebuglog('`navigator.gpu.requestAdapter resolved with following adapter:`'); }}}
        {{{ wdebugdir('adapter'); }}}
        {{{ makeDynCall('vii', 'adapterCallback') }}}(wgpuStore(adapter), userData);
      }
      gpu['requestAdapter'](opts).then(_wgpuMuteJsExceptions(cb)).catch(
#if ASSERTIONS || global.WEBGPU_DEBUG
        (e)=>{console.error(`navigator.gpu.requestAdapter() Promise failed: ${e}`); cb(/*intentionally omit arg to pass undefined*/)}
#else
        ()=>{cb(/*intentionally omit arg to pass undefined*/)}
#endif
      );
      return 1/*EM_TRUE*/;
    }
    {{{ werror('`WebGPU is not supported by the current browser!`'); }}}
    // Implicit return EM_FALSE, WebGPU is not supported.
  },

#if ASYNCIFY
  _wgpuNumAsyncifiedOperationsPending: 0,

  $wgpuStoreAsyncifiedOp__deps: ['_wgpuNumAsyncifiedOperationsPending', '$wgpuStore'],
  $wgpuStoreAsyncifiedOp: function(object) {
    {{{ wassert('__wgpuNumAsyncifiedOperationsPending > 0'); }}}
    --__wgpuNumAsyncifiedOperationsPending;
    return wgpuStore(object);
  },

  wgpu_sync_operations_pending: function() {
    return __wgpuNumAsyncifiedOperationsPending;
  },

  navigator_gpu_request_adapter_sync__deps: ['$wgpuStoreAsyncifiedOp', '$debugDir', '_wgpuNumAsyncifiedOperationsPending'],
  navigator_gpu_request_adapter_sync__sig: 'ip',
  navigator_gpu_request_adapter_sync__async: true,
  navigator_gpu_request_adapter_sync: function(options) {
    return Asyncify.handleAsync(() => {
      {{{ wdebuglog('`navigator_gpu_request_adapter_sync: options: ${options}`'); }}}
      {{{ wassert('navigator["gpu"], "Your browser does not support WebGPU!"'); }}}
      {{{ wassert('options != 0'); }}}
      {{{ wassert('options % 4 == 0'); }}} // Must be aligned at uint32_t boundary

      {{{ ptrToIdx('options', 2); }}}

      let gpu = navigator['gpu'],
        powerPreference = [, 'low-power', 'high-performance'][HEAPU32[options]],
        opts = {};

      if (gpu) {
        if (options) {
          opts['forceFallbackAdapter'] = !!HEAPU32[options+1];
          if (powerPreference) opts['powerPreference'] = powerPreference;
        }

        {{{ wdebuglog('`navigator.gpu.requestAdapter(options=${JSON.stringify(opts)})`'); }}}
        ++__wgpuNumAsyncifiedOperationsPending;
        return gpu['requestAdapter'](opts).then(wgpuStoreAsyncifiedOp);
      }
      {{{ werror('`WebGPU is not supported by the current browser!`'); }}}
      // Implicit return EM_FALSE, WebGPU is not supported.
    });
  },
#else
  wgpu_sync_operations_pending: function() {
    return 0;
  },
#endif

  // A "_simple" variant of navigator_gpu_request_adapter_async() that does
  // not take in any descriptor params, for building tiny code with default
  // args and creating readable test cases etc.
  navigator_gpu_request_adapter_async_simple__deps: ['$wgpuStore'],
  navigator_gpu_request_adapter_async_simple: function(adapterCallback) {
    {{{ wdebuglog('`navigator_gpu_request_adapter_async_simple(adapterCallback=${adapterCallback})`'); }}}
    {{{ wassert('navigator["gpu"], "Your browser does not support WebGPU!"'); }}}
    navigator['gpu']['requestAdapter']().then(adapter => {
      // N.b. this function deliberately invokes a callback with signature 'vii',
      // the second integer parameter is intentionally passed as undefined and coerced to zero to save code bytes.
      {{{ makeDynCall('vii', 'adapterCallback') }}}(wgpuStore(adapter));
    });
  },

#if ASYNCIFY
  navigator_gpu_request_adapter_sync_simple__deps: ['$wgpuStoreAsyncifiedOp', '_wgpuNumAsyncifiedOperationsPending'],
  navigator_gpu_request_adapter_sync_simple__sig: 'i',
  navigator_gpu_request_adapter_sync_simple__async: true,
  navigator_gpu_request_adapter_sync_simple: function() {
    return Asyncify.handleAsync(() => {
      {{{ wdebuglog('`navigator_gpu_request_adapter_sync_simple()`'); }}}
      {{{ wassert('navigator["gpu"], "Your browser does not support WebGPU!"'); }}}
      ++__wgpuNumAsyncifiedOperationsPending;
      return navigator['gpu']['requestAdapter']().then(wgpuStoreAsyncifiedOp);
    });
  },
#endif

  navigator_gpu_get_preferred_canvas_format__deps: ['$GPUTextureAndVertexFormats'],
  navigator_gpu_get_preferred_canvas_format: function() {
    {{{ wdebuglog('`navigator_gpu_get_preferred_canvas_format()`'); }}}
    {{{ wassert('navigator["gpu"], "Your browser does not support WebGPU!"'); }}}

    {{{ wassert('GPUTextureAndVertexFormats.includes(navigator["gpu"]["getPreferredCanvasFormat"]())'); }}}
    return GPUTextureAndVertexFormats.indexOf(navigator['gpu']['getPreferredCanvasFormat']());
  },

  $wgpuSupportedWgslLanguageFeatures: 0,
  navigator_gpu_get_wgsl_language_features__deps: ['$wgpuSupportedWgslLanguageFeatures', 'malloc', '$stringToNewUTF8'],
  navigator_gpu_get_wgsl_language_features: function() {
    if (!wgpuSupportedWgslLanguageFeatures) {
      // This function allocates an un-freeable constant memory block for an immutable array of strings representing the WGSL
      // language features. The intention is that this allocation is done only once, and behaves like global static data.
      let f = navigator['gpu']['wgslLanguageFeatures'];
      wgpuSupportedWgslLanguageFeatures = _malloc(f.size * 4);
      for(var i = 0; i < f.size; ++i) wgpuSupportedWgslLanguageFeatures[i] = stringToNewUTF8(f[i]);
    }
    return wgpuSupportedWgslLanguageFeatures;
  },

  navigator_gpu_is_wgsl_language_feature_supported: function(feature) {
    feature = UTF8ToString(feature);
    let f = navigator['gpu']['wgslLanguageFeatures'];
    for(var i = 0; i < f.size; ++i) if (f[i] == feature) return 1;
  },

  wgpu_adapter_or_device_get_features__deps: ['wgpuFeatures'],
  wgpu_adapter_or_device_get_features: function(adapterOrDevice) {
    {{{ wdebuglog('`wgpu_adapter_or_device_get_features(adapterOrDevice: ${adapterOrDevice})`'); }}}
    {{{ wassert('adapterOrDevice != 0'); }}}
    {{{ wassert('wgpu[adapterOrDevice]'); }}}
    {{{ wassert('wgpu[adapterOrDevice] instanceof GPUAdapter || wgpu[adapterOrDevice] instanceof GPUDevice'); }}}
    let id = 1,
      featuresBitMask = 0;
    for(let feature of _wgpuFeatures) {
      if (wgpu[adapterOrDevice]['features'].has(feature)) featuresBitMask |= id;
      id *= 2;
    }
    return featuresBitMask;
  },

  wgpu_adapter_or_device_supports_feature__deps: ['wgpuFeatures'],
  wgpu_adapter_or_device_supports_feature: function(adapterOrDevice, feature) {
    {{{ wdebuglog('`wgpu_adapter_or_device_supports_feature(adapterOrDevice: ${adapterOrDevice}, feature: ${feature})`'); }}}
    {{{ wassert('adapterOrDevice != 0'); }}}
    {{{ wassert('wgpu[adapterOrDevice]'); }}}
    {{{ wassert('wgpu[adapterOrDevice] instanceof GPUAdapter || wgpu[adapterOrDevice] instanceof GPUDevice'); }}}
    return wgpu[adapterOrDevice]['features'].has(_wgpuFeatures[32 - Match.clz32(feature)])
  },

  wgpu_adapter_or_device_get_limits__deps: ['wgpu32BitLimitNames', 'wgpu64BitLimitNames', '$wgpuWriteI53ToU64HeapIdx'],
  wgpu_adapter_or_device_get_limits: function(adapterOrDevice, limits) {
    {{{ wdebuglog('`wgpu_adapter_or_device_get_limits(adapterOrDevice: ${adapterOrDevice}, limits: ${limits})`'); }}}
    {{{ wassert('limits != 0, "passed a null limits struct pointer"'); }}}
    {{{ wassert('limits % 4 == 0, "passed an unaligned limits struct pointer"'); }}}
    {{{ wassert('adapterOrDevice != 0'); }}}
    {{{ wassert('wgpu[adapterOrDevice]'); }}}
    {{{ wassert('wgpu[adapterOrDevice] instanceof GPUAdapter || wgpu[adapterOrDevice] instanceof GPUDevice'); }}}

    let l = wgpu[adapterOrDevice]['limits'];

    {{{ ptrToIdx('limits', 2); }}}
    for(let limitName of _wgpu64BitLimitNames) {
      {{{ wassert('l[limitName] !== undefined, `Browser WebGPU implementation incorrect: it should advertise limit ${limitName}`'); }}}
      wgpuWriteI53ToU64HeapIdx(limits, l[limitName]);
      limits += 2;
    }

    for(let limitName of _wgpu32BitLimitNames) {
      HEAPU32[limits++] = l[limitName];
    }
  },

  wgpu_adapter_is_fallback_adapter: function(adapter) {
    {{{ wdebuglog('`wgpu_adapter_is_fallback_adapter(adapter: ${adapter}`'); }}}
    {{{ wassert('adapter != 0'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter] instanceof GPUAdapter'); }}}
    return wgpu[adapter]['isFallbackAdapter'];
  },

  wgpu_adapter_request_device_async__deps: ['$wgpuStore', 'wgpuFeatures', 'wgpu32BitLimitNames', 'wgpu64BitLimitNames', '$wgpuReadI53FromU64HeapIdx', 'wgpuMuteJsExceptions'],
  wgpu_adapter_request_device_async__docs: '/** @suppress{checkTypes} */', // This function intentionally calls cb() without args.
  wgpu_adapter_request_device_async: function(adapter, descriptor, deviceCallback, userData) {
    {{{ wdebuglog('`wgpu_adapter_request_device_async(adapter: ${adapter}, deviceCallback: ${deviceCallback}, userData: ${userData})`'); }}}
    {{{ wassert('adapter != 0'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter] instanceof GPUAdapter'); }}}
    {{{ wassert('descriptor != 0'); }}}
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary

    {{{ ptrToIdx('descriptor', 2); }}}
    let requiredFeatures = [], requiredLimits = {}, v = HEAPU32[descriptor], defaultQueueLabel;
    descriptor += 2;

    {{{ wassert('_wgpuFeatures.length == 11'); }}}
    {{{ wassert('_wgpuFeatures.length <= 30'); }}} // We can only do up to 30 distinct feature bits here with the current code.

    for(let i = 0; i < 11/*_wgpuFeatures.length*/; ++i) {
      if (v & (1 << i)) requiredFeatures.push(_wgpuFeatures[i]);
    }

    // Marshal all the complex 64-bit quantities first ..
    for(let limitName of _wgpu64BitLimitNames) {
      if ((v = wgpuReadI53FromU64HeapIdx(descriptor))) requiredLimits[limitName] = v;
      descriptor += 2;
    }

    // .. followed by the 32-bit quantities.
    for(let limitName of _wgpu32BitLimitNames) {
      if ((v = HEAPU32[descriptor++])) requiredLimits[limitName] = v;
    }

    function cb(device) {
      // If device is non-null, initialization succeeded.
      {{{ wdebuglog('`wgpu[adapter].requestDevice resolved with following device:`'); }}}
      {{{ wdebugdir('device'); }}}
      if (device) {
        device.derivedObjects = []; // A large number of objects are derived from GPUDevice (GPUBuffers, GPUTextures, GPUSamplers, ....)

        // Register an ID for the queue of this newly created device
        wgpuStore(device['queue']);
      }

      {{{ makeDynCall('vii', 'deviceCallback') }}}(wgpuStore(device), userData);
    }

    defaultQueueLabel = HEAPU32[descriptor];
    wgpu[adapter]['requestDevice'](
      debugDir(
        {
          'requiredFeatures': requiredFeatures,
          'requiredLimits': requiredLimits,
          'defaultQueue': defaultQueueLabel ? { 'label': UTF8ToString(defaultQueueLabel) } : void 0
        },
        'GPUAdapter.requestDevice() with desc'
      )
    ).then(_wgpuMuteJsExceptions(cb)).catch(
#if ASSERTIONS || global.WEBGPU_DEBUG
      (e)=>{console.error(`GPUAdapter.requestDevice() Promise failed: ${e}`); cb(/*intentionally omit arg to pass undefined*/)}
#else
      ()=>{cb(/*intentionally omit arg to pass undefined*/)}
#endif
    );
  },

#if ASYNCIFY
  wgpu_adapter_request_device_sync__deps: ['$wgpuStore', '$wgpuStoreAsyncifiedOp', 'wgpuFeatures', 'wgpu32BitLimitNames', 'wgpu64BitLimitNames', '$wgpuReadI53FromU64HeapIdx', '_wgpuNumAsyncifiedOperationsPending'],
  wgpu_adapter_request_device_sync__sig: 'iip',
  wgpu_adapter_request_device_sync__async: true,
  wgpu_adapter_request_device_sync: function(adapter, descriptor) {
    return Asyncify.handleAsync(() => {
      {{{ wdebuglog('`wgpu_adapter_request_device_sync(adapter: ${adapter})`'); }}}
      {{{ wassert('adapter != 0'); }}}
      {{{ wassert('wgpu[adapter]'); }}}
      {{{ wassert('wgpu[adapter] instanceof GPUAdapter'); }}}
      {{{ wassert('descriptor != 0'); }}}
      {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary

      {{{ ptrToIdx('descriptor', 2); }}}
      let requiredFeatures = [], requiredLimits = {}, v = HEAPU32[descriptor], defaultQueueLabel;
      descriptor += 2;

      {{{ wassert('_wgpuFeatures.length == 11'); }}}
      {{{ wassert('_wgpuFeatures.length <= 30'); }}} // We can only do up to 30 distinct feature bits here with the current code.

      for(let i = 0; i < 11/*_wgpuFeatures.length*/; ++i) {
        if (v & (1 << i)) requiredFeatures.push(_wgpuFeatures[i]);
      }

      // Marshal all the complex 64-bit quantities first ..
      for(let limitName of _wgpu64BitLimitNames) {
        if ((v = wgpuReadI53FromU64HeapIdx(descriptor))) requiredLimits[limitName] = v;
        descriptor += 2;
      }

      // .. followed by the 32-bit quantities.
      for(let limitName of _wgpu32BitLimitNames) {
        if ((v = HEAPU32[descriptor++])) requiredLimits[limitName] = v;
      }

      function cb(device) {
        // If device is non-null, initialization succeeded.
        {{{ wdebuglog('`wgpu[adapter].requestDevice resolved with following device:`'); }}}
        {{{ wdebugdir('device'); }}}
        if (device) {
          device.derivedObjects = []; // A large number of objects are derived from GPUDevice (GPUBuffers, GPUTextures, GPUSamplers, ....)
          // Register an ID for the queue of this newly created device
          wgpuStore(device['queue']);
        }
        return wgpuStoreAsyncifiedOp(device);
      }

      defaultQueueLabel = HEAPU32[descriptor];
      ++__wgpuNumAsyncifiedOperationsPending;
      return wgpu[adapter]['requestDevice'](
          debugDir(
            {
              'requiredFeatures': requiredFeatures,
              'requiredLimits': requiredLimits,
              'defaultQueue': defaultQueueLabel ? { 'label': UTF8ToString(defaultQueueLabel) } : void 0
            },
            'GPUAdapter.requestDevice() with desc'
          )
        ).then(cb);
    });
  },
#endif

  // A "_simple" variant of wgpu_adapter_request_device_async() that does
  // not take in any descriptor params, for building tiny code with default
  // args and creating readable test cases etc.
  wgpu_adapter_request_device_async_simple__deps: ['$wgpuStore'],
  wgpu_adapter_request_device_async_simple: function(adapter, deviceCallback) {
    {{{ wdebuglog('`wgpu_adapter_request_device_async_simple(adapter: ${adapter}, deviceCallback=${deviceCallback})`'); }}}
    {{{ wassert('adapter != 0'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter] instanceof GPUAdapter'); }}}
    wgpu[adapter]['requestDevice']().then(device => {
      device.derivedObjects = [];
      wgpuStore(device['queue']);
      {{{ makeDynCall('vii', 'deviceCallback') }}}(wgpuStore(device));
    });
  },

#if ASYNCIFY
  wgpu_adapter_request_device_sync_simple__deps: ['$wgpuStore', '$wgpuStoreAsyncifiedOp', '_wgpuNumAsyncifiedOperationsPending'],
  wgpu_adapter_request_device_sync_simple__sig: 'ii',
  wgpu_adapter_request_device_sync_simple__sync: true,
  wgpu_adapter_request_device_sync_simple: function(adapter) {
    return Asyncify.handleAsync(() => {
      {{{ wdebuglog('`wgpu_adapter_request_device_sync_simple(adapter: ${adapter})`'); }}}
      {{{ wassert('adapter != 0'); }}}
      {{{ wassert('wgpu[adapter]'); }}}
      {{{ wassert('wgpu[adapter] instanceof GPUAdapter'); }}}
      ++__wgpuNumAsyncifiedOperationsPending;
      return wgpu[adapter]['requestDevice']().then(device => {
        device.derivedObjects = [];
        wgpuStore(device['queue']);
        return wgpuStoreAsyncifiedOp(device);
      });
    });
  },
#endif

  wgpu_adapter_request_adapter_info_async__docs: '/** @suppress{checkTypes} */', // This function intentionally calls cb() without args.
  wgpu_adapter_request_adapter_info_async__deps: ['$stringToUTF8', 'wgpuMuteJsExceptions'],
  wgpu_adapter_request_adapter_info_async: function(adapter, callback, userData) {
    {{{ wdebuglog('`wgpu_adapter_request_adapter_info_async(adapter: ${adapter}, callback: ${callback}, userData: ${userData})`'); }}}
    {{{ wassert('adapter != 0'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter] instanceof GPUAdapter'); }}}
    {{{ wassert('callback != 0'); }}}

    function cb(adapterInfo) {
      {{{ wdebuglog('`GPUAdapter.requestAdapterInfo() resolved with following adapterInfo:`'); }}}
      {{{ wdebugdir('adapterInfo'); }}}

      let stackTop = stackSave(),
        info = stackAlloc(2048);
      stringToUTF8(adapterInfo['vendor'],       info, 512);
      stringToUTF8(adapterInfo['architecture'], info + 512,  512);
      stringToUTF8(adapterInfo['device'],       info + 1024, 512);
      stringToUTF8(adapterInfo['description'],  info + 1536, 512);
      {{{ makeDynCall('viii', 'callback') }}}(adapter, info, userData);
      stackRestore(stackTop);
    }

    {{{ wdebuglog('`wgpu_adapter_request_adapter_info_async() requesting adapter info`'); }}}
    return wgpu[adapter]['requestAdapterInfo']().then(_wgpuMuteJsExceptions(cb)).catch(
#if ASSERTIONS || global.WEBGPU_DEBUG
      (e)=>{console.error(`GPUAdapter.requestAdapterInfo() Promise failed: ${e}`); cb(/*intentionally omit arg to pass undefined*/)}
#else
      ()=>{cb(/*intentionally omit arg to pass undefined*/)}
#endif
    );
  },

  // TODO: Create asyncified wgpu_adapter_request_adapter_info_sync() function.

  wgpu_device_get_queue: function(device) {
    {{{ wdebuglog('`wgpu_device_get_queue(device=${device})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('wgpu[device].wid == device', "GPUDevice has lost its wid member field!"); }}}
    {{{ wassert('wgpu[device]["queue"].wid', "GPUDevice.queue must have been assigned an ID in function wgpu_adapter_request_device!"); }}}
    return wgpu[device]['queue'].wid;
  },

  $wgpuReadShaderModuleCompilationHints__deps: ['$GPUAutoLayoutMode'],
  $wgpuReadShaderModuleCompilationHints: function(index) {
    let numHints = HEAP32[index],
      hints = [],
      hintsIndex = {{{ shiftPtr('HEAPU32[index+1]', 2) }}},
      hint;
    {{{ wassert('numHints >= 0'); }}}
    while(numHints--) {
      hint = HEAPU32[hintsIndex+1];
      // hint == 0 (WGPU_AUTO_LAYOUT_MODE_NO_HINT) means no compilation hints are passed,
      // hint == 1 (WGPU_AUTO_LAYOUT_MODE_AUTO) means { layout: 'auto' } hint will be passed.
      // hint > 1: A handle to a given GPUPipelineLayout object is specified as a hint for creating the shader.
      // See https://github.com/gpuweb/gpuweb/pull/2876#issuecomment-1218341636
      {{{ wassert('hint <= 1 || wgpu[hint]'); }}}
      {{{ wassert('hint <= 1 || wgpu[hint] instanceof GPUPipelineLayout'); }}}
      hints.push({
        'entryPoint': UTF8ToString(HEAPU32[hintsIndex]),
        'layout': hint > 1 ? wgpu[hint] : (hint ? GPUAutoLayoutMode : null)
      });
      hintsIndex += 2;
    }
    return hints;
  },

  $wgpuReadShaderModuleDescriptor__deps: ['$wgpuReadShaderModuleCompilationHints'],
  $wgpuReadShaderModuleDescriptor: function(descriptor) {
    {{{ wassert('descriptor != 0'); }}}
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    descriptor = {{{ shiftPtr('descriptor', 2) }}};
    return {
      'code': UTF8ToString(HEAPU32[descriptor]),
      // TODO: add support for 'sourceMap' field
      'compilationHints': wgpuReadShaderModuleCompilationHints(descriptor+1)
    }
  },

  wgpu_device_create_shader_module__deps: ['$wgpuStoreAndSetParent', '$wgpuReadShaderModuleDescriptor'],
  wgpu_device_create_shader_module: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_shader_module(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    return wgpuStoreAndSetParent(
      wgpu[device]['createShaderModule'](
        debugDir(wgpuReadShaderModuleDescriptor(descriptor), 'device.createShaderModule() with desc')
      ),
      wgpu[device]
    );
  },

  wgpu_shader_module_get_compilation_info_async__deps: ['$lengthBytesUTF8', '$stringToUTF8', 'malloc'],
  wgpu_shader_module_get_compilation_info_async: function(shaderModule, callback, userData) {
    {{{ wdebuglog('`wgpu_shader_module_get_compilation_info_async(shaderModule=${shaderModule}, callback=${callback}, userData=${userData})`'); }}}
    {{{ wassert('shaderModule != 0'); }}}
    {{{ wassert('wgpu[shaderModule]'); }}}
    {{{ wassert('wgpu[shaderModule] instanceof GPUShaderModule'); }}}
    {{{ wassert('callback != 0'); }}}
    wgpu[shaderModule]['getCompilationInfo']().then(info => {
      {{{ wdebuglog('`shaderModule.getCompilationInfo() completed with info:`'); }}}
      {{{ wdebugdir('info'); }}}
      // To optimize marshalling, call into malloc() just once, and marshal the compilationInfo
      // object into one memory block, with the following layout:

      // A) 1 * struct WGpuCompilationInfo, followed by
      // B) info.messages.length * struct WGpuCompilationMessage, followed by
      // C) info.messages.length * null-terminated C strings for the messages.

      let msgs = info['messages'],
        len = msgs['length'],
        structLen = len * 24/*sizeof(WGpuCompilationMessage) */
                    + 4/*sizeof(WGpuCompilationInfo)*/,
        totalLen = structLen, msg, infoPtr, msgPtr, i;

      for(msg of msgs) totalLen += lengthBytesUTF8(msg['message']) + 1;

      infoPtr = _malloc(totalLen);
      msgPtr = infoPtr + structLen;
      i = {{{ shiftPtr('infoPtr', 2) }}};

      // Write A) struct WGpuCompilationInfo.
      HEAPU32[i++] = len;

      for(msg of msgs) {
        // Write B) struct WGpuCompilationMessage.
        HEAPU32[i++] = msgPtr;
        HEAPU32[i++] = ['error', 'warning', 'info'].indexOf(msg['type']);
        HEAPU32[i++] = msg['lineNum'];
        HEAPU32[i++] = msg['linePos'];
        HEAPU32[i++] = msg['offset'];
        HEAPU32[i++] = msg['length'];

        // Write C) null-terminated C string for the message.
        msgPtr += stringToUTF8(msg['message'], msgPtr, 2**32) + 1;
      }
      {{{ makeDynCall('viii', 'callback') }}}(shaderModule, infoPtr, userData);
    });
  },

  wgpu_device_create_buffer__deps: ['$wgpuReadI53FromU64HeapIdx', '$wgpuStoreAndSetParent'],
  wgpu_device_create_buffer: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_buffer(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('descriptor != 0'); }}}
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    device = wgpu[device];
    {{{ ptrToIdx('descriptor', 2); }}}

    let buffer = device['createBuffer'](
      debugDir(
        {
          'size': wgpuReadI53FromU64HeapIdx(descriptor),
          'usage': HEAPU32[descriptor+2],
          'mappedAtCreation': !!HEAPU32[descriptor+3]
        },
        'GPUDevice.createBuffer() with desc'
      )
    );
    // Add tracking space for mapped ranges
    buffer.mappedRanges = {};
    // Mark this object to be of type GPUBuffer for wgpu_device_create_bind_group().
    buffer.isBuffer = 1;
    return wgpuStoreAndSetParent(buffer, device);
  },

  wgpu_buffer_get_mapped_range: function(gpuBuffer, offset, size) {
    {{{ wdebuglog('`wgpu_buffer_get_mapped_range(gpuBuffer=${gpuBuffer}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('gpuBuffer != 0'); }}}
    {{{ wassert('wgpu[gpuBuffer]'); }}}
    {{{ wassert('wgpu[gpuBuffer] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(offset)'); }}}
    {{{ wassert('offset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(size)'); }}}
    {{{ wassert('size >= -1'); }}}

    {{{ wdebuglog("`gpuBuffer.getMappedRange(offset=${offset}, size=${size}):`"); }}}
    gpuBuffer = wgpu[gpuBuffer];
    try {
      // Awkward polymorphism: cannot pass undefined as size to map whole buffer, but must call the shorter function.
      gpuBuffer.mappedRanges[offset] = size < 0 ? gpuBuffer['getMappedRange'](offset) : gpuBuffer['getMappedRange'](offset, size);
    } catch(e) {
      // E.g. if the GPU ran out of memory when creating a new buffer, this can fail. 
      {{{ wdebuglog('`gpuBuffer.getMappedRange() failed!`'); }}}
      {{{ wdebugdir('e'); }}}
      return -1;
    }
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('gpuBuffer.mappedRanges[offset]'); }}}
    return offset;
  },

  wgpu_buffer_read_mapped_range: function(gpuBuffer, startOffset, subOffset, dst, size) {
    {{{ wdebuglog('`wgpu_buffer_read_mapped_range(gpuBuffer=${gpuBuffer}, startOffset=${startOffset}, subOffset=${subOffset}, dst=${dst}, size=${size})`'); }}}
    {{{ wassert('gpuBuffer != 0'); }}}
    {{{ wassert('wgpu[gpuBuffer]'); }}}
    {{{ wassert('wgpu[gpuBuffer] instanceof GPUBuffer'); }}}
    {{{ wassert('wgpu[gpuBuffer].mappedRanges[startOffset]', "wgpu_buffer_read_mapped_range: No such mapped range with specified startOffset!"); }}}
    {{{ wassert('Number.isSafeInteger(startOffset)'); }}}
    {{{ wassert('startOffset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(subOffset)'); }}}
    {{{ wassert('subOffset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(size)'); }}}
    {{{ wassert('size >= 0'); }}}
    {{{ wassert('dst || size == 0'); }}}

    // N.b. this generates garbage because JavaScript does not allow ArrayBufferView.set(ArrayBuffer, offset, size, dst)
    // but must create a dummy view.
    HEAPU8.set(new Uint8Array(wgpu[gpuBuffer].mappedRanges[startOffset], subOffset, size), dst);
  },

  wgpu_buffer_write_mapped_range: function(gpuBuffer, startOffset, subOffset, src, size) {
    {{{ wdebuglog('`wgpu_buffer_write_mapped_range(gpuBuffer=${gpuBuffer}, startOffset=${startOffset}, subOffset=${subOffset}, src=${src}, size=${size})`'); }}}
    {{{ wassert('gpuBuffer != 0'); }}}
    {{{ wassert('wgpu[gpuBuffer]'); }}}
    {{{ wassert('wgpu[gpuBuffer] instanceof GPUBuffer'); }}}
    {{{ wassert('wgpu[gpuBuffer].mappedRanges[startOffset]', "wgpu_buffer_write_mapped_range: No such mapped range with specified startOffset!"); }}}
    {{{ wassert('Number.isSafeInteger(startOffset)'); }}}
    {{{ wassert('startOffset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(subOffset)'); }}}
    {{{ wassert('subOffset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(size)'); }}}
    {{{ wassert('size >= 0'); }}}
    {{{ wassert('src || size == 0'); }}}

    // Here 'buffer' refers to the global Wasm memory buffer.
    // N.b. generates garbage.
    new Uint8Array(wgpu[gpuBuffer].mappedRanges[startOffset]).set(new Uint8Array(HEAPU8.buffer, src, size), subOffset);
  },

  wgpu_buffer_unmap: function(gpuBuffer) {
    {{{ wdebuglog('`wgpu_buffer_unmap(gpuBuffer=${gpuBuffer})`'); }}}
    {{{ wassert('gpuBuffer != 0'); }}}
    {{{ wassert('wgpu[gpuBuffer]'); }}}
    {{{ wassert('wgpu[gpuBuffer] instanceof GPUBuffer'); }}}
    gpuBuffer = wgpu[gpuBuffer];
    gpuBuffer['unmap']();

    // Let GC reclaim all previous getMappedRange()s for this buffer.
    gpuBuffer.mappedRanges = {};
  },

  wgpu_buffer_size: function(gpuBuffer) {
    {{{ wdebuglog('`wgpu_buffer_size(gpuBuffer=${gpuBuffer})`'); }}}
    {{{ wassert('gpuBuffer != 0'); }}}
    {{{ wassert('wgpu[gpuBuffer]'); }}}
    {{{ wassert('wgpu[gpuBuffer] instanceof GPUBuffer'); }}}
    return wgpu[gpuBuffer]['size'];
  },

  wgpu_buffer_usage: function(gpuBuffer) {
    {{{ wdebuglog('`wgpu_buffer_usage(gpuBuffer=${gpuBuffer})`'); }}}
    {{{ wassert('gpuBuffer != 0'); }}}
    {{{ wassert('wgpu[gpuBuffer]'); }}}
    {{{ wassert('wgpu[gpuBuffer] instanceof GPUBuffer'); }}}
    return wgpu[gpuBuffer]['usage'];
  },

  wgpu_buffer_map_state__deps: ['$GPUBufferMapStates'],
  wgpu_buffer_map_state: function(gpuBuffer) {
    {{{ wdebuglog('`wgpu_buffer_map_state(gpuBuffer=${gpuBuffer})`'); }}}
    {{{ wassert('gpuBuffer != 0'); }}}
    {{{ wassert('wgpu[gpuBuffer]'); }}}
    {{{ wassert('wgpu[gpuBuffer] instanceof GPUBuffer'); }}}
    {{{ wassert('GPUBufferMapStates.indexOf(wgpu[gpuBuffer]["mapState"]) != -1'); }}}
    return GPUBufferMapStates.indexOf(wgpu[gpuBuffer]['mapState']);
  },

  $wgpuReadGpuStencilFaceState__deps: ['$GPUCompareFunctions', '$GPUStencilOperations'],
  $wgpuReadGpuStencilFaceState: function(idx) {
    {{{ wassert('idx != 0'); }}}
    return {
      'compare': GPUCompareFunctions[HEAPU32[idx]],
      'failOp': GPUStencilOperations[HEAPU32[idx+1]],
      'depthFailOp': GPUStencilOperations[HEAPU32[idx+2]],
      'passOp': GPUStencilOperations[HEAPU32[idx+3]]
    };
  },

  $wgpuReadGpuBlendComponent__deps: ['$GPUBlendOperations', '$GPUBlendFactors'],
  $wgpuReadGpuBlendComponent: function(idx) {
    {{{ wassert('idx != 0'); }}}
    {{{ wassert('GPUBlendOperations[HEAPU32[idx]]'); }}}
    {{{ wassert('GPUBlendFactors[HEAPU32[idx+1]]'); }}}
    {{{ wassert('GPUBlendFactors[HEAPU32[idx+2]]'); }}}
    return {
      'operation': GPUBlendOperations[HEAPU32[idx]],
      'srcFactor': GPUBlendFactors[HEAPU32[idx+1]],
      'dstFactor': GPUBlendFactors[HEAPU32[idx+2]]
    };
  },

  $wgpuReadRenderPipelineDescriptor__deps: ['$wgpuReadGpuStencilFaceState', '$wgpuReadGpuBlendComponent', '$wgpuReadI53FromU64HeapIdx', '$wgpuReadConstants', '$GPUIndexFormats', '$GPUTextureAndVertexFormats', '$GPUCompareFunctions', '$GPUPrimitiveTopologys', '$GPUAutoLayoutMode'],
  $wgpuReadRenderPipelineDescriptor: function(descriptor) {
    {{{ wassert('descriptor != 0'); }}}
    {{{ wassert('descriptor % 4 == 0'); }}}

    let vertexBuffers = [],
        targets = [],
        vertexIdx = {{{ shiftPtr('descriptor', 2) }}},
        numVertexBuffers = HEAP32[vertexIdx+2],
        vertexBuffersIdx = {{{ shiftPtr('HEAPU32[vertexIdx+3]', 2) }}},
        primitiveIdx = vertexIdx + 6,
        depthStencilIdx = primitiveIdx + 5,
        multisampleIdx = depthStencilIdx + 17,
        fragmentIdx = multisampleIdx + 3,
        numTargets = HEAP32[fragmentIdx+2],
        targetsIdx = {{{ shiftPtr('HEAPU32[fragmentIdx+3]', 2) }}},
        depthStencilFormat = HEAPU32[depthStencilIdx++],
        multisampleCount = HEAPU32[multisampleIdx],
        fragmentModule = HEAPU32[fragmentIdx],
        pipelineLayoutId = HEAPU32[fragmentIdx+6],
        desc;

    {{{ wassert('pipelineLayoutId <= 1/*"auto"*/ || wgpu[pipelineLayoutId]'); }}}
    {{{ wassert('pipelineLayoutId <= 1/*"auto"*/ || wgpu[pipelineLayoutId] instanceof GPUPipelineLayout'); }}}

    // Read GPUVertexState
    {{{ wassert('numVertexBuffers >= 0'); }}}
    while(numVertexBuffers--) {
      let attributes = [],
          numAttributes = HEAP32[vertexBuffersIdx],
          attributesIdx = {{{ shiftPtr('HEAPU32[vertexBuffersIdx+1]', 2) }}};
      {{{ wassert('numAttributes >= 0'); }}}
      while(numAttributes--) {
        attributes.push({
          'offset': wgpuReadI53FromU64HeapIdx(attributesIdx),
          'shaderLocation': HEAPU32[attributesIdx+2],
          'format': GPUTextureAndVertexFormats[HEAPU32[attributesIdx+3]]
        });
        attributesIdx += 4;
      }
      vertexBuffers.push({
        'arrayStride': wgpuReadI53FromU64HeapIdx(vertexBuffersIdx+2),
        'stepMode': [, 'vertex', 'instance'][HEAPU32[vertexBuffersIdx+4]],
        'attributes': attributes
      });
      vertexBuffersIdx += 6;
    }

    {{{ wassert('numTargets >= 0'); }}}
    while(numTargets--) {
      // If target format is 0 (WGPU_TEXTURE_FORMAT_INVALID), then this target
      // is sparse and specified as 'null'. 
      targets.push(HEAPU32[targetsIdx] ? {
        'format': GPUTextureAndVertexFormats[HEAPU32[targetsIdx]],
        'blend': HEAPU32[targetsIdx+1] ? {
          'color': wgpuReadGpuBlendComponent(targetsIdx+1),
          'alpha': wgpuReadGpuBlendComponent(targetsIdx+4)
        } : void 0,
        'writeMask': HEAPU32[targetsIdx+7]
      } : null);
      targetsIdx += 8;
    }

    desc = {
      'vertex': {
        'module': wgpu[HEAPU32[vertexIdx]],
        'entryPoint': UTF8ToString(HEAPU32[vertexIdx+1]) || void 0, // If null pointer was passed to use the default entry point name, then UTF8ToString() would return '', but spec requires undefined.
        'buffers': vertexBuffers,
        'constants': wgpuReadConstants(HEAPU32[vertexIdx+5], HEAP32[vertexIdx+4])
      },
      'primitive': {
        'topology': GPUPrimitiveTopologys[HEAPU32[primitiveIdx]],
        'stripIndexFormat': GPUIndexFormats[HEAPU32[primitiveIdx+1]],
        'frontFace': [, 'ccw', 'cw'][HEAPU32[primitiveIdx+2]],
        'cullMode': [, 'none', 'front', 'back'][HEAPU32[primitiveIdx+3]],
        'unclippedDepth': !!HEAPU32[primitiveIdx+4]
      },
      'layout': pipelineLayoutId > 1 ? wgpu[pipelineLayoutId] : GPUAutoLayoutMode
    };

    // Awkward polymorphism: we cannot statically write desc['depthStencil'] = null above.
    // but Chrome will complain that null.format member does not exist.
    if (depthStencilFormat) desc['depthStencil'] = {
        'format': GPUTextureAndVertexFormats[depthStencilFormat],
        'depthWriteEnabled': !!HEAPU32[depthStencilIdx++],
        'depthCompare': GPUCompareFunctions[HEAPU32[depthStencilIdx++]],
        'stencilReadMask': HEAPU32[depthStencilIdx++],
        'stencilWriteMask': HEAPU32[depthStencilIdx++],
        'depthBias': HEAP32[depthStencilIdx++],
        'depthBiasSlopeScale': HEAPF32[depthStencilIdx++],
        'depthBiasClamp': HEAPF32[depthStencilIdx++],
        'stencilFront': wgpuReadGpuStencilFaceState(depthStencilIdx),
        'stencilBack': wgpuReadGpuStencilFaceState(depthStencilIdx+4),
        'clampDepth': !!HEAPU32[depthStencilIdx+8],
      };

    // Awkward polymorphism: we cannot statically write desc['multisample'] with count=0,
    // but must dynamically populate the multisample object only if doing MSAA.
    if (multisampleCount) desc['multisample'] = {
        'count': multisampleCount,
        'mask': HEAPU32[multisampleIdx+1],
        'alphaToCoverageEnabled': !!HEAPU32[multisampleIdx+2]
      };

    // Awkward polymorphism: we cannot statically write desc['fragment']['module'] = null,
    // but must omit 'fragment' object altogether if no fragment module is to be used.
    if (fragmentModule) desc['fragment'] = {
        'module': wgpu[fragmentModule],
        'entryPoint': UTF8ToString(HEAPU32[fragmentIdx+1]) || void 0, // If null pointer was passed to use the default entry point name, then UTF8ToString() would return '', but spec requires undefined.
        'targets': targets,
        'constants': wgpuReadConstants(HEAPU32[fragmentIdx+5], HEAP32[fragmentIdx+4])
      };

    return desc;
  },

  wgpu_device_create_render_pipeline__deps: ['$wgpuReadRenderPipelineDescriptor', '$wgpuStoreAndSetParent'],
  wgpu_device_create_render_pipeline: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_render_pipeline(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('descriptor'); }}}

    return wgpuStoreAndSetParent(
      wgpu[device]['createRenderPipeline'](
        debugDir(wgpuReadRenderPipelineDescriptor(descriptor), 'GPUDevice.createRenderPipeline() with desc')
      ),
      wgpu[device]
    );
  },

  $wgpuPipelineCreationFailed: function(device, error, userData) {
    {{{ wdebuglog('`createComputePipelineAsync failed with error:`'); }}}
    {{{ wdebugdir('error'); }}}
    let e = _malloc(12);
    HEAPU32[e>>2] = stringToNewUTF8(error['name']);
    HEAPU32[e+4>>2] = stringToNewUTF8(error['message']);
    HEAPU32[e+8>>2] = stringToNewUTF8(error['message']);
    {{{ makeDynCall('viiii', 'callback') }}}(device, e, 0, userData);
    _free(HEAPU32[e+8>>2]);
    _free(HEAPU32[e+4>>2]);
    _free(HEAPU32[e>>2]);
    _free(e);
  },

  wgpu_device_create_render_pipeline_async__deps: ['$wgpuReadRenderPipelineDescriptor', '$wgpuStoreAndSetParent', '$wgpuPipelineCreationFailed', 'wgpuMuteJsExceptions'],
  wgpu_device_create_render_pipeline_async__docs: '/** @suppress{checkTypes} */', // This function intentionally calls cb() without args.
  wgpu_device_create_render_pipeline_async: function(device, descriptor, callback, userData) {
    {{{ wdebuglog('`wgpu_device_create_render_pipeline_async(device=${device}, descriptor=${descriptor}, callback=${callback}, userData=${userData})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('descriptor'); }}}
    {{{ wassert('callback'); }}}
    let deviceObject = wgpu[device];

    let cb = (pipeline) => {
      {{{ wdebuglog('`createRenderPipelineAsync completed with pipeline:`'); }}}
      {{{ wdebugdir('pipeline'); }}}
      {{{ makeDynCall('viii', 'callback') }}}(device, wgpuStoreAndSetParent(pipeline, deviceObject), userData);
    };

    deviceObject['createRenderPipelineAsync'](
      debugDir(wgpuReadRenderPipelineDescriptor(descriptor), 'GPUDevice.createRenderPipelineAsync() with desc')
    ).then(_wgpuMuteJsExceptions(cb)).catch(
#if ASSERTIONS || global.WEBGPU_DEBUG
      (e)=>{console.error(`GPUDevice.createRenderPipelineAsync() Promise failed: ${e}`); wgpuPipelineCreationFailed(device, e, userData); }
#else
      ()=>{cb(/*intentionally omit arg to pass undefined*/)}
#endif
    );
  },

  wgpu_device_create_command_encoder__deps: ['$wgpuStoreAndSetParent'],
  wgpu_device_create_command_encoder: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_command_encoder(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('descriptor == 0 && "TODO: passing non-zero desciptor to wgpu_device_create_command_encoder() not yet implemented!"'); }}}

    return wgpuStoreAndSetParent(
      wgpu[device]['createCommandEncoder'](
        debugDir(
          void 0,
          'GPUDevice.createCommandEncoder() with desc'
        )
      ),
      wgpu[device]
    );
  },

  // A "_simple" variant of wgpu_device_create_command_encoder() that does
  // not take in any descriptor params, for building tiny code with default
  // args and creating readable test cases etc.
  wgpu_device_create_command_encoder_simple__deps: ['$wgpuStoreAndSetParent'],
  wgpu_device_create_command_encoder_simple: function(device) {
    return wgpuStoreAndSetParent(wgpu[device]['createCommandEncoder'](), wgpu[device]);
  },

  wgpu_device_create_render_bundle_encoder__deps: ['$GPUTextureAndVertexFormats', '$wgpuStoreAndSetParent'],
  wgpu_device_create_render_bundle_encoder: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_render_bundle_encoder(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('descriptor != 0'); }}} // Must be non-null
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    device = wgpu[device];
    {{{ ptrToIdx('descriptor', 2); }}}

    let colorFormats = [],
      numColorFormats = HEAP32[descriptor],
      colorFormatsIdx = {{{ shiftPtr('HEAPU32[descriptor+1]', 2) }}};

    {{{ wassert('numColorFormats >= 0'); }}}
    while(numColorFormats--) {
      // Color formats are allowed to be zero here, in which case a null/sparse target will be used in that slot.
      colorFormats.push(GPUTextureAndVertexFormats[HEAPU32[colorFormatsIdx++]]);
    }

    return wgpuStoreAndSetParent(
      device['createRenderBundleEncoder'](
        debugDir(
          { 'colorFormats': colorFormats,
            'depthStencilFormat': GPUTextureAndVertexFormats[HEAPU32[descriptor+2]],
            'sampleCount': HEAPU32[descriptor+3]
          },
          'GPUDevice.createRenderBundleEncoder() with desc'
        )
      ),
      device
    );
  },

  wgpu_device_create_query_set__deps: ['$GPUPipelineStatisticNames', '$GPUQueryTypes', '$wgpuStoreAndSetParent'],
  wgpu_device_create_query_set: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_query_set(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('descriptor != 0'); }}} // Must be non-null
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    device = wgpu[device];
    {{{ ptrToIdx('descriptor', 2); }}}

    let pipelineStatistics = [],
      numPipelineStatistics = HEAP32[descriptor+2],
      pipelineStatisticsIdx = {{{ shiftPtr('HEAPU32[descriptor+3]', 2) }}};

    {{{ wassert('numPipelineStatistics >= 0'); }}}
    while(numPipelineStatistics--) {
      pipelineStatistics.push(GPUPipelineStatisticNames[HEAPU32[pipelineStatisticsIdx++]]);
    }

    return wgpuStoreAndSetParent(
      device['createQuerySet'](
        debugDir(
          {
            'type': GPUQueryTypes[HEAPU32[descriptor]],
            'count': HEAPU32[descriptor+1],
            'pipelineStatistics': pipelineStatistics
          },
          'GPUDevice.createQuerySet() with desc'
        )
      ),
      device
    );
  },

  wgpu_buffer_map_async: function(buffer, callback, userData, mode, offset, size) {
    {{{ wdebuglog('`wgpu_buffer_map_async(buffer=${buffer}, callback=${callback}, userData=${userData}, mode=${mode}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('buffer != 0'); }}}
    {{{ wassert('wgpu[buffer]'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(offset)'); }}}
    {{{ wassert('offset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(size)'); }}}
    {{{ wassert('size >= -1'); }}}

    let bufferObject = wgpu[buffer];
    // Awkward polymorphism: must call different version of mapAsync to use default 'size' value.
    (size < 0 ? bufferObject['mapAsync'](mode, offset) : bufferObject['mapAsync'](mode, offset, size)).then(() => {
      {{{ makeDynCall('viiidd', 'callback') }}}(buffer, userData, mode, offset, size);
    });
  },

#if ASYNCIFY
  wgpu_buffer_map_sync__deps: ['_wgpuNumAsyncifiedOperationsPending', '$wgpuStoreAsyncifiedOp'],
  wgpu_buffer_map_sync__sig: 'viidd',
  wgpu_buffer_map_sync__async: true,
  wgpu_buffer_map_sync: function(buffer, mode, offset, size) {
    return Asyncify.handleAsync(() => {
      {{{ wdebuglog('`wgpu_buffer_map_sync(buffer=${buffer}, mode=${mode}, offset=${offset}, size=${size})`'); }}}
      {{{ wassert('buffer != 0'); }}}
      {{{ wassert('wgpu[buffer]'); }}}
      {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}
      {{{ wassert('Number.isSafeInteger(offset)'); }}}
      {{{ wassert('offset >= 0'); }}}
      {{{ wassert('Number.isSafeInteger(size)'); }}}
      {{{ wassert('size >= -1'); }}}

      buffer = wgpu[buffer];
      ++__wgpuNumAsyncifiedOperationsPending;
      // Awkward polymorphism: must call different version of mapAsync to use default 'size' value.
      return (size < 0 ? buffer['mapAsync'](mode, offset) : buffer['mapAsync'](mode, offset, size))
        .then(wgpuStoreAsyncifiedOp);
    });
  },
#endif

  wgpu_device_create_texture__deps: ['$wgpuStoreAndSetParent', '$GPUTextureAndVertexFormats', '$wgpuReadArrayOfWgpuObjects'],
  wgpu_device_create_texture: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_texture(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('descriptor != 0'); }}} // Must be non-null
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    device = wgpu[device];

    {{{ ptrToIdx('descriptor', 2); }}}
    let desc = {
      'size': [HEAP32[descriptor], HEAP32[descriptor+1], HEAP32[descriptor+2]],
      'mipLevelCount': HEAP32[descriptor+3],
      'sampleCount': HEAP32[descriptor+4],
      'dimension': HEAPU32[descriptor+5] + 'd',
      'format': GPUTextureAndVertexFormats[HEAPU32[descriptor+6]],
      'usage': HEAPU32[descriptor+7],
      'viewFormats': wgpuReadArrayOfWgpuObjects(HEAPU32[descriptor+9], HEAPU32[descriptor+8])
    };
    {{{ wdebuglog('`GPUDevice.createTexture() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    let texture = device['createTexture'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('texture'); }}}
    // Textures get TextureViews derived from them.
    texture.derivedObjects = [];

    return wgpuStoreAndSetParent(texture, device);
  },

  wgpu_device_create_sampler__deps: ['$wgpuStoreAndSetParent', '$GPUAddressModes', '$GPUFilterModes', '$GPUMipmapFilterModes', '$GPUCompareFunctions'],
  wgpu_device_create_sampler: function(device, descriptor) {
    {{{ wdebuglog('`wgpu_device_create_sampler(device=${device}, descriptor=${descriptor})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    device = wgpu[device];

    {{{ ptrToIdx('descriptor', 2); }}}
    let desc = descriptor ? {
      'addressModeU': GPUAddressModes[HEAPU32[descriptor]],
      'addressModeV': GPUAddressModes[HEAPU32[descriptor+1]],
      'addressModeW': GPUAddressModes[HEAPU32[descriptor+2]],
      'magFilter': GPUFilterModes[HEAPU32[descriptor+3]],
      'minFilter': GPUFilterModes[HEAPU32[descriptor+4]],
      'mipmapFilter': GPUMipmapFilterModes[HEAPU32[descriptor+5]],
      'lodMinClamp': HEAPF32[descriptor+6],
      'lodMaxClamp': HEAPF32[descriptor+7],
      'compare': GPUCompareFunctions[HEAPU32[descriptor+8]],
      'maxAnisotropy': HEAPU32[descriptor+9]
    } : void 0;
    {{{ wdebuglog('`GPUDevice.createSampler() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    let sampler = device['createSampler'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('sampler'); }}}

    return wgpuStoreAndSetParent(sampler, device);
  },

  // This is a JavaScript facing API function for calling GPUDevice.importExternalTexture().
  // Returns a integer handle to the imported texture that can be passed to Wasm code.
  // device: a integer handle to the WebGPU device, obtained from Wasm code.
  // descriptor: a JSON object of type GPUExternalTextureDescriptor
  wgpuDeviceImportExternalTexture__deps: ['$wgpuStoreAndSetParent'],
  wgpuDeviceImportExternalTexture: function(device, descriptor) {
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('descriptor'); }}}
    {{{ wassert('descriptor["source"]'); }}}
    device = wgpu[device];

    return wgpuStoreAndSetParent(device['importExternalTexture'](descriptor), device);
  },

  // This is a Wasm facing API function for calling GPUDevice.importExternalTexture().
  // Returns a integer handle to the imported texture that can be used in Wasm code.
  // device: a integer handle to the WebGPU device.
  // descriptor: a pointer to a GPUExternalTextureDescriptor struct in Wasm heap
  wgpu_device_import_external_texture: function(device, descriptor) {
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('descriptor'); }}}
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    {{{ wassert('wgpu[HEAPU32[descriptor>>2]]'); }}}
    {{{ wassert('wgpu[HEAPU32[descriptor>>2]] instanceof HTMLVideoElement'); }}}

    device = wgpu[device];

    return wgpuStoreAndSetParent(device['importExternalTexture']({
      'source': wgpu[HEAPU32[descriptor>>2]]
      // TODO: If/when GPUExternalTextureDescriptor.colorSpace field gains other values than 'srgb', add reading those fields in here.
    }), device);
  },

  $wgpuReadBindGroupLayoutDescriptor__deps: ['$GPUBufferBindingTypes', '$wgpuReadI53FromU64HeapIdx', '$GPUSamplerBindingTypes', '$GPUTextureSampleTypes', '$GPUTextureViewDimensions', '$GPUTextureAndVertexFormats', '$GPUStorageTextureSampleTypes'],
  $wgpuReadBindGroupLayoutDescriptor: function(entries, numEntries) {
    {{{ wassert('numEntries >= 0'); }}}
    {{{ wassert('entries != 0 || numEntries == 0'); }}} // Must be non-null pointer
    {{{ wassert('entries % 4 == 0'); }}} // Must be aligned at uint32_t boundary

    {{{ ptrToIdx('entries', 2); }}}
    let e = [];
    while(numEntries--) {
      let entry = {
        'binding': HEAPU32[entries],
        'visibility': HEAPU32[entries+1],
      }, type = HEAPU32[entries+2];
      entries += 4;
      {{{ wassert('type >= 1 && type <= 5'); }}}
      if (type == 1/*WGPU_BIND_GROUP_LAYOUT_TYPE_BUFFER*/) {
        entry['buffer'] = {
          'type': GPUBufferBindingTypes[HEAPU32[entries]],
          'hasDynamicOffset': !!HEAPU32[entries+1],
          'minBindingSize': wgpuReadI53FromU64HeapIdx(entries+2)
        };
      } else if (type == 2/*WGPU_BIND_GROUP_LAYOUT_TYPE_SAMPLER*/) {
        entry['sampler'] = {
          'type': GPUSamplerBindingTypes[HEAPU32[entries]]
        };
      } else if (type == 3/*WGPU_BIND_GROUP_LAYOUT_TYPE_TEXTURE*/) {
        entry['texture'] = {
          'sampleType': GPUTextureSampleTypes[HEAPU32[entries]],
          'viewDimension': GPUTextureViewDimensions[HEAPU32[entries+1]],
          'multisampled': !!HEAPU32[entries+2]
        };
      } else if (type == 4/*WGPU_BIND_GROUP_LAYOUT_TYPE_STORAGE_TEXTURE*/) {
        entry['storageTexture'] = {
          'access': GPUStorageTextureSampleTypes[HEAPU32[entries]],
          'format': GPUTextureAndVertexFormats[HEAPU32[entries+1]],
          'viewDimension': GPUTextureViewDimensions[HEAPU32[entries+2]]
        };
      } else { // type == 5/*WGPU_BIND_GROUP_LAYOUT_TYPE_EXTERNAL_TEXTURE*/ {
        entry['externalTexture'] = {};
      }
      entries += 4;
      e.push(entry);
    }
    return {
      'entries': e
    }
  },

  wgpu_device_create_bind_group_layout__deps: ['$wgpuStoreAndSetParent', '$wgpuReadBindGroupLayoutDescriptor'],
  wgpu_device_create_bind_group_layout: function(device, entries, numEntries) {
    {{{ wdebuglog('`wgpu_device_create_bind_group_layout(device=${device}, entries=${entries}, numEntries=${numEntries})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    device = wgpu[device];

    let desc = wgpuReadBindGroupLayoutDescriptor(entries, numEntries);
    {{{ wdebuglog('`GPUDevice.createBindGroupLayout() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    let bgl = device['createBindGroupLayout'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('bgl'); }}}

    return wgpuStoreAndSetParent(bgl, device);
  },

  wgpu_device_create_pipeline_layout__deps: ['$wgpuStoreAndSetParent', '$wgpuReadArrayOfWgpuObjects'],
  wgpu_device_create_pipeline_layout: function(device, layouts, numLayouts) {
    {{{ wdebuglog('`wgpu_device_create_pipeline_layout(device=${device}, layouts=${layouts}, numLayouts=${numLayouts})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    device = wgpu[device];

    return wgpuStoreAndSetParent(
      device['createPipelineLayout'](
        debugDir(
          {
            'bindGroupLayouts': wgpuReadArrayOfWgpuObjects(layouts, numLayouts)
          },
          'GPUDevice.createPipelineLayout() with desc'
        )
      ),
      device
    );
  },

  wgpu_device_create_bind_group__deps: ['$wgpuStoreAndSetParent', '$wgpuReadI53FromU64HeapIdx'],
  wgpu_device_create_bind_group: function(device, layout, entries, numEntries) {
    {{{ wdebuglog('`wgpu_device_create_bind_group(device=${device}, layout=${layout}, entries=${entries}, numEntries=${numEntries})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('layout != 0'); }}} // Must be a valid BindGroupLayout
    {{{ wassert('layout > 1'); }}} // Cannot pass WGPU_AUTO_LAYOUT_MODE_NO_HINT or WGPU_AUTO_LAYOUT_MODE_AUTO to this function
    {{{ wassert('wgpu[layout]'); }}}
    {{{ wassert('wgpu[layout] instanceof GPUBindGroupLayout'); }}}
    {{{ wassert('numEntries >= 0'); }}}
    {{{ wassert('entries != 0 || numEntries == 0'); }}} // Must be non-null pointer
    {{{ wassert('entries % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    device = wgpu[device];
    {{{ ptrToIdx('entries', 2); }}}
    let e = [];
    while(numEntries--) {
      let bindingIdx = HEAPU32[entries],
        resource = wgpu[HEAPU32[entries + 1]];
      {{{ wassert('resource'); }}}
      let binding = {
        'binding': bindingIdx,
        'resource': resource
      };
      if (resource.isBuffer) {
        let resourceBinding = {
          'buffer': resource,
          'offset': wgpuReadI53FromU64HeapIdx(entries + 2),
        }, size = wgpuReadI53FromU64HeapIdx(entries + 4);
        // Awkward polymorphism: cannot specify 'size' field if the whole buffer is to be bound.
        if (size) resourceBinding['size'] = size;
        binding['resource'] = resourceBinding;
      };
      e.push(binding);
      entries += 6;
    }

    return wgpuStoreAndSetParent(
      device['createBindGroup'](
        debugDir(
          {
            'layout': wgpu[layout],
            'entries': e
          },
          'GPUDevice.createBindGroup() with desc'
        )
      ),
      device
    );
  },

  $wgpuReadConstants: function(constants, numConstants) {
    {{{ wassert('numConstants >= 0'); }}}
    {{{ wassert('constants != 0 || numConstants == 0'); }}}
    {{{ wassert('constants % 4 == 0'); }}}

    let c = {};
    while(numConstants--) {
      c[UTF8ToString(HEAPU32[{{{ shiftPtr('constants', 2) }}}])] = HEAPF64[{{{ shiftPtr('constants + 8', 3) }}}];
      constants += 16;
    }
    return c;
  },

  wgpu_device_create_compute_pipeline__deps: ['$wgpuStoreAndSetParent', '$wgpuReadConstants', '$GPUAutoLayoutMode'],
  wgpu_device_create_compute_pipeline: function(device, computeModule, entryPoint, layout, constants, numConstants) {
    {{{ wdebuglog('`wgpu_device_create_compute_pipeline(device=${device}, computeModule=${computeModule}, entryPoint=${entryPoint}, layout=${layout}, constants=${constants}, numConstants=${numConstants})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('computeModule != 0'); }}}
    {{{ wassert('wgpu[computeModule]'); }}}
    {{{ wassert('wgpu[computeModule] instanceof GPUShaderModule'); }}}
    {{{ wassert('layout <= 1/*"auto"*/ || wgpu[layout]'); }}}
    {{{ wassert('layout <= 1/*"auto"*/ || wgpu[layout] instanceof GPUPipelineLayout'); }}}
    {{{ wassert('numConstants >= 0'); }}}
    {{{ wassert('numConstants == 0 || constants'); }}}
    {{{ wassert('!entryPoint || UTF8ToString(entryPoint).length > 0'); }}} // If entry point string is provided, it must be a nonempty JS string
    device = wgpu[device];

    return wgpuStoreAndSetParent(
      device['createComputePipeline'](
        debugDir(
          {
            'layout': layout > 1 ? wgpu[layout] : GPUAutoLayoutMode,
            'compute': {
              'module': wgpu[computeModule],
              'entryPoint': UTF8ToString(entryPoint) || void 0, // If null pointer was passed to use the default entry point name, then UTF8ToString() would return '', but spec requires undefined.
              'constants': wgpuReadConstants(constants, numConstants)
            }
          },
          'GPUDevice.createComputePipeline() with desc'
        )
      ),
      device
    );
  },

  wgpu_device_create_compute_pipeline_async__deps: ['$wgpuStoreAndSetParent', '$wgpuReadConstants', '$wgpuPipelineCreationFailed', 'wgpuMuteJsExceptions'],
  wgpu_device_create_compute_pipeline_async__docs: '/** @suppress{checkTypes} */', // This function intentionally calls cb() without args.
  wgpu_device_create_compute_pipeline_async: function(device, computeModule, entryPoint, layout, constants, numConstants, callback, userDate) {
    {{{ wdebuglog('`wgpu_device_create_compute_pipeline_async(device=${device}, computeModule=${computeModule}, entryPoint=${entryPoint}, layout=${layout}, constants=${constants}, numConstants=${numConstants}, callback=${callback}, userData=${userData})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('computeModule != 0'); }}}
    {{{ wassert('wgpu[computeModule]'); }}}
    {{{ wassert('wgpu[computeModule] instanceof GPUShaderModule'); }}}
    {{{ wassert('layout <= 1/*"auto"*/ || wgpu[layout]'); }}}
    {{{ wassert('layout <= 1/*"auto"*/ || wgpu[layout] instanceof GPUPipelineLayout'); }}}
    {{{ wassert('numConstants >= 0'); }}}
    {{{ wassert('numConstants == 0 || constants'); }}}
    {{{ wassert('!entryPoint || UTF8ToString(entryPoint).length > 0'); }}} // If entry point string is provided, it must be a nonempty JS string
    {{{ wassert('callback'); }}}
    let deviceObject = wgpu[device];

    let cb = (pipeline) => {
      {{{ wdebuglog('`createComputePipelineAsync succeeded with pipeline:`'); }}}
      {{{ wdebugdir('pipeline'); }}}
      {{{ makeDynCall('viiii', 'callback') }}}(device, /*error*/ 0, wgpuStoreAndSetParent(pipeline, deviceObject), userData);
    };

    deviceObject['createComputePipelineAsync'](
      debugDir(
        {
          'layout': layout > 1 ? wgpu[layout] : GPUAutoLayoutMode,
          'compute': {
            'module': wgpu[computeModule],
            'entryPoint': UTF8ToString(entryPoint) || void 0, // If null pointer was passed to use the default entry point name, then UTF8ToString() would return '', but spec requires undefined.
            'constants': wgpuReadConstants(constants, numConstants)
          }
        },
        'GPUDevice.createComputePipelineAsync() with desc'
      )
    ).then(_wgpuMuteJsExceptions(cb)).catch(
#if ASSERTIONS || global.WEBGPU_DEBUG
      (e)=>{console.error(`GPUDevice.createComputePipelineAsync() Promise failed: ${e}`); wgpuPipelineCreationFailed(device, e, userData); }
#else
      ()=>{cb(/*intentionally omit arg to pass undefined*/)}
#endif
    );
  },

  wgpu_texture_create_view__deps: ['$wgpuStoreAndSetParent', '$GPUTextureAndVertexFormats', '$GPUTextureViewDimensions', '$GPUTextureAspects'],
  wgpu_texture_create_view: function(texture, descriptor) {
    {{{ wdebuglog('`wgpu_texture_create_view(texture=${texture}, descriptor=${descriptor})`'); }}}
    {{{ wassert('texture != 0'); }}}
    {{{ wassert('wgpu[texture]'); }}}
    {{{ wassert('wgpu[texture] instanceof GPUTexture'); }}}
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary

    {{{ ptrToIdx('descriptor', 2); }}}
    return wgpuStoreAndSetParent(
      wgpu[texture]['createView'](
        debugDir(
          descriptor ? {
            'format': GPUTextureAndVertexFormats[HEAPU32[descriptor]],
            'dimension': GPUTextureViewDimensions[HEAPU32[descriptor+1]],
            'aspect': GPUTextureAspects[HEAPU32[descriptor+2]],
            'baseMipLevel': HEAP32[descriptor+3],
            'mipLevelCount': HEAP32[descriptor+4],
            'baseArrayLayer': HEAP32[descriptor+5],
            'arrayLayerCount': HEAP32[descriptor+6],
          } : void 0,
          'GPUTexture.createView() with desc'
        )
      ),
      wgpu[texture]
    );
  },

  // A "_simple" variant of wgpu_texture_create_view() that does
  // not take in any descriptor params, for building tiny code with default
  // args and creating readable test cases etc.
  wgpu_texture_create_view_simple__deps: ['$wgpuStoreAndSetParent'],
  wgpu_texture_create_view_simple: function(texture) {
    {{{ wdebuglog('`wgpu_texture_create_view_simple(texture=${texture})`'); }}}
    {{{ wassert('texture != 0'); }}}
    {{{ wassert('wgpu[texture]'); }}}
    {{{ wassert('wgpu[texture] instanceof GPUTexture'); }}}
    return wgpuStoreAndSetParent(wgpu[texture]['createView'](), wgpu[texture]);
  },

  wgpu_texture_width: function(texture) {
    {{{ wdebuglog('`wgpu_texture_width(texture=${texture})`'); }}}
    {{{ wassert('texture != 0'); }}}
    {{{ wassert('wgpu[texture]'); }}}
    {{{ wassert('wgpu[texture] instanceof GPUTexture'); }}}
    return wgpu[texture]['width'];
  },

  wgpu_texture_height: function(texture) {
    {{{ wdebuglog('`wgpu_texture_height(texture=${texture})`'); }}}
    {{{ wassert('texture != 0'); }}}
    {{{ wassert('wgpu[texture]'); }}}
    {{{ wassert('wgpu[texture] instanceof GPUTexture'); }}}
    return wgpu[texture]['height'];
  },

  wgpu_texture_depth_or_array_layers: function(texture) {
    {{{ wdebuglog('`wgpu_texture_depth_or_array_layers(texture=${texture})`'); }}}
    {{{ wassert('texture != 0'); }}}
    {{{ wassert('wgpu[texture]'); }}}
    {{{ wassert('wgpu[texture] instanceof GPUTexture'); }}}
    return wgpu[texture]['depthOrArrayLayers'];
  },

  wgpu_texture_mip_level_count: function(texture) {
    {{{ wdebuglog('`wgpu_texture_mip_level_count(texture=${texture})`'); }}}
    {{{ wassert('texture != 0'); }}}
    {{{ wassert('wgpu[texture]'); }}}
    {{{ wassert('wgpu[texture] instanceof GPUTexture'); }}}
    return wgpu[texture]['mipLevelCount'];
  },

  wgpu_texture_sample_count: function(texture) {
    {{{ wdebuglog('`wgpu_texture_sample_count(texture=${texture})`'); }}}
    {{{ wassert('texture != 0'); }}}
    {{{ wassert('wgpu[texture]'); }}}
    {{{ wassert('wgpu[texture] instanceof GPUTexture'); }}}
    return wgpu[texture]['sampleCount'];
  },

  wgpu_texture_dimension__deps: ['$GPUTextureDimensions'],
  wgpu_texture_dimension: function(texture) {
    {{{ wdebuglog('`wgpu_texture_dimension(texture=${texture})`'); }}}
    {{{ wassert('texture != 0'); }}}
    {{{ wassert('wgpu[texture]'); }}}
    {{{ wassert('wgpu[texture] instanceof GPUTexture'); }}}
    {{{ wassert('GPUTextureDimensions.indexOf(wgpu[texture]["dimension"]) != -1'); }}}
    return GPUTextureDimensions.indexOf(wgpu[texture]['dimension']);
  },

  wgpu_texture_format__deps: ['$GPUTextureAndVertexFormats'],
  wgpu_texture_format: function(texture) {
    {{{ wdebuglog('`wgpu_texture_format(texture=${texture})`'); }}}
    {{{ wassert('texture != 0'); }}}
    {{{ wassert('wgpu[texture]'); }}}
    {{{ wassert('wgpu[texture] instanceof GPUTexture'); }}}
    {{{ wassert('GPUTextureAndVertexFormats.indexOf(wgpu[texture]["format"]) != -1'); }}}
    return GPUTextureAndVertexFormats.indexOf(wgpu[texture]['format']);
  },

  wgpu_texture_usage: function(texture) {
    {{{ wdebuglog('`wgpu_texture_usage(texture=${texture})`'); }}}
    {{{ wassert('texture != 0'); }}}
    {{{ wassert('wgpu[texture]'); }}}
    {{{ wassert('wgpu[texture] instanceof GPUTexture'); }}}
    return wgpu[texture]['usage'];
  },

  wgpu_pipeline_get_bind_group_layout: function(pipelineBase, index) {
    {{{ wdebuglog('`wgpu_pipeline_get_bind_group_layout(pipelineBase=${pipelineBase}, index=${index})`'); }}}
    {{{ wassert('pipelineBase != 0'); }}}
    {{{ wassert('wgpu[pipelineBase]'); }}}
    {{{ wassert('wgpu[pipelineBase] instanceof GPURenderPipeline || wgpu[pipelineBase] instanceof GPUComputePipeline'); }}}

    return wgpuStore(debugDir(wgpu[pipelineBase]['getBindGroupLayout'](index), 'returned'));
  },

  $wgpuReadTimestampWrites: function(timestampWritesIndex) {
    let querySet = HEAPU32[timestampWritesIndex];
    if (querySet) {
      let timestampWrites = { 'querySet': wgpu[querySet] }, i;
      if ((i = HEAP32[timestampWritesIndex+1]) >= 0) timestampWrites['beginningOfPassWriteIndex'] = i;
      if ((i = HEAP32[timestampWritesIndex+2]) >= 0) timestampWrites['endOfPassWriteIndex'] = i;
      return timestampWrites;
    }
  },

  wgpu_command_encoder_begin_render_pass__deps: ['$GPULoadOps', '$GPUStoreOps', '$wgpuReadTimestampWrites'],
  wgpu_command_encoder_begin_render_pass: function(commandEncoder, descriptor) {
    {{{ wdebuglog('`wgpu_command_encoder_begin_render_pass(commandEncoder=${commandEncoder}, descriptor=${descriptor})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('descriptor != 0'); }}}
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary

    {{{ ptrToIdx('descriptor', 2); }}}

    let colorAttachments = [],
      numColorAttachments = HEAP32[descriptor++],
      colorAttachmentsIdx = {{{ shiftPtr('HEAPU32[descriptor++]', 2) }}},
      colorAttachmentsIdxDbl = {{{ shiftPtr('colorAttachmentsIdx + 6', 1) }}}, // Alias the view for HEAPF64.
      depthStencilView = wgpu[HEAPU32[descriptor]];

    {{{ wassert('colorAttachmentsIdx % 2 == 0'); }}} // Must be aligned at double boundary
    {{{ wassert('depthStencilView || !HEAPU32[descriptor]'); }}}

    {{{ wassert('numColorAttachments >= 0'); }}}
    while(numColorAttachments--) {
      // If view is 0, then this attachment is to be sparse.
      colorAttachments.push(HEAPU32[colorAttachmentsIdx] ? {
        'view': wgpu[HEAPU32[colorAttachmentsIdx]],
        'depthSlice': HEAP32[colorAttachmentsIdx+1] < 0 ? undefined : HEAP32[colorAttachmentsIdx+1], // Awkward polymorphism: spec does not allow 'depthSlice' to be given a value (even 0) if attachment is not a 3D texture.
        'resolveTarget': wgpu[HEAPU32[colorAttachmentsIdx+2]],
        'storeOp': GPUStoreOps[HEAPU32[colorAttachmentsIdx+3]],
        'loadOp': GPULoadOps[HEAPU32[colorAttachmentsIdx+4]],
        'clearValue': [HEAPF64[colorAttachmentsIdxDbl  ], HEAPF64[colorAttachmentsIdxDbl+1],
                       HEAPF64[colorAttachmentsIdxDbl+2], HEAPF64[colorAttachmentsIdxDbl+3]]
      } : null);

      colorAttachmentsIdx += 14;
      colorAttachmentsIdxDbl += 7;
    }

    {{{ wassert('Number.isSafeInteger(HEAPF64[descriptor+10>>1])'); }}} // 'maxDrawCount' is a double_int53_t
    {{{ wassert('HEAPF64[descriptor+10>>1] >= 0'); }}}

    return wgpuStore(
      debugDir(
        wgpu[commandEncoder]['beginRenderPass'](
          debugDir(
            {
              'colorAttachments': colorAttachments,
              // Awkward polymorphism: cannot specify 'view': undefined if no depth-stencil attachment
              // is to be present, but must pass null as the whole attachment object.
              'depthStencilAttachment': depthStencilView ? {
                'view': depthStencilView,
                'depthLoadOp': GPULoadOps[HEAPU32[descriptor+1]],
                'depthClearValue': HEAPF32[descriptor+2],
                'depthStoreOp': GPUStoreOps[HEAPU32[descriptor+3]],
                'depthReadOnly': !!HEAPU32[descriptor+4],
                'stencilLoadOp': GPULoadOps[HEAPU32[descriptor+5]],
                'stencilClearValue': HEAPU32[descriptor+6],
                'stencilStoreOp': GPUStoreOps[HEAPU32[descriptor+7]],
                'stencilReadOnly': !!HEAPU32[descriptor+8],
              } : void 0,
              'occlusionQuerySet': wgpu[HEAPU32[descriptor+9]],
              // Read 'maxDrawCount'. If set to zero, pass in undefined to use the default value
              // (likely 50 million, but omit it in case the spec might change in the future)
              'maxDrawCount': HEAPF64[descriptor+10>>1] || void 0,
              'timestampWrites': wgpuReadTimestampWrites(descriptor+12)
            },
            'GPUCommandEncoder.beginRenderPass() with desc'
          )
        ),
        'returned'
      )
    );
  },

  wgpu_command_encoder_begin_render_pass_1color_0depth__deps: ['$GPULoadOps', '$GPUStoreOps'],
  wgpu_command_encoder_begin_render_pass_1color_0depth: function(commandEncoder, descriptor) {
    {{{ wdebuglog('`wgpu_command_encoder_begin_render_pass_1color_0depth(commandEncoder=${commandEncoder}, descriptor=${descriptor})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('descriptor != 0'); }}}
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary

    {{{ ptrToIdx('descriptor', 2); }}}

    {{{ wassert('HEAP32[descriptor] == 1'); }}} // Must be passing exactly one color target.
    {{{ wassert('HEAPU32[descriptor+2] == 0'); }}} // Must be passing no depth-stencil target.

    let colorAttachmentsIdx = {{{ shiftPtr('HEAPU32[descriptor+1]', 2) }}},
      colorAttachmentsIdxDbl = {{{ shiftPtr('colorAttachmentsIdx + 6', 1) }}}; // Alias the view for HEAPF64.

    {{{ wassert('colorAttachmentsIdx % 2 == 0'); }}} // Must be aligned at double boundary

    return wgpuStore(debugDir(wgpu[commandEncoder]['beginRenderPass'](debugDir({
        'colorAttachments': [{
          'view': wgpu[HEAPU32[colorAttachmentsIdx]],
          'depthSlice': HEAP32[colorAttachmentsIdx+1] < 0 ? undefined : HEAP32[colorAttachmentsIdx+1], // Awkward polymorphism: spec does not allow 'depthSlice' to be given a value (even 0) if attachment is not a 3D texture.
          'resolveTarget': wgpu[HEAPU32[colorAttachmentsIdx+2]],
          'storeOp': GPUStoreOps[HEAPU32[colorAttachmentsIdx+3]],
          'loadOp': GPULoadOps[HEAPU32[colorAttachmentsIdx+4]],
          'clearValue': [HEAPF64[colorAttachmentsIdxDbl  ], HEAPF64[colorAttachmentsIdxDbl+1],
                         HEAPF64[colorAttachmentsIdxDbl+2], HEAPF64[colorAttachmentsIdxDbl+3]]
        }]
      }, 'GPUCommandEncoder.beginRenderPass() with desc')), 'returned'));
  },

  wgpu_command_encoder_begin_compute_pass__deps: ['$wgpuReadTimestampWrites'],
  wgpu_command_encoder_begin_compute_pass: function(commandEncoder, descriptor) {
    {{{ wdebuglog('`wgpu_command_encoder_begin_compute_pass(commandEncoder=${commandEncoder}, descriptor=${descriptor})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    // descriptor may be a null pointer
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary

    commandEncoder = wgpu[commandEncoder];
    {{{ ptrToIdx('descriptor', 2); }}}

    let desc = {
      'timestampWrites': wgpuReadTimestampWrites(descriptor)
    };

    {{{ wdebuglog('`GPUCommandEncoder.beginComputePass() with desc=`'); }}}
    {{{ wdebugdir('desc'); }}}
    let computePassEncoder = commandEncoder['beginComputePass'](desc);
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('computePassEncoder'); }}}

    return wgpuStore(computePassEncoder);
  },

  wgpu_command_encoder_copy_buffer_to_buffer: function(commandEncoder, source, sourceOffset, destination, destinationOffset, size) {
    {{{ wdebuglog('`wgpu_command_encoder_copy_buffer_to_buffer(commandEncoder=${commandEncoder}, source=${source}, sourceOffset=${sourceOffset}, destination=${destination}, destinationOffset=${destinationOffset}, size=${size})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('wgpu[source] instanceof GPUBuffer'); }}}
    {{{ wassert('wgpu[destination] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(sourceOffset)'); }}}
    {{{ wassert('sourceOffset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(destinationOffset)'); }}}
    {{{ wassert('destinationOffset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(size)'); }}}
    {{{ wassert('size >= 0'); }}}
    wgpu[commandEncoder]['copyBufferToBuffer'](wgpu[source], sourceOffset, wgpu[destination], destinationOffset, size);
  },

  $wgpuReadGpuImageCopyBuffer__deps: ['$wgpuReadI53FromU64HeapIdx'],
  $wgpuReadGpuImageCopyBuffer: function(ptr) {
    {{{ wassert('ptr != 0'); }}}
    {{{ wassert('ptr % 4 == 0'); }}} // Must be aligned at uint32_t boundary
    {{{ ptrToIdx('ptr', 2); }}}
    return {
      'offset': wgpuReadI53FromU64HeapIdx(ptr),
      'bytesPerRow': HEAP32[ptr+2],
      'rowsPerImage': HEAP32[ptr+3],
      'buffer': wgpu[HEAPU32[ptr+4]]
    };
  },

  $wgpuReadGpuImageCopyTexture__deps: ['$GPUTextureAspects'],
  $wgpuReadGpuImageCopyTexture: function(ptr) {
    {{{ wassert('ptr'); }}}
    {{{ wassert('ptr % 4 == 0'); }}}
    {{{ ptrToIdx('ptr', 2); }}}
    return {
      'texture': wgpu[HEAPU32[ptr]],
      'mipLevel': HEAP32[ptr+1],
      'origin': [HEAP32[ptr+2], HEAP32[ptr+3], HEAP32[ptr+4]],
      'aspect': GPUTextureAspects[HEAPU32[ptr+5]]
    };
  },

  wgpu_command_encoder_copy_buffer_to_texture__deps: ['$wgpuReadGpuImageCopyBuffer', '$wgpuReadGpuImageCopyTexture'],
  wgpu_command_encoder_copy_buffer_to_texture: function(commandEncoder, source, destination, copyWidth, copyHeight, copyDepthOrArrayLayers) {
    {{{ wdebuglog('`wgpu_command_encoder_copy_buffer_to_texture(commandEncoder=${commandEncoder}, source=${source}, destination=${destination}, copyWidth=${copyWidth}, copyHeight=${copyHeight}, copyDepthOrArrayLayers=${copyDepthOrArrayLayers})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('source'); }}}
    {{{ wassert('destination'); }}}
    wgpu[commandEncoder]['copyBufferToTexture'](wgpuReadGpuImageCopyBuffer(source), wgpuReadGpuImageCopyTexture(destination), [copyWidth, copyHeight, copyDepthOrArrayLayers]);
  },

  wgpu_command_encoder_copy_texture_to_buffer__deps: ['$wgpuReadGpuImageCopyTexture', '$wgpuReadGpuImageCopyBuffer'],
  wgpu_command_encoder_copy_texture_to_buffer: function(commandEncoder, source, destination, copyWidth, copyHeight, copyDepthOrArrayLayers) {
    {{{ wdebuglog('`wgpu_command_encoder_copy_texture_to_buffer(commandEncoder=${commandEncoder}, source=${source}, destination=${destination}, copyWidth=${copyWidth}, copyHeight=${copyHeight}, copyDepthOrArrayLayers=${copyDepthOrArrayLayers})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('source'); }}}
    {{{ wassert('destination'); }}}
    wgpu[commandEncoder]['copyTextureToBuffer'](wgpuReadGpuImageCopyTexture(source), wgpuReadGpuImageCopyBuffer(destination), [copyWidth, copyHeight, copyDepthOrArrayLayers]);
  },

  wgpu_command_encoder_copy_texture_to_texture__deps: ['$wgpuReadGpuImageCopyTexture'],
  wgpu_command_encoder_copy_texture_to_texture: function(commandEncoder, source, destination, copyWidth, copyHeight, copyDepthOrArrayLayers) {
    {{{ wdebuglog('`wgpu_command_encoder_copy_texture_to_texture(commandEncoder=${commandEncoder}, source=${source}, destination=${destination}, copyWidth=${copyWidth}, copyHeight=${copyHeight}, copyDepthOrArrayLayers=${copyDepthOrArrayLayers})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('source'); }}}
    {{{ wassert('destination'); }}}
    wgpu[commandEncoder]['copyTextureToTexture'](wgpuReadGpuImageCopyTexture(source), wgpuReadGpuImageCopyTexture(destination), [copyWidth, copyHeight, copyDepthOrArrayLayers]);
  },

  wgpu_command_encoder_clear_buffer: function(commandEncoder, buffer, offset, size) { 
    {{{ wdebuglog('`wgpu_command_encoder_clear_buffer(commandEncoder=${commandEncoder}, buffer=${buffer}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('wgpu[buffer]'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(offset)'); }}}
    {{{ wassert('offset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(size)'); }}}
    {{{ wassert('size >= 0'); }}}
    wgpu[commandEncoder]['clearBuffer'](buffer, offset, size < Infinity ? size : void 0);
    /*
    TODO: Verify that passing size: undefined in the above works to fill the remaining of the buffer. If not,
          will have to use this form:
    buffer = wgpu[buffer];
    size < Infinity
      ? wgpu[commandEncoder]['clearBuffer'](buffer, offset, size);
      : wgpu[commandEncoder]['clearBuffer'](buffer, offset);
    */
  },

  wgpu_encoder_push_debug_group: function(encoder, groupLabel) {
    {{{ wdebuglog('`wgpu_command_encoder_push_debug_group(encoder=${encoder}, groupLabel=${groupLabel})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUCommandEncoder || wgpu[encoder] instanceof GPUComputePassEncoder || wgpu[encoder] instanceof GPURenderPassEncoder || wgpu[encoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('groupLabel != 0'); }}}
    wgpu[encoder]['pushDebugGroup'](UTF8ToString(groupLabel));
  },

  wgpu_encoder_pop_debug_group: function(encoder) {
    {{{ wdebuglog('`wgpu_command_encoder_pop_debug_group(encoder=${encoder})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUCommandEncoder || wgpu[encoder] instanceof GPUComputePassEncoder || wgpu[encoder] instanceof GPURenderPassEncoder || wgpu[encoder] instanceof GPURenderBundleEncoder'); }}}
    wgpu[encoder]['popDebugGroup']();
  },

  wgpu_encoder_insert_debug_marker: function(encoder, markerLabel) {
    {{{ wdebuglog('`wgpu_command_encoder_insert_debug_marker(encoder=${encoder}, markerLabel=${markerLabel})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUCommandEncoder || wgpu[encoder] instanceof GPUComputePassEncoder || wgpu[encoder] instanceof GPURenderPassEncoder || wgpu[encoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('markerLabel != 0'); }}}
    wgpu[encoder]['insertDebugMarker'](UTF8ToString(markerLabel));
  },

  wgpu_command_encoder_resolve_query_set: function(commandEncoder, querySet, firstQuery, queryCount, destination, destinationOffset) {
    {{{ wdebuglog('`wgpu_command_encoder_resolve_query_set(commandEncoder=${commandEncoder}, querySet=${querySet}, firstQuery=${firstQuery}, queryCount=${queryCount}, destination=${destination}, destinationOffset=${destinationOffset})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('wgpu[querySet] instanceof GPUQuerySet'); }}}
    {{{ wassert('wgpu[destination] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(destinationOffset)'); }}}
    {{{ wassert('destinationOffset >= 0'); }}}
    wgpu[commandEncoder]['resolveQuerySet'](wgpu[querySet], firstQuery, queryCount, wgpu[destination], destinationOffset);
  },

  wgpu_encoder_set_pipeline: function(encoder, pipeline) {
    {{{ wdebuglog('`wgpu_encoder_set_pipeline(encoder=${encoder}, pipeline=${pipeline})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUComputePassEncoder || wgpu[encoder] instanceof GPURenderPassEncoder || wgpu[encoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('wgpu[pipeline] instanceof GPURenderPipeline || wgpu[pipeline] instanceof GPUComputePipeline'); }}}
    {{{ wassert('(wgpu[encoder] instanceof GPUComputePassEncoder) == (wgpu[pipeline] instanceof GPUComputePipeline)'); }}}
    wgpu[encoder]['setPipeline'](wgpu[pipeline]);
  },

  wgpu_render_commands_mixin_set_index_buffer__deps: ['$GPUIndexFormats'],
  wgpu_render_commands_mixin_set_index_buffer: function(passEncoder, buffer, indexFormat, offset, size) {
    {{{ wdebuglog('`wgpu_render_commands_mixin_set_index_buffer(passEncoder=${passEncoder}, buffer=${buffer}, indexFormat=${indexFormat}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('passEncoder != 0'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder] instanceof GPURenderPassEncoder || wgpu[passEncoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(offset)'); }}}
    {{{ wassert('offset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(size)'); }}}
    {{{ wassert('size >= -1'); }}}

    // Awkward API polymorphism: must omit size parameter altogether (instead of specifying e.g. -1 for whole buffer)
    size < 0 ? wgpu[passEncoder]['setIndexBuffer'](wgpu[buffer], GPUIndexFormats[indexFormat], offset)
      : wgpu[passEncoder]['setIndexBuffer'](wgpu[buffer], GPUIndexFormats[indexFormat], offset, size);
  },

  wgpu_render_commands_mixin_set_vertex_buffer: function(passEncoder, slot, buffer, offset, size) {
    {{{ wdebuglog('`wgpu_render_commands_mixin_set_vertex_buffer(passEncoder=${passEncoder}, slot=${slot}, buffer=${buffer}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('passEncoder != 0'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder] instanceof GPURenderPassEncoder || wgpu[passEncoder] instanceof GPURenderBundleEncoder'); }}}
    // N.b. buffer may be null here, in which case the existing buffer is intended to be unbound.
    {{{ wassert('buffer == 0 || wgpu[buffer]'); }}}
    {{{ wassert('buffer == 0 || wgpu[buffer] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(offset)'); }}}
    {{{ wassert('offset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(size)'); }}}
    {{{ wassert('size >= -1'); }}}

    // Awkward API polymorphism: must omit size parameter altogether (instead of specifying e.g. -1 for whole buffer)
    size < 0 ? wgpu[passEncoder]['setVertexBuffer'](slot, wgpu[buffer], offset)
      : wgpu[passEncoder]['setVertexBuffer'](slot, wgpu[buffer], offset, size);
  },

  wgpu_render_commands_mixin_draw: function(passEncoder, vertexCount, instanceCount, firstVertex, firstInstance) {
    {{{ wdebuglog('`wgpu_render_commands_mixin_draw(passEncoder=${passEncoder}, vertexCount=${vertexCount}, instanceCount=${instanceCount}, firstVertex=${firstVertex}, firstInstance=${firstInstance})`'); }}}
    {{{ wassert('passEncoder != 0'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder] instanceof GPURenderPassEncoder || wgpu[passEncoder] instanceof GPURenderBundleEncoder'); }}}

    wgpu[passEncoder]['draw'](vertexCount, instanceCount, firstVertex, firstInstance);
  },

  wgpu_render_commands_mixin_draw_indexed: function(passEncoder, indexCount, instanceCount, firstVertex, baseVertex, firstInstance) {
    {{{ wdebuglog('`wgpu_render_commands_mixin_draw_indexed(passEncoder=${passEncoder}, indexCount=${indexCount}, instanceCount=${instanceCount}, firstVertex=${firstVertex}, baseVertex=${baseVertex}, firstInstance=${firstInstance})`'); }}}
    {{{ wassert('passEncoder != 0'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder] instanceof GPURenderPassEncoder || wgpu[passEncoder] instanceof GPURenderBundleEncoder'); }}}

    wgpu[passEncoder]['drawIndexed'](indexCount, instanceCount, firstVertex, baseVertex, firstInstance);
  },

  wgpu_render_commands_mixin_draw_indirect: function(passEncoder, indirectBuffer, indirectOffset) {
    {{{ wdebuglog('`wgpu_render_commands_mixin_draw_indirect(passEncoder=${passEncoder}, indirectBuffer=${indirectBuffer}, indirectOffset=${indirectOffset})`'); }}}
    {{{ wassert('passEncoder != 0'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder] instanceof GPURenderPassEncoder || wgpu[passEncoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('wgpu[indirectBuffer] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(indirectOffset)'); }}}
    {{{ wassert('indirectOffset >= 0'); }}}

    wgpu[passEncoder]['drawIndirect'](wgpu[indirectBuffer], indirectOffset);
  },

  wgpu_render_commands_mixin_draw_indexed_indirect: function(passEncoder, indirectBuffer, indirectOffset) {
    {{{ wdebuglog('`wgpu_render_commands_mixin_draw_indexed_indirect(passEncoder=${passEncoder}, indirectBuffer=${indirectBuffer}, indirectOffset=${indirectOffset})`'); }}}
    {{{ wassert('passEncoder != 0'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder] instanceof GPURenderPassEncoder || wgpu[passEncoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('wgpu[indirectBuffer] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(indirectOffset)'); }}}
    {{{ wassert('indirectOffset >= 0'); }}}

    wgpu[passEncoder]['drawIndexedIndirect'](wgpu[indirectBuffer], indirectOffset);
  },

  wgpu_encoder_end__deps: ['wgpu_object_destroy'],
  wgpu_encoder_end: function(encoder) {
    {{{ wdebuglog('`wgpu_encoder_end(encoder=${encoder})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUComputePassEncoder || wgpu[encoder] instanceof GPURenderPassEncoder'); }}}

    wgpu[encoder]['end']();

    /* https://gpuweb.github.io/gpuweb/#render-pass-encoder-finalization:
      "The render pass encoder can be ended by calling end() once the user has
      finished recording commands for the pass. Once end() has been called the
      render pass encoder can no longer be used."

      Hence to help make C/C++ side code read easier, immediately release all references
      to the pass encoder after end() occurs, so that C/C++ side code does not need
      to release references manually. */
    _wgpu_object_destroy(encoder);
  },

  // WebGPU specification has two functions GPUCommandBuffer.finish() and GPURenderBundleEncoder.finish(),
  // which currently have identical semantics and structure. Therefore instead of emitting two functions for
  // each, generate only one to save code size.
  wgpu_encoder_finish__deps: ['wgpu_object_destroy'],
  wgpu_encoder_finish: function(encoder) {
    {{{ wdebuglog('`wgpu_encoder_finish(encoder=${encoder})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUCommandEncoder || wgpu[encoder] instanceof GPURenderBundleEncoder'); }}}

    {{{ wdebuglog('`GPU${wgpu[encoder] instanceof GPUCommandEncoder ? "Command" : "RenderBundle"}Encoder.finish()`'); }}}
    let cmdBuffer = wgpu[encoder]['finish']();
    {{{ wdebuglog('`returned:`'); }}}
    {{{ wdebugdir('cmdBuffer'); }}}

    /* https://gpuweb.github.io/gpuweb/#command-encoder-finalization:
      "A GPUCommandBuffer containing the commands recorded by the GPUCommandEncoder can be
      created by calling finish(). Once finish() has been called the command encoder can no
      longer be used."

      Hence to help make C/C++ side code read easier, immediately release all references to
      the command encoder after finish() occurs, so that C/C++ side code does not need to
      release references manually. */
    _wgpu_object_destroy(encoder);

    return wgpuStore(cmdBuffer);
  },

  wgpu_encoder_set_bind_group: function(encoder, index, /*nullable*/ bindGroup, dynamicOffsets, numDynamicOffsets) {
    {{{ wdebuglog('`wgpu_encoder_set_bind_group(encoder=${encoder}, index=${index}, bindGroup=${bindGroup}, dynamicOffsets=${dynamicOffsets}, numDynamicOffsets=${numDynamicOffsets})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUComputePassEncoder || wgpu[encoder] instanceof GPURenderPassEncoder || wgpu[encoder] instanceof GPURenderBundleEncoder'); }}}
    // N.b. bindGroup may be null here, in which case the existing bind group is intended to be unbound.
    {{{ wassert('bindGroup == 0 || wgpu[bindGroup]'); }}}
    {{{ wassert('bindGroup == 0 || wgpu[bindGroup] instanceof GPUBindGroup'); }}}
    {{{ wassert('dynamicOffsets != 0 || numDynamicOffsets == 0'); }}}
    {{{ wassert('dynamicOffsets % 4 == 0'); }}}
    wgpu[encoder]['setBindGroup'](index, wgpu[bindGroup], HEAPU32, {{{ shiftPtr('dynamicOffsets', 2) }}}, numDynamicOffsets);
  },

  wgpu_compute_pass_encoder_dispatch_workgroups: function(encoder, workgroupCountX, workgroupCountY, workgroupCountZ) {
    {{{ wdebuglog('`wgpu_compute_pass_encoder_dispatch_workgroups(encoder=${encoder}, workgroupCountX=${workgroupCountX}, workgroupCountY=${workgroupCountY}, workgroupCountZ=${workgroupCountZ})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUComputePassEncoder'); }}}
    wgpu[encoder]['dispatchWorkgroups'](workgroupCountX, workgroupCountY, workgroupCountZ);
  },

  wgpu_compute_pass_encoder_dispatch_workgroups_indirect: function(encoder, indirectBuffer, indirectOffset) {
    {{{ wdebuglog('`wgpu_compute_pass_encoder_dispatch_workgroups_indirect(encoder=${encoder}, indirectBuffer=${indirectBuffer}, indirectOffset=${indirectOffset})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUComputePassEncoder'); }}}
    {{{ wassert('indirectBuffer != 0'); }}}
    {{{ wassert('wgpu[indirectBuffer]'); }}}
    {{{ wassert('wgpu[indirectBuffer] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(indirectOffset)'); }}}
    {{{ wassert('indirectOffset >= 0'); }}}
    wgpu[encoder]['dispatchWorkgroupsIndirect'](wgpu[indirectBuffer], indirectOffset);
  },

  wgpu_render_pass_encoder_set_viewport: function(encoder, x, y, width, height, minDepth, maxDepth) {
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_viewport(encoder=${encoder}, x=${x}, y=${y}, width=${width}, height=${height}, minDepth=${minDepth}, maxDepth=${maxDepth})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['setViewport'](x, y, width, height, minDepth, maxDepth);
  },

  wgpu_render_pass_encoder_set_scissor_rect: function(encoder, x, y, width, height) {
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_scissor_rect(encoder=${encoder}, x=${x}, y=${y}, width=${width}, height=${height})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['setScissorRect'](x, y, width, height);
  },

  wgpu_render_pass_encoder_set_blend_constant: function(encoder, r, g, b, a) {
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_blend_constant(encoder=${encoder}, r=${r}, g=${g}, b=${b}, a=${a})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['setBlendConstant'](r, g, b, a);
  },

  wgpu_render_pass_encoder_set_stencil_reference: function(encoder, stencilValue) {
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_stencil_reference(stencilValue=${stencilValue})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['setStencilReference'](stencilValue);
  },

  wgpu_render_pass_encoder_begin_occlusion_query: function(encoder, queryIndex) {
    {{{ wdebuglog('`wgpu_render_pass_encoder_begin_occlusion_query(queryIndex=${queryIndex})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['beginOcclusionQuery'](queryIndex);
  },

  wgpu_render_pass_encoder_end_occlusion_query: function(encoder) {
    {{{ wdebuglog('`wgpu_render_pass_encoder_end_occlusion_query()`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['endOcclusionQuery']();
  },

  wgpu_render_pass_encoder_execute_bundles__deps: ['$wgpuReadArrayOfWgpuObjects'],
  wgpu_render_pass_encoder_execute_bundles: function(encoder, bundles, numBundles) {
    {{{ wdebuglog('`wgpu_render_pass_encoder_execute_bundles(encoder=${encoder}, bundles=${bundles}, numBundles=${numBundles})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['executeBundles'](wgpuReadArrayOfWgpuObjects(bundles, numBundles));
  },

  wgpu_queue_submit_one_and_destroy__deps: ['wgpu_object_destroy'],
  wgpu_queue_submit_one_and_destroy: function(queue, commandBuffer) {
    {{{ wdebuglog('`wgpu_queue_submit_one_and_destroy(queue=${queue}, commandBuffer=${commandBuffer})`'); }}}
    {{{ wassert('queue != 0'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue] instanceof GPUQueue'); }}}
    {{{ wassert('commandBuffer != 0'); }}}
    {{{ wassert('wgpu[commandBuffer]'); }}}
    {{{ wassert('wgpu[commandBuffer] instanceof GPUCommandBuffer'); }}}
    wgpu[queue]['submit']([wgpu[commandBuffer]]);
    _wgpu_object_destroy(commandBuffer);
  },

  wgpu_queue_submit_multiple_and_destroy__deps: ['wgpu_object_destroy'],
  wgpu_queue_submit_multiple_and_destroy: function(queue, commandBuffers, numCommandBuffers) {
    {{{ wdebuglog('`wgpu_queue_submit_multiple_and_destroy(queue=${queue}, commandBuffers=${commandBuffers}, numCommandBuffers=${numCommandBuffers})`'); }}}
    {{{ wassert('queue != 0'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue] instanceof GPUQueue'); }}}
    wgpu[queue]['submit'](wgpuReadArrayOfWgpuObjects(commandBuffers, numCommandBuffers));

    {{{ ptrToIdx('commandBuffers', 2); }}}
    let end = commandBuffers + numCommandBuffers;
    while(commandBuffers < end) _wgpu_object_destroy(HEAPU32[commandBuffers++]);
  },

  wgpu_queue_set_on_submitted_work_done_callback: function(queue, callback, userData) {
    {{{ wassert('queue != 0'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue] instanceof GPUQueue'); }}}
    {{{ wassert('callback'); }}}
    wgpu[queue]['onSubmittedWorkDone']().then(() => {
      {{{ makeDynCall('vii', 'callback') }}}(queue, userData);
    });
  },

#if MIN_CHROME_VERSION != TARGET_NOT_SUPPORTED && MAXIMUM_MEMORY > 2147483647
  // Chrome has a bug that it cannot pass > 2GB typed arrays into the writeBuffer or writeTexture functions.
  // https://issues.chromium.org/issues/339049388
  $wgpuCloneArrayToLessThan2GB: function(offset, size) {
    if (size > 2147483647) {
      console.error(`Attempted to upload a buffer/texture larger than 2GB (${size} bytes), which is not supported`);
      throw 'WebGPU >2GB not supported';
    }
    return new Uint8Array(new Uint8Array(HEAPU8, offset, size));
  },
#endif

#if MIN_CHROME_VERSION != TARGET_NOT_SUPPORTED && MAXIMUM_MEMORY > 2147483647
  wgpu_queue_write_buffer__deps: ['$wgpuCloneArrayForLessThan2GB'],
#endif
  wgpu_queue_write_buffer: function(queue, buffer, bufferOffset, data, size) {
    {{{ wdebuglog('`wgpu_queue_write_buffer(queue=${queue}, buffer=${buffer}, bufferOffset=${bufferOffset}, data=${data}, size=${size})`'); }}}
    {{{ wassert('queue != 0'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue] instanceof GPUQueue'); }}}
    {{{ wassert('buffer != 0'); }}}
    {{{ wassert('wgpu[buffer]'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}
#if MIN_CHROME_VERSION != TARGET_NOT_SUPPORTED && MAXIMUM_MEMORY > 2147483647
    wgpu[queue]['writeBuffer'](wgpu[buffer], bufferOffset, wgpuCloneArrayToLessThan2GB(data, size), 0, size);
#else
    wgpu[queue]['writeBuffer'](wgpu[buffer], bufferOffset, HEAPU8, data, size);
#endif
  },

#if MIN_CHROME_VERSION != TARGET_NOT_SUPPORTED && MAXIMUM_MEMORY > 2147483647
  wgpu_queue_write_texture__deps: ['$wgpuReadGpuImageCopyTexture', '$wgpuCloneArrayForLessThan2GB'],
#else
  wgpu_queue_write_texture__deps: ['$wgpuReadGpuImageCopyTexture'],
#endif
  wgpu_queue_write_texture: function(queue, destination, data, bytesPerBlockRow, blockRowsPerImage, writeWidth, writeHeight, writeDepthOrArrayLayers) {
    {{{ wdebuglog('`wgpu_queue_write_texture(queue=${queue}, destination=${destination}, data=${data}, bytesPerBlockRow=${bytesPerBlockRow}, blockRowsPerImage=${blockRowsPerImage}, writeWidth=${writeWidth}, writeHeight=${writeHeight}, writeDepthOrArrayLayers=${writeDepthOrArrayLayers})`'); }}}
    {{{ wassert('queue != 0'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue] instanceof GPUQueue'); }}}
    {{{ wassert('destination'); }}}
#if MIN_CHROME_VERSION != TARGET_NOT_SUPPORTED && MAXIMUM_MEMORY > 2147483647
    wgpu[queue]['writeTexture'](wgpuReadGpuImageCopyTexture(destination), wgpuCloneArrayToLessThan2GB(data, bytesPerBlockRow*blockRowsPerImage), { 'offset': 0, 'bytesPerRow': bytesPerBlockRow, 'rowsPerImage': blockRowsPerImage }, [writeWidth, writeHeight, writeDepthOrArrayLayers]);
#else
    wgpu[queue]['writeTexture'](wgpuReadGpuImageCopyTexture(destination), HEAPU8, { 'offset': data, 'bytesPerRow': bytesPerBlockRow, 'rowsPerImage': blockRowsPerImage }, [writeWidth, writeHeight, writeDepthOrArrayLayers]);
#endif
  },

  wgpu_queue_copy_external_image_to_texture__deps: ['$wgpuReadGpuImageCopyTexture', '$HTMLPredefinedColorSpaces'],
  wgpu_queue_copy_external_image_to_texture: function(queue, source, destination, copyWidth, copyHeight, copyDepthOrArrayLayers) {
    {{{ wdebuglog('`wgpu_queue_copy_external_image_to_texture(queue=${queue}, source=${source}, destination=${destination}, copyWidth=${copyWidth}, copyHeight=${copyHeight}, copyDepthOrArrayLayers=${copyDepthOrArrayLayers})`'); }}}
    {{{ wassert('queue != 0'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue] instanceof GPUQueue'); }}}
    {{{ wassert('source'); }}}
    {{{ wassert('source % 4 == 0'); }}}
    {{{ wassert('destination'); }}}
    {{{ wassert('destination % 4 == 0'); }}}

    {{{ ptrToIdx('source', 2); }}}
    let dest = wgpuReadGpuImageCopyTexture(destination);
    {{{ ptrToIdx('destination', 2); }}}
    dest['colorSpace'] = HTMLPredefinedColorSpaces[HEAP32[destination+6]];
    dest['premultipliedAlpha'] = !!HEAP32[destination+7];

#if ASSERTIONS
    let src = {
      'source': wgpu[HEAPU32[source]],
      'origin': [HEAP32[source+1], HEAP32[source+2]],
      'flipY': !!HEAPU32[source+3]
      };
    {{{ wdebuglog('`Calling copyExternalImageToTexture with src:`'); }}}
    {{{ wdebugdir('src'); }}}
    {{{ wdebuglog('`dst:`'); }}}
    {{{ wdebugdir('dest'); }}}
    {{{ wdebuglog('`copy dimensions: ${[copyWidth, copyHeight, copyDepthOrArrayLayers]}`'); }}}
    wgpu[queue]['copyExternalImageToTexture'](src, dest, [copyWidth, copyHeight, copyDepthOrArrayLayers]);
#else
    wgpu[queue]['copyExternalImageToTexture']({
      'source': wgpu[HEAPU32[source]],
      'origin': [HEAP32[source+1], HEAP32[source+2]],
      'flipY': !!HEAPU32[source+3]
      }, dest, [copyWidth, copyHeight, copyDepthOrArrayLayers]);
#endif
  },

  wgpu_query_set_type__deps: ['$GPUQueryTypes'],
  wgpu_query_set_type: function(querySet) {
    {{{ wdebuglog('`wgpu_query_set_type(querySet=${querySet})`'); }}}
    {{{ wassert('querySet != 0'); }}}
    {{{ wassert('wgpu[querySet]'); }}}
    {{{ wassert('wgpu[querySet] instanceof GPUQuerySet'); }}}
    {{{ wassert('GPUQueryTypes.indexOf(wgpu[querySet]["type"]) != -1'); }}}
    return GPUQueryTypes.indexOf(wgpu[querySet]['type']);
  },

  wgpu_query_set_count: function(querySet) {
    {{{ wdebuglog('`wgpu_query_set_count(querySet=${querySet})`'); }}}
    {{{ wassert('querySet != 0'); }}}
    {{{ wassert('wgpu[querySet]'); }}}
    {{{ wassert('wgpu[querySet] instanceof GPUQuerySet'); }}}
    return wgpu[querySet]['count'];
  },

  wgpu_load_image_bitmap_from_url_async__deps: ['wgpuMuteJsExceptions'],
  wgpu_load_image_bitmap_from_url_async: function(url, flipY, callback, userData) {
    {{{ wdebuglog('`wgpu_load_image_bitmap_from_url_async(url=\"${UTF8ToString(url)}\" (${url}), callback=${callback}, userData=${userData})`'); }}}
    let img = new Image();
    img.src = UTF8ToString(url);

    function dispatchCallback(imageBitmapOrError) {
      {{{ wdebuglog('`createImageBitmap(img) loaded:`'); }}}
      {{{ wdebugdir('imageBitmapOrError'); }}}
      {{{ wdebuglog('`from:`'); }}}
      {{{ wdebugdir('img'); }}}
      {{{ makeDynCall('viiii', 'callback') }}}(imageBitmapOrError.width && wgpuStore(imageBitmapOrError), imageBitmapOrError.width, imageBitmapOrError.height, userData);
    }

    img.decode().then(() => {
      return createImageBitmap(img, flipY ? { 'imageOrientation': 'flipY' } : {});
    }).then(_wgpuMuteJsExceptions(dispatchCallback)).catch(dispatchCallback);
  }
};

// If building with -jsDWEBGPU_PROFILE=1, then wrap all WebGPU API calls into
// performance.now() timers to get a log of any slow running functions.
#if global.WEBGPU_PROFILE
function argList(len) {
  var args = [];
  for(let i = 0; i < len; ++i) args.push(`a${i}`);
    return args;
}

for(const [name, func] of Object.entries(api)) {
  if (name.startsWith('wgpu_') && func instanceof Function) {
    const benchmarked_name = `benchmarked_${name}`;
    api[benchmarked_name] = func;
    if (api[`${name}__deps`]) api[`${name}__deps`].push(benchmarked_name);
    else api[`${name}__deps`] = [benchmarked_name];

    const args = argList(func.length);
    api[name] = new Function(...args, `const t0 = performance.now();
const ret = _${benchmarked_name}(${args.join(',')});
const t1 = performance.now();
if (t1-t0 > 1) console.warn(\`[wgpu perf] Call to ${name} took \${t1-t0} msecs.\`);
return ret;`);
  }
}
#endif

mergeInto(LibraryManager.library, api);
