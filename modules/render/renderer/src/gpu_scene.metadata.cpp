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

    // 0. Create sparse upload compute pipeline
    CreateSparseUploadPipeline(device);
    // 1. Initialize component type registry
    InitializeComponentTypes(config);
    // 2. Create core data buffer and calculate segments
    CreateDataBuffer(device, config);

    SKR_LOG_INFO(u8"GPUScene initialized successfully");
    SKR_LOG_INFO(u8"  - Core data: %lldMB initial, %lldMB max",
        config.initial_size / (1024 * 1024),
        config.max_size / (1024 * 1024));
}

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
    dirty_comp_count += 1;
    dirty_mtx.unlock();
}

void GPUScene::InitializeComponentTypes(const GPUSceneConfig& config)
{
    SKR_LOG_DEBUG(u8"Initializing component type registry...");

    // Clear existing registry
    type_registry.clear();
    component_types.clear();

    // Register core data component types
    for (const auto& [cpu_type, gpu_type_id] : config.types)
    {
        GPUSceneComponentType component_type;
        component_type.cpu_type_guid = cpu_type;
        component_type.gpu_type_id = gpu_type_id;
        component_type.element_size = ecs_world->GetComponentSize(cpu_type);
        component_type.element_align = ecs_world->GetComponentAlign(cpu_type);
        component_type.name = ecs_world->GetComponentName(cpu_type);

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

    SKR_LOG_DEBUG(u8"Component type registry initialized with %lld types", component_types.size());
}

void GPUScene::Shutdown()
{
    SKR_LOG_INFO(u8"Shutting down GPUScene...");

    // Shutdown allocators (will release buffers)
    core_data.shutdown();

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

    ecs_world = nullptr;
    render_device = nullptr;

    SKR_LOG_INFO(u8"GPUScene shutdown complete");
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

uint32_t GPUScene::GetComponentSegmentOffset(GPUComponentTypeID type_id) const
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

uint32_t GPUScene::GetInstanceStride() const
{
    // Total size of all core components per instance
    uint32_t stride = 0;
    for (const auto& info : core_data.get_infos())
    {
        stride += info.element_size;
    }
    return stride;
}

uint32_t GPUScene::GetComponentTypeCount() const
{
    uint32_t count = 0;
    for (const auto& component_type : component_types)
    {
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
    case CGPU_BACKEND_METAL:
        strcat((char*)shader_file, (const char*)SKR_UTF8(".metallib"));
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