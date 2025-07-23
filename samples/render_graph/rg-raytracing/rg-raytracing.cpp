#include "SkrCore/module/module_manager.hpp"
#include "SkrRT/ecs/world.hpp"
#include "SkrTask/fib_task.hpp"
#include "SkrScene/transform_system.h"
#include "SkrSystem/system_app.h"
#include "SkrScene/scene_components.h"
#include "SkrGraphics/api.h"
#include "SkrCore/log.hpp"
#include "SkrGraphics/raytracing.h"
#include <random>
#include <cmath>

extern "C"
{
#define LODEPNG_NO_COMPILE_CPP
#include "lodepng.h"
}

class RGRaytracingSampleModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

public:
    static RGRaytracingSampleModule* Get();

    void spawn_entities();
    void create_as();
    void create_sphere_blas();
    void create_scene_tlas();
    void create_compute_pipeline();
    void create_swapchain(skr::SystemWindow* window);
    void render();
    
    RGRaytracingSampleModule()
        : world(scheduler)
    {

    }
    skr::SystemApp app;
    skr::ecs::World world;
    skr::task::scheduler_t scheduler;
    skr::TransformSystem* transform_system = nullptr;

    // Raytracing resources
    CGPUInstanceId instance = nullptr;
    CGPUDeviceId device = nullptr;
    CGPUQueueId gfx_queue = nullptr;
    CGPUAccelerationStructureId sphere_blas = nullptr;
    CGPUAccelerationStructureId scene_tlas = nullptr;
    CGPUBufferId sphere_vertex_buffer = nullptr;
    CGPUBufferId sphere_index_buffer = nullptr;
    
    // Compute pipeline resources
    CGPUShaderLibraryId compute_shader = nullptr;
    CGPURootSignatureId root_signature = nullptr;
    CGPUComputePipelineId compute_pipeline = nullptr;
    CGPUDescriptorSetId descriptor_set = nullptr;
    CGPUBufferId output_buffer = nullptr;
    CGPUBufferId readback_buffer = nullptr;
    
    // Swapchain resources for profiler hook
    CGPUSwapChainId swapchain = nullptr;
    
    // Render constants
    static constexpr uint32_t RENDER_WIDTH = 1920;
    static constexpr uint32_t RENDER_HEIGHT = 1080;
};

IMPLEMENT_DYNAMIC_MODULE(RGRaytracingSampleModule, RenderGraphRaytracingSample);

// Helper function to read shader bytes
inline static void read_shader_bytes(const char* virtual_path, uint32_t** bytes, uint32_t* length, ECGPUBackend backend)
{
    char shader_file[256];
    const char* shader_path = "./../resources/shaders/";
    strcpy(shader_file, shader_path);
    strcat(shader_file, virtual_path);
    switch (backend)
    {
        case CGPU_BACKEND_VULKAN:
            strcat(shader_file, ".spv");
            break;
        case CGPU_BACKEND_D3D12:
        case CGPU_BACKEND_XBOX_D3D12:
            strcat(shader_file, ".dxil");
            break;
        case CGPU_BACKEND_METAL:
            strcat(shader_file, ".metallib");
            break;
        default:
            SKR_UNIMPLEMENTED_FUNCTION();
            break;
    }
    
    FILE* f = fopen(shader_file, "rb");
    if (!f) {
        SKR_LOG_ERROR(u8"Failed to open shader file: %s", shader_file);
        return;
    }
    fseek(f, 0, SEEK_END);
    *length = ftell(f);
    fseek(f, 0, SEEK_SET);
    *bytes = (uint32_t*)malloc(*length + 1);
    fread(*bytes, *length, 1, f);
    fclose(f);
}

RGRaytracingSampleModule* RGRaytracingSampleModule::Get()
{
    auto mm = skr_get_module_manager();
    static auto rm = static_cast<RGRaytracingSampleModule*>(mm->get_module(u8"RenderGraphRaytracingSample"));
    return rm;
}

void RGRaytracingSampleModule::on_load(int argc, char8_t** argv)
{
    scheduler.initialize({});
    scheduler.bind();
    world.initialize();
    transform_system = skr_transform_system_create(&world);

    spawn_entities();
    create_as();
}

int RGRaytracingSampleModule::main_module_exec(int argc, char8_t** argv) 
{
    app.initialize(nullptr);
    auto wm = app.get_window_manager();
    auto eq = app.get_event_queue();
    auto main_window = wm->create_window({
        .title = u8"Render Graph Raytracing Sample",
        .size = { 1280, 720 },
        .is_resizable = false
    });
    main_window->show();
    SKR_DEFER({ wm->destroy_window(main_window); });

    // Create swapchain for profiler hook support
    create_swapchain(main_window);

    static bool want_quit = false;
    struct QuitListener : public skr::ISystemEventHandler
    {
        void handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT
        {
            if (event.type == SKR_SYSTEM_EVENT_QUIT)
                want_quit = true;
            if (event.type == SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED)
                want_quit = event.window.window_native_handle == main_window->get_native_handle();
        }
        skr::SystemWindow* main_window = nullptr;
    } quitListener;
    quitListener.main_window = main_window;
    eq->add_handler(&quitListener);
    SKR_DEFER({ eq->remove_handler(&quitListener); });

    while (!want_quit)
    {
        eq->pump_messages();
        render();
    }

    return 0;
}

