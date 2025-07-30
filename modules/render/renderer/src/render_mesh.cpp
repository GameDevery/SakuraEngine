#include "SkrBase/misc/debug.h"
#include "SkrGraphics/raytracing.h"
#include "SkrRenderer/render_mesh.h"
#include <SkrOS/filesystem.hpp>

#include "SkrProfile/profile.h"

const bool UseRayTracing = true;

void skr_render_mesh_initialize(skr_render_mesh_id render_mesh, skr_mesh_resource_id mesh_resource)
{
    uint32_t ibv_c = 0;
    uint32_t vbv_c = 0;
    // 1. calculate the number of index buffer views and vertex buffer views
    for (uint32_t i = 0; i < mesh_resource->sections.size(); i++)
    {
        const auto& section = mesh_resource->sections[i];
        for (auto prim_idx : section.primive_indices)
        {
            auto& prim = mesh_resource->primitives[prim_idx];
            vbv_c += (uint32_t)prim.vertex_buffers.size();
            ibv_c++;
        }
    }
    
    // 2. do early reserve
    render_mesh->mesh_resource_id = mesh_resource;
    render_mesh->index_buffer_views.reserve(ibv_c);
    render_mesh->vertex_buffer_views.reserve(vbv_c);
    
    // 3. fill sections
    for (uint32_t i = 0; i < mesh_resource->sections.size(); i++)
    {
        const auto& section = mesh_resource->sections[i];
        for (auto prim_idx : section.primive_indices)
        {
            SKR_ASSERT(render_mesh->index_buffer_views.capacity() >= render_mesh->index_buffer_views.size());
            SKR_ASSERT(render_mesh->vertex_buffer_views.capacity() >= render_mesh->vertex_buffer_views.size());
            auto& draw_cmd = render_mesh->primitive_commands.add_default().ref();
            auto& prim = mesh_resource->primitives[prim_idx];
            auto& mesh_ibv = render_mesh->index_buffer_views.add_default().ref();
            auto vbv_start = render_mesh->vertex_buffer_views.size();
            // 3.1 fill vbvs
            for (uint32_t j = 0; j < prim.vertex_buffers.size(); j++)
            {
                auto& mesh_vbv = render_mesh->vertex_buffer_views.add_default().ref();
                const auto buffer_index = prim.vertex_buffers[j].buffer_index;
                mesh_vbv.buffer = render_mesh->buffers[buffer_index];
                mesh_vbv.offset = prim.vertex_buffers[j].offset;
                mesh_vbv.stride = prim.vertex_buffers[j].stride;
                SKR_LOG_INFO(u8"Mesh VBV %d: buffer %p, offset %d, stride %d", j, mesh_vbv.buffer, mesh_vbv.offset, mesh_vbv.stride);
            }
            // 3.2 fill ibv
            const auto buffer_index = prim.index_buffer.buffer_index;
            mesh_ibv.buffer = render_mesh->buffers[buffer_index];
            mesh_ibv.offset = prim.index_buffer.index_offset;
            mesh_ibv.stride = prim.index_buffer.stride;
            mesh_ibv.index_count = prim.index_buffer.index_count;
            mesh_ibv.first_index = prim.index_buffer.first_index;

            draw_cmd.ibv = &mesh_ibv;
            draw_cmd.vbvs = { render_mesh->vertex_buffer_views.data() + vbv_start, prim.vertex_buffers.size() };
            draw_cmd.primitive_index = prim_idx;
            draw_cmd.material_index = prim.material_index;
        }
    }
    
    // 4. construct blas
    if (UseRayTracing && (mesh_resource->primitives.size() > 0))
    {
        skr::InlineVector<CGPUAccelerationStructureGeometryDesc, 4> geoms;
        for (auto primitve : mesh_resource->primitives)
        {
            auto pos_vb = primitve.vertex_buffers.find_if(
                [](auto prim){ return prim.attribute == SKR_VERT_ATTRIB_POSITION; }
            ).ptr();
            if (!pos_vb) continue;
            
            CGPUAccelerationStructureGeometryDesc geom = {};
            geom.vertex_buffer = render_mesh->buffers[pos_vb->buffer_index];
            geom.vertex_offset = pos_vb->offset;
            geom.vertex_stride = pos_vb->stride;
            geom.index_buffer = render_mesh->buffers[primitve.index_buffer.buffer_index];
            geom.index_offset = primitve.index_buffer.index_offset;
            geom.index_stride = primitve.index_buffer.stride;
            geom.vertex_format = CGPU_FORMAT_R32G32B32_SFLOAT;
            geoms.add(geom);
        }

        CGPUAccelerationStructureDescriptor blas_desc = {};
        blas_desc.type = CGPU_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
        blas_desc.bottom.geometries = geoms.data();
        blas_desc.bottom.count = geoms.size();
        render_mesh->blas = cgpu_create_acceleration_structure(geoms[0].vertex_buffer->device, &blas_desc);
    }
}

void skr_render_mesh_free(skr_render_mesh_id render_mesh)
{
    if (UseRayTracing && render_mesh->blas)
    {
        cgpu_free_acceleration_structure(render_mesh->blas);
    }

    for (auto&& buffer : render_mesh->buffers)
    {
        cgpu_free_buffer(buffer);
    }
    SkrDelete(render_mesh);
}