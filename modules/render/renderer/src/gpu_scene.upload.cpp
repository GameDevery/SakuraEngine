#include "SkrCore/log.h"
#include "SkrRT/ecs/world.hpp"
#include "SkrRenderer/gpu_scene.h"
#include "SkrRenderer/render_device.h"
#include "SkrRenderer/render_mesh.h"
#include "SkrRenderer/shared/gpu_scene.hpp"
#include "SkrProfile/profile.h"
#include <atomic>

namespace skr::renderer
{

struct GPUSceneInstanceTask
{
    void build(skr::ecs::AccessBuilder& builder)
    {
        builder.write(&GPUSceneInstanceTask::instances);
        for (auto [cpu_type, gpu_type] : pScene->GetTypeRegistry())
        {
            builder.optional_read(cpu_type);
        }
    }
    skr::ecs::ComponentView<GPUSceneInstance> instances;
    skr::renderer::GPUScene* pScene = nullptr;
};

struct RemoveEntityFromGPUScene : public GPUSceneInstanceTask
{
    void run(skr::ecs::TaskContext& Context)
    {
        SkrZoneScopedN("GPUScene::RemoveEntityFromGPUScene");

        sugoi_chunk_view_t v = {};
        sugoiS_access(pScene->ecs_world->get_storage(), (sugoi_entity_t)Context.entities()[0], &v);
        {
            for (auto i = 0; i < Context.size(); i++)
            {
                const auto& instance_data = instances[i];
                pScene->free_ids.enqueue(instance_data.instance_index);
            }
        }
    }
};

struct AddEntityToGPUScene : public GPUSceneInstanceTask
{
    void build(skr::ecs::AccessBuilder& builder)
    {
        GPUSceneInstanceTask::build(builder);
        builder.read(&AddEntityToGPUScene::meshes);
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
                auto& instance_data = instances[i];
                instance_data.entity = entity;
                if (!pScene->free_ids.try_dequeue(instance_data.instance_index))
                {
                    instance_data.instance_index = pScene->latest_index++;
                }

                // TODO: USE ASYNC
                const auto& mesh = meshes[i];
                while (!mesh.mesh_resource.is_resolved())
                    ;
                auto mesh_resource = mesh.mesh_resource.get_resolved();
                if (mesh_resource->render_mesh->need_build_blas)
                {
                    pScene->dirty_blases.enqueue(mesh_resource->render_mesh->blas);
                    mesh_resource->render_mesh->need_build_blas = false;
                }

                // add tlas
                auto& o2w = *(const GPUSceneObjectToWorld*)Context.read(sugoi_id_of<GPUSceneObjectToWorld>::get()).at(i);
                auto& transform = o2w.matrix;
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
            }
        }
    }
    skr::ecs::ComponentView<const MeshComponent> meshes;
};

