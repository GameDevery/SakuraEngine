#include "SkrCore/log.h"
#include "SkrRT/ecs/world.hpp"
#include "SkrRT/ecs/component.hpp"
#include "SkrRenderGraph/backend/graph_backend.hpp"
#include "SkrRenderer/gpu_scene.h"
#include "SkrRenderer/render_device.h"
#include "SkrRenderer/resources/material_resource.hpp"
#include "SkrRenderer/render_mesh.h"
#include "SkrRenderer/shared/gpu_scene.hpp"
#include "SkrProfile/profile.h"
#include <atomic>

namespace skr
{
struct GPUSceneInstanceTask
{
    void build(skr::ecs::AccessBuilder& builder)
    {
        builder.write(&GPUSceneInstanceTask::instances);
    }
    skr::ecs::ComponentView<GPUSceneInstance> instances;
    skr::GPUScene* pScene = nullptr;
};

struct AddEntityToGPUScene : public GPUSceneInstanceTask
{
    void build(skr::ecs::AccessBuilder& builder)
    {
        GPUSceneInstanceTask::build(builder);
        builder.read(&AddEntityToGPUScene::meshes);
        builder.write(&AddEntityToGPUScene::instances);
        builder.write(&AddEntityToGPUScene::gpu_instances);
        builder.write(&AddEntityToGPUScene::primitive_comps);
        builder.write(&AddEntityToGPUScene::material_comps);
    }

