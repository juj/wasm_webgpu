{{{ (function() {
  global.wassert = function(condition) {
    if (ASSERTIONS || global.WEBGPU_DEBUG) return `assert(${condition}, "assert(${condition.replace(/"/g, "'")}) failed!");`;
    else return '';
  };
  return null;
})(); }}}
{{{ (function() { global.wdebuglog = function(condition) { return global.WEBGPU_DEBUG ? `console.log(${condition});` : ''; }; return null; })(); }}}
{{{ (function() { global.wdebugwarn = function(condition) { return global.WEBGPU_DEBUG ? `console.warn(${condition});` : ''; }; return null; })(); }}}
{{{ (function() { global.wdebugerror = function(condition) { return global.WEBGPU_DEBUG ? `console.error(${condition});` : ''; }; return null; })(); }}}
{{{ (function() { global.wdebugdir = function(condition) { return global.WEBGPU_DEBUG ? `console.dir(${condition});` : ''; }; return null; })(); }}}
{{{ (function() { global.werror = function(condition) { return global.WEBGPU_DEBUG ? `console.error(${condition});` : ''; }; return null; })(); }}}

// Implement safe heap accesses for 2GB, 4GB and Wasm64 build modes. (TODO: lib_webgpu is not yet really Wasm64-capable except for the two functions below)
{{{ (function() { global.ptrToIdx = function(ptr, accessWidth) {
  if (MEMORY64) return `${ptr} >>= ${accessWidth}n`;
  if (MAXIMUM_MEMORY > 2*1024*1024*1024) return `${ptr} >>>= ${accessWidth}`;
  return `${ptr} >>= ${accessWidth}`; }; return null; })(); }}}

{{{ (function() { global.shiftPtr = function(ptr, accessWidth) {
  if (MEMORY64) return `${ptr} >> ${accessWidth}n`;
  if (MAXIMUM_MEMORY > 2*1024*1024*1024) return `${ptr} >>> ${accessWidth}`;
  return `${ptr} >> ${accessWidth}`; }; return null; })(); }}}