struct ScanGPUScene : public GPUSceneInstanceTask
{
    void run(skr::ecs::TaskContext& Context)
    {
        SkrZoneScopedN("GPUScene::ScanGPUScene");

        sugoi_chunk_view_t v = {};
        sugoiS_access(pScene->ecs_world->get_storage(), (sugoi_entity_t)Context.entities()[0], &v);
        {
            // 预先计算此批次需要的上传操作数量，分类统计
            uint32_t soa_segments_op_count = 0;
            for (auto i = 0; i < Context.size(); i++)
            {
                for (auto [cpu_type, gpu_type] : pScene->type_registry)
                {
                    const auto data = Context.read(cpu_type).at(i);
                    if (!data)
                        continue;
                    const auto& component_info = pScene->component_types[gpu_type];
                    soa_segments_op_count++;
                }
            }

            // 为Core Data和Additional Data分别分配索引范围
            auto core_batch_start = pCoreCounter->fetch_add(soa_segments_op_count);
            uint32_t core_local_index = 0;
            uint32_t additional_local_index = 0;

            for (auto i = 0; i < Context.size(); i++)
            {
                auto entity = Context.entities()[i];
                const auto& instance_data = instances[i];
                for (auto [cpu_type, gpu_type] : pScene->type_registry)
                {
                    const auto data = Context.read(cpu_type).at(i);
                    const auto& component_info = pScene->component_types[gpu_type];

                    if (!data) continue;

                    const uint64_t dst_offset = pScene->soa_segments.get_component_offset(gpu_type, instance_data.instance_index);
                    const uint64_t src_offset = pScene->upload_ctx.get(graph).upload_cursor.fetch_add(component_info.element_size);

                    if (src_offset + component_info.element_size <= pScene->upload_ctx.get(graph).upload_buffer->info->size)
                    {
                        memcpy(DRAMCache->data() + src_offset, data, component_info.element_size);

                        // 使用预分配的连续索引范围，添加到Core Data上传列表
                        auto upload_index = core_batch_start + core_local_index;
                        if (upload_index < pScene->upload_ctx.get(graph).soa_segments_uploads.size())
                        {
                            GPUScene::Upload upload;
                            upload.src_offset = src_offset;
                            upload.dst_offset = dst_offset;
                            upload.data_size = component_info.element_size;
                            pScene->upload_ctx.get(graph).soa_segments_uploads[upload_index] = upload;
                            core_local_index++;
                        }
                        else
                        {
                            SKR_LOG_ERROR(u8"GPUScene: Core Data upload operations array overflow - index %u >= size %u",
                                upload_index,
                                (uint32_t)pScene->upload_ctx.get(graph).soa_segments_uploads.size());
                        }
                    }
                    else
                    {
                        SKR_LOG_ERROR(u8"GPUScene: Upload buffer overflow");
                    }
                }
            }
        }
    }
    ScanGPUScene(skr::render_graph::RenderGraph* g, skr::Vector<uint8_t>* DRAM, std::atomic_uint32_t* pCoreCounter, std::atomic_uint32_t* pAdditionalCounter)
        : graph(g)
        , DRAMCache(DRAM)
        , pCoreCounter(pCoreCounter)
        , pAdditionalCounter(pAdditionalCounter)
    {
    }
    skr::render_graph::RenderGraph* graph;
    skr::Vector<uint8_t>* DRAMCache;
    std::atomic_uint32_t* pCoreCounter;
    std::atomic_uint32_t* pAdditionalCounter;
};

void GPUScene::PrepareUploadBuffer(skr::render_graph::RenderGraph* graph)
{
    SkrZoneScopedN("GPUScene::PrepareUploadBuffer");

    // Calculate required upload buffer size
    uint64_t required_size = CalculateDirtySize();
    if (required_size == 0)
        return;

    // Add some padding for alignment
    required_size = (required_size + 255) & ~255; // Align to 256 bytes

    // Check if we need to recreate the upload buffer
    auto& upload_buffer = upload_ctx.get(graph).upload_buffer;
    if (!upload_buffer || upload_buffer->info->size < required_size)
    {
        // Release old buffer if exists
        if (upload_buffer)
        {
            cgpu_free_buffer(upload_buffer);
            upload_buffer = nullptr;
        }

        // Create new upload buffer
        CGPUBufferDescriptor upload_desc = {};
        upload_desc.name = u8"GPUScene-UploadBuffer";
        upload_desc.flags = CGPU_BUFFER_FLAG_PERSISTENT_MAP_BIT;
        upload_desc.usages = CGPU_BUFFER_USAGE_NONE;
        upload_desc.memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU;
        upload_desc.size = required_size;
        upload_desc.prefer_on_device = true;

        upload_buffer = cgpu_create_buffer(render_device->get_cgpu_device(), &upload_desc);

        SKR_LOG_INFO(u8"GPUScene: Created upload buffer with size %llu bytes", required_size);
    }
}

