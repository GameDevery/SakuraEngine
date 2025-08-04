#include "SkrRenderer/gpu_scene.h"
#include "SkrRenderer/render_device.h"
#include "SkrGraphics/api.h"
#include "SkrCore/log.h"

namespace skr::renderer
{

void GPUScene::Initialize(CGPUDeviceId device, const GPUSceneConfig& cfg)
{
    SKR_LOG_INFO(u8"Initializing GPUScene...");

    // Validate configuration
    if (!cfg.world)
    {
        SKR_LOG_ERROR(u8"GPUScene: ECS World is null!");
        return;
    }

    if (!cfg.render_device)
    {
        SKR_LOG_ERROR(u8"GPUScene: RendererDevice is null!");
        return;
    }

    // Store configuration
    config = cfg;
    ecs_world = config.world;
    render_device = config.render_device;

    // 1. Initialize component type registry
    InitializeComponentTypes(config);

    // 2. Create core data buffer and calculate segments
    CreateCoreDataBuffer(device, config);

    // 3. Create additional data buffers
    CreateAdditionalDataBuffers(device, config);

    // 4. Initialize page allocator
    InitializePageAllocator(config);

    // 5. Create sparse upload compute pipeline
    CreateSparseUploadPipeline(device);

    SKR_LOG_INFO(u8"GPUScene initialized successfully");
    SKR_LOG_INFO(u8"  - Core data: %lldMB initial, %lldMB max",
        config.core_data_initial_size / (1024 * 1024),
        config.core_data_max_size / (1024 * 1024));
    SKR_LOG_INFO(u8"  - Additional data: %lldMB initial, %lldMB max",
        config.additional_data_initial_size / (1024 * 1024),
        config.additional_data_max_size / (1024 * 1024));
    SKR_LOG_INFO(u8"  - Registered %d core component types, %d additional component types",
        GetCoreComponentTypeCount(),
        GetAdditionalComponentTypeCount());
}

void GPUScene::InitializeComponentTypes(const GPUSceneConfig& config)
{
    SKR_LOG_DEBUG(u8"Initializing component type registry...");

    // Clear existing registry
    type_registry.clear();
    component_types.clear();

    // Register core data component types
    for (const auto& [cpu_type, gpu_type_id] : config.core_data_types)
    {
        GPUSceneComponentType component_type;
        component_type.cpu_type_guid = cpu_type;
        component_type.gpu_type_id = gpu_type_id;
        component_type.element_size = ecs_world->GetComponentSize(cpu_type);
        component_type.element_align = ecs_world->GetComponentAlign(cpu_type);
        component_type.name = ecs_world->GetComponentName(cpu_type);
        component_type.storage_class = GPUSceneComponentType::StorageClass::CORE_DATA;

        // Add to registry
        type_registry.add(cpu_type, component_type.gpu_type_id);
        component_types.push_back(component_type);

        SKR_LOG_INFO(u8"  Registered core component: %s (size: %d, align: %d, cpu_type_id:%d, gpu_type_id: %d)",
            component_type.name,
            component_type.element_size,
            component_type.element_align,
            component_type.cpu_type_guid,
            component_type.gpu_type_id);
    }

    // Register additional data component types
    for (const auto& [cpu_type, gpu_type_id] : config.additional_data_types)
    {
        GPUSceneComponentType component_type;
        component_type.cpu_type_guid = cpu_type;
        component_type.gpu_type_id = gpu_type_id;
        component_type.element_size = ecs_world->GetComponentSize(cpu_type);
        component_type.element_align = ecs_world->GetComponentAlign(cpu_type);
        component_type.name = ecs_world->GetComponentName(cpu_type);
        component_type.storage_class = GPUSceneComponentType::StorageClass::ADDITIONAL_DATA;

        // Add to registry
        type_registry.add(cpu_type, component_type.gpu_type_id);
        component_types.push_back(component_type);

        SKR_LOG_INFO(u8"  Registered additional component: %s (size: %d, align: %d, cpu_type_id: %d, gpu_type_id: %d)",
            component_type.name,
            component_type.element_size,
            component_type.element_align,
            component_type.cpu_type_guid,
            component_type.gpu_type_id);
    }

    SKR_LOG_DEBUG(u8"Component type registry initialized with %lld types", component_types.size());
}

void GPUScene::CreateCoreDataBuffer(CGPUDeviceId device, const GPUSceneConfig& config)
{
    SKR_LOG_DEBUG(u8"Creating core data buffer...");

    // Use Builder to create SOA allocator
    auto builder = SOASegmentBuffer::Builder(device)
        .with_size(config.core_data_initial_size, config.core_data_max_size)
        .allow_resize(config.enable_auto_resize);
    
    // Register core components
    for (const auto& component_type : component_types)
    {
        if (component_type.storage_class == GPUSceneComponentType::StorageClass::CORE_DATA)
        {
            builder.add_component(
                component_type.gpu_type_id,
                component_type.element_size,
                component_type.element_align
            );
            SKR_LOG_DEBUG(u8"  Added core component to SOASegmentBuffer: gpu_type_id=%u, size=%u, align=%u",
                component_type.gpu_type_id, component_type.element_size, component_type.element_align);
        }
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

void GPUScene::CreateAdditionalDataBuffers(CGPUDeviceId device, const GPUSceneConfig& config)
{
    SKR_LOG_DEBUG(u8"Creating additional data buffers...");

    // Initialize additional data region
    additional_data.buffer_size = config.additional_data_initial_size;
    additional_data.bytes_used = 0;

    // Create additional data buffer
    additional_data.buffer = CreateBuffer(device, u8"GPUScene-AdditionalData", config.additional_data_initial_size);
    if (!additional_data.buffer)
    {
        SKR_LOG_ERROR(u8"Failed to create additional data buffer!");
        return;
    }
    SKR_LOG_INFO(u8"  Additional data buffer created: %dMB", additional_data.buffer_size / (1024 * 1024));

    // Create page table buffer
    size_t page_table_size = config.initial_page_count * sizeof(GPUPageTableEntry);
    additional_data.page_table_buffer = CreateBuffer(device, u8"GPUScene-PageTable", page_table_size);
    if (!additional_data.page_table_buffer)
    {
        SKR_LOG_ERROR(u8"Failed to create page table buffer!");
        return;
    }

    // Initialize CPU page table mirror
    additional_data.page_table_cpu.resize_zeroed(config.initial_page_count);

    SKR_LOG_INFO(u8"  Page table buffer created: %d entries", config.initial_page_count);
}

void GPUScene::InitializePageAllocator(const GPUSceneConfig& config)
{
    SKR_LOG_DEBUG(u8"Initializing page allocator...");

    page_allocator.free_pages.clear();
    page_allocator.total_page_count = config.initial_page_count;
    page_allocator.next_new_page_id = 0;

    // Initially all pages are free
    for (uint32_t i = 0; i < config.initial_page_count; ++i)
    {
        page_allocator.free_pages.push_back(i);
    }

    SKR_LOG_INFO(u8"Page allocator initialized with %d free pages", page_allocator.free_pages.size());
}

void GPUScene::Shutdown()
{
    SKR_LOG_INFO(u8"Shutting down GPUScene...");

    // Shutdown allocators (will release buffers)
    core_data.shutdown();

    if (additional_data.buffer)
    {
        cgpu_free_buffer(additional_data.buffer);
        additional_data.buffer = nullptr;
    }

    if (additional_data.page_table_buffer)
    {
        cgpu_free_buffer(additional_data.page_table_buffer);
        additional_data.page_table_buffer = nullptr;
    }

    if (upload_ctx.upload_buffer)
    {
        cgpu_free_buffer(upload_ctx.upload_buffer);
        upload_ctx.upload_buffer = nullptr;
    }

    // Release SparseUpload pipeline resources
    if (sparse_upload_pipeline)
    {
        cgpu_free_compute_pipeline(sparse_upload_pipeline);
        sparse_upload_pipeline = nullptr;
    }

    if (sparse_upload_root_signature)
    {
        cgpu_free_root_signature(sparse_upload_root_signature);
        sparse_upload_root_signature = nullptr;
    }

    if (sparse_upload_shader)
    {
        cgpu_free_shader_library(sparse_upload_shader);
        sparse_upload_shader = nullptr;
    }

    // Clear data structures
    type_registry.clear();
    component_types.clear();
    archetype_registry.clear();
    pages.clear();

    ecs_world = nullptr;
    render_device = nullptr;

    SKR_LOG_INFO(u8"GPUScene shutdown complete");
}

// Helper method implementations
uint32_t GPUScene::GetCoreComponentTypeCount() const
{
    uint32_t count = 0;
    for (const auto& component_type : component_types)
    {
        if (component_type.storage_class == GPUSceneComponentType::StorageClass::CORE_DATA)
            count++;
    }
    return count;
}

uint32_t GPUScene::GetAdditionalComponentTypeCount() const
{
    uint32_t count = 0;
    for (const auto& component_type : component_types)
    {
        if (component_type.storage_class == GPUSceneComponentType::StorageClass::ADDITIONAL_DATA)
            count++;
    }
    return count;
}

const skr::Map<CPUTypeID, GPUComponentTypeID>& GPUScene::GetTypeRegistry() const
{
    return type_registry;
}

GPUComponentTypeID GPUScene::GetComponentTypeID(const CPUTypeID& type_guid) const
{
    auto found = type_registry.find(type_guid);
    return found ? found.value() : UINT16_MAX;
}

bool GPUScene::IsComponentTypeRegistered(const CPUTypeID& type_guid) const
{
    return type_registry.contains(type_guid);
}

const GPUSceneComponentType* GPUScene::GetComponentType(GPUComponentTypeID type_id) const
{
    for (const auto& component_type : component_types)
    {
        if (component_type.gpu_type_id == type_id)
            return &component_type;
    }
    return nullptr;
}

// Helper function implementations
CGPUBufferId GPUScene::CreateBuffer(CGPUDeviceId device, const char8_t* name, size_t size, ECGPUMemoryUsage usage)
{
    CGPUBufferDescriptor buffer_desc = {};
    buffer_desc.name = name;
    buffer_desc.size = size;
    buffer_desc.memory_usage = usage;
    buffer_desc.descriptors = CGPU_RESOURCE_TYPE_BUFFER_RAW | CGPU_RESOURCE_TYPE_RW_BUFFER_RAW;
    buffer_desc.flags = CGPU_BCF_DEDICATED_BIT;

    CGPUBufferId buffer = cgpu_create_buffer(device, &buffer_desc);
    if (!buffer)
    {
        SKR_LOG_ERROR(u8"Failed to create buffer: %s", name);
    }

    return buffer;
}

GPUScene::MemoryUsageInfo GPUScene::GetMemoryUsageInfo() const
{
    MemoryUsageInfo info;
    info.core_data_used_bytes = core_data.get_capacity_bytes();
    info.core_data_capacity_bytes = core_data.get_capacity_bytes();
    info.additional_data_used_bytes = additional_data.bytes_used;
    info.additional_data_capacity_bytes = additional_data.buffer_size;
    return info;
}

GPUScene::GPUAccessInfo GPUScene::GetGPUAccessInfo() const
{
    GPUAccessInfo info;
    info.core_data_buffer = core_data.get_buffer();
    info.additional_data_buffer = additional_data.buffer;
    info.page_table_buffer = additional_data.page_table_buffer;
    return info;
}

inline static void read_bytes(const char8_t* file_name, uint32_t** bytes, uint32_t* length)
{
    FILE* f = fopen((const char*)file_name, "rb");
    fseek(f, 0, SEEK_END);
    *length = ftell(f);
    fseek(f, 0, SEEK_SET);
    *bytes = (uint32_t*)malloc(*length);
    fread(*bytes, *length, 1, f);
    fclose(f);
}

inline static void read_shader_bytes(const char8_t* virtual_path, uint32_t** bytes, uint32_t* length, ECGPUBackend backend)
{
    char8_t shader_file[256];
    const char8_t* shader_path = SKR_UTF8("./../resources/shaders/");
    strcpy((char*)shader_file, (const char*)shader_path);
    strcat((char*)shader_file, (const char*)virtual_path);
    switch (backend)
    {
    case CGPU_BACKEND_VULKAN:
        strcat((char*)shader_file, (const char*)SKR_UTF8(".spv"));
        break;
    case CGPU_BACKEND_D3D12:
    case CGPU_BACKEND_XBOX_D3D12:
        strcat((char*)shader_file, (const char*)SKR_UTF8(".dxil"));
        break;
    default:
        break;
    }
    read_bytes(shader_file, bytes, length);
}

void GPUScene::CreateSparseUploadPipeline(CGPUDeviceId device)
{
    SKR_LOG_DEBUG(u8"Creating SparseUpload compute pipeline...");

    // 1. Create compute shader
    uint32_t *shader_bytes, shader_length;
    read_shader_bytes(u8"sparse_upload.sparse_upload", &shader_bytes, &shader_length, device->adapter->instance->backend);

    CGPUShaderLibraryDescriptor shader_desc = {
        .name = u8"SparseUploadComputeShader",
        .code = shader_bytes,
        .code_size = shader_length
    };
    sparse_upload_shader = cgpu_create_shader_library(device, &shader_desc);
    free(shader_bytes);

    if (!sparse_upload_shader)
    {
        SKR_LOG_ERROR(u8"Failed to create SparseUpload compute shader");
        return;
    }

    // 2. Create root signature
    const char8_t* push_constant_name = u8"constants";
    CGPUShaderEntryDescriptor compute_shader_entry = {
        .library = sparse_upload_shader,
        .entry = u8"sparse_upload",
        .stage = CGPU_SHADER_STAGE_COMPUTE
    };

    CGPURootSignatureDescriptor root_desc = {
        .shaders = &compute_shader_entry,
        .shader_count = 1,
        .push_constant_names = &push_constant_name,
        .push_constant_count = 1,
    };
    sparse_upload_root_signature = cgpu_create_root_signature(device, &root_desc);

    if (!sparse_upload_root_signature)
    {
        SKR_LOG_ERROR(u8"Failed to create SparseUpload root signature");
        return;
    }

    // 3. Create compute pipeline
    CGPUComputePipelineDescriptor pipeline_desc = {
        .root_signature = sparse_upload_root_signature,
        .compute_shader = &compute_shader_entry,
        .name = u8"SparseUploadPipeline"
    };
    sparse_upload_pipeline = cgpu_create_compute_pipeline(device, &pipeline_desc);

    if (!sparse_upload_pipeline)
    {
        SKR_LOG_ERROR(u8"Failed to create SparseUpload compute pipeline");
        return;
    }

    SKR_LOG_INFO(u8"SparseUpload compute pipeline created successfully");
}

} // namespace skr::renderer