mergeInto(LibraryManager.library, {

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
    // TODO: return Number(HEAPU64[heap32Idx>>1]);
#else
    return HEAPU32[heap32Idx] + HEAPU32[heap32Idx+1] * 4294967296;
#endif
  },

  $wgpuWriteU64HeapIdx: function(heap32Idx, number) {
    {{{ wassert('heap32Idx != 0'); }}}
#if WASM_BIGINT
    {{{ wassert('heap32Idx % 2 == 0'); }}}
    // TODO: HEAPU64[heap32Idx>>1] = number;
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
    if (o) {
      // WebGPU objects of type GPUDevice, GPUBuffer, GPUTexture and GPUQuerySet have an explicit .destroy() function. Call that if applicable.
      if (o['destroy']) o['destroy']();
      // If the given object has derived objects (GPUTexture -> GPUTextureViews), delete those in a hierarchy as well.
      if (o.derivedObjects) o.derivedObjects.forEach(_wgpu_object_destroy);
      // Finally erase reference to this object.
      delete wgpu[object];
    }
    {{{ wassert(`!(object in wgpu), 'wgpu dictionary should not be storing nulls/undefineds/zeroes!'`); }}}
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

  $wgpuDecodeStrings__docs: '/** @param {number=} ch */',
  $wgpuDecodeStrings: function(s, c, ch) {
    ch = ch || 65;
    for(c = c.split('|'); c[0];) s = s['replaceAll'](String.fromCharCode(ch++), c.pop());
    return [,].concat(s.split(' '));
  },

  $GPUTextureAndVertexFormats__deps: ['$wgpuDecodeStrings'],
//$GPUTextureAndVertexFormats: [undefined (0), 'r8unorm' (1), 'r8snorm' (2), 'r8uint' (3), 'r8sint' (4), 'r16uint' (5), 'r16sint' (6), 'r16float' (7), 'rg8unorm' (8), 'rg8snorm' (9), 'rg8uint' (10), 'rg8sint' (11), 'r32uint' (12), 'r32sint' (13), 'r32float' (14), 'rg16uint' (15), 'rg16sint' (16), 'rg16float' (17), 'rgba8unorm' (18), 'rgba8unorm-srgb' (19), 'rgba8snorm' (20), 'rgba8uint' (21), 'rgba8sint' (22), 'bgra8unorm' (23), 'bgra8unorm-srgb' (24), 'rgb9e5ufloat' (25), 'rgb10a2unorm' (26), 'rg11b10ufloat' (27), 'rg32uint' (28), 'rg32sint' (29), 'rg32float' (30), 'rgba16uint' (31), 'rgba16sint' (32), 'rgba16float' (33), 'rgba32uint' (34), 'rgba32sint' (35), 'rgba32float' (36), 'stencil8' (37), 'depth16unorm' (38), 'depth24plus' (39), 'depth24plus-stencil8' (40), 'depth32float' (41), 'depth32float-stencil8' (42), 'bc1-rgba-unorm' (43), 'bc1-rgba-unorm-srgb' (44), 'bc2-rgba-unorm' (45), 'bc2-rgba-unorm-srgb' (46), 'bc3-rgba-unorm' (47), 'bc3-rgba-unorm-srgb' (48), 'bc4-r-unorm' (49), 'bc4-r-snorm' (50), 'bc5-rg-unorm' (51), 'bc5-rg-snorm' (52), 'bc6h-rgb-ufloat' (53), 'bc6h-rgb-float' (54), 'bc7-rgba-unorm' (55), 'bc7-rgba-unorm-srgb' (56), 'etc2-rgb8unorm' (57), 'etc2-rgb8unorm-srgb' (58), 'etc2-rgb8a1unorm' (59), 'etc2-rgb8a1unorm-srgb' (60), 'etc2-rgba8unorm' (61), 'etc2-rgba8unorm-srgb' (62), 'eac-r11unorm' (63), 'eac-r11snorm' (64), 'eac-rg11unorm' (65), 'eac-rg11snorm' (66), 'astc-4x4-unorm' (67), 'astc-4x4-unorm-srgb' (68), 'astc-5x4-unorm' (69), 'astc-5x4-unorm-srgb' (70), 'astc-5x5-unorm' (71), 'astc-5x5-unorm-srgb' (72), 'astc-6x5-unorm' (73), 'astc-6x5-unorm-srgb' (74), 'astc-6x6-unorm' (75), 'astc-6x6-unorm-srgb' (76), 'astc-8x5-unorm' (77), 'astc-8x5-unorm-srgb' (78), 'astc-8x6-unorm' (79), 'astc-8x6-unorm-srgb' (80), 'astc-8x8-unorm' (81), 'astc-8x8-unorm-srgb' (82), 'astc-10x5-unorm' (83), 'astc-10x5-unorm-srgb' (84), 'astc-10x6-unorm' (85), 'astc-10x6-unorm-srgb' (86), 'astc-10x8-unorm' (87), 'astc-10x8-unorm-srgb' (88), 'astc-10x10-unorm' (89), 'astc-10x10-unorm-srgb' (90), 'astc-12x10-unorm' (91), 'astc-12x10-unorm-srgb' (92), 'astc-12x12-unorm' (93), 'astc-12x12-unorm-srgb' (94), 'uint8x2' (95), 'uint8x4' (96), 'sint8x2' (97), 'sint8x4' (98), 'unorm8x2' (99), 'unorm8x4' (100), 'snorm8x2' (101), 'snorm8x4' (102), 'uint16x2' (103), 'uint16x4' (104), 'sint16x2' (105), 'sint16x4' (106), 'unorm16x2' (107), 'unorm16x4' (108), 'snorm16x2' (109), 'snorm16x4' (110), 'float16x2' (111), 'float16x4' (112), 'float32' (113), 'float32x2' (114), 'float32x3' (115), 'float32x4' (116), 'uint32' (117), 'uint32x2' (118), 'uint32x3' (119), 'uint32x4' (120), 'sint32' (121), 'sint32x2' (122), 'sint32x3' (123), 'sint32x4' (124)],
  $GPUTextureAndVertexFormats: "wgpuDecodeStrings('r8YA8RmA8UA8TAHUAHTAHVO8YO8RmO8UO8TALUALTALVOHUOHTOHV W8Y W8Z W8Rm W8U W8T bgra8Y bgra8ZOb9e5uVOb10a2YO11b10uVOLUOLTOLV WHU WHT WHV WLU WLT WLV GJHYJ24plusJ24plus-GJLVJLV-GQ1-W-YQ1-W-ZQ2-W-YQ2-W-ZQ3-W-YQ3-W-ZQ4-r-YQ4-r-RmQ5-rg-YQ5-rg-RmQ6h-rgb-uVQ6h-rgb-VQ7-W-YQ7-W-ZSYSZSa1YSa1Z etc2-W8Y etc2-W8ZI11YI11RmIg11YIg11RmX4x4-YX4x4-ZX5x4-YX5x4-ZX5x5-YX5x5-ZX6x5-YX6x5-ZX6x6-YX6x6-ZX8x5-YX8x5-ZX8x6-YX8x6-ZX8x8-YX8x8-ZXE5-YXE5-ZXE6-YXE6-ZXE8-YXE8-ZXE10-YXE10-ZX12x10-YX12x10-ZX12x12-YX12x12-Z U8MU8KT8MT8KY8MY8KRm8MRm8KUHMUHKTHMTHKYHMYHKRmHMRmHKVHMVHKVL VLMVLx3 VLKUL ULMULx3 ULKTL TLMTLx3 TLx4', 'unorm-srgb|unorm| astc-|rgba|float|uint|sint| etc2-rgb8|snor| bc|-BC| rg|-AC|x2 |32|x4 | depth| eac-r|16|stencil8|-D-BJ|10x| D|Im|-D-AJ| r')",

  wgpu32BitLimitNames__deps: ['$wgpuDecodeStrings'],
//wgpu32BitLimitNames: ['maxTextureDimension1D' (0), 'maxTextureDimension2D' (1), 'maxTextureDimension3D' (2), 'maxTextureArrayLayers' (3), 'maxBindGroups' (4), 'maxBindingsPerBindGroup' (5), 'maxDynamicUniformBuffersPerPipelineLayout' (6), 'maxDynamicStorageBuffersPerPipelineLayout' (7), 'maxSampledTexturesPerShaderStage' (8), 'maxSamplersPerShaderStage' (9), 'maxStorageBuffersPerShaderStage' (10), 'maxStorageTexturesPerShaderStage' (11), 'maxUniformBuffersPerShaderStage' (12), 'minUniformBufferOffsetAlignment' (13), 'minStorageBufferOffsetAlignment' (14), 'maxVertexBuffers' (15), 'maxVertexAttributes' (16), 'maxVertexBufferArrayStride' (17), 'maxInterStageShaderComponents' (18), 'maxInterStageShaderVariables' (19), 'maxColorAttachments' (20), 'maxColorAttachmentBytesPerSample' (21), 'maxComputeWorkgroupStorageSize' (22), 'maxComputeInvocationsPerWorkgroup' (23), 'maxComputeWorkgroupSizeX' (24), 'maxComputeWorkgroupSizeY' (25), 'maxComputeWorkgroupSizeZ' (26)],
  wgpu32BitLimitNames: "wgpuDecodeStrings('>1D >2D >3D max6ArrayLayer<BindGroup<BindingsPerBindGroup maxDynamic5m=DynamicS:e=4d6?ax4r?axS:eB7?axS:e6?ax5mB7?in5m;minS:e;maxVertexB7<VertexAttribute<VertexB7ArrayStride max9Component<9Variable<8<8BytesPer4@:eSize maxComputeInvocationsPerWorkgroup@izeX@izeY@izeZ', ' maxComputeWorkgroupS|sPerShaderStage m|maxTextureDimension|BuffersPerPipelineLayout max|s max|BufferOffsetAlignment |torag|InterStageShader|ColorAttachment|uffer|Texture|Unifor|Sample', 52).slice(1)",

  wgpu64BitLimitNames__deps: ['$wgpuDecodeStrings'],
//wgpu64BitLimitNames: ['maxUniformBufferBindingSize' (0), 'maxStorageBufferBindingSize' (1), 'maxBufferSize' (2)],
  wgpu64BitLimitNames: "wgpuDecodeStrings('maxUniform4Storage4BufferSize', 'BufferBindingSize max', 52).slice(1)",

  wgpuFeatures__deps: ['$wgpuDecodeStrings'],
//wgpuFeatures: ['depth-clip-control' (0), 'depth32float-stencil8' (1), 'texture-compression-bc' (2), 'texture-compression-etc2' (3), 'texture-compression-astc' (4), 'timestamp-query' (5), 'indirect-first-instance' (6), 'shader-f16' (7), 'bgra8unorm-storage' (8), 'rg11b10ufloat-renderable' (9)],
  wgpuFeatures: "wgpuDecodeStrings('C-clip-control C32BAencil8DbcDetc2DaAc timeAamp-query indirect-firA-inAance shader-f16 bgra8unorm-Aorage rg11b10uBrenderable', ' texture-compression-|depth|float-|st').slice(1)",

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

  $GPULoadOps: ['load', 'clear'],

  $GPUStoreOps: ['store', 'discard'],

  $GPUComputePassTimestampLocations: ['beginning', 'end'],

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
        deviceLostInfo['reason'] == 'destroyed' ? 1/*WGPU_DEVICE_LOST_REASON_DESTROYED*/ : 0,
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

  wgpu_device_pop_error_scope_async__deps: ['wgpuDispatchWebGpuErrorEvent'],
  wgpu_device_pop_error_scope_async: function(device, callback, userData) {
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('callback'); }}}

    function dispatchErrorCallback(error) {
      _wgpuDispatchWebGpuErrorEvent(device, callback, error, userData);
    }

    wgpu[device]['popErrorScope']().then(dispatchErrorCallback).catch(dispatchErrorCallback);
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

  navigator_gpu_request_adapter_async__deps: ['$wgpuStore', '$debugDir'],
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
      gpu['requestAdapter'](opts).then(cb).catch(()=>{cb(/*intentionally omit arg to pass undefined*/)});
      return 1/*EM_TRUE*/;
    }
    {{{ werror('`WebGPU is not supported by the current browser!`'); }}}
    // Implicit return EM_FALSE, WebGPU is not supported.
  },