    void run(skr::ecs::TaskContext& Context)
    {
        SkrZoneScopedN("GPUScene::AddEntityToGPUScene");

        sugoi_chunk_view_t v = {};
        sugoiS_access(pScene->ecs_world->get_storage(), (sugoi_entity_t)Context.entities()[0], &v);
        {
            for (auto i = 0; i < Context.size(); i++)
            {
                auto entity = Context.entities()[i];
                const auto& mesh = meshes[i];
                const bool MeshNotResolved = !mesh.mesh_resource.is_resolved();
                bool PrimitiveBufferOversized = false;
                bool MaterialBufferOversized = false;
                if (!MeshNotResolved)
                {
                    auto mesh_resource = mesh.mesh_resource.get_resolved();
                    PrimitiveBufferOversized = (mesh_resource->primitives.size() + pScene->total_prim_count) > pScene->primitives_table->GetInstanceCapacity();
                    MaterialBufferOversized = (mesh_resource->materials.size() + pScene->total_mat_count) > pScene->materials_table->GetInstanceCapacity();
                }
                if (MeshNotResolved || PrimitiveBufferOversized || MaterialBufferOversized)
                {
                    pScene->AddEntity(entity);
                    continue;
                }

                auto mesh_resource = mesh.mesh_resource.get_resolved();
                auto& gpu_inst = gpu_instances[i];

                // 分配 instnace id
                auto& instance_data = instances[i];
                instance_data.entity = entity;
                if (pScene->free_insts.try_dequeue(instance_data.instance_index))
                    pScene->free_inst_count -= 1;
                else
                    instance_data.instance_index = pScene->latest_inst_index++;

                // 分配 prim ids
                auto& prim_comp = primitive_comps[i];
                prim_comp.resize(mesh_resource->primitives.size());
                pScene->total_prim_count += prim_comp.size();
                for (auto& prim : prim_comp)
                {
                    if (pScene->free_prims.try_dequeue(prim.global_index))
                        pScene->free_prim_count -= 1;
                    else
                        prim.global_index = pScene->latest_prim_index++;
                }

                // 分配 mat ids
                auto& mat_comp = material_comps[i];
                mat_comp.resize(mesh_resource->materials.size());
                pScene->total_mat_count += mat_comp.size();
                for (auto& mat : mat_comp)
                {
                    if (pScene->free_mats.try_dequeue(mat.global_index))
                        pScene->free_mat_count -= 1;
                    else
                        mat.global_index = pScene->latest_mat_index++;
                }

                if (mesh_resource->render_mesh->need_build_blas)
                {
                    pScene->dirty_blases.enqueue(mesh_resource->render_mesh->blas);
                    mesh_resource->render_mesh->need_build_blas = false;
                }

                // record index/vertex buffers
                auto StorePrimitiveToTable = pScene->store_map.find(sugoi_id_of<gpu::Primitive>::get()).value();
                for (uint32_t prim_id = 0; prim_id < mesh_resource->primitives.size(); prim_id++)
                {
                    const auto& prim_info = mesh_resource->primitives[prim_id];
                    auto& prim_data = prim_comp[prim_id];
                    prim_data.material_index = prim_info.material_index;
                    for (const auto& vb : prim_info.vertex_buffers)
                    {
                        if (vb.vertex_count == 0)
                            continue;

                        const auto buffer_id = mesh_resource->render_mesh->buffer_ids[vb.buffer_index];
                        if (vb.attribute == EVertexAttribute::POSITION)
                            prim_data.positions = { vb.offset / vb.stride, vb.vertex_count, buffer_id };
                        else if (vb.attribute == EVertexAttribute::TEXCOORD)
                            prim_data.uvs = { vb.offset / vb.stride, vb.vertex_count, buffer_id };
                        else if (vb.attribute == EVertexAttribute::NORMAL)
                            prim_data.normals = { vb.offset / vb.stride, vb.vertex_count, buffer_id };
                        else if (vb.attribute == EVertexAttribute::TANGENT)
                            prim_data.tangents = { vb.offset / vb.stride, vb.vertex_count, buffer_id };
                    }
                    {
                        const auto& ib = prim_info.index_buffer;
                        const auto buffer_id = mesh_resource->render_mesh->buffer_ids[ib.buffer_index];
                        const auto buffer_offset = ib.index_offset;
                        prim_data.triangles = { ib.index_offset / (3 * ib.stride), ib.index_count / 3, buffer_id };
                    }
                    auto& table = pScene->primitives_table;
                    StorePrimitiveToTable(*table, prim_data.global_index, &prim_data);                    
                }
                gpu_inst.primitives = { prim_comp[0].global_index, (uint32_t)prim_comp.size() };

                auto StoreMaterialToTable = pScene->store_map.find(sugoi_id_of<gpu::Material>::get()).value();
                for (uint32_t mat_id = 0; mat_id < mesh_resource->materials.size(); mat_id++)
                {
                    auto& mat_data = mat_comp[mat_id];
                    mesh_resource->materials[mat_id].resolve(true, 1, ESkrRequesterType::SKR_REQUESTER_UNKNOWN);
                    const auto& mat = mesh_resource->materials[mat_id].get_resolved();
                    mat_data.basecolor_tex = ~0;
                    mat_data.metallic_roughness_tex = ~0;
                    mat_data.emission_tex = ~0;
                    for (const auto& tex : mat->overrides.textures)
                    {
                        if (tex.slot_name == u8"BaseColor")
                            mat_data.basecolor_tex = tex.bindless_id;
                        else if (tex.slot_name == u8"MetallicRoughness")
                            mat_data.metallic_roughness_tex = tex.bindless_id;
                        else if (tex.slot_name == u8"Emissive")
                            mat_data.emission_tex = tex.bindless_id;
                    }
                    auto& table = pScene->materials_table;
                    StoreMaterialToTable(*table, mat_data.global_index, &mat_data);
                }
                gpu_inst.materials = { mat_comp[0].global_index, (uint32_t)mat_comp.size() };

                // add tlas
                auto& transform = gpu_inst.transform;
                auto& tlas_instance = pScene->tlas_instances[instance_data.instance_index];
                tlas_instance.bottom = mesh_resource->render_mesh->blas;
                tlas_instance.instance_id = instance_data.instance_index;
                tlas_instance.instance_mask = 255;
                float transform34[12] = {
                    transform.m00, transform.m10, transform.m20, transform.m30, // Row 0: X axis + X translation
                    transform.m01,
                    transform.m11,
                    transform.m21,
                    transform.m31, // Row 1: Y axis + Y translation
                    transform.m02,
                    transform.m12,
                    transform.m22,
                    transform.m32 // Row 2: Z axis + Z translation
                };
                memcpy(tlas_instance.transform, transform34, sizeof(transform34));

                pScene->tlas_dirty = true;
                pScene->total_inst_count += 1;
                pScene->entity_ids[entity] = instance_data.instance_index;
            }
        }
    }
    skr::ecs::ComponentView<const MeshComponent> meshes;
    skr::ecs::ComponentView<GPUSceneInstance> instances;
    skr::ecs::ComponentView<gpu::Instance> gpu_instances;
    skr::ecs::ComponentView<gpu::Primitive> primitive_comps;
    skr::ecs::ComponentView<gpu::Material> material_comps;
};

struct ScanGPUScene : public GPUSceneInstanceTask
{
    void run(skr::ecs::TaskContext& Context)
    {
        SkrZoneScopedN("GPUScene::ScanGPUScene");

        const auto& Lane = pScene->GetLaneForUpload();
        sugoi_chunk_view_t v = {};
        sugoiS_access(pScene->ecs_world->get_storage(), (sugoi_entity_t)Context.entities()[0], &v);
        {
            for (auto [cpu_type, table] : pScene->table_map)
            {
                auto StoreToTable = pScene->store_map.find(cpu_type).value();
                for (auto i = 0; i < Context.size(); i++)
                {
                    auto entity = Context.entities()[i];
                    const auto& instance_data = instances[i];
                    if (instance_data.instance_index == ~0)
                        continue;

                    auto& dirties = Lane.dirties.find(entity).value();
                    if (!dirties.contains(cpu_type))
                        continue;

                    if (const auto data = Context.read(cpu_type).at(i))
                    {
                        StoreToTable(*table, instance_data.instance_index, data);
                    }
                }
            }
        }
    }
    ScanGPUScene(skr::render_graph::RenderGraph* g)
        : graph(g)
    {
    }
    skr::render_graph::RenderGraph* graph;
};

void GPUScene::AdjustDatabase(skr::render_graph::RenderGraph* graph)
{
    SkrZoneScopedN("GPUScene::AdjustBuffer");
    const auto& Lane = GetLaneForUpload();
    auto& frame_ctx = frame_ctxs.get(graph);

    const auto existed_instances = tlas_instances.size();
    if (free_inst_count < Lane.add_ents.size())
    {
        tlas_instances.resize_zeroed(tlas_instances.size() + Lane.add_ents.size());
    }
    const auto required_instances = tlas_instances.size();
    for (auto& [cpu_type, table] : table_map)
    {
        auto& imported = frame_ctx.table_handles.try_add_default(cpu_type).value();
        imported = table->AdjustDatabase(graph, required_instances);
    }
    // TODO: RESIZE MAT AND PRIMS
    frame_ctx.primitives_handle = primitives_table->AdjustDatabase(graph, 16 * required_instances);
    frame_ctx.materials_handle = materials_table->AdjustDatabase(graph, 16 * required_instances);
}

void GPUScene::ExecuteUpload(skr::render_graph::RenderGraph* graph)
{
    using namespace skr::render_graph;

    SkrZoneScopedN("GPUScene::ExecuteUpload");
    SwitchLane();

    // Reset Frame Resources
    auto& frame_ctx = frame_ctxs.get(graph);
    frame_ctx.frame_tlas = {};
    frame_ctx.tlas_handle = {};

    // Ensure buffers are sized correctly and import buffers to render graph
    AdjustDatabase(graph);

    auto ImportTLAS = [&frame_ctx, this, graph]() {
        // Import TLAS to RenderGraph if it exists
        frame_ctx.frame_tlas = tlas_manager->GetLatestTLAS(graph);
        frame_ctx.tlas_handle = {};
        if (frame_ctx.frame_tlas.get() != nullptr)
        {
            frame_ctx.tlas_handle = graph->create_acceleration_structure(
                [&frame_ctx, graph](RenderGraph& rg, class RenderGraph::AccelerationStructureBuilder& builder) {
                    builder.set_name(u8"GPUScene-TLAS")
                        .import(frame_ctx.frame_tlas.get());
                });
        }
    };
    SKR_DEFER({ ImportTLAS(); });

    auto& Lane = GetLaneForUpload();
    if (Lane.dirty_buffer_size == 0)
        return;

    // Schedule remove & add & scan
    auto get_batchsize = +[](uint64_t ecount) { return std::max(ecount / 8ull, 1024ull); };
    auto& upload_ctx = upload_ctxs.get(graph);
    {
        SkrZoneScopedN("GPUScene::Tasks");
        if (!Lane.remove_ents.is_empty())
        {
            SkrZoneScopedN("GPUScene::RemoveEntityFromGPUScene");
            for (auto to_remove : Lane.remove_ents)
            {
                RemoveEntity(to_remove);
            }
        }
        if (!Lane.add_ents.is_empty())
        {
            SkrZoneScopedN("GPUScene::AddEntityToGPUScene");
            skr::ecs::TaskOptions options;
            upload_ctx.add_finish.clear();
            options.on_finishes.add(upload_ctx.add_finish);

            AddEntityToGPUScene add;
            add.pScene = this;
            ecs_world->dispatch_task(add, get_batchsize(Lane.add_ents.size()), Lane.add_ents, std::move(options));
        }
        if (!Lane.dirty_ents.is_empty())
        {
            SkrZoneScopedN("GPUScene::ScanGPUScene");

            skr::ecs::TaskOptions options;
            upload_ctx.scan_finish.clear();
            options.on_finishes.add(upload_ctx.scan_finish);

            ScanGPUScene scan(graph);
            scan.pScene = this;
            ecs_world->dispatch_task(scan, get_batchsize(Lane.dirty_ents.size()), Lane.dirty_ents, std::move(options));
        }
    }

    for (auto& [cpu_type, gpu_table] : table_map)
    {
        if (cpu_type != instance_type)
        {
            gpu_table->DispatchSparseUpload(graph, {});
            continue;
        }

        gpu_table->DispatchSparseUpload(graph,
            [this](skr::render_graph::RenderGraph& g, skr::render_graph::ComputePassContext& ctx) {
            auto& upload_ctx = upload_ctxs.get(ctx.graph);
            upload_ctx.add_finish.wait(true);
            upload_ctx.scan_finish.wait(true);

            // Send BLAS / TLAS requests
            {
                SkrZoneScopedN("GPUScene::UpdateAccelerationStructure");
                TLASUpdateRequest update_request;
                {
                    CGPUAccelerationStructureId blas = nullptr;
                    while (dirty_blases.try_dequeue(blas) && blas)
                    {
                        update_request.blases_to_build.add_unique(blas);
                    }
                }
                if (tlas_dirty || !update_request.blases_to_build.is_empty())
                {
                    if (total_inst_count != 0)
                    {
                        CGPUAccelerationStructureDescriptor tlas_desc = {};
                        tlas_desc.type = CGPU_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
                        tlas_desc.flags = CGPU_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
                        tlas_desc.top.count = total_inst_count;
                        tlas_desc.top.instances = tlas_instances.data();
                        update_request.tlas_desc = tlas_desc;
                    }
                    tlas_manager->Request(ctx.graph, update_request);
                }
            }
            auto& Lane = GetLaneForUpload();
            {
                SkrZoneScopedN("GPUScene::CleanUpLane");

                Lane.add_ents.clear();
                Lane.remove_ents.clear();
                Lane.dirty_ents.clear();
                Lane.dirty_comp_count = 0;
                Lane.dirties.clear();
                Lane.dirty_buffer_size = 0;
            } });
    }
    materials_table->DispatchSparseUpload(graph, {});
    primitives_table->DispatchSparseUpload(graph, {});
}

void GPUScene::AddEntity(skr::ecs::Entity entity)
{
    auto& Lane = GetFrontLane();
    {
        Lane.add_mtx.lock();
        Lane.add_ents.add(entity);
        Lane.add_mtx.unlock();
    }
    for (auto& [cpu_type, table] : table_map)
    {
        RequireUpload(entity, cpu_type);
    }
}

bool GPUScene::CanRemoveEntity(skr::ecs::Entity entity)
{
    return entity_ids.contains(entity);
}

void GPUScene::RemoveEntity(skr::ecs::Entity entity)
{
    auto& Lane = GetFrontLane();
    auto iter = entity_ids.find(entity);
    if (iter != entity_ids.end())
    {
        const auto instance_data = ecs_world->random_readwrite<GPUSceneInstance>().get(entity);
        const auto prims = ecs_world->random_readwrite<gpu::Primitive>().get(entity);
        const auto mats = ecs_world->random_readwrite<gpu::Material>().get(entity);

        free_insts.enqueue(instance_data->instance_index);
        free_inst_count += 1;
        total_inst_count -= 1;

        for (auto prim : *prims)
            free_prims.enqueue(prim.global_index);
        free_prim_count += prims->size();
        total_prim_count -= prims->size();

        for (auto mat : *mats)
            free_mats.enqueue(mat.global_index);
        free_mat_count += mats->size();
        total_mat_count -= mats->size();

        auto& tlas_instance = tlas_instances[instance_data->instance_index];
        memset(tlas_instance.transform, 0, sizeof(tlas_instance.transform));
        total_inst_count -= 1;
        tlas_dirty = true;

        entity_ids.erase(entity);
    }
    else
    {
        Lane.remove_mtx.lock();
        Lane.remove_ents.add(entity);
        Lane.remove_mtx.unlock();
    }
}

void GPUScene::RequireUpload(skr::ecs::Entity entity, CPUTypeID component)
{
    auto& Lane = GetFrontLane();
    Lane.dirty_mtx.lock();
    auto cs = Lane.dirties.try_add_default(entity);

    if (!cs.already_exist())
    {
        Lane.dirty_ents.add(entity);
    }

    if (!cs.value().find(component))
    {
        cs.value().add(component);
        if (auto type_iter = table_map.find(component))
        {
            Lane.dirty_buffer_size += type_iter.value()->GetComponentSize(0);
        }
        Lane.dirty_comp_count += 1;
    }

    Lane.dirty_mtx.unlock();
}

void GPUScene::Initialize(gpu::TableManager* table_manager, skr::RenderDevice* render_device, skr::ecs::World* world)
{
    SKR_LOG_INFO(u8"Initializing GPUScene...");

    ecs_world = world;
    render_device = render_device;
    instance_type = sugoi_id_of<gpu::Instance>::get();

    tlas_manager = TLASManager::Create(1 + RG_MAX_FRAME_IN_FLIGHT, render_device);
    {
        auto& instance_table = table_map.try_add_default(instance_type).value();
        gpu::TableConfig table_builder(render_device->get_cgpu_device(), u8"Instances");
        table_builder.with_page_size(16 * 1024)
            .with_instances(16 * 1024)
            .add_component(0, sizeof(gpu::GPUDatablock<gpu::Instance>));
        instance_table = table_manager->CreateTable(table_builder);
        store_map.add(sugoi_id_of<gpu::Instance>::get(), +[](gpu::TableInstance& table, uint32_t inst, const void* data){
            const gpu::Instance* d = (const gpu::Instance*)data;
            const gpu::GPUDatablock<gpu::Instance> block(*d);
            const uint64_t offset = table.GetComponentOffset(0, inst);
            table.Store(offset, block);
        });
    }
    {
        gpu::TableConfig table_builder(render_device->get_cgpu_device(), u8"Primitives");
        table_builder.with_page_size(16 * 1024)
            .with_instances(16 * 1024)
            .add_component(0, sizeof(gpu::GPUDatablock<gpu::Primitive>));
        primitives_table = table_manager->CreateTable(table_builder);
        store_map.add(sugoi_id_of<gpu::Primitive>::get(), +[](gpu::TableInstance& table, uint32_t inst, const void* data){
            const gpu::Primitive* d = (const gpu::Primitive*)data;
            const gpu::GPUDatablock<gpu::Primitive> block(*d);
            const uint64_t offset = table.GetComponentOffset(0, inst);
            table.Store(offset, block);
        });
    }
    {
        gpu::TableConfig table_builder(render_device->get_cgpu_device(), u8"Materials");
        table_builder.with_page_size(16 * 1024)
            .with_instances(16 * 1024)
            .add_component(0, sizeof(gpu::GPUDatablock<gpu::Material>));
        materials_table = table_manager->CreateTable(table_builder);
        store_map.add(sugoi_id_of<gpu::Material>::get(), +[](gpu::TableInstance& table, uint32_t inst, const void* data){
            const gpu::Material* d = (const gpu::Material*)data;
            const gpu::GPUDatablock<gpu::Material> block(*d);
            const uint64_t offset = table.GetComponentOffset(0, inst);
            table.Store(offset, block);
        });
    }

    SKR_LOG_INFO(u8"GPUScene initialized successfully");
}

void GPUScene::Shutdown()
{
    SKR_LOG_INFO(u8"Shutting down GPUScene...");

    // Shutdown allocators (will release buffers)
    for (auto& [type, gpu_table] : table_map)
        gpu_table.reset();

    for (uint32_t i = 0; i < frame_ctxs.max_frames_in_flight(); ++i)
    {
        auto& ctx = frame_ctxs[i];
        ctx.frame_tlas = {};
        if (ctx.buffer_to_discard)
        {
            cgpu_free_buffer(ctx.buffer_to_discard);
            ctx.buffer_to_discard = nullptr;
        }
    }
    TLASManager::Destroy(tlas_manager);

    ecs_world = nullptr;
    render_device = nullptr;

    SKR_LOG_INFO(u8"GPUScene shutdown complete");
}

} // namespace skr