void GPUScene::AdjustBuffer(skr::render_graph::RenderGraph* graph)
{
    SkrZoneScopedN("GPUScene::AdjustBuffer");

    uint32_t existed_instances = GetInstanceCount();
    auto required_instances = instance_count = existed_instances + add_ents.size() - remove_ents.size();
    tlas_instances.resize_zeroed(required_instances);
    
    // Get current frame's buffer context for deferred destruction
    auto& current_buffer_ctx = buffer_ctx.get(graph);
    
    // First, clean up any buffer marked for discard from previous frame
    if (current_buffer_ctx.buffer_to_discard != nullptr)
    {
        cgpu_free_buffer(current_buffer_ctx.buffer_to_discard);
        current_buffer_ctx.buffer_to_discard = nullptr;
    }
    
    if (soa_segments.needs_resize(required_instances))
    {
        SKR_LOG_INFO(u8"GPUScene: Core data resize needed. Current: %u/%u instances",
            required_instances,
            soa_segments.get_instance_capacity());

        // Calculate new capacity
        uint32_t new_capacity = static_cast<uint32_t>(required_instances * config.resize_growth_factor);

        // Resize and get old buffer (no blocking sync needed now)
        auto old_buffer = soa_segments.resize(new_capacity);
        scene_buffer = soa_segments.import_buffer(graph, u8"scene_buffer");
        
        if (old_buffer && soa_segments.get_buffer())
        {
            // Import old buffer for copy source
            auto old_buffer_handle = graph->create_buffer(
                [=](skr::render_graph::RenderGraph& g, skr::render_graph::BufferBuilder& builder) {
                    builder.set_name(u8"old_scene_buffer")
                        .import(old_buffer, CGPU_RESOURCE_STATE_SHADER_RESOURCE);
                });
            // Copy data from old to new buffer
            soa_segments.copy_segments(graph, old_buffer_handle, scene_buffer, existed_instances);
            
            // Mark old buffer for deferred destruction (will be freed next time this frame slot comes around)
            // Unlike TLAS, we don't reuse the old buffer - we always discard it after copy
            current_buffer_ctx.buffer_to_discard = old_buffer;
        }
    }
    else
    {
        // Import scene buffer with proper state management
        scene_buffer = soa_segments.import_buffer(graph, u8"scene_buffer");
    }
}

