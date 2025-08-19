#pragma once
#include "SkrContainersDef/map.hpp"
#include "SkrGraphics/cgpux.h"
#include "SkrRenderer/primitive_draw.h"
#include "model_resource.h"

namespace animd
{

typedef struct anim_render_model_t anim_render_model_t;
typedef struct anim_render_model_t* anim_render_model_id;

typedef struct anim_debug_render_model_future_t
{
    const char* model_name;
    skr_vfs_t* vfs_override;
    CGPUQueueId queue_override;
    ECGPUDStorageSource dstorage_source;
    anim_render_model_id render_model;
    SAtomicU32 io_status;
    SKR_ANIM_API ESkrIOStage get_status() const SKR_NOEXCEPT;
    SKR_ANIM_API bool is_ready() const SKR_NOEXCEPT;
} anim_debug_render_model_future_t;

struct anim_debug_render_model_comp_t
{
    skr_guid_t resource_guid;
    animd_ram_io_future_t ram_future;
    anim_debug_render_model_future_t vram_future;
};
typedef struct anim_debug_render_model_comp_t anim_debug_render_model_comp_t;

struct anim_debug_render_model_t
{
    virtual ~anim_debug_render_model_t() = default;
    animd_model_resource_id model_resource_id;
    bool use_dynamic_buffer = true;
    // clipping
    skr::Vector<CGPUTextureId> textures;
    skr::Vector<CGPUTextureViewId> texture_views;
    skr::Vector<skr::VertexBufferView> vertex_buffer_views;
    skr::Vector<skr::IndexBufferView> index_buffer_views;
    skr::Vector<skr::PrimitiveCommand> primitive_commands;
    // bind table cache
    skr::Map<CGPUTextureViewId, CGPUXBindTableId> bind_tables;
};

} // namespace animd