void RGRaytracingSampleModule::spawn_entities()
{
    using namespace skr::ecs;
    
    // Scene dimensions: 2000x2000x2000 units 
    constexpr float SCENE_SIZE = 2000.0f;
    constexpr int TOTAL_ENTITIES = 50000;
    
    // Three attachment levels hierarchy
    constexpr int LEVEL_1_COUNT = 100;    // Root level: 100 entities (cities)
    constexpr int LEVEL_2_COUNT = 900;    // Mid level: 900 entities (buildings) - 9 per city
    constexpr int LEVEL_3_COUNT = 45000;  // Leaf level: 45000 entities (objects) - ~50 per building

    static std::atomic_uint32_t entities_count = 0;
    struct LevelSpawner
    {
        void build(skr::ecs::ArchetypeBuilder& Builder)
        {
            Builder.add_component<skr::scene::TransformComponent>()
                .add_component(&LevelSpawner::children)
                .add_component(&LevelSpawner::translations)
                .add_component(&LevelSpawner::rotations)
                .add_component(&LevelSpawner::scales)
                .add_component(&LevelSpawner::indices);
        }

        skr::Vector<Entity> ents;

        ComponentView<skr::scene::ChildrenComponent> children;
        ComponentView<skr::scene::TranslationComponent> translations;
        ComponentView<skr::scene::RotationComponent> rotations;
        ComponentView<skr::scene::ScaleComponent> scales;
        ComponentView<skr::scene::IndexComponent> indices;
    };

    // Level 1 spawner (Cities) - Root entities without parents
    struct Level1Spawner : public LevelSpawner
    {
        void run(skr::ecs::TaskContext& Context)
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> pos_dist(-SCENE_SIZE * 0.5f, SCENE_SIZE * 0.5f);
            std::uniform_real_distribution<float> scale_dist(0.5f, 3.0f);
            std::uniform_real_distribution<float> rotation_dist(0.0f, 2.0f * skr::kPi);

            ents.append(Context.entities(), Context.size());
            for (int i = 0; i < Context.size(); ++i)
            {
                // Position cities in a grid with some randomness
                int grid_x = i % 10;
                int grid_z = i / 10;
                float base_x = (grid_x - 4.5f) * (SCENE_SIZE * 0.9f / 10.0f);
                float base_z = (grid_z - 4.5f) * (SCENE_SIZE * 0.9f / 10.0f);
                
                indices[i].value = entities_count++;
                auto final_pos = skr_float3_t{
                    base_x + pos_dist(gen) * 0.1f,
                    0.0f,
                    base_z + pos_dist(gen) * 0.1f
                };
                translations[i].value = final_pos;
                rotations[i].euler = skr::RotatorF{ 0.0f, rotation_dist(gen), 0.0f };
                float city_scale = scale_dist(gen) * 4.0f; // Moderate scale
                scales[i].value = skr_float3_t{ city_scale, city_scale, city_scale };
            
                // Debug: print first few city positions
                if (i < 10) {
                    SKR_LOG_FMT_INFO(u8"City {}: position ({}, {}, {}), scale ({}, {}, {})", 
                                i, final_pos.x, final_pos.y, final_pos.z,
                                city_scale, city_scale, city_scale);
                }
            }
        }
    } level1_spawner;

    // Level 2 spawner (Buildings) - With parent relationships
    struct Level2Spawner : public LevelSpawner
    {
        const Level1Spawner& lv1;
        Level2Spawner(const Level1Spawner& lv1)
            : lv1(lv1)
        {

        }

        void build(skr::ecs::ArchetypeBuilder& Builder)
        {
            LevelSpawner::build(Builder);
            Builder.add_component(&Level2Spawner::parents)
                .access(&Level2Spawner::children_writer);
        }

        void run(skr::ecs::TaskContext& Context)
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> pos_dist(-SCENE_SIZE * 0.5f, SCENE_SIZE * 0.5f);
            std::uniform_real_distribution<float> scale_dist(0.5f, 3.0f);
            std::uniform_real_distribution<float> rotation_dist(0.0f, 2.0f * skr::kPi);

            ents.append(Context.entities(), Context.size());
            for (int i = 0; i < Context.size(); ++i)
            {
                // Position buildings around city centers
                float angle = (i % 9) * (2.0f * skr::kPi / 9.0f) + rotation_dist(gen) * 0.1f;
                float radius = 100.0f + pos_dist(gen) * 0.1f;
                
                indices[i].value = entities_count++;
                translations[i].value = skr_float3_t{
                    std::cos(angle) * radius,
                    pos_dist(gen) * 10.0f,
                    std::sin(angle) * radius
                };

                rotations[i].euler = skr::RotatorF{ 0.0f, rotation_dist(gen), 0.0f };

                float building_scale = scale_dist(gen) * 2.0f; // Medium scale for buildings
                scales[i].value = skr_float3_t{ building_scale, building_scale, building_scale };
                
                const auto parent = lv1.ents[index_in_level / 9];
                skr::scene::ChildrenComponent as_child = { .entity = Context.entities()[i] };
                parents[i].entity = parent; // Attach to the corresponding city
                children_writer[parent].push_back(as_child); // Add this building to the city's children
                index_in_level += 1;
            }
        }
        uint64_t index_in_level = 0;
        ComponentView<skr::scene::ParentComponent> parents;
        RandomComponentReadWrite<skr::scene::ChildrenComponent> children_writer;
    } level2_spawner(level1_spawner);

    // Level 3 spawner (Objects) - Leaf entities
    struct Level3Spawner : public LevelSpawner
    {
        const Level2Spawner& lv2;
        Level3Spawner(const Level2Spawner& lv2)
            : lv2(lv2)
        {

        }

        void build(skr::ecs::ArchetypeBuilder& Builder)
        {
            LevelSpawner::build(Builder);
            Builder.add_component(&Level3Spawner::parents)
                .access(&Level2Spawner::children_writer);
        }

        void run(skr::ecs::TaskContext& Context)
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> pos_dist(-SCENE_SIZE * 0.5f, SCENE_SIZE * 0.5f);
            std::uniform_real_distribution<float> scale_dist(0.5f, 3.0f);
            std::uniform_real_distribution<float> rotation_dist(0.0f, 2.0f * skr::kPi);

            ents.append(Context.entities(), Context.size());
            for (int i = 0; i < Context.size(); ++i)
            {
                // Distribute objects around buildings
                float local_radius = 50.0f;
                float local_angle = rotation_dist(gen);
                float local_distance = pos_dist(gen) * 0.01f * local_radius;
                
                indices[i].value = entities_count++;
                translations[i].value = skr_float3_t{
                    std::cos(local_angle) * local_distance,
                    pos_dist(gen) * 5.0f,
                    std::sin(local_angle) * local_distance
                };

                rotations[i].euler = skr::RotatorF{
                    rotation_dist(gen) * 0.2f,  // Small pitch variation
                    rotation_dist(gen),         // Full yaw rotation
                    rotation_dist(gen) * 0.1f   // Small roll variation
                };

                float object_scale = scale_dist(gen) * 1.0f; // Small scale for objects
                scales[i].value = skr_float3_t{ object_scale, object_scale, object_scale };

                const auto parent = lv2.ents[index_in_level / 50];
                skr::scene::ChildrenComponent as_child = { .entity = Context.entities()[i] };
                parents[i].entity = parent; // Attach to the corresponding building
                children_writer[parent].push_back(as_child); // Add this object to the building's children
                index_in_level += 1;
            }
        }
        uint64_t index_in_level = 0;
        ComponentView<skr::scene::ParentComponent> parents;
        RandomComponentReadWrite<skr::scene::ChildrenComponent> children_writer;
    } level3_spawner(level2_spawner);

    world.create_entities(level1_spawner, LEVEL_1_COUNT);
    world.create_entities(level2_spawner, LEVEL_2_COUNT);
    world.create_entities(level3_spawner, LEVEL_3_COUNT);

    transform_system->update();

    SKR_LOG_INFO(u8"Spawned hierarchical scene with {} entities:", LEVEL_1_COUNT + LEVEL_2_COUNT + LEVEL_3_COUNT);
    SKR_LOG_INFO(u8"  Level 1 (Cities): {} entities", LEVEL_1_COUNT);
    SKR_LOG_INFO(u8"  Level 2 (Buildings): {} entities", LEVEL_2_COUNT);
    SKR_LOG_INFO(u8"  Level 3 (Objects): {} entities", LEVEL_3_COUNT);
    SKR_LOG_INFO(u8"Scene distributed in {}x{}x{} space", 
                 static_cast<int>(SCENE_SIZE), 
                 static_cast<int>(SCENE_SIZE), 
                 static_cast<int>(SCENE_SIZE));
    SKR_LOG_INFO(u8"Established parent-child relationships successfully");
}

