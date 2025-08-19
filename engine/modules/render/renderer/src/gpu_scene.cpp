#include "SkrCore/log.h"
#include "SkrRT/ecs/world.hpp"
#include "SkrRenderer/gpu_scene.h"
#include "SkrRenderer/render_device.h"
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
        for (auto [cpu_type, gpu_type] : pScene->GetTypeRegistry())
        {
            builder.optional_read(cpu_type);
        }
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
                if (!mesh.mesh_resource.is_resolved())
                {                    
                    pScene->AddEntity(entity);
                    continue;
                }

                auto& instance_data = instances[i];
                instance_data.entity = entity;
                if (pScene->free_ids.try_dequeue(instance_data.instance_index))
                    pScene->free_id_count -= 1;
                else
                    instance_data.instance_index = pScene->latest_index++;

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

                pScene->instance_count += 1;
                pScene->entity_ids[entity] = instance_data.instance_index;
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

        const auto& Lane = pScene->GetLaneForUpload();
        sugoi_chunk_view_t v = {};
        sugoiS_access(pScene->ecs_world->get_storage(), (sugoi_entity_t)Context.entities()[0], &v);
        {
            // compute batch count
            uint32_t soa_segments_op_count = 0;
            for (auto i = 0; i < Context.size(); i++)
            {
                const auto& instance_data = instances[i];
                if (instance_data.instance_index == ~0)
                    continue;

                for (auto [cpu_type, gpu_type] : pScene->type_registry)
                {
                    const auto data = Context.read(cpu_type).at(i);
                    if (!data)
                        continue;
                    const auto& component_info = pScene->component_types[gpu_type];
                    soa_segments_op_count++;
                }
            }

            auto batch_start = pUploadCounter->fetch_add(soa_segments_op_count);
            uint32_t local_index = 0;
            uint32_t additional_local_index = 0;

            for (auto i = 0; i < Context.size(); i++)
            {
                auto entity = Context.entities()[i];
                const auto& instance_data = instances[i];
                if (instance_data.instance_index == ~0)
                    continue;

                auto& dirties = Lane.dirties.find(entity).value();
                for (auto [cpu_type, gpu_type] : pScene->type_registry)
                {
                    if (!dirties.contains(cpu_type))
                        continue;

                    const auto data = Context.read(cpu_type).at(i);
                    if (!data) 
                        continue;

                    const auto& component_info = pScene->component_types[gpu_type];
                    const uint64_t dst_offset = pScene->soa_segments.get_component_offset(gpu_type, instance_data.instance_index);
                    const uint64_t src_offset = pScene->upload_ctxs.get(graph).upload_cursor.fetch_add(component_info.element_size);

                    if (src_offset + component_info.element_size <= pScene->upload_ctxs.get(graph).upload_buffer->info->size)
                    {
                        memcpy(DRAMCache->data() + src_offset, data, component_info.element_size);

                        auto upload_index = batch_start + local_index;
                        if (upload_index < pScene->upload_ctxs.get(graph).soa_segments_uploads.size())
                        {
                            GPUScene::Upload upload;
                            upload.src_offset = src_offset;
                            upload.dst_offset = dst_offset;
                            upload.data_size = component_info.element_size;
                            pScene->upload_ctxs.get(graph).soa_segments_uploads[upload_index] = upload;
                            local_index++;
                        }
                        else
                        {
                            SKR_LOG_ERROR(u8"GPUScene: data upload operations array overflow - index %u >= size %u",
                                upload_index,
                                (uint32_t)pScene->upload_ctxs.get(graph).soa_segments_uploads.size());
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
    ScanGPUScene(skr::render_graph::RenderGraph* g, skr::Vector<uint8_t>* DRAM, std::atomic_uint32_t* pUploadCounter)
        : graph(g)
        , DRAMCache(DRAM)
        , pUploadCounter(pUploadCounter)
    {
    }
    skr::render_graph::RenderGraph* graph;
    skr::Vector<uint8_t>* DRAMCache;
    std::atomic_uint32_t* pUploadCounter;
};

void GPUScene::AdjustBuffer(skr::render_graph::RenderGraph* graph)
{
    SkrZoneScopedN("GPUScene::AdjustBuffer");
    const auto& Lane = GetLaneForUpload();

    const auto existed_instances = tlas_instances.size();
    if (free_id_count < Lane.add_ents.size())
    {
        tlas_instances.resize_zeroed(tlas_instances.size() + Lane.add_ents.size());
    }
    const auto required_instances = tlas_instances.size();
    
    // Get current frame's buffer context for deferred destruction
    auto& current_buffer_ctx = frame_ctxs.get(graph);
    
    // First, clean up any buffer marked for discard from previous frame
    if (current_buffer_ctx.buffer_to_discard != nullptr)
    {
        cgpu_free_buffer(current_buffer_ctx.buffer_to_discard);
        current_buffer_ctx.buffer_to_discard = nullptr;
    }
    
    if (soa_segments.needs_resize(required_instances))
    {
        SKR_LOG_INFO(u8"GPUScene: GPUScene data resize needed. Current: %u/%u instances",
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

void GPUScene::PrepareUploadBuffer(skr::render_graph::RenderGraph* graph)
{
    SkrZoneScopedN("GPUScene::PrepareUploadBuffer");

    // Calculate required upload buffer size
    const auto& Lane = GetLaneForUpload();

    // Add some padding for alignment
    uint64_t required_size = (Lane.dirty_buffer_size + 255) & ~255; // Align to 256 bytes

    // Check if we need to recreate the upload buffer
    auto& upload_buffer = upload_ctxs.get(graph).upload_buffer;
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

void GPUScene::ExecuteUpload(skr::render_graph::RenderGraph* graph)
{
    SkrZoneScopedN("GPUScene::ExecuteUpload");
    SwitchLane();

    using namespace skr::render_graph;
    auto& Lane = GetLaneForUpload();
    if (Lane.dirty_buffer_size == 0)
        return;

    // Ensure buffers are sized correctly
    AdjustBuffer(graph);

    // Prepare upload buffer based on dirty size
    PrepareUploadBuffer(graph);

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
            AddEntityToGPUScene add;
            add.pScene = this;
            ecs_world->dispatch_task(add, get_batchsize(Lane.add_ents.size()), Lane.add_ents);
        }
        std::atomic_uint32_t UploadCounter;
        if (!Lane.dirty_ents.is_empty())
        {
            SkrZoneScopedN("GPUScene::ScanGPUScene");
            upload_ctx.DRAMCache.resize_unsafe(upload_ctx.upload_buffer->info->size);

            uint64_t total_dirty_count = Lane.dirty_comp_count.load();
            upload_ctx.soa_segments_uploads.resize_unsafe(total_dirty_count);

            ScanGPUScene scan(graph, &upload_ctx.DRAMCache, &UploadCounter);
            scan.pScene = this;
            ecs_world->dispatch_task(scan, get_batchsize(Lane.dirty_ents.size()), Lane.dirty_ents);
        }
        // TODO: FULL ASYNC
        {
            skr::ecs::TaskScheduler::Get()->sync_all();
        
            TLASUpdateRequest update_request;
            {
                CGPUAccelerationStructureId blas = nullptr;
                while (dirty_blases.try_dequeue(blas) && blas)
                {
                    update_request.blases_to_build.add_unique(blas);
                }
            }
            if (instance_count != 0)
            {
                CGPUAccelerationStructureDescriptor tlas_desc = {};
                tlas_desc.type = CGPU_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
                tlas_desc.flags = CGPU_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
                tlas_desc.top.count = instance_count;
                tlas_desc.top.instances = tlas_instances.data();
                update_request.tlas_desc = tlas_desc;
            }
            tlas_manager->Request(update_request);
        }
        {
            Lane.add_ents.clear();
            Lane.remove_ents.clear();
            Lane.dirty_ents.clear();
            Lane.dirty_comp_count = 0;
            Lane.dirties.clear();
            Lane.dirty_buffer_size = 0;

            ::memcpy(upload_ctx.upload_buffer->info->cpu_mapped_address, upload_ctx.DRAMCache.data(), upload_ctx.DRAMCache.size());
            uint32_t actual_core_uploads = UploadCounter.load();
            upload_ctx.soa_segments_uploads.resize_unsafe(actual_core_uploads);
        }

        // Import TLAS to RenderGraph if it exists
        auto& frame_ctx = frame_ctxs.get(graph);
        frame_ctx.frame_tlas = tlas_manager->GetLatestTLAS();
        if (frame_ctx.frame_tlas)
        {
            frame_ctx.tlas_handle = graph->create_acceleration_structure([&frame_ctx](RenderGraph& graph, class RenderGraph::AccelerationStructureBuilder& builder) 
            {
                builder.set_name(u8"GPUScene-TLAS")
                    .import(frame_ctx.frame_tlas.get());
            });
        }
    }

    // Dispatch SparseUpload compute shaders for both Core Data and Additional Data
    if (!upload_ctx.soa_segments_uploads.is_empty())
    {
        DispatchSparseUpload(graph, std::move(upload_ctx.soa_segments_uploads));
        upload_ctx.upload_cursor.store(0);
    }
}

void GPUScene::DispatchSparseUpload(skr::render_graph::RenderGraph* graph, skr::Vector<Upload>&& core_uploads)
{
    SkrZoneScopedN("GPUScene::DispatchSparseUpload");
    SKR_LOG_DEBUG(u8"GPUScene: Dispatching SparseUpload - Core: %u ops", (uint32_t)core_uploads.size());

    // Import upload buffer that will be used by both passes
    auto upload_buffer_handle = graph->create_buffer(
        [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::BufferBuilder& builder) {
            builder.set_name(u8"upload_buffer")
                .import(upload_ctxs.get(&g).upload_buffer, CGPU_RESOURCE_STATE_GENERIC_READ)
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

void GPUScene::AddEntity(skr::ecs::Entity entity)
{
    auto& Lane = GetFrontLane();
    {
        Lane.add_mtx.lock();
        Lane.add_ents.add(entity);
        Lane.add_mtx.unlock();
    }
    for (auto [cpu_type, gpu_type] : type_registry)
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
        
        free_ids.enqueue(instance_data->instance_index);
        free_id_count += 1;

        auto& tlas_instance = tlas_instances[instance_data->instance_index];
        memset(tlas_instance.transform, 0, sizeof(tlas_instance.transform));
        instance_count -= 1;

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
        if (auto type_iter = type_registry.find(component))
        {
            auto gpu_type = type_iter.value();
            const auto& component_info = component_types[gpu_type];
            Lane.dirty_buffer_size += component_info.element_size;
        }
        Lane.dirty_comp_count += 1;
    }

    Lane.dirty_mtx.unlock();
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
        SKR_LOG_ERROR(u8"GPUScene: RenderDevice is null!");
        return;
    }

    // Store configuration
    config = cfg;
    ecs_world = config.world;
    render_device = config.render_device;

    // 1. Create TLAS manager
    tlas_manager = TLASManager::Create(1 + RG_MAX_FRAME_IN_FLIGHT, render_device);
    // 2. Create sparse upload compute pipeline
    CreateSparseUploadPipeline(render_device->get_cgpu_device());
    // 3. Initialize component type registry
    InitializeComponentTypes(cfg);
    // 4. Create core data buffer
    soa_segments.initialize(soa_builder);

    SKR_LOG_INFO(u8"GPUScene initialized successfully");
}

void GPUScene::Shutdown()
{
    SKR_LOG_INFO(u8"Shutting down GPUScene...");

    // Shutdown allocators (will release buffers)
    soa_segments.shutdown();

    // Release upload buffers for all frames
    for (uint32_t i = 0; i < upload_ctxs.max_frames_in_flight(); ++i)
    {
        auto& ctx = upload_ctxs[i];
        if (ctx.upload_buffer)
        {
            cgpu_free_buffer(ctx.upload_buffer);
            ctx.upload_buffer = nullptr;
        }
    }

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

} // namespace skr