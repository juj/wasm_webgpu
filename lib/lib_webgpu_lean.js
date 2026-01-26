mergeInto(LibraryManager.library, {
  $wgpu_passEncoder: '0',
  $wgpu_queue: '0',
  $wgpu_commandBufferArray: '[0]',
  $wgpu_command_buffer: '0',
  $wgpu_encoder: '0',

  wg_get_queue__deps: ['$wgpu_queue'],
  wg_get_queue: function(gpuDevice) {
    wgpu_queue = wgpu[gpuDevice]['queue'];
  },

  wg_set_index_buffer__deps: ['$GPUIndexFormats', '$wgpu_passEncoder'],
  wg_set_index_buffer: function(buffer, indexFormat, offset) {
    {{{ wdebuglog('`wg_set_index_buffer(buffer=${buffer}, indexFormat=${indexFormat}, offset=${offset})`'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(offset)'); }}}
    {{{ wassert('offset >= 0'); }}}

    wgpu_passEncoder['setIndexBuffer'](wgpu[buffer], GPUIndexFormats[indexFormat], offset);
  },

  wg_set_index_buffer_sz__deps: ['$GPUIndexFormats', '$wgpu_passEncoder'],
  wg_set_index_buffer_sz: function(buffer, indexFormat, offset, size) {
    {{{ wdebuglog('`wg_set_index_buffer_sz(buffer=${buffer}, indexFormat=${indexFormat}, offset=${offset}, size=${size})`'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(offset)'); }}}
    {{{ wassert('offset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(size)'); }}}
    {{{ wassert('size >= -1'); }}}

    wgpu_passEncoder['setIndexBuffer'](wgpu[buffer], GPUIndexFormats[indexFormat], offset, size);
  },

  wg_set_vertex_buffer__deps: ['$wgpu_passEncoder'],
  wg_set_vertex_buffer: function(slot, buffer, offset) {
    {{{ wdebuglog('`wg_set_vertex_buffer(slot=${slot}, buffer=${buffer}, offset=${offset})`'); }}}
    // N.b. buffer may be null here, in which case the existing buffer is intended to be unbound.
    {{{ wassert('buffer == 0 || wgpu[buffer]'); }}}
    {{{ wassert('buffer == 0 || wgpu[buffer] instanceof GPUBuffer'); }}}
    {{{ wassert('buffer != 0 || offset == 0'); }}}
    {{{ wassert('Number.isSafeInteger(offset)'); }}}
    {{{ wassert('offset >= 0'); }}}

    wgpu_passEncoder['setVertexBuffer'](slot, wgpu[buffer], offset);
  },

  wg_set_vertex_buffer_sz__deps: ['$wgpu_passEncoder'],
  wg_set_vertex_buffer_sz: function(slot, buffer, offset, size) {
    {{{ wdebuglog('`wg_set_vertex_buffer_sz(slot=${slot}, buffer=${buffer}, offset=${offset}, size=${size})`'); }}}
    // N.b. buffer may be null here, in which case the existing buffer is intended to be unbound.
    {{{ wassert('buffer == 0 || wgpu[buffer]'); }}}
    {{{ wassert('buffer == 0 || wgpu[buffer] instanceof GPUBuffer'); }}}
    {{{ wassert('buffer != 0 || offset == 0'); }}}
    {{{ wassert('buffer != 0 || size <= 0'); }}}
    {{{ wassert('Number.isSafeInteger(offset)'); }}}
    {{{ wassert('offset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(size)'); }}}
    {{{ wassert('size >= -1'); }}}

    wgpu_passEncoder['setVertexBuffer'](slot, wgpu[buffer], offset, size);
  },

  wg_draw: function(vertexCount, instanceCount, firstVertex, firstInstance) {
    {{{ wdebuglog('`wg_draw(vertexCount=${vertexCount}, instanceCount=${instanceCount}, firstVertex=${firstVertex}, firstInstance=${firstInstance})`'); }}}

    wgpu_passEncoder['draw'](vertexCount, instanceCount, firstVertex, firstInstance);
  },

  wg_draw_indexed__deps: ['$wgpu_passEncoder'],
  wg_draw_indexed: function(indexCount, instanceCount, firstIndex, baseVertex, firstInstance) {
    {{{ wdebuglog('`wg_draw_indexed(indexCount=${indexCount}, instanceCount=${instanceCount}, firstIndex=${firstIndex}, baseVertex=${baseVertex}, firstInstance=${firstInstance})`'); }}}

    wgpu_passEncoder['drawIndexed'](indexCount, instanceCount, firstIndex, baseVertex, firstInstance);
  },

  wg_write_buffer__deps: ['$wgpu_queue'],
  wg_write_buffer: function(buffer, bufferOffset, data, size) {
    {{{ wdebuglog('`wg_write_buffer(buffer=${buffer}, bufferOffset=${bufferOffset}, data=${Number(data)>>>0}, size=${size})`'); }}}
    {{{ wassert('buffer != 0'); }}}
    {{{ wassert('wgpu[buffer]'); }}}
    {{{ wassert('wgpu[buffer] instanceof GPUBuffer'); }}}
    wgpu_queue['writeBuffer'](wgpu[buffer], bufferOffset, HEAPU8, {{{ shiftPtr('data', 0) }}}, size);
  },

  wg_finish_and_submit__deps: ['$wgpu_queue', '$wgpu_commandBufferArray'],
  wg_finish_and_submit: function() {
    {{{ wdebuglog('`wg_finish_and_submit()`'); }}}
    wgpu_commandBufferArray[0] = wgpu_encoder['finish']();
    wgpu_queue['submit'](wgpu_commandBufferArray);
  },

  wg_set_pipeline__deps: ['$wgpu_passEncoder'],
  wg_set_pipeline: function(pipeline) {
    {{{ wdebuglog('`wg_set_pipeline(pipeline=${pipeline})`'); }}}
    {{{ wassert('wgpu[pipeline] instanceof GPURenderPipeline || wgpu[pipeline] instanceof GPUComputePipeline'); }}}
    wgpu_passEncoder['setPipeline'](wgpu[pipeline]);
  },

  wg_set_bind_group__deps: ['$wgpu_passEncoder'],
  wg_set_bind_group: function(index, /*nullable*/ bindGroup, dynamicOffsets, numDynamicOffsets) {
    {{{ wdebuglog('`wg_set_bind_group(index=${index}, bindGroup=${bindGroup}, dynamicOffsets=${dynamicOffsets}, numDynamicOffsets=${numDynamicOffsets})`'); }}}
    // N.b. bindGroup may be null here, in which case the existing bind group is intended to be unbound.
    {{{ wassert('bindGroup == 0 || wgpu[bindGroup]'); }}}
    {{{ wassert('bindGroup == 0 || wgpu[bindGroup] instanceof GPUBindGroup'); }}}
    {{{ wassert('dynamicOffsets != 0 || numDynamicOffsets == 0'); }}}
    wgpu_passEncoder['setBindGroup'](index, wgpu[bindGroup], HEAPU32, {{{ shiftPtr('dynamicOffsets', 2) }}}, numDynamicOffsets);
  },

  wg_dispatch_workgroups: function(workgroupCountX, workgroupCountY, workgroupCountZ) {
    {{{ wdebuglog('`wg_dispatch_workgroups(workgroupCountX=${workgroupCountX}, workgroupCountY=${workgroupCountY}, workgroupCountZ=${workgroupCountZ})`'); }}}
    wgpu_passEncoder['dispatchWorkgroups'](workgroupCountX, workgroupCountY, workgroupCountZ);
  },

  wg_end_pass__deps: ['$wgpu_passEncoder'],
  wg_end_pass: function() {
    {{{ wdebuglog('`wg_end_pass()`'); }}}

    wgpu_passEncoder['end']();
  },

  wg_copy_buffer_to_buffer: function(source, sourceOffset, destination, destinationOffset, size) {
    {{{ wdebuglog('`wg_copy_buffer_to_buffer(source=${source}, sourceOffset=${sourceOffset}, destination=${destination}, destinationOffset=${destinationOffset}, size=${size})`'); }}}
    {{{ wassert('wgpu[source] instanceof GPUBuffer'); }}}
    {{{ wassert('wgpu[destination] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(sourceOffset)'); }}}
    {{{ wassert('sourceOffset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(destinationOffset)'); }}}
    {{{ wassert('destinationOffset >= 0'); }}}
    {{{ wassert('Number.isSafeInteger(size) || size == Infinity'); }}}
    {{{ wassert('size >= 0'); }}}
    wgpu_encoder['copyBufferToBuffer'](wgpu[source], sourceOffset, wgpu[destination], destinationOffset, size < 1/0 ? size : void 0);
  },

  wg_resolve_query_set: function(querySet, firstQuery, queryCount, destination, destinationOffset) {
    {{{ wdebuglog('`wg_resolve_query_set(querySet=${querySet}, firstQuery=${firstQuery}, queryCount=${queryCount}, destination=${destination}, destinationOffset=${destinationOffset})`'); }}}
    {{{ wassert('wgpu[querySet] instanceof GPUQuerySet'); }}}
    {{{ wassert('wgpu[destination] instanceof GPUBuffer'); }}}
    {{{ wassert('Number.isSafeInteger(destinationOffset)'); }}}
    {{{ wassert('destinationOffset >= 0'); }}}
    wgpu_encoder['resolveQuerySet'](wgpu[querySet], firstQuery, queryCount, wgpu[destination], destinationOffset);
  },

  wg_create_command_encoder__deps: ['$wgpu_encoder'],
  wg_create_command_encoder: function(device) {
    {{{ wdebuglog('`wg_create_command_encoder(device=${device})`'); }}}
    {{{ wassert('device != 0'); }}}
    {{{ wassert('wgpu[device]'); }}}
    {{{ wassert('wgpu[device] instanceof GPUDevice'); }}}

    wgpu_encoder = wgpu[device]['createCommandEncoder']();
  },

  wg_begin_compute_pass__deps: ['$wgpuReadTimestampWrites', '$wgpu_encoder', '$wgpu_passEncoder'],
  wg_begin_compute_pass: function(descriptor) {
    {{{ wdebuglog('`wg_begin_compute_pass(descriptor=${descriptor})`'); }}}
    // descriptor may be a null pointer

    {{{ replacePtrToIdx('descriptor', 2); }}}

    let desc = descriptor ? {
      'timestampWrites': wgpuReadTimestampWrites(descriptor)
    } : void 0;

    {{{ wdebugdir('desc', '`GPUCommandEncoder.beginComputePass() with descriptor:`'); }}}
    wgpu_passEncoder = wgpu_encoder['beginComputePass'](desc);
  },

  $depth_stencil_attachment_desc: {},
  $render_pass_desc: {},
  $timestamp_writes_desc: {},
  $render_pass_color_attachments: [],
  $render_pass_color_attachments_desc: [],

  wg_begin_render_pass__deps: ['$GPULoadOps', '$GPUStoreOps', '$render_pass_desc', '$render_pass_color_attachments', '$render_pass_color_attachments_desc', '$depth_stencil_attachment_desc', '$timestamp_writes_desc'],
  wg_begin_render_pass__postset: 'for(var i = 0; i <= 16; ++i) { render_pass_color_attachments.push(new Array(i)); render_pass_color_attachments_desc.push({"clearValue":[0,0,0,0]}); }',
  wg_begin_render_pass: function(descriptor) {
    {{{ wdebuglog('`wg_begin_render_pass(descriptor=${descriptor})`'); }}}
    {{{ wassert('descriptor != 0'); }}}

    {{{ replacePtrToIdx('descriptor', 2); }}}

    let numColorAttachments = HEAP32[descriptor+4],
      colorAttachmentsIdx = {{{ readIdx32('descriptor+2') }}},
      colorAttachmentsIdxDbl = {{{ shiftIndex('colorAttachmentsIdx + 6', 1) }}}, // Alias the view for HEAPF64.
      maxDrawCount = HEAPF64[{{{ shiftIndex('descriptor', 1) }}}],
      depthStencilAttachmentIdx = descriptor+5,
      timestampWritesIdx = descriptor+15;

    {{{ wassert('Number.isSafeInteger(maxDrawCount)'); }}} // 'maxDrawCount' is a double_int53_t
    {{{ wassert('maxDrawCount >= 0'); }}}

    {{{ wassert('colorAttachmentsIdx % 2 == 0'); }}} // Must be aligned at double boundary

    {{{ wassert('numColorAttachments >= 0'); }}}

    var attachments = render_pass_desc['colorAttachments'] = render_pass_color_attachments[numColorAttachments];

    for(var ai = 0; ai < numColorAttachments; ++ai) {
      // If view is 0, then this attachment is to be sparse.
      if (HEAPU32[colorAttachmentsIdx]) {
        attachments[ai] = render_pass_color_attachments_desc[ai];
        attachments[ai]['view'] = wgpu[HEAPU32[colorAttachmentsIdx]];
        attachments[ai]['depthSlice'] = HEAP32[colorAttachmentsIdx+1] < 0 ? void 0 : HEAP32[colorAttachmentsIdx+1];
        attachments[ai]['resolveTarget'] = wgpu[HEAPU32[colorAttachmentsIdx+2]];
        attachments[ai]['storeOp'] = GPUStoreOps[HEAPU32[colorAttachmentsIdx+3]];
        attachments[ai]['loadOp'] = GPULoadOps[HEAPU32[colorAttachmentsIdx+4]];
        attachments[ai]['clearValue'][0] = HEAPF64[colorAttachmentsIdxDbl  ];
        attachments[ai]['clearValue'][1] = HEAPF64[colorAttachmentsIdxDbl+1];
        attachments[ai]['clearValue'][2] = HEAPF64[colorAttachmentsIdxDbl+2];
        attachments[ai]['clearValue'][3] = HEAPF64[colorAttachmentsIdxDbl+3];
      } else {
        attachments[ai] = null;
      }

      colorAttachmentsIdx += 14; // sizeof(WGpuRenderPassColorAttachment)
      colorAttachmentsIdxDbl += 7; // sizeof(WGpuRenderPassColorAttachment)/2
    }

    render_pass_desc['depthStencilAttachment'] = render_pass_desc['timestampWrites'] = void 0;
    if (HEAPU32[depthStencilAttachmentIdx]) {
      depth_stencil_attachment_desc['view'] = wgpu[HEAPU32[depthStencilAttachmentIdx]];
      depth_stencil_attachment_desc['depthLoadOp'] = GPULoadOps[HEAPU32[depthStencilAttachmentIdx+1]];
      depth_stencil_attachment_desc['depthClearValue'] = HEAPF32[depthStencilAttachmentIdx+2];
      depth_stencil_attachment_desc['depthStoreOp'] = GPUStoreOps[HEAPU32[depthStencilAttachmentIdx+3]];
      depth_stencil_attachment_desc['depthReadOnly'] = !!HEAPU32[depthStencilAttachmentIdx+4];
      depth_stencil_attachment_desc['stencilLoadOp'] = GPULoadOps[HEAPU32[depthStencilAttachmentIdx+5]];
      depth_stencil_attachment_desc['stencilClearValue'] = HEAPU32[depthStencilAttachmentIdx+6];
      depth_stencil_attachment_desc['stencilStoreOp'] = GPUStoreOps[HEAPU32[depthStencilAttachmentIdx+7]];
      depth_stencil_attachment_desc['stencilReadOnly'] = !!HEAPU32[depthStencilAttachmentIdx+8];
      render_pass_desc['depthStencilAttachment'] = depth_stencil_attachment_desc;
    }
    if (HEAPU32[timestampWritesIdx]) {
      timestamp_writes_desc['querySet'] = wgpu[HEAPU32[timestampWritesIdx]];
      timestamp_writes_desc['beginningOfPassWriteIndex'] = (i = HEAP32[timestampWritesIdx+1]) >= 0 ? i : void 0;
      timestamp_writes_desc['endOfPassWriteIndex'] = (i = HEAP32[timestampWritesIdx+2]) >= 0 ? i : void 0;
      render_pass_desc['timestampWrites'] = timestamp_writes_desc;
    }
    render_pass_desc['occlusionQuerySet'] = wgpu[HEAPU32[descriptor+14]];
    render_pass_desc['maxDrawCount'] = maxDrawCount || void 0;

    {{{ wdebugdir('desc', '`GPUCommandEncoder.beginRenderPass() with descriptor:`') }}};
    wgpu_passEncoder = wgpu_encoder['beginRenderPass'](render_pass_desc);
  },

  wg_get_current_texture: function(canvasContext) {
    {{{ wdebuglog('`wg_get_current_texture(canvasContext=${canvasContext})`'); }}}
    {{{ wassert('canvasContext != 0'); }}}
    {{{ wassert('wgpu[canvasContext]'); }}}
    {{{ wassert('wgpu[canvasContext] instanceof GPUCanvasContext'); }}}

    // The canvas context texture is a special texture that automatically invalidates itself after the current rAF()
    // callback if over. Therefore when a new swap chain texture is produced, we need to delete the old one to avoid
    // accumulating references to stale textures from each frame.
    wgpu[1] = wgpu[canvasContext]['getCurrentTexture']();
    // The canvas context texture is hardcoded the special ID 1. Return that ID to caller.
    return 1;
  },
});