void RGRaytracingSampleModule::create_as()
{
    // Initialize CGPU
    CGPUInstanceDescriptor instance_desc = {
        .backend = CGPU_BACKEND_D3D12,
        .enable_debug_layer = true,
        .enable_gpu_based_validation = true,
        .enable_set_name = true
    };
    instance = cgpu_create_instance(&instance_desc);

    // Get adapter and create device
    uint32_t adapters_count = 0;
    cgpu_enum_adapters(instance, nullptr, &adapters_count);
    skr::Vector<CGPUAdapterId> adapters;
    adapters.resize_zeroed(adapters_count);
    cgpu_enum_adapters(instance, adapters.data(), &adapters_count);
    CGPUAdapterId adapter = adapters[0];

    CGPUQueueGroupDescriptor queue_group = {
        .queue_type = CGPU_QUEUE_TYPE_GRAPHICS,
        .queue_count = 1
    };
    CGPUDeviceDescriptor device_desc = {
        .queue_groups = &queue_group,
        .queue_group_count = 1
    };
    device = cgpu_create_device(adapter, &device_desc);
    gfx_queue = cgpu_get_queue(device, CGPU_QUEUE_TYPE_GRAPHICS, 0);


    // Create sphere geometry (icosphere or simple sphere)
    create_sphere_blas();
    
    // Create TLAS from ECS entities
    create_scene_tlas();
    
    // Create compute pipeline and resources
    create_compute_pipeline();
    
    SKR_LOG_INFO(u8"Created raytracing acceleration structures:");
    SKR_LOG_INFO(u8"  BLAS: Sphere geometry");
}

