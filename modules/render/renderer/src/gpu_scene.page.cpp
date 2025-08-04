#include "SkrRenderer/gpu_scene.h"
#include "SkrRenderer/render_device.h"
#include "SkrCore/log.h"
#include "SkrRT/ecs/world.hpp"
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
        if (auto archetype = pScene->EnsureArchetype(v.chunk->group->archetype))
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
    void run(skr::ecs::TaskContext& Context)
    {
        SkrZoneScopedN("GPUScene::AddEntityToGPUScene");
        
        sugoi_chunk_view_t v = {};
        sugoiS_access(pScene->ecs_world->get_storage(), (sugoi_entity_t)Context.entities()[0], &v);
        if (auto archetype = pScene->EnsureArchetype(v.chunk->group->archetype))
        {
            for (auto i = 0; i < Context.size(); i++)
            {
                auto entity = Context.entities()[i];
                auto archetype_id = archetype->gpu_archetype_id;
                auto& instance_data = instances[i];
                instance_data.entity = entity;
                instance_data.archetype_id = archetype_id;
                if (!archetype->free_ids.try_dequeue(instance_data.entity_index_in_archetype))
                {
                    instance_data.entity_index_in_archetype = archetype->latest_index++;
                }
                if (!archetype->free_ids.try_dequeue(instance_data.instance_index))
                {
                    instance_data.instance_index = pScene->latest_index++;
                }
                instance_data.custom_index = pScene->EncodeCustomIndex(archetype_id, instance_data.entity_index_in_archetype);
                instance_data.pArchetype = archetype.get();
            }
        }
    }
};

