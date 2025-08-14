#include "SkrRenderer/gpu_scene.h"
#include "SkrRenderer/render_device.h"
#include "SkrGraphics/api.h"
#include "SkrCore/log.h"

namespace skr::renderer
{

void GPUScene::Initialize(const GPUSceneConfig& cfg, const SOASegmentBuffer::Builder& soa_builder)
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
    CreateSparseUploadPipeline(render_device->get_cgpu_device());
    // 1. Initialize component type registry from config
    InitializeComponentTypes(cfg);
    // 2. Create core data buffer using provided SOA builder
    soa_segments.initialize(soa_builder);

    SKR_LOG_INFO(u8"GPUScene initialized successfully");
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

    // Register components from config
    for (const auto& comp_info : config.components)
    {
        GPUSceneComponentType component_type;
        component_type.cpu_type_guid = comp_info.cpu_type;
        component_type.soa_index = comp_info.soa_index;
        component_type.element_size = comp_info.element_size;
        component_type.element_align = comp_info.element_align;
        component_type.name = ecs_world->GetComponentName(comp_info.cpu_type);

        // Add to registry
        type_registry.add(comp_info.cpu_type, comp_info.soa_index);
        component_types.push_back(component_type);

        SKR_LOG_DEBUG(u8"  Registered component: %s (size: %d, align: %d, cpu_type:%u, soa_index: %u)",
            component_type.name,
            component_type.element_size,
            component_type.element_align,
            component_type.cpu_type_guid,
            component_type.soa_index);
    }

    SKR_LOG_DEBUG(u8"Component type registry initialized with %lld types", component_types.size());
}

void GPUScene::Shutdown()
{
    SKR_LOG_INFO(u8"Shutting down GPUScene...");

    // Shutdown allocators (will release buffers)
    soa_segments.shutdown();

    // Release upload buffers for all frames
    for (uint32_t i = 0; i < upload_ctx.max_frames_in_flight(); ++i)
    {
        auto& ctx = upload_ctx[i];
        if (ctx.upload_buffer)
        {
            cgpu_free_buffer(ctx.upload_buffer);
            ctx.upload_buffer = nullptr;
        }
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

const skr::Map<CPUTypeID, SOAIndex>& GPUScene::GetTypeRegistry() const
{
    return type_registry;
}

SOAIndex GPUScene::GetComponentSOAIndex(const CPUTypeID& type_guid) const
{
    auto found = type_registry.find(type_guid);
    return found ? found.value() : UINT16_MAX;
}

bool GPUScene::IsComponentTypeRegistered(const CPUTypeID& type_guid) const
{
    return type_registry.contains(type_guid);
}

const GPUSceneComponentType* GPUScene::GetComponentType(SOAIndex soa_index) const
{
    for (const auto& component_type : component_types)
    {
        if (component_type.soa_index == soa_index)
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