void RGRaytracingSampleModule::create_sphere_blas()
{
    // Generate sphere vertices and indices (simple icosphere or UV sphere)
    constexpr float radius = 3.0f;
    constexpr int segments = 16;
    constexpr int rings = 8;
    
    skr::Vector<skr_float3_t> vertices;
    skr::Vector<uint16_t> indices;
    
    // Generate sphere vertices (UV sphere)
    for (int ring = 0; ring <= rings; ++ring) {
        float phi = skr::kPi * ring / rings;
        float sin_phi = std::sin(phi);
        float cos_phi = std::cos(phi);
        
        for (int segment = 0; segment <= segments; ++segment) {
            float theta = 2.0f * skr::kPi * segment / segments;
            float sin_theta = std::sin(theta);
            float cos_theta = std::cos(theta);
            
            skr_float3_t vertex = {
                radius * sin_phi * cos_theta,
                radius * cos_phi,
                radius * sin_phi * sin_theta
            };
            vertices.add(vertex);
        }
    }
    
    // Generate sphere indices
    for (int ring = 0; ring < rings; ++ring) {
        for (int segment = 0; segment < segments; ++segment) {
            int current = ring * (segments + 1) + segment;
            int next = current + segments + 1;
            
            // First triangle
            indices.add(current);
            indices.add(next);
            indices.add(current + 1);
            
            // Second triangle
            indices.add(current + 1);
            indices.add(next);
            indices.add(next + 1);
        }
    }
    
    // Create vertex buffer
    CGPUBufferDescriptor vertex_buffer_desc = {
        .size = vertices.size() * sizeof(skr_float3_t),
        .name = u8"SphereVertexBuffer",
        .descriptors = CGPU_RESOURCE_TYPE_VERTEX_BUFFER,
        .memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU,
        .flags = CGPU_BCF_PERSISTENT_MAP_BIT,
        .element_count = vertices.size(),
        .element_stride = sizeof(skr_float3_t),
        .start_state = CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER
    };
    sphere_vertex_buffer = cgpu_create_buffer(device, &vertex_buffer_desc);
    memcpy(sphere_vertex_buffer->info->cpu_mapped_address, vertices.data(), vertex_buffer_desc.size);
    
    // Create index buffer
    CGPUBufferDescriptor index_buffer_desc = {
        .size = indices.size() * sizeof(uint16_t),
        .name = u8"SphereIndexBuffer",
        .descriptors = CGPU_RESOURCE_TYPE_INDEX_BUFFER,
        .memory_usage = CGPU_MEM_USAGE_CPU_TO_GPU,
        .flags = CGPU_BCF_PERSISTENT_MAP_BIT,
        .element_count = indices.size(),
        .element_stride = sizeof(uint16_t),
        .start_state = CGPU_RESOURCE_STATE_INDEX_BUFFER
    };
    sphere_index_buffer = cgpu_create_buffer(device, &index_buffer_desc);
    memcpy(sphere_index_buffer->info->cpu_mapped_address, indices.data(), index_buffer_desc.size);
    
    // Create BLAS
    CGPUAccelerationStructureGeometryDesc blas_geom = { 0 };
    blas_geom.flags = CGPU_ACCELERATION_STRUCTURE_GEOMETRY_FLAG_OPAQUE;
    blas_geom.vertex_buffer = sphere_vertex_buffer;
    blas_geom.index_buffer = sphere_index_buffer;
    blas_geom.vertex_offset = 0;
    blas_geom.vertex_count = vertices.size();
    blas_geom.vertex_stride = sizeof(skr_float3_t);
    blas_geom.vertex_format = CGPU_FORMAT_R32G32B32_SFLOAT;
    blas_geom.index_offset = 0;
    blas_geom.index_count = indices.size();
    blas_geom.index_stride = sizeof(uint16_t);
    
    CGPUAccelerationStructureDescriptor blas_desc = { };
    blas_desc.type = CGPU_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    blas_desc.flags = CGPU_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    blas_desc.bottom.count = 1;
    blas_desc.bottom.geometries = &blas_geom;
    sphere_blas = cgpu_create_acceleration_structure(device, &blas_desc);
}