void GPUScene::ExecuteUpload(skr::render_graph::RenderGraph* graph)
{
    SkrZoneScopedN("GPUScene::ExecuteUpload");

    // Ensure buffers are sized correctly
    AdjustBuffer(graph);

    // Prepare upload buffer based on dirty size
    PrepareUploadBuffer(graph);

    // Schedule remove & add & scan
    auto get_batchsize = +[](uint64_t ecount) { return std::max(ecount / 8ull, 1024ull); };
    auto& current_ctx = upload_ctx.get(graph);
    {
        SkrZoneScopedN("GPUScene::Tasks");
        if (!remove_ents.is_empty())
        {
            SkrZoneScopedN("GPUScene::RemoveEntityFromGPUScene");
            RemoveEntityFromGPUScene remove;
            remove.pScene = this;
            ecs_world->dispatch_task(remove, get_batchsize(remove_ents.size()), remove_ents);
        }
        if (!add_ents.is_empty())
        {
            SkrZoneScopedN("GPUScene::AddEntityToGPUScene");
            AddEntityToGPUScene add;
            add.pScene = this;
            ecs_world->dispatch_task(add, get_batchsize(add_ents.size()), add_ents);
        }
        std::atomic_uint32_t CoreUploadCounter;
        std::atomic_uint32_t AdditionalUploadCounter;
        if (!dirty_ents.is_empty())
        {
            SkrZoneScopedN("GPUScene::ScanGPUScene");
            current_ctx.DRAMCache.resize_unsafe(current_ctx.upload_buffer->info->size);

            uint64_t total_dirty_count = dirty_comp_count.load();
            current_ctx.soa_segments_uploads.resize_unsafe(total_dirty_count);
            current_ctx.additional_data_uploads.resize_unsafe(total_dirty_count);

            ScanGPUScene scan(graph, &current_ctx.DRAMCache, &CoreUploadCounter, &AdditionalUploadCounter);
            scan.pScene = this;
            ecs_world->dispatch_task(scan, get_batchsize(dirty_ents.size()), dirty_ents);
        }
        // TODO: FULL ASYNC
        skr::ecs::TaskScheduler::Get()->sync_all();
        using namespace skr::render_graph;
        
        // Get current frame's TLAS context
        auto& current_tlas_ctx = tlas_ctx.get(graph);
        
        // First, clean up any TLAS marked for discard from previous frame
        if (current_tlas_ctx.tlas_to_discard != nullptr)
        {
            cgpu_free_acceleration_structure(current_tlas_ctx.tlas_to_discard);
            current_tlas_ctx.tlas_to_discard = nullptr;
        }
        
        // Check if we need to rebuild TLAS
        bool need_rebuild = !add_ents.is_empty();
        if (need_rebuild && instance_count > 0)
        {
            // Mark old TLAS for discard (will be freed next time this frame slot comes around)
            if (current_tlas_ctx.tlas != nullptr)
            {
                current_tlas_ctx.tlas_to_discard = current_tlas_ctx.tlas;
                current_tlas_ctx.tlas = nullptr;
            }
            
            // Create new TLAS
            CGPUAccelerationStructureDescriptor tlas_desc = {};
            tlas_desc.type = CGPU_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
            tlas_desc.flags = CGPU_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
            tlas_desc.top.count = tlas_instances.size();
            tlas_desc.top.instances = tlas_instances.data();
            current_tlas_ctx.tlas = cgpu_create_acceleration_structure(render_device->get_cgpu_device(), &tlas_desc);
            current_tlas_ctx.instance_count = instance_count;

            graph->add_copy_pass(
                [=](RenderGraph& g, CopyPassBuilder& builder) {
                    builder.set_name(SKR_UTF8("BuildAccelerationStructures"))
                        .can_be_lone();
                },
                [this, &current_tlas_ctx](class RenderGraph& graph, CopyPassContext& ctx) {
                    // Build BLAS first
                    skr::Vector<CGPUAccelerationStructureId> blases;
                    blases.reserve(dirty_blases.size_approx());
                    CGPUAccelerationStructureId blas = nullptr;
                    while (dirty_blases.try_dequeue(blas) && blas)
                    {
                        blases.add_unique(blas);
                    }
                    if (blases.size())
                    {
                        CGPUAccelerationStructureBuildDescriptor blas_build = {
                            .type = CGPU_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL,
                            .as_count = (uint32_t)blases.size(),
                            .as = blases.data()
                        };
                        cgpu_cmd_build_acceleration_structures(ctx.cmd, &blas_build);
                    }
                    if (current_tlas_ctx.tlas != nullptr)
                    {
                        // Build TLAS
                        CGPUAccelerationStructureBuildDescriptor tlas_build = {
                            .type = CGPU_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL,
                            .as_count = 1,
                            .as = &current_tlas_ctx.tlas
                        };
                        cgpu_cmd_build_acceleration_structures(ctx.cmd, &tlas_build);
                    }
                });
        }
        else
        {
            current_tlas_ctx.tlas = tlas_ctx.get_frame_offset(graph, 1).tlas;
        }
        
        // Import TLAS to RenderGraph if it exists
        if (current_tlas_ctx.tlas != nullptr)
        {
            current_tlas_ctx.tlas_handle = graph->create_acceleration_structure([&current_tlas_ctx](RenderGraph& graph, class RenderGraph::AccelerationStructureBuilder& builder) {
                builder.set_name(u8"GPUScene-TLAS")
                    .import(current_tlas_ctx.tlas);
            });
        }

        {
            ::memcpy(current_ctx.upload_buffer->info->cpu_mapped_address, current_ctx.DRAMCache.data(), current_ctx.DRAMCache.size());
            remove_ents.clear();
            add_ents.clear();
            dirty_ents.clear();
            dirties.clear();

            // 调整上传列表大小到实际使用的大小
            uint32_t actual_core_uploads = CoreUploadCounter.load();
            uint32_t actual_additional_uploads = AdditionalUploadCounter.load();
            current_ctx.soa_segments_uploads.resize_unsafe(actual_core_uploads);
            current_ctx.additional_data_uploads.resize_unsafe(actual_additional_uploads);

            dirty_comp_count -= (actual_core_uploads + actual_additional_uploads);
        }
    }

    // Dispatch SparseUpload compute shaders for both Core Data and Additional Data
    if (!current_ctx.soa_segments_uploads.is_empty() || !current_ctx.additional_data_uploads.is_empty())
    {
        DispatchSparseUpload(graph, std::move(current_ctx.soa_segments_uploads), std::move(current_ctx.additional_data_uploads));
        current_ctx.upload_cursor.store(0);
    }
}