#if ASYNCIFY
  $wgpuAsync: function(promise) {
    return Asyncify.handleAsync(() => { return promise; });
  },

  navigator_gpu_request_adapter_sync__deps: ['$wgpuStore', '$debugDir', '$wgpuAsync'],
  navigator_gpu_request_adapter_sync: function(options) {
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
      return wgpuAsync(gpu['requestAdapter'](opts).then(wgpuStore));
    }
    {{{ werror('`WebGPU is not supported by the current browser!`'); }}}
    // Implicit return EM_FALSE, WebGPU is not supported.
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
  navigator_gpu_request_adapter_sync_simple__deps: ['$wgpuStore', '$wgpuAsync'],
  navigator_gpu_request_adapter_sync_simple: function() {
    {{{ wdebuglog('`navigator_gpu_request_adapter_sync_simple()`'); }}}
    {{{ wassert('navigator["gpu"], "Your browser does not support WebGPU!"'); }}}
    return wgpuAsync(navigator['gpu']['requestAdapter']().then(wgpuStore));
  },
#endif

  navigator_gpu_get_preferred_canvas_format__deps: ['$GPUTextureAndVertexFormats'],
  navigator_gpu_get_preferred_canvas_format: function() {
    {{{ wdebuglog('`navigator_gpu_get_preferred_canvas_format()`'); }}}
    {{{ wassert('navigator["gpu"], "Your browser does not support WebGPU!"'); }}}

    {{{ wassert('GPUTextureAndVertexFormats.includes(navigator["gpu"]["getPreferredCanvasFormat"]())'); }}}
    return GPUTextureAndVertexFormats.indexOf(navigator['gpu']['getPreferredCanvasFormat']());
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

  wgpu_adapter_or_device_get_limits__deps: ['wgpu32BitLimitNames', 'wgpu64BitLimitNames', '$wgpuWriteU64HeapIdx'],
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
      wgpuWriteU64HeapIdx(limits, l[limitName]);
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

  wgpu_adapter_request_device_async__deps: ['$wgpuStore', 'wgpuFeatures', 'wgpu32BitLimitNames', 'wgpu64BitLimitNames', '$wgpuReadI53FromU64HeapIdx'],
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

    {{{ wassert('_wgpuFeatures.length == 10'); }}}
    {{{ wassert('_wgpuFeatures.length <= 30'); }}} // We can only do up to 30 distinct feature bits here with the current code.

    for(let i = 0; i < 10/*_wgpuFeatures.length*/; ++i) {
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
    ).then(cb).catch(()=>{cb(/*intentionally omit arg to pass undefined*/)});
  },