void RGRaytracingSampleModule::create_scene_tlas()
{
    using namespace skr::ecs;

    // Collect all entities with transform components
    skr::Vector<CGPUAccelerationStructureInstanceDesc> tlas_instances;
    std::recursive_mutex instance_mutex;
    
    // Query all entities with translation, rotation, and scale components using a system job
    struct TransformQueryJob {
        CGPUAccelerationStructureId sphere_blas;

        void build(skr::ecs::AccessBuilder& Builder) {
            Builder.read(&TransformQueryJob::transforms)
                   .read(&TransformQueryJob::indices);
        }
        
        void run(skr::ecs::TaskContext& Context) 
        {
            task_instances.reserve(Context.size());
            for (int i = 0; i < Context.size(); ++i) {
                // Create transform matrix from translation, rotation, scale
                auto transform = transforms[i].value.to_matrix();
                auto index = indices[i].value;
                
                // Debug: print first few transforms and extreme positions
                if (index < 10) {
                    SKR_LOG_FMT_INFO(u8"Entity {}: transform translation ({}, {}, {}), scale ({}, {}, {})", 
                                index, transform.m30, transform.m31, transform.m32,
                                transform.m00, transform.m11, transform.m22);
                }
                // Print some extreme entity positions to understand actual coordinate range
                if (index == 0 || index == 99 || index == 999 || index == 49999) {
                    SKR_LOG_FMT_INFO(u8"Entity {} final world position: ({}, {}, {})", 
                                index, transform.m30, transform.m31, transform.m32);
                }
                
                CGPUAccelerationStructureInstanceDesc instance = { 0 };
                instance.bottom = sphere_blas;
                instance.instance_id = index;
                instance.instance_mask = 255;
                
                // Use computed transform matrix (now that we know the format is correct)
                float transform34[12] = {
                    transform.m00, transform.m10, transform.m20, transform.m30,  // Row 0: X axis + X translation
                    transform.m01, transform.m11, transform.m21, transform.m31,  // Row 1: Y axis + Y translation
                    transform.m02, transform.m12, transform.m22, transform.m32   // Row 2: Z axis + Z translation
                };                
                memcpy(instance.transform, transform34, sizeof(transform34));
                task_instances.add(instance);
            }
            {
                std::lock_guard<std::recursive_mutex> lock(*pInstancesMutex);
                pInstances->append(task_instances.data(), task_instances.size());
            }
        }
        
        ComponentView<const skr::scene::TransformComponent> transforms;
        ComponentView<const skr::scene::IndexComponent> indices;

        skr::Vector<CGPUAccelerationStructureInstanceDesc> task_instances;
        skr::Vector<CGPUAccelerationStructureInstanceDesc>* pInstances;
        std::recursive_mutex* pInstancesMutex;
    } transform_job;
    transform_job.sphere_blas = sphere_blas;
    transform_job.pInstances = &tlas_instances;
    transform_job.pInstancesMutex = &instance_mutex;
    
    // Execute the job to collect transform data
    auto query = world.dispatch_task(transform_job, 1280, nullptr);
    TaskScheduler::Get()->sync_all();
    world.destroy_query(query);
    
    // Create TLAS
    CGPUAccelerationStructureDescriptor tlas_desc = { };
    tlas_desc.type = CGPU_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    tlas_desc.flags = CGPU_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    tlas_desc.top.count = tlas_instances.size();
    tlas_desc.top.instances = tlas_instances.data();
    scene_tlas = cgpu_create_acceleration_structure(device, &tlas_desc);
    
    // Build acceleration structures
    CGPUCommandPoolId pool = cgpu_create_command_pool(gfx_queue, nullptr);
    CGPUCommandBufferDescriptor cmd_desc = { .is_secondary = false };
    CGPUCommandBufferId cmd = cgpu_create_command_buffer(pool, &cmd_desc);
    
    cgpu_cmd_begin(cmd);
    
    // Build BLAS first
    CGPUAccelerationStructureBuildDescriptor blas_build = { 
        .type = CGPU_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL, 
        .as_count = 1,
        .as = &sphere_blas 
    };
    cgpu_cmd_build_acceleration_structures(cmd, &blas_build);
    
    // Build TLAS
    CGPUAccelerationStructureBuildDescriptor tlas_build = { 
        .type = CGPU_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL, 
        .as_count = 1,
        .as = &scene_tlas 
    };
    cgpu_cmd_build_acceleration_structures(cmd, &tlas_build);
    
    cgpu_cmd_end(cmd);
    
    CGPUQueueSubmitDescriptor submit_desc = {
        .cmds = &cmd,
        .cmds_count = 1
    };
    cgpu_submit_queue(gfx_queue, &submit_desc);
    cgpu_wait_queue_idle(gfx_queue);
    
    cgpu_free_command_buffer(cmd);
    cgpu_free_command_pool(pool);
}

