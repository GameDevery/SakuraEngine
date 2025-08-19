#pragma once
#include "resources/mesh_resource.h"
#include "SkrRT/sugoi/sugoi_meta.hpp"
#include "SkrRenderer/primitive_draw.h"
#include "SkrGraphics/api.h"
#ifndef __meta__
    #include "SkrRenderer/render_mesh.generated.h" // IWYU pragma: export
#endif

#ifdef __cplusplus
namespace skr
{
struct RenderMesh
{
    enum BufferTag
    {
        Index,
        Vertex
    };
    MeshResource* mesh_resource;
    skr::Vector<CGPUBufferId> buffers;
    skr::Vector<skr_vertex_buffer_view_t> vertex_buffer_views;
    skr::Vector<skr_index_buffer_view_t> index_buffer_views;
    skr::Vector<PrimitiveCommand> primitive_commands;
    CGPUAccelerationStructureId blas = nullptr;
    std::atomic_bool need_build_blas = true;
};

sreflect_managed_component(guid = "c66ab7ef-bde9-4e0f-8023-a2d99ba5134c")
MeshComponent
{
    SKR_RESOURCE_FIELD(MeshResource, mesh_resource);
};

} // namespace skr
#endif

SKR_EXTERN_C SKR_RENDERER_API void skr_render_mesh_initialize(skr::RenderMesh* render_mesh, skr::MeshResource* mesh_resource);

SKR_EXTERN_C SKR_RENDERER_API void skr_render_mesh_free(skr::RenderMesh* render_mesh);
