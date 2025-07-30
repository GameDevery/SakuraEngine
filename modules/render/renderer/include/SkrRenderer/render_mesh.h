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
namespace renderer
{

struct RenderMesh
{
    enum BufferTag
    {
        Index,
        Vertex
    };
    skr_mesh_resource_id mesh_resource_id;
    skr::Vector<CGPUBufferId> buffers;
    skr::Vector<skr_vertex_buffer_view_t> vertex_buffer_views;
    skr::Vector<skr_index_buffer_view_t> index_buffer_views;
    skr::Vector<PrimitiveCommand> primitive_commands;
    CGPUAccelerationStructureId blas = nullptr;
};

sreflect_managed_component(guid = "c66ab7ef-bde9-4e0f-8023-a2d99ba5134c")
MeshComponent
{
    SKR_RESOURCE_FIELD(MeshResource, mesh_resource);
};

} // namespace renderer
} // namespace skr
#endif

SKR_EXTERN_C SKR_RENDERER_API void skr_render_mesh_initialize(skr_render_mesh_id render_mesh, skr_mesh_resource_id mesh_resource);

SKR_EXTERN_C SKR_RENDERER_API void skr_render_mesh_free(skr_render_mesh_id render_mesh);