void GPUScene::DispatchSparseUpload(skr::render_graph::RenderGraph* graph, skr::Vector<Upload>&& core_uploads, skr::Vector<Upload>&& additional_uploads)
{
    SkrZoneScopedN("GPUScene::DispatchSparseUpload");
    SKR_LOG_DEBUG(u8"GPUScene: Dispatching SparseUpload - Core: %u ops, Additional: %u ops",
        (uint32_t)core_uploads.size(),
        (uint32_t)additional_uploads.size());

    // Import upload buffer that will be used by both passes
    auto upload_buffer_handle = graph->create_buffer(
        [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::BufferBuilder& builder) {
            builder.set_name(u8"upload_buffer")
                .import(upload_ctx.get(&g).upload_buffer, CGPU_RESOURCE_STATE_GENERIC_READ)
                .allow_shader_read();
        });

    // Pass 1: Core Data Upload
    if (!core_uploads.is_empty())
    {
        size_t core_ops_size = core_uploads.size() * sizeof(Upload);
        const uint32_t max_threads_per_op = 4;
        const uint32_t core_dispatch_groups = (core_uploads.size() * max_threads_per_op + 255) / 256;

        auto core_operations_buffer_handle = graph->create_buffer(
            [=](skr::render_graph::RenderGraph& g, skr::render_graph::BufferBuilder& builder) {
                builder.set_name(u8"core_upload_operations")
                    .size(core_ops_size)
                    .memory_usage(CGPU_MEM_USAGE_CPU_TO_GPU)
                    .as_upload_buffer()
                    .allow_shader_read()
                    .prefer_on_device();
            });

        graph->add_compute_pass(
            [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::ComputePassBuilder& builder) {
                builder.set_name(u8"CoreDataSparseUploadPass")
                    .set_pipeline(sparse_upload_pipeline)
                    .read(u8"upload_buffer", upload_buffer_handle)
                    .read(u8"upload_operations", core_operations_buffer_handle)
                    .readwrite(u8"target_buffer", scene_buffer);
            },
            [=, this, core_uploads = std::move(core_uploads)](skr::render_graph::RenderGraph& g, skr::render_graph::ComputePassContext& ctx) {
                // Upload core data operations
                if (auto mapped_ptr = static_cast<Upload*>(ctx.resolve(core_operations_buffer_handle)->info->cpu_mapped_address))
                {
                    memcpy(mapped_ptr, core_uploads.data(), core_ops_size);
                }

                struct SparseUploadConstants
                {
                    uint32_t num_operations;
                    uint32_t max_threads_per_op;
                    uint32_t alignment = 16;
                    uint32_t padding0 = 0;
                } constants;
                constants.num_operations = static_cast<uint32_t>(core_uploads.size());
                constants.max_threads_per_op = max_threads_per_op;

                cgpu_compute_encoder_push_constants(ctx.encoder, sparse_upload_root_signature, u8"constants", &constants);
                cgpu_compute_encoder_dispatch(ctx.encoder, core_dispatch_groups, 1, 1);
            });
    }
}

} // namespace skr::renderer