void RGRaytracingSampleModule::create_compute_pipeline()
{
    // Create compute shader
    uint32_t *shader_bytes, shader_length;
    read_shader_bytes("rg-raytracing/rg-raytracing.cs_main", &shader_bytes, &shader_length, CGPU_BACKEND_D3D12);
    CGPUShaderLibraryDescriptor shader_desc = {
        .name = u8"RayTracingComputeShader",
        .code = shader_bytes,
        .code_size = shader_length
    };
    compute_shader = cgpu_create_shader_library(device, &shader_desc);
    free(shader_bytes);

    // Create root signature
    const char8_t* push_constant_name = SKR_UTF8("camera_constants");
    CGPUShaderEntryDescriptor compute_shader_entry = {
        .library = compute_shader,
        .entry = u8"cs_main",
        .stage = CGPU_SHADER_STAGE_COMPUTE
    };
    CGPURootSignatureDescriptor root_desc = {
        .shaders = &compute_shader_entry,
        .shader_count = 1,
        .push_constant_names = &push_constant_name,
        .push_constant_count = 1,
    };
    root_signature = cgpu_create_root_signature(device, &root_desc);

    // Create compute pipeline
    CGPUComputePipelineDescriptor pipeline_desc = {
        .root_signature = root_signature,
        .compute_shader = &compute_shader_entry,
    };
    compute_pipeline = cgpu_create_compute_pipeline(device, &pipeline_desc);

    // Create descriptor set
    CGPUDescriptorSetDescriptor set_desc = {
        .root_signature = root_signature,
        .set_index = 0
    };
    descriptor_set = cgpu_create_descriptor_set(device, &set_desc);

    // Create output buffer
    CGPUBufferDescriptor buffer_desc = {
        .size = RENDER_WIDTH * RENDER_HEIGHT * sizeof(float) * 4,
        .name = u8"RayTracingOutputBuffer",
        .descriptors = CGPU_RESOURCE_TYPE_RW_BUFFER,
        .memory_usage = CGPU_MEM_USAGE_GPU_ONLY,
        .flags = CGPU_BCF_NONE,
        .element_count = RENDER_WIDTH * RENDER_HEIGHT,
        .element_stride = sizeof(float) * 4,
        .start_state = CGPU_RESOURCE_STATE_UNORDERED_ACCESS,
    };
    output_buffer = cgpu_create_buffer(device, &buffer_desc);

    // Create readback buffer
    CGPUBufferDescriptor rb_desc = {
        .size = buffer_desc.size,
        .name = u8"ReadbackBuffer",
        .descriptors = CGPU_RESOURCE_TYPE_NONE,
        .memory_usage = CGPU_MEM_USAGE_GPU_TO_CPU,
        .flags = CGPU_BCF_NONE,
        .element_count = buffer_desc.element_count,
        .element_stride = buffer_desc.element_stride,
        .start_state = CGPU_RESOURCE_STATE_COPY_DEST,
    };
    readback_buffer = cgpu_create_buffer(device, &rb_desc);

    // Update descriptor set
    CGPUDescriptorData descriptor_data[2] = {
        {
            .name = u8"output_buffer",
            .binding_type = CGPU_RESOURCE_TYPE_RW_BUFFER,
            .buffers = &output_buffer,
            .count = 1
        },
        {
            .name = u8"scene_tlas",
            .binding_type = CGPU_RESOURCE_TYPE_ACCELERATION_STRUCTURE,
            .acceleration_structures = &scene_tlas,
            .count = 1
        }
    };
    cgpu_update_descriptor_set(descriptor_set, descriptor_data, 2);
}

