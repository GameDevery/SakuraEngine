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
        {
            for (auto i = 0; i < Context.size(); i++)
            {
                auto entity = Context.entities()[i];
                auto& instance_data = instances[i];
                instance_data.entity = entity;
                instance_data.instance_index = pScene->latest_index++;
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
        {
            // 预先计算此批次需要的上传操作数量，分类统计
            uint32_t core_data_op_count = 0;
            for (auto i = 0; i < Context.size(); i++)
            {
                for (auto [cpu_type, gpu_type] : pScene->type_registry)
                {
                    const auto data = Context.read(cpu_type).at(i);
                    if (!data)
                        continue;
                    const auto& component_info = pScene->component_types[gpu_type];
                    core_data_op_count++;
                }
            }
            
            // 为Core Data和Additional Data分别分配索引范围
            auto core_batch_start = pCoreCounter->fetch_add(core_data_op_count);
            uint32_t core_local_index = 0;
            uint32_t additional_local_index = 0;
            
            for (auto i = 0; i < Context.size(); i++)
            {
                auto entity = Context.entities()[i];
                const auto& instance_data = instances[i];
                for (auto [cpu_type, gpu_type] : pScene->type_registry)
                {
                    const auto data = Context.read(cpu_type).at(i);
                    if (!data) continue;

                    const auto& component_info = pScene->component_types[gpu_type];
                    // Core Data处理
                    const uint64_t dst_offset = pScene->core_data.get_component_offset(gpu_type, instance_data.instance_index);
                    const uint64_t src_offset = pScene->upload_ctx.upload_cursor.fetch_add(component_info.element_size);
                    if (src_offset + component_info.element_size <= pScene->upload_ctx.upload_buffer->info->size)
                    {
                        memcpy(DRAMCache->data() + src_offset, data, component_info.element_size);
                        
                        // 使用预分配的连续索引范围，添加到Core Data上传列表
                        auto upload_index = core_batch_start + core_local_index;
                        if (upload_index < pScene->upload_ctx.core_data_uploads.size())
                        {
                            GPUScene::Upload upload;
                            upload.src_offset = src_offset;
                            upload.dst_offset = dst_offset;
                            upload.data_size = component_info.element_size;
                            pScene->upload_ctx.core_data_uploads[upload_index] = upload;
                            core_local_index++;
                        }
                        else
                        {
                            SKR_LOG_ERROR(u8"GPUScene: Core Data upload operations array overflow - index %u >= size %u", 
                                            upload_index, (uint32_t)pScene->upload_ctx.core_data_uploads.size());
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
    ScanGPUScene(skr::Vector<uint8_t>* DRAM, std::atomic_uint32_t* pCoreCounter, std::atomic_uint32_t* pAdditionalCounter)
        : DRAMCache(DRAM), pCoreCounter(pCoreCounter), pAdditionalCounter(pAdditionalCounter)
    {

    }
    skr::Vector<uint8_t>* DRAMCache;
    std::atomic_uint32_t* pCoreCounter;
    std::atomic_uint32_t* pAdditionalCounter;
};

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
    auto& upload_buffer = upload_ctx.upload_buffer;
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
    if (core_data.needs_resize(required_instances))
    {
        SKR_LOG_INFO(u8"GPUScene: Core data resize needed. Current: %u/%u instances",
                     required_instances, core_data.get_instance_capacity());
        
        // Calculate new capacity
        uint32_t new_capacity = static_cast<uint32_t>(required_instances * config.resize_growth_factor);
        
        // Resize and get old buffer
        auto old_buffer = core_data.resize(new_capacity);
        scene_buffer = core_data.import_buffer(graph, u8"scene_buffer");
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
    }
    else
    {
        // Import scene buffer with proper state management
        scene_buffer = core_data.import_buffer(graph, u8"scene_buffer");
    }
}


void GPUScene::CreateDataBuffer(CGPUDeviceId device, const GPUSceneConfig& config)
{
    SKR_LOG_DEBUG(u8"Creating core data buffer...");

    // Calculate initial size based on instances and page size
    size_t page_stride = 0;
    for (const auto& comp : config.components)
    {
        page_stride = (page_stride + comp.element_align - 1) & ~(comp.element_align - 1);
        page_stride += comp.element_size * config.page_size;
    }
    page_stride = (page_stride + 255) & ~255;  // Align to 256 bytes
    
    uint32_t initial_pages = (config.initial_instances + config.page_size - 1) / config.page_size;
    uint32_t max_pages = (config.max_instances + config.page_size - 1) / config.page_size;
    size_t initial_size = page_stride * initial_pages;
    size_t max_size = page_stride * max_pages;

    // Use Builder to create SOA allocator
    auto builder = SOASegmentBuffer::Builder(device)
        .with_size(initial_size, max_size)
        .with_page_size(config.page_size);
    
    // Register core components
    for (const auto& component_type : component_types)
    {
        builder.add_component(
            component_type.soa_index,
            component_type.element_size,
            component_type.element_align
        );
        SKR_LOG_DEBUG(u8"  Added core component to SOASegmentBuffer: local_id=%u, size=%u, align=%u",
            component_type.soa_index, component_type.element_size, component_type.element_align);
    }
    
    // Build the allocator
    core_data.initialize(builder);
    
    if (!core_data.get_buffer())
    {
        SKR_LOG_ERROR(u8"Failed to create core data buffer!");
        return;
    }

    SKR_LOG_INFO(u8"Core data buffer created: %lldMB, %d segments, %d instances capacity",
        core_data.get_capacity_bytes() / (1024 * 1024),
        core_data.get_segments().size(),
        core_data.get_instance_capacity());
    
    // 打印实际注册的组件类型
    const auto& infos = core_data.get_infos();
    SKR_LOG_INFO(u8"Registered %u core components in SOASegmentBuffer:", (uint32_t)infos.size());
    for (size_t i = 0; i < infos.size(); ++i)
    {
        SKR_LOG_INFO(u8"  Component[%u]: type_id=%u, size=%u, align=%u",
            (uint32_t)i, infos[i].type_id, infos[i].element_size, infos[i].element_align);
    }
}

void GPUScene::ExecuteUpload(skr::render_graph::RenderGraph* graph)
{
    SkrZoneScopedN("GPUScene::ExecuteUpload");

    // Ensure buffers are sized correctly
    AdjustBuffer(graph);

    // Prepare upload buffer based on dirty size
    PrepareUploadBuffer();

    // Schedule remove & add & scan
    auto get_batchsize = +[](uint64_t ecount) { return std::max(ecount / 8ull, 1024ull); };
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
            upload_ctx.DRAMCache.resize_unsafe(upload_ctx.upload_buffer->info->size);
            
            uint64_t total_dirty_count = dirty_comp_count.load();
            upload_ctx.core_data_uploads.resize_unsafe(total_dirty_count);
            upload_ctx.additional_data_uploads.resize_unsafe(total_dirty_count);

            ScanGPUScene scan(&upload_ctx.DRAMCache, &CoreUploadCounter, &AdditionalUploadCounter);
            scan.pScene = this;
            ecs_world->dispatch_task(scan, get_batchsize(dirty_ents.size()), dirty_ents);
        }
        // TODO: FULL ASYNC 
        {
            skr::ecs::TaskScheduler::Get()->sync_all();

            ::memcpy(upload_ctx.upload_buffer->info->cpu_mapped_address, upload_ctx.DRAMCache.data(), upload_ctx.DRAMCache.size());
            remove_ents.clear();
            add_ents.clear();
            dirty_ents.clear();
            dirties.clear();
            
            // 调整上传列表大小到实际使用的大小
            uint32_t actual_core_uploads = CoreUploadCounter.load();
            uint32_t actual_additional_uploads = AdditionalUploadCounter.load();
            upload_ctx.core_data_uploads.resize_unsafe(actual_core_uploads);
            upload_ctx.additional_data_uploads.resize_unsafe(actual_additional_uploads);
            
            dirty_comp_count -= (actual_core_uploads + actual_additional_uploads);
        }
    }

    // Dispatch SparseUpload compute shaders for both Core Data and Additional Data
    if (!upload_ctx.core_data_uploads.is_empty() || !upload_ctx.additional_data_uploads.is_empty())
    {
        DispatchSparseUpload(graph, std::move(upload_ctx.core_data_uploads), std::move(upload_ctx.additional_data_uploads));
        upload_ctx.upload_cursor.store(0);
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
                .import(upload_ctx.upload_buffer, CGPU_RESOURCE_STATE_GENERIC_READ)
                .allow_structured_read();
        }
    );

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
                    .allow_raw_read()
                    .prefer_on_device();
            }
        );
        
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
                
                struct SparseUploadConstants {
                    uint32_t num_operations;
                    uint32_t max_threads_per_op;
                    uint32_t alignment = 16;
                    uint32_t padding0 = 0;
                } constants;
                constants.num_operations = static_cast<uint32_t>(core_uploads.size());
                constants.max_threads_per_op = max_threads_per_op;
                
                cgpu_compute_encoder_push_constants(ctx.encoder, sparse_upload_root_signature, u8"constants", &constants);
                cgpu_compute_encoder_dispatch(ctx.encoder, core_dispatch_groups, 1, 1);
            }
        );
    }
}

} // namespace skr::renderer