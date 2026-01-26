#pragma once

#ifdef __cplusplus
extern "C" {
#endif

void wg_set_index_buffer_sz(WGpuBuffer buffer, WGPU_INDEX_FORMAT indexFormat, double_int53_t offset, double_int53_t size);
void wg_set_vertex_buffer_sz(int32_t slot, WGpuBuffer buffer, double_int53_t offset, double_int53_t size);

void wg_set_index_buffer(WGpuBuffer buffer, WGPU_INDEX_FORMAT indexFormat, double_int53_t offset _WGPU_DEFAULT_VALUE(0));
void wg_set_vertex_buffer(int32_t slot, WGpuBuffer buffer, double_int53_t offset _WGPU_DEFAULT_VALUE(0));

void wg_draw(uint32_t vertexCount, uint32_t instanceCount _WGPU_DEFAULT_VALUE(1), uint32_t firstVertex _WGPU_DEFAULT_VALUE(0), uint32_t firstInstance _WGPU_DEFAULT_VALUE(0));
void wg_draw_indexed(uint32_t indexCount, uint32_t instanceCount _WGPU_DEFAULT_VALUE(1), uint32_t firstIndex _WGPU_DEFAULT_VALUE(0), int32_t baseVertex _WGPU_DEFAULT_VALUE(0), uint32_t firstInstance _WGPU_DEFAULT_VALUE(0));

void wg_write_buffer(WGpuBuffer buffer, double_int53_t bufferOffset, const void *data NOTNULL, double_int53_t size);

void wg_finish_and_submit(void);
void wg_end_pass(void);

void wg_create_command_encoder(WGpuDevice device);
void wg_begin_compute_pass(const WGpuComputePassDescriptor *computePassDesc _WGPU_DEFAULT_VALUE(0));
void wg_begin_render_pass(const WGpuRenderPassDescriptor *renderPassDesc NOTNULL);
WGpuTexture wg_get_current_texture(WGpuCanvasContext canvasContext);

void wg_dispatch_workgroups(uint32_t workgroupCountX, uint32_t workgroupCountY _WGPU_DEFAULT_VALUE(1), uint32_t workgroupCountZ _WGPU_DEFAULT_VALUE(1));

void wg_set_pipeline(WGpuObjectBase pipeline);
void wg_set_bind_group(uint32_t index, WGpuBindGroup bindGroup, const uint32_t *dynamicOffsets _WGPU_DEFAULT_VALUE(0), uint32_t numDynamicOffsets _WGPU_DEFAULT_VALUE(0));

void wg_copy_buffer_to_buffer(WGpuBuffer source, double_int53_t sourceOffset, WGpuBuffer destination, double_int53_t destinationOffset, double_int53_t size _WGPU_DEFAULT_VALUE(WGPU_INFINITY));
void wg_resolve_query_set(WGpuQuerySet querySet, uint32_t firstQuery, uint32_t queryCount, WGpuBuffer destination, double_int53_t destinationOffset);

void wg_get_queue(WGpuDevice gpuDevice);

#ifdef __cplusplus
}
#endif