void RGRaytracingSampleModule::create_swapchain(skr::SystemWindow* window)
{
    if (!window || !device) return;
    
    CGPUSurfaceId surface = cgpu_surface_from_native_view(device, window->get_native_view());
    
    CGPUSwapChainDescriptor swapchain_desc = {
        .present_queues = &gfx_queue,
        .present_queues_count = 1,
        .surface = surface,
        .image_count = 2,
        .width = 1280,
        .height = 720,
        .enable_vsync = false,
        .format = CGPU_FORMAT_B8G8R8A8_UNORM,
    };
    swapchain = cgpu_create_swapchain(device, &swapchain_desc);
}

void RGRaytracingSampleModule::render()
{
    // Camera constants structure matching shader
    struct CameraConstants {
        skr::math::float4x4 invViewMatrix;
        skr::math::float4x4 invProjMatrix;
        skr::math::float3 cameraPos;
        float _padding0;
        skr::math::float3 cameraDir;
        float _padding1;
        skr::math::float2 screenSize;
    };

    // Setup camera (looking at the scene from a distance)
    CameraConstants camera_constants = {};
    
    // Camera setup - start with a working position and adjust
    const float nearPlane = 0.1f;
    const float farPlane = 99999999.0f;
    
    // Position camera much further back to see complete scene
    // Scene spans: X(-800 to 1600), Y(0 to 4500), Z(-1800 to 800)  
    skr::math::float3 eye = {-25000.0f, 15000.0f, -35000.0f};   // Much further back and higher
    skr::math::float3 target = {0.0f, 2000.0f, 0.0f};           // Look towards scene center
    skr::math::float3 up = {0.0f, 1.0f, 0.0f};             // Y-up
    
    camera_constants.cameraPos = eye;
    camera_constants.cameraDir = skr::math::normalize(target - eye);
    camera_constants.screenSize = {static_cast<float>(RENDER_WIDTH), static_cast<float>(RENDER_HEIGHT)};
    
    // Create view matrix (lookAt)
    skr::math::float3 f = skr::math::normalize(target - eye);
    skr::math::float3 s = skr::math::normalize(skr::math::cross(f, up));
    skr::math::float3 u = skr::math::cross(s, f);
    
    const auto viewMatrix = skr::math::float4x4(
        s.x, u.x, -f.x, 0.0f,
        s.y, u.y, -f.y, 0.0f,
        s.z, u.z, -f.z, 0.0f,
        -skr::math::dot(s, eye), -skr::math::dot(u, eye), skr::math::dot(f, eye), 1.0f
    );
    
    // Create projection matrix (perspective)
    float fov = 45.0f * skr::kPi / 180.0f;
    float aspect = static_cast<float>(RENDER_WIDTH) / static_cast<float>(RENDER_HEIGHT);
    float tan_half_fov = std::tan(fov / 2.0f);
    
    const auto projMatrix = skr::math::float4x4(
        1.0f / (aspect * tan_half_fov), 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f / tan_half_fov, 0.0f, 0.0f,
        0.0f, 0.0f, -(farPlane + nearPlane) / (farPlane - nearPlane), -1.0f,
        0.0f, 0.0f, -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane), 0.0f
    );
    
    // Calculate inverse matrices
    camera_constants.invViewMatrix = skr::math::inverse(viewMatrix);
    camera_constants.invProjMatrix = skr::math::inverse(projMatrix);

    // Create command objects
    CGPUCommandPoolId pool = cgpu_create_command_pool(gfx_queue, nullptr);
    CGPUCommandBufferDescriptor cmd_desc = { .is_secondary = false };
    CGPUCommandBufferId cmd = cgpu_create_command_buffer(pool, &cmd_desc);

    // Execute compute shader
    cgpu_cmd_begin(cmd);
    
    // Begin compute pass
    CGPUComputePassDescriptor pass_desc = { .name = u8"RayTracingComputePass" };
    CGPUComputePassEncoderId encoder = cgpu_cmd_begin_compute_pass(cmd, &pass_desc);
    
    // Bind pipeline and resources
    cgpu_compute_encoder_bind_pipeline(encoder, compute_pipeline);
    cgpu_compute_encoder_bind_descriptor_set(encoder, descriptor_set);
    cgpu_compute_encoder_push_constants(encoder, root_signature, u8"camera_constants", &camera_constants);
    
    // Dispatch compute shader
    uint32_t group_count_x = (RENDER_WIDTH + 15) / 16;
    uint32_t group_count_y = (RENDER_HEIGHT + 15) / 16;
    cgpu_compute_encoder_dispatch(encoder, group_count_x, group_count_y, 1);
    
    cgpu_cmd_end_compute_pass(cmd, encoder);
    
    // Barrier UAV buffer to transfer source
    CGPUBufferBarrier buffer_barrier = {
        .buffer = output_buffer,
        .src_state = CGPU_RESOURCE_STATE_UNORDERED_ACCESS,
        .dst_state = CGPU_RESOURCE_STATE_COPY_SOURCE
    };
    CGPUResourceBarrierDescriptor barriers_desc = {
        .buffer_barriers = &buffer_barrier,
        .buffer_barriers_count = 1
    };
    cgpu_cmd_resource_barrier(cmd, &barriers_desc);
    
    // Copy buffer to readback
    CGPUBufferToBufferTransfer cpy_desc = {
        .dst = readback_buffer,
        .dst_offset = 0,
        .src = output_buffer,
        .src_offset = 0,
        .size = RENDER_WIDTH * RENDER_HEIGHT * sizeof(float) * 4,
    };
    cgpu_cmd_transfer_buffer_to_buffer(cmd, &cpy_desc);
    
    cgpu_cmd_end(cmd);
    
    // Submit and wait
    CGPUQueueSubmitDescriptor submit_desc = {
        .cmds = &cmd,
        .cmds_count = 1
    };
    cgpu_submit_queue(gfx_queue, &submit_desc);
    cgpu_wait_queue_idle(gfx_queue);

    // Map buffer and save to PNG
    {
        CGPUBufferRange map_range = {
            .offset = 0,
            .size = RENDER_WIDTH * RENDER_HEIGHT * sizeof(float) * 4
        };
        cgpu_map_buffer(readback_buffer, &map_range);
        
        float* mapped_memory = (float*)readback_buffer->info->cpu_mapped_address;
        unsigned char* image = (unsigned char*)malloc(RENDER_WIDTH * RENDER_HEIGHT * 4);
        
        // Convert float4 to RGBA8
        for (uint32_t i = 0; i < RENDER_WIDTH * RENDER_HEIGHT; i++) {
            image[i * 4 + 0] = (uint8_t)(255.0f * skr::math::clamp(mapped_memory[i * 4 + 0], 0.0f, 1.0f));
            image[i * 4 + 1] = (uint8_t)(255.0f * skr::math::clamp(mapped_memory[i * 4 + 1], 0.0f, 1.0f));
            image[i * 4 + 2] = (uint8_t)(255.0f * skr::math::clamp(mapped_memory[i * 4 + 2], 0.0f, 1.0f));
            image[i * 4 + 3] = (uint8_t)(255.0f * skr::math::clamp(mapped_memory[i * 4 + 3], 0.0f, 1.0f));
        }
        
        cgpu_unmap_buffer(readback_buffer);
        
        // Save to PNG
        const char* png_filename = "rg-raytracing-output.png";
        unsigned error = lodepng_encode32_file(png_filename, image, RENDER_WIDTH, RENDER_HEIGHT);
        if (error) {
            SKR_LOG_ERROR(u8"PNG encoder error %d: %s", error, lodepng_error_text(error));
        } else {
            SKR_LOG_INFO(u8"Ray tracing output saved to: %s", png_filename);
        }
        
        free(image);
    }

    // Clean up command objects
    cgpu_free_command_buffer(cmd);
    cgpu_free_command_pool(pool);
    
    // Present swapchain for profiler hook support
    if (swapchain) {
        CGPUAcquireNextDescriptor acquire_desc = {};
        uint8_t backbuffer_index = cgpu_acquire_next_image(swapchain, &acquire_desc);
        
        CGPUQueuePresentDescriptor present_desc = {
            .swapchain = swapchain,
            .index = backbuffer_index
        };
        cgpu_queue_present(gfx_queue, &present_desc);
    }
}