#if ASYNCIFY
  wgpu_adapter_request_device_sync__deps: ['$wgpuStore', 'wgpuFeatures', 'wgpu32BitLimitNames', 'wgpu64BitLimitNames', '$wgpuReadI53FromU64HeapIdx', '$wgpuAsync'],
  wgpu_adapter_request_device_sync: function(adapter, descriptor) {
    {{{ wdebuglog('`wgpu_adapter_request_device_sync(adapter: ${adapter})`'); }}}
    {{{ wassert('adapter != 0'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter] instanceof GPUAdapter'); }}}
    {{{ wassert('descriptor != 0'); }}}
    {{{ wassert('descriptor % 4 == 0'); }}} // Must be aligned at uint32_t boundary

    {{{ ptrToIdx('descriptor', 2); }}}
    let requiredFeatures = [], requiredLimits = {}, v = HEAPU32[descriptor], defaultQueueLabel;
    descriptor += 2;

    {{{ wassert('_wgpuFeatures.length == 10'); }}}
    {{{ wassert('_wgpuFeatures.length <= 30'); }}} // We can only do up to 30 distinct feature bits here with the current code.

    for(let i = 0; i < 10/*_wgpuFeatures.length*/; ++i) {
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

      return wgpuStore(device);
    }

    defaultQueueLabel = HEAPU32[descriptor];
    return wgpuAsync(wgpu[adapter]['requestDevice'](
        debugDir(
          {
            'requiredFeatures': requiredFeatures,
            'requiredLimits': requiredLimits,
            'defaultQueue': defaultQueueLabel ? { 'label': UTF8ToString(defaultQueueLabel) } : void 0
          },
          'GPUAdapter.requestDevice() with desc'
        )
      ).then(cb));
  },
#endif

  // A "_simple" variant of wgpu_adapter_request_device_async() that does
  // not take in any descriptor params, for building tiny code with default
  // args and creating readable test cases etc.
  wgpu_adapter_request_device_async_simple__deps: ['$wgpuStore'],
  wgpu_adapter_request_device_async_simple: function(adapter, deviceCallback) {
    {{{ wdebuglog('`wgpu_adapter_request_device_sync_simple(adapter: ${adapter}, deviceCallback=${deviceCallback})`'); }}}
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
  wgpu_adapter_request_device_sync_simple__deps: ['$wgpuStore', '$wgpuAsync'],
  wgpu_adapter_request_device_sync_simple: function(adapter) {
    {{{ wdebuglog('`wgpu_adapter_request_device_sync_simple(adapter: ${adapter})`'); }}}
    {{{ wassert('adapter != 0'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter] instanceof GPUAdapter'); }}}
    return wgpuAsync(wgpu[adapter]['requestDevice']().then(device => {
      device.derivedObjects = [];
      wgpuStore(device['queue']);
      return wgpuStore(device);
    }));
  },
#endif

  wgpu_adapter_request_adapter_info_async__docs: '/** @suppress{checkTypes} */', // This function intentionally calls cb() without args.
  wgpu_adapter_request_adapter_info_async: function(adapter, unmaskHints, callback, userData) {
    {{{ wdebuglog('`wgpu_adapter_request_adapter_info_async(adapter: ${adapter}, unmaskHints: ${unmaskHints}, callback: ${callback}, userData: ${userData})`'); }}}
    {{{ wassert('adapter != 0'); }}}
    {{{ wassert('wgpu[adapter]'); }}}
    {{{ wassert('wgpu[adapter] instanceof GPUAdapter'); }}}
    {{{ wassert('callback != 0'); }}}
    {{{ wassert('unmaskHints != 0'); }}}

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

    let hints = [];
    {{{ ptrToIdx('unmaskHints', 2); }}}
    while(HEAPU32[unmaskHints]) {
      hints.push(UTF8ToString(HEAPU32[unmaskHints++]));
    }

    {{{ wdebuglog('`wgpu_adapter_request_adapter_info_async() requesting adapter info with hints [${hints.join(", ")}]`'); }}}
    return wgpu[adapter]['requestAdapterInfo'](hints).then(cb).catch(()=>{cb(/*intentionally omit arg to pass undefined*/)});
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
      hints = {},
      hintsIndex = {{{ shiftPtr('HEAPU32[index+1]', 2) }}},
      hint;
    {{{ wassert('numHints >= 0'); }}}
    while(numHints--) {
      hint = HEAPU32[hintsIndex+1];
      // hint == 0 (WGPU_AUTO_LAYOUT_MODE_NO_HINT) means no compilation hints are passed,
      // hint == 1 (WGPU_AUTO_LAYOUT_MODE_AUTO) means { layout: 'auto' } hint will be passed.
      // See https://github.com/gpuweb/gpuweb/pull/2876#issuecomment-1218341636
      hints[UTF8ToString(HEAPU32[hintsIndex])] = hint ? { 'layout': hint > 1 ? wgpu[hint] : GPUAutoLayoutMode } : null;
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
      'hints': wgpuReadShaderModuleCompilationHints(descriptor+1)
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

  wgpu_shader_module_get_compilation_info_async: function(shaderModule, callback, userData) {
    {{{ wdebuglog('`wgpu_shader_module_get_compilation_info_async(shaderModule=${shaderModule}, callback=${callback}, userData=${userData})`'); }}}
    {{{ wassert('shaderModule != 0'); }}}
    {{{ wassert('wgpu[shaderModule]'); }}}
    {{{ wassert('wgpu[shaderModule] instanceof GPUShaderModule'); }}}
    {{{ wassert('callback != 0'); }}}
    wgpu[shaderModule]['compilationInfo']().then(info => {
      {{{ wdebuglog('`shaderModule.compilationInfo() completed with info:`'); }}}
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
    {{{ wassert('offset >= 0'); }}}

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
    {{{ wassert('size >= 0'); }}}
    {{{ wassert('dst || size == 0'); }}}
    {{{ wassert('subOffset >= 0'); }}}

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
    {{{ wassert('size >= 0'); }}}
    {{{ wassert('src || size == 0'); }}}
    {{{ wassert('subOffset >= 0'); }}}

    // Here 'buffer' refers to the global Wasm memory buffer.
    // N.b. generates garbage.
    new Uint8Array(wgpu[gpuBuffer].mappedRanges[startOffset]).set(new Uint8Array(buffer, src, size), subOffset);
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
        'entryPoint': UTF8ToString(HEAPU32[vertexIdx+1]),
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
        'entryPoint': UTF8ToString(HEAPU32[fragmentIdx+1]),
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

  wgpu_device_create_render_pipeline_async__deps: ['$wgpuReadRenderPipelineDescriptor', '$wgpuStoreAndSetParent'],
  wgpu_device_create_render_pipeline_async__docs: '/** @suppress{checkTypes} */', // This function intentionally calls cb() without args.
  wgpu_device_create_render_pipeline_async: function(device, descriptor, callback, userData) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_device_create_render_pipeline_async(device=${device}, descriptor=${descriptor}, callback=${callback}, userData=${userData})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('descriptor'); }}}
    {{{ wassert('callback'); }}}
    let dev = wgpu[device];

    let cb = (pipeline) => {
      {{{ wdebuglog('`createRenderPipelineAsync completed with pipeline:`'); }}}
      {{{ wdebugdir('pipeline'); }}}
      {{{ makeDynCall('viii', 'callback') }}}(device, wgpuStoreAndSetParent(pipeline, dev), userData);
    };

    dev['createRenderPipelineAsync'](
      debugDir(wgpuReadRenderPipelineDescriptor(descriptor), 'GPUDevice.createRenderPipelineAsync() with desc')
    ).then(cb).catch(()=>{cb(/*intentionally omit arg to pass undefined*/)});
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
  wgpu_device_create_render_bundle_encoder: function(device, descriptor) { // TODO: this function is untested. Write a test case
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
  wgpu_device_create_query_set: function(device, descriptor) { // TODO: this function is untested. Write a test case
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

  wgpu_buffer_map_async: function(buffer, callback, userData, mode, offset, size) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_buffer_map_async(buffer=${buffer}, callback=${callback}, userData=${userData}, mode=${mode}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('buffer != 0'); }}}
    {{{ wassert('wgpu[buffer]'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}
    buffer = wgpu[buffer];
    // Awkward polymorphism: must call different version of mapAsync to use default 'size' value.
    (size < 0 ? buffer['mapAsync'](mode, offset) : buffer['mapAsync'](mode, offset, size)).then(() => {
      {{{ makeDynCall('viiidd', 'callback') }}}(buffer, userData, mode, offset, size);
    });
  },

#if ASYNCIFY
  wgpu_buffer_map_sync__deps: ['$wgpuAsync'],
  wgpu_buffer_map_sync: function(buffer, mode, offset, size) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_buffer_map_sync(buffer=${buffer}, mode=${mode}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('buffer != 0'); }}}
    {{{ wassert('wgpu[buffer]'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}
    buffer = wgpu[buffer];
    // Awkward polymorphism: must call different version of mapAsync to use default 'size' value.
    return wgpuAsync(size < 0 ? buffer['mapAsync'](mode, offset) : buffer['mapAsync'](mode, offset, size));
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

  $wgpuReadBindGroupLayoutDescriptor__deps: ['$GPUBufferBindingTypes', '$wgpuReadI53FromU64HeapIdx', '$GPUSamplerBindingTypes', '$GPUTextureSampleTypes', '$GPUTextureViewDimensions', '$GPUTextureAndVertexFormats'],
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
          'viewDimension': GPUTextureViewDimensions[HEAPU32[entries+1]]
        };
      } else if (type == 4/*WGPU_BIND_GROUP_LAYOUT_TYPE_STORAGE_TEXTURE*/) {
        entry['storageTexture'] = {
          'access': [, 'write-only'][HEAPU32[entries]],
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

  wgpu_external_texture_is_expired: function(externalTexture) {
    {{{ wdebuglog('`wgpu_external_texture_is_expired(externalTexture=${externalTexture})`'); }}}
    {{{ wassert('externalTexture != 0'); }}}
    {{{ wassert('wgpu[externalTexture]'); }}}
    {{{ wassert('wgpu[externalTexture] instanceof GPUExternalTexture'); }}}
    return wgpu[externalTexture]['expired'];
  },

  wgpu_device_create_bind_group_layout__deps: ['$wgpuStoreAndSetParent', '$wgpuReadBindGroupLayoutDescriptor'],
  wgpu_device_create_bind_group_layout: function(device, entries, numEntries) { // TODO: this function is untested. Write a test case
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
  wgpu_device_create_pipeline_layout: function(device, layouts, numLayouts) { // TODO: this function is untested. Write a test case
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
    {{{ wassert('layout != 0'); }}}
    {{{ wassert('wgpu[layout]'); }}} // Must be a valid BindGroupLayout
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
  wgpu_device_create_compute_pipeline: function(device, computeModule, entryPoint, layout, constants, numConstants) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_device_create_compute_pipeline(device=${device}, computeModule=${computeModule}, entryPoint=${entryPoint}, layout=${layout}, constants=${constants}, numConstants=${numConstants})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('computeModule != 0'); }}}
    {{{ wassert('wgpu[computeModule]'); }}}
    {{{ wassert('wgpu[computeModule] instanceof GPUShaderModule'); }}}
    {{{ wassert('layout == 1/*"auto"*/ || wgpu[layout]'); }}}
    {{{ wassert('layout == 1/*"auto"*/ || wgpu[layout] instanceof GPUPipelineLayout'); }}}
    {{{ wassert('numConstants >= 0'); }}}
    {{{ wassert('numConstants == 0 || constants'); }}}
    {{{ wassert('entryPoint'); }}} // Must be a non-null C string pointer
    {{{ wassert('UTF8ToString(entryPoint).length > 0'); }}} // Must be a nonempty JS string
    device = wgpu[device];

    return wgpuStoreAndSetParent(
      device['createComputePipeline'](
        debugDir(
          {
            'layout': layout ? wgpu[layout] : GPUAutoLayoutMode,
            'compute': {
              'module': wgpu[computeModule],
              'entryPoint': UTF8ToString(entryPoint),
              'constants': wgpuReadConstants(constants, numConstants)
            }
          },
          'GPUDevice.createComputePipeline() with desc'
        )
      ),
      device
    );
  },

  wgpu_device_create_compute_pipeline_async__deps: ['$wgpuStoreAndSetParent', '$wgpuReadConstants'],
  wgpu_device_create_compute_pipeline_async__docs: '/** @suppress{checkTypes} */', // This function intentionally calls cb() without args.
  wgpu_device_create_compute_pipeline_async: function(device, computeModule, entryPoint, layout, constants, numConstants, callback, userDate) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_device_create_compute_pipeline_async(device=${device}, computeModule=${computeModule}, entryPoint=${entryPoint}, layout=${layout}, constants=${constants}, numConstants=${numConstants}, callback=${callback}, userData=${userData})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}
    {{{ wassert('computeModule != 0'); }}}
    {{{ wassert('wgpu[computeModule]'); }}}
    {{{ wassert('wgpu[computeModule] instanceof GPUShaderModule'); }}}
    {{{ wassert('layout == 1/*"auto"*/ || wgpu[layout]'); }}}
    {{{ wassert('layout == 1/*"auto"*/ || wgpu[layout] instanceof GPUPipelineLayout'); }}}
    {{{ wassert('numConstants >= 0'); }}}
    {{{ wassert('numConstants == 0 || constants'); }}}
    {{{ wassert('entryPoint'); }}} // Must be a non-null C string pointer
    {{{ wassert('UTF8ToString(entryPoint).length > 0'); }}} // Must be a nonempty JS string
    {{{ wassert('callback'); }}}
    let dev = wgpu[device];

    let cb = (pipeline) => {
      {{{ wdebuglog('`createComputePipelineAsync completed with pipeline:`'); }}}
      {{{ wdebugdir('pipeline'); }}}
      {{{ makeDynCall('viii', 'callback') }}}(device, wgpuStoreAndSetParent(pipeline, dev), userData);
    };

    dev['createComputePipelineAsync'](
      debugDir(
        {
          'layout': layout ? wgpu[layout] : GPUAutoLayoutMode,
          'compute': {
            'module': wgpu[computeModule],
            'entryPoint': UTF8ToString(entryPoint),
            'constants': wgpuReadConstants(constants, numConstants)
          }
        },
        'GPUDevice.createComputePipelineAsync() with desc'
      )
    ).then(cb).catch(()=>{cb(/*intentionally omit arg to pass undefined*/)});
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

  $wgpuReadTimestampWrites__deps: ['$GPUComputePassTimestampLocations'],
  $wgpuReadTimestampWrites: function(timestampWritesIndex) {
    let numTimestampWrites = timestampWritesIndex && HEAP32[timestampWritesIndex++];
    {{{ wassert('numTimestampWrites >= 0, numTimestampWrites'); }}} // Sanity check against corrupted memory
    {{{ wassert('numTimestampWrites <= 1024, numTimestampWrites'); }}} // Sanity check against corrupted memory (this limit is arbitrary to catch likely corrupted data)
    let timestampWrites = [];
    {{{ wassert('HEAPU32[timestampWritesIndex] % 4 == 0'); }}} // Pointer to timestamp write struct must be aligned to uint32_t.
    let idx = {{{ shiftPtr('HEAPU32[timestampWritesIndex]', 2) }}};
    {{{ wassert('numTimestampWrites == 0 || idx != 0'); }}} // If numTimestampWrites > 0, then we must have a nonzero index.
    while(numTimestampWrites--) {
      timestampWrites.push({
        'querySet': wgpu[HEAPU32[idx]],
        'queryIndex': HEAPU32[idx+1],
        'location': GPUComputePassTimestampLocations[HEAPU32[idx+2]]
      });
      idx += 3;
    }
    return timestampWrites;
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
      colorAttachmentsIdxDbl = {{{ shiftPtr('colorAttachmentsIdx + 4', 1) }}}, // Alias the view for HEAPF64.
      depthStencilView = wgpu[HEAPU32[descriptor]];

    {{{ wassert('colorAttachmentsIdx % 2 == 0'); }}} // Must be aligned at double boundary
    {{{ wassert('depthStencilView || !HEAPU32[descriptor]'); }}}

    {{{ wassert('numColorAttachments >= 0'); }}}
    while(numColorAttachments--) {
      // If view is 0, then this attachment is to be sparse.
      colorAttachments.push(HEAPU32[colorAttachmentsIdx] ? {
        'view': wgpu[HEAPU32[colorAttachmentsIdx]],
        'resolveTarget': wgpu[HEAPU32[colorAttachmentsIdx+1]],
        'storeOp': GPUStoreOps[HEAPU32[colorAttachmentsIdx+2]],
        'loadOp': GPULoadOps[HEAPU32[colorAttachmentsIdx+3]],
        'clearValue': [HEAPF64[colorAttachmentsIdxDbl  ], HEAPF64[colorAttachmentsIdxDbl+1],
                       HEAPF64[colorAttachmentsIdxDbl+2], HEAPF64[colorAttachmentsIdxDbl+3]]
      } : null);

      colorAttachmentsIdx += 12;
      colorAttachmentsIdxDbl += 6;
    }

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
              // TODO: Add support for 'timestampWrites': [...] array,
              // Read 'maxDrawCount'. If set to zero, pass in undefined to use the default value
              // (likely 50 million, but omit it in case the spec might change in the future)
              'maxDrawCount': HEAPF64[descriptor+10>>1] || void 0,
              'timestampWrites': wgpuReadTimestampWrites(descriptor+11)

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
      colorAttachmentsIdxDbl = {{{ shiftPtr('colorAttachmentsIdx + 4', 1) }}}; // Alias the view for HEAPF64.

    {{{ wassert('colorAttachmentsIdx % 2 == 0'); }}} // Must be aligned at double boundary

    return wgpuStore(wgpu[commandEncoder]['beginRenderPass'](debugDir({
        'colorAttachments': [{
          'view': wgpu[HEAPU32[colorAttachmentsIdx]],
          'resolveTarget': wgpu[HEAPU32[colorAttachmentsIdx+1]],
          'storeOp': GPUStoreOps[HEAPU32[colorAttachmentsIdx+2]],
          'loadOp': GPULoadOps[HEAPU32[colorAttachmentsIdx+3]],
          'clearValue': [HEAPF64[colorAttachmentsIdxDbl  ], HEAPF64[colorAttachmentsIdxDbl+1],
                         HEAPF64[colorAttachmentsIdxDbl+2], HEAPF64[colorAttachmentsIdxDbl+3]]
        }]
      })));
  },

  wgpu_command_encoder_begin_compute_pass__deps: ['$wgpuReadTimestampWrites'],
  wgpu_command_encoder_begin_compute_pass: function(commandEncoder, descriptor) { // TODO: this function is untested. Write a test case
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

  wgpu_command_encoder_copy_buffer_to_buffer: function(commandEncoder, source, sourceOffset, destination, destinationOffset, size) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_copy_buffer_to_buffer(commandEncoder=${commandEncoder}, source=${source}, sourceOffset=${sourceOffset}, destination=${destination}, destinationOffset=${destinationOffset}, size=${size})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('wgpu[source] instanceof GPUBuffer'); }}}
    {{{ wassert('wgpu[destination] instanceof GPUBuffer'); }}}
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
  wgpu_command_encoder_copy_buffer_to_texture: function(commandEncoder, source, destination, copyWidth, copyHeight, copyDepthOrArrayLayers) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_copy_buffer_to_texture(commandEncoder=${commandEncoder}, source=${source}, destination=${destination}, copyWidth=${copyWidth}, copyHeight=${copyHeight}, copyDepthOrArrayLayers=${copyDepthOrArrayLayers})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('source'); }}}
    {{{ wassert('destination'); }}}
    wgpu[commandEncoder]['copyBufferToTexture'](wgpuReadGpuImageCopyBuffer(source), wgpuReadGpuImageCopyTexture(destination), [copyWidth, copyHeight, copyDepthOrArrayLayers]);
  },

  wgpu_command_encoder_copy_texture_to_buffer__deps: ['$wgpuReadGpuImageCopyTexture', '$wgpuReadGpuImageCopyBuffer'],
  wgpu_command_encoder_copy_texture_to_buffer: function(commandEncoder, source, destination, copyWidth, copyHeight, copyDepthOrArrayLayers) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_copy_texture_to_buffer(commandEncoder=${commandEncoder}, source=${source}, destination=${destination}, copyWidth=${copyWidth}, copyHeight=${copyHeight}, copyDepthOrArrayLayers=${copyDepthOrArrayLayers})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('source'); }}}
    {{{ wassert('destination'); }}}
    wgpu[commandEncoder]['copyTextureToBuffer'](wgpuReadGpuImageCopyTexture(source), wgpuReadGpuImageCopyBuffer(destination), [copyWidth, copyHeight, copyDepthOrArrayLayers]);
  },

  wgpu_command_encoder_copy_texture_to_texture__deps: ['$wgpuReadGpuImageCopyTexture'],
  wgpu_command_encoder_copy_texture_to_texture: function(commandEncoder, source, destination, copyWidth, copyHeight, copyDepthOrArrayLayers) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_copy_texture_to_texture(commandEncoder=${commandEncoder}, source=${source}, destination=${destination}, copyWidth=${copyWidth}, copyHeight=${copyHeight}, copyDepthOrArrayLayers=${copyDepthOrArrayLayers})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('source'); }}}
    {{{ wassert('destination'); }}}
    wgpu[commandEncoder]['copyTextureToTexture'](wgpuReadGpuImageCopyTexture(source), wgpuReadGpuImageCopyTexture(destination), [copyWidth, copyHeight, copyDepthOrArrayLayers]);
  },

  wgpu_command_encoder_clear_buffer: function(commandEncoder, buffer, offset, size) {  // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_clear_buffer(commandEncoder=${commandEncoder}, buffer=${buffer}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('wgpu[buffer]'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}
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

  wgpu_encoder_push_debug_group: function(encoder, groupLabel) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_push_debug_group(encoder=${encoder}, groupLabel=${groupLabel})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUCommandEncoder || wgpu[encoder] instanceof GPUComputePassEncoder || wgpu[encoder] instanceof GPURenderPassEncoder || wgpu[encoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('wgpu[groupLabel]'); }}}
    wgpu[encoder]['pushDebugGroup'](UTF8ToString(groupLabel));
  },

  wgpu_encoder_pop_debug_group: function(encoder) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_pop_debug_group(encoder=${encoder})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUCommandEncoder || wgpu[encoder] instanceof GPUComputePassEncoder || wgpu[encoder] instanceof GPURenderPassEncoder || wgpu[encoder] instanceof GPURenderBundleEncoder'); }}}
    wgpu[encoder]['popDebugGroup']();
  },

  wgpu_encoder_insert_debug_marker: function(encoder, markerLabel) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_insert_debug_marker(encoder=${encoder}, markerLabel=${markerLabel})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUCommandEncoder || wgpu[encoder] instanceof GPUComputePassEncoder || wgpu[encoder] instanceof GPURenderPassEncoder || wgpu[encoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('wgpu[markerLabel]'); }}}
    wgpu[encoder]['insertDebugMarker'](UTF8ToString(markerLabel));
  },

  wgpu_command_encoder_write_timestamp: function(commandEncoder, querySet, queryIndex) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_write_timestamp(commandEncoder=${commandEncoder}, querySet=${querySet}, queryIndex=${queryIndex})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('wgpu[querySet] instanceof GPUQuerySet'); }}}
    wgpu[commandEncoder]['writeTimestamp'](wgpu[querySet], queryIndex);
  },

  wgpu_command_encoder_resolve_query_set: function(commandEncoder, querySet, firstQuery, queryCount, destination, destinationOffset) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_command_encoder_resolve_query_set(commandEncoder=${commandEncoder}, querySet=${querySet}, firstQuery=${firstQuery}, queryCount=${queryCount}, destination=${destination}, destinationOffset=${destinationOffset})`'); }}}
    {{{ wassert('commandEncoder != 0'); }}}
    {{{ wassert('wgpu[commandEncoder]'); }}}
    {{{ wassert('wgpu[commandEncoder] instanceof GPUCommandEncoder'); }}}
    {{{ wassert('wgpu[querySet] instanceof GPUQuerySet'); }}}
    {{{ wassert('wgpu[destination] instanceof GPUBuffer'); }}}
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
  wgpu_render_commands_mixin_set_index_buffer: function(passEncoder, buffer, indexFormat, offset, size) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_commands_mixin_set_index_buffer(passEncoder=${passEncoder}, buffer=${buffer}, indexFormat=${indexFormat}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('passEncoder != 0'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder] instanceof GPURenderPassEncoder || wgpu[passEncoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}

    // Awkward API polymorphism: must omit size parameter altogether (instead of specifying e.g. -1 for whole buffer)
    size < 0 ? wgpu[passEncoder]['setIndexBuffer'](wgpu[buffer], GPUIndexFormats[indexFormat], offset)
      : wgpu[passEncoder]['setIndexBuffer'](wgpu[buffer], GPUIndexFormats[indexFormat], offset, size);
  },

  wgpu_render_commands_mixin_set_vertex_buffer: function(passEncoder, slot, buffer, offset, size) {
    {{{ wdebuglog('`wgpu_render_commands_mixin_set_vertex_buffer(passEncoder=${passEncoder}, slot=${slot}, buffer=${buffer}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('passEncoder != 0'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder] instanceof GPURenderPassEncoder || wgpu[passEncoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}

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

  wgpu_render_commands_mixin_draw_indexed: function(passEncoder, indexCount, instanceCount, firstVertex, baseVertex, firstInstance) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_commands_mixin_draw_indexed(passEncoder=${passEncoder}, indexCount=${indexCount}, instanceCount=${instanceCount}, firstVertex=${firstVertex}, baseVertex=${baseVertex}, firstInstance=${firstInstance})`'); }}}
    {{{ wassert('passEncoder != 0'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder] instanceof GPURenderPassEncoder || wgpu[passEncoder] instanceof GPURenderBundleEncoder'); }}}

    wgpu[passEncoder]['drawIndexed'](indexCount, instanceCount, firstVertex, baseVertex, firstInstance);
  },

  wgpu_render_commands_mixin_draw_indirect: function(passEncoder, indirectBuffer, indirectOffset) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_commands_mixin_draw_indirect(passEncoder=${passEncoder}, indirectBuffer=${indirectBuffer}, indirectOffset=${indirectOffset})`'); }}}
    {{{ wassert('passEncoder != 0'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder] instanceof GPURenderPassEncoder || wgpu[passEncoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('wgpu[indirectBuffer] instanceof GPUBuffer'); }}}

    wgpu[passEncoder]['drawIndirect'](wgpu[indirectBuffer], indirectOffset);
  },

  wgpu_render_commands_mixin_draw_indexed_indirect: function(passEncoder, indirectBuffer, indirectOffset) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_commands_mixin_draw_indexed_indirect(passEncoder=${passEncoder}, indirectBuffer=${indirectBuffer}, indirectOffset=${indirectOffset})`'); }}}
    {{{ wassert('passEncoder != 0'); }}}
    {{{ wassert('wgpu[passEncoder]'); }}}
    {{{ wassert('wgpu[passEncoder] instanceof GPURenderPassEncoder || wgpu[passEncoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('wgpu[indirectBuffer] instanceof GPUBuffer'); }}}

    wgpu[passEncoder]['drawIndexedIndirect'](wgpu[indirectBuffer], indirectOffset);
  },

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

  wgpu_encoder_set_bind_group: function(encoder, index, bindGroup, dynamicOffsets, numDynamicOffsets) {
    {{{ wdebuglog('`wgpu_encoder_set_bind_group(encoder=${encoder}, index=${index}, bindGroup=${bindGroup}, dynamicOffsets=${dynamicOffsets}, numDynamicOffsets=${numDynamicOffsets})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUComputePassEncoder || wgpu[encoder] instanceof GPURenderPassEncoder || wgpu[encoder] instanceof GPURenderBundleEncoder'); }}}
    {{{ wassert('bindGroup != 0'); }}}
    {{{ wassert('wgpu[bindGroup]'); }}}
    {{{ wassert('wgpu[bindGroup] instanceof GPUBindGroup'); }}}
    {{{ wassert('dynamicOffsets != 0 || numDynamicOffsets == 0'); }}}
    {{{ wassert('dynamicOffsets % 4 == 0'); }}}
    wgpu[encoder]['setBindGroup'](index, wgpu[bindGroup], HEAPU32, {{{ shiftPtr('dynamicOffsets', 2) }}}, numDynamicOffsets);
  },

  wgpu_compute_pass_encoder_dispatch_workgroups: function(encoder, workgroupCountX, workgroupCountY, workgroupCountZ) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_compute_pass_encoder_dispatch_workgroups(encoder=${encoder}, workgroupCountX=${workgroupCountX}, workgroupCountY=${workgroupCountY}, workgroupCountZ=${workgroupCountZ})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUComputePassEncoder'); }}}
    wgpu[encoder]['dispatchWorkgroups'](workgroupCountX, workgroupCountY, workgroupCountZ);
  },

  wgpu_compute_pass_encoder_dispatch_workgroups_indirect: function(encoder, indirectBuffer, indirectOffset) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_compute_pass_encoder_dispatch_workgroups_indirect(encoder=${encoder}, indirectBuffer=${indirectBuffer}, indirectOffset=${indirectOffset})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPUComputePassEncoder'); }}}
    {{{ wassert('indirectBuffer != 0'); }}}
    {{{ wassert('wgpu[indirectBuffer]'); }}}
    {{{ wassert('wgpu[indirectBuffer] instanceof GPUBuffer'); }}}
    wgpu[encoder]['dispatchWorkgroupsIndirect'](wgpu[indirectBuffer], indirectOffset);
  },

  wgpu_render_pass_encoder_set_viewport: function(encoder, x, y, width, height, minDepth, maxDepth) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_viewport(encoder=${encoder}, x=${x}, y=${y}, width=${width}, height=${height}, minDepth=${minDepth}, maxDepth=${maxDepth})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['setViewport'](x, y, width, height, minDepth, maxDepth);
  },

  wgpu_render_pass_encoder_set_scissor_rect: function(encoder, x, y, width, height) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_scissor_rect(encoder=${encoder}, x=${x}, y=${y}, width=${width}, height=${height})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['setScissorRect'](x, y, width, height);
  },

  wgpu_render_pass_encoder_set_blend_constant: function(encoder, r, g, b, a) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_blend_constant(encoder=${encoder}, r=${r}, g=${g}, b=${b}, a=${a})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['setBlendConstant'](r, g, b, a);
  },

  wgpu_render_pass_encoder_set_stencil_reference: function(encoder, stencilValue) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_set_stencil_reference(stencilValue=${stencilValue})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['setStencilReference'](stencilValue);
  },

  wgpu_render_pass_encoder_begin_occlusion_query: function(encoder, queryIndex) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_begin_occlusion_query(queryIndex=${queryIndex})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['beginOcclusionQuery'](queryIndex);
  },

  wgpu_render_pass_encoder_end_occlusion_query: function(encoder) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_end_occlusion_query()`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['endOcclusionQuery']();
  },

  wgpu_render_pass_encoder_execute_bundles__deps: ['$wgpuReadArrayOfWgpuObjects'],
  wgpu_render_pass_encoder_execute_bundles: function(encoder, bundles, numBundles) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_render_pass_encoder_execute_bundles(encoder=${encoder}, bundles=${bundles}, numBundles=${numBundles})`'); }}}
    {{{ wassert('encoder != 0'); }}}
    {{{ wassert('wgpu[encoder]'); }}}
    {{{ wassert('wgpu[encoder] instanceof GPURenderPassEncoder'); }}}
    wgpu[encoder]['executeBundles'](wgpuReadArrayOfWgpuObjects(bundles, numBundles));
  },

  wgpu_queue_submit_one: function(queue, commandBuffer) {
    {{{ wdebuglog('`wgpu_queue_submit(queue=${queue}, commandBuffer=${commandBuffer})`'); }}}
    {{{ wassert('queue != 0'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue] instanceof GPUQueue'); }}}
    {{{ wassert('commandBuffer != 0'); }}}
    {{{ wassert('wgpu[commandBuffer]'); }}}
    {{{ wassert('wgpu[commandBuffer] instanceof GPUCommandBuffer'); }}}
    wgpu[queue]['submit']([wgpu[commandBuffer]]);
  },

  wgpu_queue_submit_one_and_destroy__deps: ['wgpu_queue_submit_one', 'wgpu_object_destroy'],
  wgpu_queue_submit_one_and_destroy: function(queue, commandBuffer) {
    _wgpu_queue_submit_one(queue, commandBuffer);
    _wgpu_object_destroy(commandBuffer);
  },

  wgpu_queue_submit_multiple__deps: ['$wgpuReadArrayOfWgpuObjects'],
  wgpu_queue_submit_multiple: function(queue, commandBuffers, numCommandBuffers) {
    {{{ wdebuglog('`wgpu_queue_submit_multiple(queue=${queue}, commandBuffers=${commandBuffers}, numCommandBuffers=${numCommandBuffers})`'); }}}
    {{{ wassert('queue != 0'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue] instanceof GPUQueue'); }}}
    wgpu[queue]['submit'](wgpuReadArrayOfWgpuObjects(commandBuffers, numCommandBuffers));
  },

  wgpu_queue_submit_multiple_and_destroy__deps: ['$wgpuReadArrayOfWgpuObjects'],
  wgpu_queue_submit_multiple_and_destroy: function(queue, commandBuffers, numCommandBuffers) {
    {{{ wdebuglog('`wgpu_queue_submit_multiple_and_destroy(queue=${queue}, commandBuffers=${commandBuffers}, numCommandBuffers=${numCommandBuffers})`'); }}}
    {{{ wassert('queue != 0'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue] instanceof GPUQueue'); }}}
    wgpu[queue]['submit'](wgpuReadArrayOfWgpuObjects(commandBuffers, numCommandBuffers));

    buffers.forEach(_wgpu_object_destroy);
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

  wgpu_queue_write_buffer: function(queue, buffer, bufferOffset, data, size) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_queue_write_buffer(queue=${queue}, buffer=${buffer}, bufferOffset=${bufferOffset}, data=${data}, size=${size})`'); }}}
    {{{ wassert('queue != 0'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue] instanceof GPUQueue'); }}}
    {{{ wassert('buffer != 0'); }}}
    {{{ wassert('wgpu[buffer]'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}
    wgpu[queue]['writeBuffer'](wgpu[buffer], bufferOffset, HEAPU8, data, size);
  },

  wgpu_queue_write_texture__deps: ['$wgpuReadGpuImageCopyTexture'],
  wgpu_queue_write_texture: function(queue, destination, data, bytesPerBlockRow, blockRowsPerImage, writeWidth, writeHeight, writeDepthOrArrayLayers) { // TODO: this function is untested. Write a test case
    {{{ wdebuglog('`wgpu_queue_write_texture(queue=${queue}, destination=${destination}, data=${data}, bytesPerBlockRow=${bytesPerBlockRow}, blockRowsPerImage=${blockRowsPerImage}, writeWidth=${writeWidth}, writeHeight=${writeHeight}, writeDepthOrArrayLayers=${writeDepthOrArrayLayers})`'); }}}
    {{{ wassert('queue != 0'); }}}
    {{{ wassert('wgpu[queue]'); }}}
    {{{ wassert('wgpu[queue] instanceof GPUQueue'); }}}
    {{{ wassert('destination'); }}}
    wgpu[queue]['writeTexture'](wgpuReadGpuImageCopyTexture(destination), HEAPU8, { 'offset': data, 'bytesPerRow': bytesPerBlockRow, 'rowsPerImage': blockRowsPerImage }, [writeWidth, writeHeight, writeDepthOrArrayLayers]);
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
    }).then(dispatchCallback).catch(dispatchCallback);
  }
});