struct ScanGPUScene : public GPUSceneInstanceTask
{
    void run(skr::ecs::TaskContext& Context)
    {
        SkrZoneScopedN("GPUScene::ScanGPUScene");

        sugoi_chunk_view_t v = {};
        sugoiS_access(pScene->ecs_world->get_storage(), (sugoi_entity_t)Context.entities()[0], &v);
        // Step 1: Ensure archetype
        if (auto archetype = pScene->EnsureArchetype(v.chunk->group->archetype))
        {
            for (auto i = 0; i < Context.size(); i++)
            {
                auto entity = Context.entities()[i];
                auto archetype_id = archetype->gpu_archetype_id;
                
                // Step 2: Ensure allocation
                const auto& instance_data = instances[i];

                // Step3: do scan and copy (只处理 Core Data)
                for (auto [cpu_type, gpu_type] : pScene->type_registry)
                {
                    const auto data = Context.read(cpu_type).at(i);
                    if (!data) continue; // 组件不存在

                    const auto& component_info = pScene->component_types[gpu_type];
                    
                    // 只处理 Core Data，Additional Data 后续实现
                    if (component_info.storage_class == GPUSceneComponentType::StorageClass::CORE_DATA)
                    {
                        // 计算在目标缓冲区 SOA 布局中的偏移
                        uint64_t dst_offset = pScene->core_data.get_component_offset(gpu_type, instance_data.instance_index);
                        
                        // 分配 upload_buffer 中的位置
                        uint64_t src_offset = pScene->upload_cursor.fetch_add(component_info.element_size);
                        
                        // 拷贝数据到 upload_buffer
                        uint8_t* upload_ptr = static_cast<uint8_t*>(pScene->upload_buffer->info->cpu_mapped_address);
                        if (upload_ptr && src_offset + component_info.element_size <= pScene->upload_buffer->info->size)
                        {
                            memcpy(upload_ptr + src_offset, data, component_info.element_size);
                            
                            // 记录拷贝操作信息
                            GPUScene::Upload upload;
                            upload.src_offset = src_offset;
                            upload.dst_offset = dst_offset;
                            upload.data_size = component_info.element_size;
                            {
                                pScene->upload_mutex.lock();
                                pScene->uploads.push_back(upload);
                                pScene->upload_mutex.unlock();
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
    }
};

void GPUScene::AddEntity(skr::ecs::Entity entity)
{
    {
        add_mtx.lock();
        add_ents.add(entity);
        add_mtx.unlock();
    }
    for (auto [cpu_type, gpu_type] : type_registry)
    {
        RequireUpload(entity, cpu_type);
    }
}

void GPUScene::RemoveEntity(skr::ecs::Entity entity)
{
    remove_mtx.lock();
    remove_ents.add(entity);
    remove_mtx.unlock();
}

void GPUScene::RequireUpload(skr::ecs::Entity entity, CPUTypeID component)
{
    dirty_mtx.lock();
    auto cs = dirties.try_add_default(entity);
    cs.value().add_unique(component);
    if (!cs.already_exist())
        dirty_ents.add(entity);
    dirty_mtx.unlock();
}

uint64_t GPUScene::CalculateDirtySize() const
{
    uint64_t total_size = 0;
    // Iterate through all dirty entities
    for (const auto& [entity, dirty_components] : dirties)
    {
        // Sum up sizes of all dirty components for this entity
        for (const auto& cpu_type : dirty_components)
        {
            if (auto type_iter = type_registry.find(cpu_type))
            {
                auto gpu_type = type_iter.value();
                const auto& component_info = component_types[gpu_type];
                total_size += component_info.element_size;
            }
        }
    }
    return total_size;
}

void GPUScene::PrepareUploadBuffer() 
{
    SkrZoneScopedN("GPUScene::PrepareUploadBuffer");
    
    // Calculate required upload buffer size
    uint64_t required_size = CalculateDirtySize();
    if (required_size == 0)
        return;
        
    // Add some padding for alignment
    required_size = (required_size + 255) & ~255; // Align to 256 bytes
    
    // Check if we need to recreate the upload buffer
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
        upload_desc.flags = CGPU_BCF_PERSISTENT_MAP_BIT;
        upload_desc.descriptors = CGPU_RESOURCE_TYPE_BUFFER_RAW;
        upload_desc.memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU;
        upload_desc.size = required_size;
        
        upload_buffer = cgpu_create_buffer(render_device->get_cgpu_device(), &upload_desc);
        
        SKR_LOG_INFO(u8"GPUScene: Created upload buffer with size %llu bytes", required_size);
    }
}

void GPUScene::AdjustCoreBuffer(skr::render_graph::RenderGraph* graph) 
{
    SkrZoneScopedN("GPUScene::AdjustCoreBuffer");

    uint32_t existed_instances = GetInstanceCount();
    auto required_instances = instance_count = existed_instances + add_ents.size() - remove_ents.size();
    if (core_data.needs_resize(required_instances))
    {
        SKR_LOG_INFO(u8"GPUScene: Core data resize needed. Current: %u/%u instances",
                     required_instances, core_data.get_instance_capacity());
        
        // Calculate new capacity
        uint32_t new_capacity = static_cast<uint32_t>(required_instances * config.resize_growth_factor);
        
        // Resize and get old buffer
        auto old_buffer = core_data.resize(new_capacity);
        scene_buffer = core_data.import_buffer(graph, u8"scene_buffer");
        
        /*
        if (old_buffer && core_data.get_buffer())
        {
            // Import old buffer for copy source
            auto old_buffer_handle = graph->create_buffer(
                [=](skr::render_graph::RenderGraph& g, skr::render_graph::BufferBuilder& builder) {
                    builder.set_name(u8"old_scene_buffer")
                        .import(old_buffer, CGPU_RESOURCE_STATE_SHADER_RESOURCE);
                }
            );
            // Copy data from old to new buffer
            core_data.copy_segments(graph, old_buffer_handle, scene_buffer, existed_instances);
        }
        */
    }
    else
    {
        // Import scene buffer with proper state management
        scene_buffer = core_data.import_buffer(graph, u8"scene_buffer");
    }
}

void GPUScene::ExecuteUpload(skr::render_graph::RenderGraph* graph)
{
    SkrZoneScopedN("GPUScene::ExecuteUpload");

    // Ensure buffers are sized correctly
    AdjustCoreBuffer(graph);

    // Prepare upload buffer based on dirty size
    PrepareUploadBuffer();

    // Schedule remove & add & scan
    {
        SkrZoneScopedN("GPUScene::Tasks");
        if (!remove_ents.is_empty())
        {
            RemoveEntityFromGPUScene remove;
            remove.pScene = this;
            ecs_world->dispatch_task(remove, 512, remove_ents);
            remove_ents.clear();
        }
        if (!add_ents.is_empty())
        {
            AddEntityToGPUScene add;
            add.pScene = this;
            ecs_world->dispatch_task(add, 512, add_ents);
            add_ents.clear();
        }
        if (!dirty_ents.is_empty())
        {
            ScanGPUScene scan;
            scan.pScene = this;
            ecs_world->dispatch_task(scan, 512, dirty_ents);
            dirty_ents.clear();
            dirties.clear();
        }
        skr::ecs::TaskScheduler::Get()->sync_all();
    }


    // Dispatch SparseUpload compute shader to copy from upload_buffer to target buffers
    if (!uploads.is_empty())
    {
        DispatchSparseUpload(graph);
        uploads.clear();
        upload_cursor.store(0);
    }
}

skr::SP<GPUSceneArchetype> GPUScene::EnsureArchetype(sugoi::archetype_t* archetype)
{
    // Fast path: check if already registered
    {
        archetype_mutex.lock_shared();
        SKR_DEFER({ archetype_mutex.unlock_shared(); });
        auto iter = archetype_registry.find(archetype);
        if (auto iter = archetype_registry.find(archetype))
            return iter.value();
    }
    
    // Slow path: register new archetype
    archetype_mutex.lock();
    SKR_DEFER({ archetype_mutex.unlock(); });
    
    // Double-check after acquiring write lock
    if (auto iter = archetype_registry.find(archetype))
        return iter.value();
    
    // Create new archetype entry
    static std::atomic<GPUArchetypeID> next_archetype_id{0};
    GPUArchetypeID new_id = next_archetype_id.fetch_add(1);
    skr::Vector<GPUComponentTypeID> comps;
    for (uint32_t i = 0; i < archetype->type.length; ++i)
    {
        auto cpu_type = archetype->type.data[i];
        if (auto it = type_registry.find(cpu_type))
            comps.push_back(it.value());
    }

    auto new_archetype = skr::SP<GPUSceneArchetype>::New(archetype, new_id, std::move(comps));
    archetype_registry.add(archetype, new_archetype);
    SKR_LOG_INFO(u8"GPUScene: Registered new archetype %u (archetype: %p) with %u component types", 
        new_id, archetype, (uint32_t)new_archetype->component_types.size());

    return new_archetype;
}

void GPUScene::RemoveArchetype(sugoi::archetype_t* archetype)
{
    archetype_mutex.lock();
    SKR_DEFER({ archetype_mutex.unlock(); });
    
    archetype_registry.remove(archetype);
    SKR_LOG_DEBUG(u8"GPUScene: Removed archetype %p", archetype);
}

GPUPageID GPUScene::AllocateComponentPage(GPUArchetypeID archetype_id, GPUComponentTypeID component_type)
{
    // Find or create page for this component type within the archetype
    // For now, use simple page allocation
    GPUPageID page_id = page_allocator.AllocatePage();
    
    if (page_id >= pages.size())
    {
        // Expand page vector
        pages.resize_default(page_id + 1);
    }
    
    GPUScenePage& page = pages[page_id];
    page.archetype_id = archetype_id;
    page.component_type = component_type;
    page.entity_count = 0;
    page.entity_capacity = GPUScenePage::PAGE_SIZE / component_types[component_type].element_size;
    page.next_free_slot = 0;
    page.gpu_buffer_offset = page_id * GPUScenePage::PAGE_SIZE; // Simple linear layout
    page.generation++;
    return page_id;
}

GPUSceneCustomIndex GPUScene::EncodeCustomIndex(GPUArchetypeID archetype_id, uint32_t entity_index_in_archetype)
{
    SKR_ASSERT(archetype_id < 1024); // 10 bits
    SKR_ASSERT(entity_index_in_archetype < 16384); // 14 bits
    
    uint32_t packed = (archetype_id & 0x3FF) | ((entity_index_in_archetype & 0x3FFF) << 10);
    return GPUSceneCustomIndex(packed);
}

uint32_t GPUScene::GetCoreComponentSegmentOffset(GPUComponentTypeID type_id) const
{
    // Get the segment offset from SOASegmentBuffer
    const auto* segment = core_data.get_segment(type_id);
    if (segment)
    {
        return segment->buffer_offset;
    }
    SKR_LOG_WARN(u8"Component type %u not found in core data segments", type_id);
    return 0;
}

uint32_t GPUScene::GetCoreInstanceStride() const
{
    // Total size of all core components per instance
    uint32_t stride = 0;
    for (const auto& info : core_data.get_infos())
    {
        stride += info.element_size;
    }
    return stride;
}

void GPUScene::DispatchSparseUpload(skr::render_graph::RenderGraph* graph)
{
    if (uploads.is_empty())
        return;

    SkrZoneScopedN("GPUScene::DispatchSparseUpload");
    // Calculate batch size to stay under D3D12's 64KB constant buffer limit
    const size_t max_buffer_size = 65536; // 64KB
    const size_t upload_size = sizeof(Upload);
    const size_t max_ops_per_batch = (max_buffer_size - 256) / upload_size; // Leave some padding
    
    SKR_LOG_DEBUG(u8"GPUScene: Dispatching SparseUpload with %u total operations", (uint32_t)uploads.size());
    
    // Import buffers that will be used across all batches
    auto upload_buffer_handle = graph->create_buffer(
        [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::BufferBuilder& builder) {
            builder.set_name(u8"upload_buffer")
                .import(upload_buffer, CGPU_RESOURCE_STATE_UNDEFINED)
                .allow_shader_read();
        }
    );

    // Process uploads in batches to avoid exceeding D3D12 constant buffer size limit
    size_t total_operations = uploads.size();
    size_t operations_processed = 0;
    uint32_t batch_index = 0;

    while (operations_processed < total_operations)
    {
        // Calculate batch size
        size_t batch_size = skr::min(max_ops_per_batch, total_operations - operations_processed);
        size_t batch_ops_size = batch_size * sizeof(Upload);
        
        // Calculate dispatch groups for this batch
        const uint32_t max_threads_per_op = 64; // 64 threads per operation for parallelism
        const uint32_t batch_threads = batch_size * max_threads_per_op;
        const uint32_t dispatch_groups = (batch_threads + 255) / 256;
        
        SKR_LOG_DEBUG(u8"  Batch %u: %u operations, %u dispatch groups", batch_index, (uint32_t)batch_size, dispatch_groups);

        // Create operations buffer for this batch
        auto operations_buffer_handle = graph->create_buffer(
            [=](skr::render_graph::RenderGraph& g, skr::render_graph::BufferBuilder& builder) {
                builder.set_name(skr::format(u8"upload_operations_batch_{}", batch_index).u8_str())
                    .size(batch_ops_size)
                    .memory_usage(CGPU_MEM_USAGE_CPU_TO_GPU)
                    .structured(0, batch_size, sizeof(Upload))
                    .allow_shader_read()
                    .as_upload_buffer();
            }
        );
        
        // Add compute pass for this batch
        graph->add_compute_pass(
            // Setup function
            [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::ComputePassBuilder& builder) {
                builder.set_name(skr::format(u8"SparseUploadPass_Batch_{}", batch_index).u8_str())
                    .set_pipeline(sparse_upload_pipeline)
                    .read(u8"upload_buffer", upload_buffer_handle)
                    .read(u8"upload_operations", operations_buffer_handle)
                    .readwrite(u8"target_buffer", scene_buffer);
            },
            
            // Execute function
            [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::ComputePassContext& ctx) {
                // Upload operations data for this batch
                if (auto mapped_ptr = static_cast<Upload*>(ctx.resolve(operations_buffer_handle)->info->cpu_mapped_address))
                {
                    memcpy(mapped_ptr, uploads.data() + operations_processed, batch_ops_size);
                }
                
                // Prepare push constants
                struct SparseUploadConstants {
                    uint32_t num_operations;
                    uint32_t max_threads_per_op;
                    uint32_t alignment = 16;
                    uint32_t padding = 0;
                } constants;
                constants.num_operations = static_cast<uint32_t>(batch_size);
                constants.max_threads_per_op = max_threads_per_op;
                
                // Push constants to shader
                cgpu_compute_encoder_push_constants(ctx.encoder, 
                    sparse_upload_root_signature,
                    u8"constants", &constants);
                
                // Dispatch compute shader
                cgpu_compute_encoder_dispatch(ctx.encoder, dispatch_groups, 1, 1);
            }
        );
        
        operations_processed += batch_size;
        batch_index++;
    }
    
    SKR_LOG_INFO(u8"GPUScene: SparseUpload completed in %u batches", batch_index);
}

} // namespace skr::renderer