void RGRaytracingSampleModule::on_unload()
{
    cgpu_wait_queue_idle(gfx_queue);
    // Clean up compute pipeline resources
    if (readback_buffer) cgpu_free_buffer(readback_buffer);
    if (output_buffer) cgpu_free_buffer(output_buffer);
    if (descriptor_set) cgpu_free_descriptor_set(descriptor_set);
    if (compute_pipeline) cgpu_free_compute_pipeline(compute_pipeline);
    if (root_signature) cgpu_free_root_signature(root_signature);
    if (compute_shader) cgpu_free_shader_library(compute_shader);
    
    // Clean up swapchain resources
    if (swapchain) cgpu_free_swapchain(swapchain);
    
    // Clean up raytracing resources
    if (scene_tlas) cgpu_free_acceleration_structure(scene_tlas);
    if (sphere_blas) cgpu_free_acceleration_structure(sphere_blas);
    if (sphere_vertex_buffer) cgpu_free_buffer(sphere_vertex_buffer);
    if (sphere_index_buffer) cgpu_free_buffer(sphere_index_buffer);
    if (gfx_queue) cgpu_free_queue(gfx_queue);
    if (device) cgpu_free_device(device);
    if (instance) cgpu_free_instance(instance);
    
    skr_transform_system_destroy(transform_system);
    world.finalize();
    app.shutdown();
    scheduler.unbind();
}