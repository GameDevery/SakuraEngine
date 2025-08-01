#include "SkrRenderer/gpu_scene.h"
#include "SkrRenderer/render_device.h"
#include "SkrCore/log.h"
#include "SkrRT/ecs/world.hpp"
#include "SkrProfile/profile.h"
#include <atomic>

namespace skr::renderer
{

void GPUScene::RequireUpload(skr::ecs::Entity entity, CPUTypeID component)
{
    auto cs = dirties.try_add_default(entity);
    cs.value().add_unique(component);
    if (!cs.already_exist())
        dirty_ents.add(entity);
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

void GPUScene::PrepareUploadContext() 
{
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
        upload_desc.name = u8"GPUScene_UploadBuffer";
        upload_desc.flags = CGPU_BCF_PERSISTENT_MAP_BIT;
        upload_desc.descriptors = CGPU_RESOURCE_TYPE_NONE;
        upload_desc.memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU;
        upload_desc.size = required_size;
        
        upload_buffer = cgpu_create_buffer(render_device->get_cgpu_device(), &upload_desc);
        
        SKR_LOG_INFO(u8"GPUScene: Created upload buffer with size %llu bytes", required_size);
    }
}

struct ScanGPUScene
{
    void build(skr::ecs::AccessBuilder& builder)
    {
        builder.write(&ScanGPUScene::instances);
        for (auto [cpu_type, gpu_type] : pScene->type_registry)
        {
            builder.optional_read(cpu_type);
        }
    }
    
    void run(skr::ecs::TaskContext& Context)
    {
        sugoi_chunk_view_t v = {};
        sugoiS_access(pScene->ecs_world->get_storage(), Context.entities()[0], &v);
        
        // Step 1: Ensure archetype
        if (auto archetype = pScene->EnsureArchetype(v.chunk->group->archetype))
        {
            for (auto i = 0; i < Context.size(); i++)
            {
                auto entity = Context.entities()[i];
                auto archetype_id = archetype->gpu_archetype_id;
                
                // Step 2: Ensure allocation
                auto& instance_data = instances[entity];
                if (instance_data.instance_index == INVALID_GPU_SCENE_INSTANCE_ID)
                {
                    // Allocate new instance
                    instance_data.entity = entity;
                    instance_data.archetype_id = archetype_id;
                    instance_data.entity_index_in_archetype = archetype->total_entity_count++;
                    instance_data.instance_index = pScene->AllocateCoreInstance();
                    instance_data.custom_index = pScene->EncodeCustomIndex(archetype_id, instance_data.entity_index_in_archetype);
                    
                    if (instance_data.instance_index == INVALID_GPU_SCENE_INSTANCE_ID)
                    {
                        SKR_LOG_ERROR(u8"GPUScene: Failed to allocate instance");
                        return;
                    }
                }

                // Step3: do scan and copy
                for (auto [cpu_type, gpu_type] : pScene->type_registry)
                {
                    const auto data = Context.read(cpu_type).at(i);
                    
                }
            }
        }
    }

    skr::renderer::GPUScene* pScene = nullptr;
    skr::ecs::ComponentView<GPUSceneInstance> instances;
};

void GPUScene::ExecuteUpload(skr::render_graph::RenderGraph* graph)
{
    SkrZoneScopedN("GPUScene::ExecuteUpload");

    // Ensure buffers are sized correctly
    AdjustDataBuffers();

    // Prepare upload buffer based on dirty size
    PrepareUploadContext();

    // Simple 3-step process: 
    // 1. Ensure archetype
    // 2. Ensure allocation 
    // 3. Submit upload
    ScanGPUScene scan;
    scan.pScene = this;
    ecs_world->dispatch_task(scan, 512, dirty_ents);
    skr::ecs::TaskScheduler::Get()->sync_all();

    // Clear dirty tracking
    dirty_ents.clear();
    dirties.clear();
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
    
    auto new_archetype = skr::SP<GPUSceneArchetype>::New();
    new_archetype->cpu_archetype = archetype;
    new_archetype->gpu_archetype_id = new_id;
    new_archetype->total_entity_count = 0;
    new_archetype->allocated_core_instances = 0;
    
    for (uint32_t i = 0; i < archetype->type.length; ++i)
    {
        auto cpu_type = archetype->type.data[i];
        if (auto it = type_registry.find(cpu_type))
            new_archetype->component_types.push_back(it.value());
    }
    
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

GPUSceneInstanceID GPUScene::AllocateCoreInstance()
{
    // Simple linear allocation in core data
    auto& core_data = data_pool.core_data;
    
    if (core_data.instance_count >= core_data.instance_capacity)
    {
        // Need to resize - this will be handled by AdjustDataBuffers
        SKR_LOG_WARN(u8"GPUScene: Core data buffer full, resize needed");
        return INVALID_GPU_SCENE_INSTANCE_ID;
    }
    
    uint32_t instance_id = core_data.instance_count++;
    return static_cast<GPUSceneInstanceID>(instance_id);
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

uint32_t GPUScene::GetCoreComponentOffset(GPUComponentTypeID type_id) const
{
    // Calculate offset within core instance based on component layout
    uint32_t offset = 0;
    for (const auto& segment : data_pool.core_data.component_segments)
    {
        if (segment.type_id == type_id)
            return offset;
        offset += segment.element_size;
    }
    return 0;
}

uint32_t GPUScene::GetCoreInstanceStride() const
{
    // Total size of all core components per instance
    uint32_t stride = 0;
    for (const auto& segment : data_pool.core_data.component_segments)
    {
        stride += segment.element_size;
    }
    return stride;
}

} // namespace skr::renderer