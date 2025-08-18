#include "SkrOS/filesystem.hpp"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrContainers/hashmap.hpp"
#include "SkrCore/module/module.hpp"
#include "SkrCore/async/thread_job.hpp"
#include "SkrTask/fib_task.hpp"
#include "SkrRT/resource/resource_system.h"
#include "SkrRT/resource/local_resource_registry.hpp"
#include "SkrSceneCore/scene_components.h"
#include "SkrSystem/system_app.h"

#include <random>
#include <chrono>
#include <thread>

#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrRenderer/resources/texture_resource.h"
#include "SkrRenderer/skr_renderer.h"
#include "SkrRenderer/render_app.hpp"
#include "SkrRenderer/gpu_scene.h"
#include "SkrRenderer/shared/gpu_scene.hpp"

#include "SkrRenderer/resources/mesh_resource.h"
#include "SkrRenderer/render_mesh.h"
#include "SkrMeshCore/mesh_processing.hpp"
#include "SkrGLTFTool/mesh_asset.hpp"
#include "common/utils.h"

using namespace skr::literals;
const auto MeshAssetID = u8"18db1369-ba32-4e91-aa52-b2ed1556f576"_guid;

struct VirtualProject : skd::SProject
{
    bool LoadAssetMeta(const skd::URI& uri, skr::String& content) noexcept override
    {
        if (MetaDatabase.contains(uri))
        {
            content = MetaDatabase[uri];
            return true;
        }
        return false;
    }

    bool SaveAssetMeta(const skd::URI& uri, const skr::String& content) noexcept override
    {
        MetaDatabase[uri] = content;
        return true;
    }

    bool ExistImportedAsset(const skd::URI& uri)
    {
        return MetaDatabase.contains(uri);
    }

    void SaveToDisk()
    {
        skr::archive::JsonWriter writer(4);
        writer.StartObject();
        writer.Key(u8"assets");
        skr::json_write(&writer, MetaDatabase);
        writer.EndObject();

        // Write to model_viewer.project file
        const auto project_path = skr::fs::current_directory() / u8"model_viewer.project";
        auto json_str = writer.Write();

        if (skr::fs::File::write_all_text(project_path, json_str.view()))
            SKR_LOG_INFO(u8"[ModelViewer] Project saved to: %s", project_path.c_str());
        else
            SKR_LOG_ERROR(u8"[ModelViewer] Failed to save project to: %s", project_path.c_str());
    }

    void LoadFromDisk()
    {
        const auto project_path = skr::fs::current_directory() / u8"model_viewer.project";

        skr::String json_content;
        if (skr::fs::File::read_all_text(project_path, json_content))
        {
            // Parse JSON
            skr::archive::JsonReader reader(json_content.view());
            reader.StartObject();
            reader.Key(u8"assets");
            {
                skr::json_read(&reader, MetaDatabase);
                SKR_LOG_INFO(u8"[ModelViewer] Project loaded from: %s, %zu assets",
                    project_path.c_str(),
                    MetaDatabase.size());
            }
            reader.EndObject();
        }
        else
        {
            // File doesn't exist, which is fine - just means empty project
            MetaDatabase.clear();
            SKR_LOG_INFO(u8"[ModelViewer] No existing project file found at: %s, starting with empty project", project_path.c_str());
        }
    }

    skr::ParallelFlatHashMap<skd::URI, skr::String, skr::Hash<skd::URI>> MetaDatabase;
};

struct ModelViewerModule : public skr::IDynamicModule
{
public:
    ModelViewerModule()
        : world(scheduler)
    {
    }
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

protected:
    void CookAndLoadGLTF();
    void InitializeAssetSystem();
    void DestroyAssetSystem();
    void InitializeReosurceSystem();
    void DestroyResourceSystem();

    void CreateEntities(uint32_t count = 4);
    void DestroyRandomEntities(uint32_t count = 1);
    void CreateComputePipeline();
    void render();

    // Camera controller for movable camera
    struct CameraController
    {
        skr::math::float3 position = { 0.f, 0.f, 0.f };
        skr::math::float3 target = { 0.0f, 0.0f, 1.0f };
        skr::math::float3 up = { 0.0f, 1.0f, 0.0f };

        float yaw = 0.0f;   // 水平旋转角度
        float pitch = 0.0f; // 垂直旋转角度
        float move_speed = 5000.0f;
        float mouse_sensitivity = 0.005f;

        bool right_mouse_pressed = false;
        float last_mouse_x = 0.0f;
        float last_mouse_y = 0.0f;

        // 键盘状态
        bool keys_pressed[256] = { false };

        void update_camera(float delta_time)
        {
            // 根据yaw和pitch计算前方向量
            skr::math::float3 forward = {
                cos(pitch) * cos(yaw),
                sin(pitch),
                cos(pitch) * sin(yaw)
            };
            forward = skr::math::normalize(forward);

            // 计算右方向量和上方向量
            skr::math::float3 right = skr::math::normalize(skr::math::cross(forward, { 0.0f, 1.0f, 0.0f }));
            skr::math::float3 camera_up = skr::math::cross(right, forward);

            // 移动速度基于帧时间
            float speed = move_speed * delta_time;

            // WASD移动
            if (keys_pressed['w'] || keys_pressed['W']) position += forward * speed;
            if (keys_pressed['s'] || keys_pressed['S']) position -= forward * speed;
            if (keys_pressed['a'] || keys_pressed['A']) position -= right * speed;
            if (keys_pressed['d'] || keys_pressed['D']) position += right * speed;
            if (keys_pressed['q'] || keys_pressed['Q']) position.y -= speed; // 下降
            if (keys_pressed['e'] || keys_pressed['E']) position.y += speed; // 上升

            // 更新target为position + forward
            target = position + forward;
            up = camera_up;
        }

        void initialize_from_lookat(const skr::math::float3& eye, const skr::math::float3& look_target)
        {
            position = eye;
            target = look_target;

            // 计算初始的yaw和pitch
            skr::math::float3 direction = skr::math::normalize(look_target - eye);
            yaw = atan2(direction.z, direction.x);
            pitch = asin(direction.y);
        }
    } camera_controller;

    skr::task::scheduler_t scheduler;
    VirtualProject project;
    SRenderDeviceId render_device = nullptr;

    skr::SP<skr::JobQueue> job_queue = nullptr;
    skr::io::IRAMService* ram_service = nullptr;
    skr::io::IVRAMService* vram_service = nullptr;

    skr::resource::LocalResourceRegistry* registry = nullptr;
    skr::resource::TextureSamplerFactory* TextureSamplerFactory = nullptr;
    skr::resource::TextureFactory* TextureFactory = nullptr;
    skr::renderer::MeshFactory* MeshFactory = nullptr;

    skr::ecs::World world;
    skr::renderer::GPUScene GPUScene;

    // Compute pipeline resources for debug rendering
    CGPUShaderLibraryId compute_shader = nullptr;
    CGPURootSignatureId root_signature = nullptr;
    CGPUComputePipelineId compute_pipeline = nullptr;

    // RenderGraph and swapchain for rendering
    CGPUSwapChainId swapchain = nullptr;
    skr::ParallelFlatHashSet<skr::ecs::Entity, skr::Hash<skr::ecs::Entity>> all_entities;
};
IMPLEMENT_DYNAMIC_MODULE(ModelViewerModule, SkrModelViewer);

void ModelViewerModule::on_load(int argc, char8_t** argv)
{
    skr_log_set_level(SKR_LOG_LEVEL_INFO);
    skr_log_initialize_async_worker();

    scheduler.initialize(skr::task::scheudler_config_t());
    scheduler.bind();

    render_device = SkrRendererModule::Get()->get_render_device();

    const auto current_path = skr::fs::current_directory();
    skd::SProjectConfig projectConfig = {
        .assetDirectory = (current_path / u8"assets").string().c_str(),
        .resourceDirectory = (current_path / u8"resources").string().c_str(),
        .artifactsDirectory = (current_path / u8"artifacts").string().c_str()
    };
    skr::String projectName = u8"ModelViewer";
    skr::String rootPath = skr::fs::current_directory().string().c_str();
    project.OpenProject(u8"ModelViewer", rootPath.c_str(), projectConfig);

    // Load existing project data from disk
    project.LoadFromDisk();

    // initialize resource & asset system
    // these two systems co-works well like producers & consumers
    // we can add resources as dependencies for one specific asset, e.g.:
    // MeshAsset[id0] depends MaterialResource[id1-4]
    // then it's output resource:
    // MeshResource[id0] depends MaterialResource[id1-4]
    // When we load AsyncResource<MeshResource>[id0] with resource system, the materials will be loaded too!
    // All these operations are async & paralleled behind background!
    {
        // resources are cooked runtime-data, i.e. DXT textures, Optimized meshes, etc.
        InitializeReosurceSystem();

        // assets are 'raw' source files, i.e. GLTF model, PNG image, etc.
        InitializeAssetSystem();
    }

    CookAndLoadGLTF();

    world.initialize();
}

int ModelViewerModule::main_module_exec(int argc, char8_t** argv)
{
    using namespace skr;
    using namespace skr::renderer;

    auto device = render_device->get_cgpu_device();
    auto gfx_queue = render_device->get_gfx_queue();
    auto resource_system = skr::resource::GetResourceSystem();
    std::atomic_bool resource_system_quit = false;
    auto resource_updater = std::thread([resource_system, &resource_system_quit]() {
        while (!resource_system_quit)
        {
            resource_system->Update();
        }
    });

    skr::render_graph::RenderGraphBuilder graph_builder;
    graph_builder.with_device(device)
        .with_gfx_queue(gfx_queue)
        .enable_memory_aliasing();
    skr::SystemWindowCreateInfo window_config = {
        .size = { 1200, 1200 },
        .is_resizable = true
    };
    skr::UPtr<skr::RenderApp> render_app = skr::UPtr<skr::RenderApp>::New(render_device, graph_builder);
    render_app->initialize();
    render_app->open_window(window_config);
    render_app->get_main_window()->show();

    struct EventListener : public skr::ISystemEventHandler
    {
        ModelViewerModule* module = nullptr;
        bool want_exit = false;

        void handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT
        {
            switch (event.type)
            {
            case SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED:
                want_exit = true;
                break;

            case SKR_SYSTEM_EVENT_KEY_DOWN:
                module->camera_controller.keys_pressed[event.key.keycode] = true;
                break;

            case SKR_SYSTEM_EVENT_KEY_UP:
                module->camera_controller.keys_pressed[event.key.keycode] = false;
                break;

            case SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN:
                if (event.mouse.button == InputMouseButtonFlags::InputMouseRightButton)
                {
                    module->camera_controller.right_mouse_pressed = true;
                    module->camera_controller.last_mouse_x = (float)event.mouse.x;
                    module->camera_controller.last_mouse_y = (float)event.mouse.y;
                }
                break;

            case SKR_SYSTEM_EVENT_MOUSE_BUTTON_UP:
                if (event.mouse.button == InputMouseButtonFlags::InputMouseRightButton)
                {
                    module->camera_controller.right_mouse_pressed = false;
                }
                break;

            case SKR_SYSTEM_EVENT_MOUSE_MOVE:
                if (module->camera_controller.right_mouse_pressed)
                {
                    float delta_x = (float)event.mouse.x - module->camera_controller.last_mouse_x;
                    float delta_y = (float)event.mouse.y - module->camera_controller.last_mouse_y;

                    module->camera_controller.yaw += delta_x * module->camera_controller.mouse_sensitivity;
                    module->camera_controller.pitch -= delta_y * module->camera_controller.mouse_sensitivity;

                    // Clamp pitch to prevent camera flip
                    const float max_pitch = 1.5f; // ~85 degrees
                    if (module->camera_controller.pitch > max_pitch) module->camera_controller.pitch = max_pitch;
                    if (module->camera_controller.pitch < -max_pitch) module->camera_controller.pitch = -max_pitch;

                    module->camera_controller.last_mouse_x = (float)event.mouse.x;
                    module->camera_controller.last_mouse_y = (float)event.mouse.y;
                }
                break;
            default:
                break;
            }
        }
    } event_listener;
    event_listener.module = this;
    render_app->get_event_queue()->add_handler(&event_listener);

    // Create compute pipeline for debug rendering
    CreateComputePipeline();

    GPUSceneBuilder cfg_builder;
    cfg_builder.with_device(render_device)
        .with_world(&world)
        .from_layout<DefaultGPUSceneLayout>();
    GPUScene.Initialize(cfg_builder.build_config(), cfg_builder.get_soa_builder());

    // AsyncResource<> is a handle can be constructed by any resource type & ids
    skr::resource::AsyncResource<MeshResource> mesh_resource;
    mesh_resource = MeshAssetID;

    uint64_t frame_index = 0;
    auto last_time = std::chrono::high_resolution_clock::now();
    while (!event_listener.want_exit)
    {
        SkrZoneScopedN("Frame");

        // Calculate delta time
        auto current_time = std::chrono::high_resolution_clock::now();
        float delta_time = std::chrono::duration<float>(current_time - last_time).count();
        last_time = current_time;

        // Update camera controller
        camera_controller.update_camera(delta_time);

        render_app->get_event_queue()->pump_messages();
        render_app->acquire_frames();

        if (frame_index < 1'000'000)
        {
            CreateEntities(4);
            DestroyRandomEntities(1);
        }

        auto render_graph = render_app->render_graph();
        // Simple render loop using compute shader to fill screen red
        const auto screen_size = render_app->get_main_window()->get_physical_size();

        // Calculate dispatch groups for 16x16 kernel
        const uint32_t group_count_x = (screen_size.x + 15) / 16;
        const uint32_t group_count_y = (screen_size.y + 15) / 16;

        // Update GPUScene
        GPUScene.ExecuteUpload(render_graph);

        // Create output render target texture
        auto render_target_handle = render_graph->create_texture(
            [=](skr::render_graph::RenderGraph& g, skr::render_graph::TextureBuilder& builder) {
                builder.set_name(u8"render_target")
                    .extent(screen_size.x, screen_size.y)
                    .format(CGPU_FORMAT_R8G8B8A8_UNORM)
                    .allow_readwrite();
            });

        // Setup camera (looking at the scene from a distance)
        struct CameraConstants
        {
            skr::math::float4 cameraPos;
            skr::math::float4 cameraDir;
            skr::math::float2 screenSize;
        } camera_constants;

        // Camera setup
        const float nearPlane = 0.1f;
        const float farPlane = 99999999.0f;

        // Use camera controller values
        skr::math::float3 eye = this->camera_controller.position;
        skr::math::float3 target = this->camera_controller.target;
        skr::math::float3 up = this->camera_controller.up;

        camera_constants.cameraPos = skr::math::float4(eye, 0.f);
        camera_constants.cameraDir = skr::math::float4(skr::math::normalize(target - eye), 0.f);
        camera_constants.screenSize = { static_cast<float>(screen_size.x), static_cast<float>(screen_size.y) };

        // Add raytracing compute pass (write to intermediate texture)
        auto TLASHandle = GPUScene.GetTLAS(render_graph);
        if (TLASHandle != skr::render_graph::kInvalidHandle)
        {
            render_graph->add_compute_pass(
                [=, this](render_graph::RenderGraph& g, render_graph::ComputePassBuilder& builder) {
                    builder.set_name(u8"RayTracingPass")
                        .set_pipeline(compute_pipeline)
                        .read(u8"scene_tlas", GPUScene.GetTLAS(render_graph))
                        .read(u8"gpu_scene", GPUScene.GetSceneBuffer())
                        .readwrite(u8"output_texture", render_target_handle);
                },
                [=, this](render_graph::RenderGraph& g, render_graph::ComputePassContext& ctx) {
                    // Push constants
                    cgpu_compute_encoder_push_constants(ctx.encoder, root_signature, u8"camera_constants", &camera_constants);

                    // Dispatch compute shader
                    uint32_t group_count_x = (static_cast<uint32_t>(camera_constants.screenSize.x) + 15) / 16;
                    uint32_t group_count_y = (static_cast<uint32_t>(camera_constants.screenSize.y) + 15) / 16;
                    cgpu_compute_encoder_dispatch(ctx.encoder, group_count_x, group_count_y, 1);
                });

            // Add copy pass to copy render target to backbuffer
            auto backbuffer_handle = render_graph->get_imported(render_app->get_backbuffer(render_app->get_main_window()));
            render_graph->add_copy_pass(
                [=](skr::render_graph::RenderGraph& g, skr::render_graph::CopyPassBuilder& builder) {
                    builder.set_name(u8"CopyToBackbuffer")
                        .texture_to_texture(render_target_handle, backbuffer_handle);
                },
                [=](skr::render_graph::RenderGraph& g, skr::render_graph::CopyPassContext& ctx) {
                    // Copy implementation handled by render graph
                });
        }

        frame_index = render_graph->execute();
        if (frame_index >= RG_MAX_FRAME_IN_FLIGHT * 10)
            render_graph->collect_garbage(frame_index - RG_MAX_FRAME_IN_FLIGHT * 10);

        render_app->present_all();
    }

    resource_system_quit = true;
    resource_updater.join();

    render_app->close_all_windows();
    render_app->get_event_queue()->remove_handler(&event_listener);
    render_app->shutdown();
    return 0;
}

void ModelViewerModule::CookAndLoadGLTF()
{
    auto& CookSystem = *skd::asset::GetCookSystem();
    const bool NeedImport = !project.ExistImportedAsset(u8"girl.model.meta");
    if (NeedImport) // Import from disk!
    {
        // source file is a GLTF so we create a gltf importer, if it is a fbx, then just use a fbx importer
        auto importer = skd::asset::GltfMeshImporter::Create<skd::asset::GltfMeshImporter>();

        // static mesh has some additional meta data to append
        auto metadata = skd::asset::MeshAsset::Create<skd::asset::MeshAsset>();
        metadata->vertexType = u8"C35BD99A-B0A8-4602-AFCC-6BBEACC90321"_guid;

        auto asset = skr::RC<skd::asset::AssetMetaFile>::New(
            u8"girl.model.meta",                            // virtual uri for this asset in the project
            MeshAssetID,                                    // guid for this asset
            skr::type_id_of<skr::renderer::MeshResource>(), // output resource is a mesh resource
            skr::type_id_of<skd::asset::MeshCooker>()       // this cooker cooks t he raw mesh data to mesh resource
        );
        // source file
        importer->assetPath = u8"C:/Code/SakuraEngine/samples/application/game/assets/sketchfab/loli/scene.gltf";
        CookSystem.ImportAssetMeta(&project, asset, importer, metadata);

        // save
        CookSystem.SaveAssetMeta(&project, asset);

        auto event = CookSystem.EnsureCooked(asset->GetGUID());
        event.wait(true);
    }
    else // Load imported asset from project...
    {
        auto asset = CookSystem.LoadAssetMeta(&project, u8"girl.model.meta");
        auto event = CookSystem.EnsureCooked(asset->GetGUID());
        event.wait(true);
    }
}

void ModelViewerModule::CreateEntities(uint32_t count)
{
    using namespace skr::ecs;
    using namespace skr::renderer;

    // Generate random transforms for 3D validation demo
    static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    static std::uniform_real_distribution<float> pos_xy_dist(-1000.f, 1000.f); // Wider XY range, some outside viewport
    static std::uniform_real_distribution<float> pos_z_dist(-1000.f, 1000.f);  // Z behind camera (negative Z)
    static std::uniform_real_distribution<float> scale_dist(0.05f, 0.2f);      // Smaller spheres
    static std::uniform_real_distribution<float> color_dist(0.2f, 1.0f);       // Bright colors
    static std::uniform_real_distribution<float> use_emission(-1.f, 1.f);

    struct Spawner
    {
        void build(skr::ecs::ArchetypeBuilder& Builder)
        {
            Builder.add_component(&Spawner::translations)
                .add_component(&Spawner::rotations)
                .add_component(&Spawner::scales)
                .add_component(&Spawner::transforms)
                .add_component(&Spawner::meshes)

                .add_component(&Spawner::instances)
                .add_component(&Spawner::colors)
                .add_component(&Spawner::indices);

            if (use_emission(*rng_ptr) > 0.f)
            {
                Builder.add_component(&Spawner::emissions);
            }
        }

        void run(skr::ecs::TaskContext& Context)
        {
            auto cnt = Context.size();
            auto entities = Context.entities();
            for (uint32_t i = 0; i < cnt; i++)
            {
                // Generate random position with Z behind camera
                skr::float3 random_pos = {
                    pos_xy_dist(*rng_ptr), // X: wider range
                    pos_xy_dist(*rng_ptr), // Y: wider range
                    pos_z_dist(*rng_ptr)   // Z: behind camera (negative)
                };

                // Generate random scale for small spheres
                float random_scale = scale_dist(*rng_ptr);

                // Generate random bright color
                skr::float4 random_color = {
                    color_dist(*rng_ptr),
                    color_dist(*rng_ptr),
                    color_dist(*rng_ptr),
                    1.0f
                };

                // Set transform components
                translations[i].set(random_pos);
                rotations[i].set(0, 0, 0);
                scales[i].set(random_scale);

                // Create transform matrix from components
                auto transform_matrix = skr::scene::Transform(
                    skr::math::QuatF(rotations[i].get()),
                    random_pos,
                    scales[i].get());
                transforms[i].matrix = transform_matrix.to_matrix();

                // Set random color
                colors[i].color = random_color;

                if (emissions)
                {
                    emissions[i].color = skr::float4(10.f, 0.f, 2.f, 1.f);
                }

                // Add to GPU scene
                pScene->AddEntity(entities[i]);

                // resolve mesh
                meshes[i].mesh_resource = MeshAssetID;
                meshes[i].mesh_resource.resolve(true, pScene->GetECSWorld()->get_storage());

                local_index += 1;

                pModule->all_entities.insert(entities[i]);
            }
        }

        ModelViewerModule* pModule = nullptr;
        skr::renderer::GPUScene* pScene = nullptr;
        ComponentView<GPUSceneInstance> instances;
        ComponentView<GPUSceneInstanceColor> colors;
        ComponentView<GPUSceneObjectToWorld> transforms;
        ComponentView<GPUSceneInstanceEmission> emissions;
        ComponentView<skr::scene::PositionComponent> translations;
        ComponentView<skr::scene::RotationComponent> rotations;
        ComponentView<skr::scene::ScaleComponent> scales;
        ComponentView<skr::scene::IndexComponent> indices;
        ComponentView<skr::renderer::MeshComponent> meshes;
        uint32_t local_index = 0;
        std::mt19937* rng_ptr = nullptr;
    } spawner;
    spawner.pScene = &GPUScene;
    spawner.rng_ptr = &rng;
    spawner.pModule = this;
    world.create_entities(spawner, count);
}

void ModelViewerModule::DestroyRandomEntities(uint32_t count)
{
    using namespace skr::ecs;
    using namespace skr::renderer;

    static std::mt19937 rng(std::chrono::steady_clock::now().time_since_epoch().count());
    static std::uniform_real_distribution<float> rand(0.f, 1.f);
    for (uint32_t i = 0; i < count; i++)
    {
        for (auto entity : all_entities)
        {
            if (rand(rng) > 0.2f && GPUScene.CanRemoveEntity(entity))
            {
                GPUScene.RemoveEntity(entity);
                world.destroy_entities({ &entity, 1 });
                all_entities.erase(entity);
                break;
            }
        }
    }
}

void ModelViewerModule::InitializeAssetSystem()
{
    auto& system = *skd::asset::GetCookSystem();
    system.Initialize();
}

void ModelViewerModule::DestroyAssetSystem()
{
    auto& system = *skd::asset::GetCookSystem();
    system.Shutdown();
}

void ModelViewerModule::InitializeReosurceSystem()
{
    using namespace skr::literals;
    auto resource_system = skr::resource::GetResourceSystem();
    registry = SkrNew<skr::resource::LocalResourceRegistry>(project.GetResourceVFS());
    resource_system->Initialize(registry, project.GetRamService());
    const auto resource_root = project.GetResourceVFS()->mount_dir;
    {
        skr::String qn = u8"ModelViewer-JobQueue";
        auto job_queueDesc = make_zeroed<skr::JobQueueDesc>();
        job_queueDesc.thread_count = 2;
        job_queueDesc.priority = SKR_THREAD_NORMAL;
        job_queueDesc.name = qn.u8_str();
        job_queue = skr::SP<skr::JobQueue>::New(job_queueDesc);

        auto ioServiceDesc = make_zeroed<skr_ram_io_service_desc_t>();
        ioServiceDesc.name = u8"ModelViewer-RAMIOService";
        ioServiceDesc.sleep_time = 1000 / 60;
        ram_service = skr_io_ram_service_t::create(&ioServiceDesc);
        ram_service->run();

        auto vramServiceDesc = make_zeroed<skr_vram_io_service_desc_t>();
        vramServiceDesc.name = u8"ModelViewer-VRAMIOService";
        vramServiceDesc.awake_at_request = true;
        vramServiceDesc.ram_service = ram_service;
        vramServiceDesc.callback_job_queue = job_queue.get();
        vramServiceDesc.use_dstorage = true;
        vramServiceDesc.gpu_device = render_device->get_cgpu_device();
        vram_service = skr_io_vram_service_t::create(&vramServiceDesc);
        vram_service->run();
    }
    // texture sampler factory
    {
        skr::resource::TextureSamplerFactory::Root factoryRoot = {};
        factoryRoot.device = render_device->get_cgpu_device();
        TextureSamplerFactory = skr::resource::TextureSamplerFactory::Create(factoryRoot);
        resource_system->RegisterFactory(TextureSamplerFactory);
    }
    // texture factory
    {
        skr::resource::TextureFactory::Root factoryRoot = {};
        factoryRoot.dstorage_root = resource_root;
        factoryRoot.vfs = project.GetResourceVFS();
        factoryRoot.ram_service = ram_service;
        factoryRoot.vram_service = vram_service;
        factoryRoot.render_device = render_device;
        TextureFactory = skr::resource::TextureFactory::Create(factoryRoot);
        resource_system->RegisterFactory(TextureFactory);
    }
    // mesh factory
    {
        skr::renderer::MeshFactory::Root factoryRoot = {};
        factoryRoot.dstorage_root = resource_root;
        factoryRoot.vfs = project.GetResourceVFS();
        factoryRoot.ram_service = ram_service;
        factoryRoot.vram_service = vram_service;
        factoryRoot.render_device = render_device;
        MeshFactory = skr::renderer::MeshFactory::Create(factoryRoot);
        resource_system->RegisterFactory(MeshFactory);
    }
}

void ModelViewerModule::DestroyResourceSystem()
{
    auto resource_system = skr::resource::GetResourceSystem();
    resource_system->Shutdown();

    skr::resource::TextureSamplerFactory::Destroy(TextureSamplerFactory);
    skr::resource::TextureFactory::Destroy(TextureFactory);
    skr::renderer::MeshFactory::Destroy(MeshFactory);

    skr_io_ram_service_t::destroy(ram_service);
    skr_io_vram_service_t::destroy(vram_service);
    job_queue.reset();
}

void ModelViewerModule::on_unload()
{
    // save imported asset meta to disk
    project.SaveToDisk();

    // shutdown
    DestroyAssetSystem();
    DestroyResourceSystem();
    project.CloseProject();

    // destroy world
    world.finalize();

    // destroy gpu scene
    GPUScene.Shutdown();

    // shutdown task system
    scheduler.unbind();

    // Release compute pipeline resources
    if (compute_pipeline)
    {
        cgpu_free_compute_pipeline(compute_pipeline);
        compute_pipeline = nullptr;
    }

    if (root_signature)
    {
        cgpu_free_root_signature(root_signature);
        root_signature = nullptr;
    }

    if (compute_shader)
    {
        cgpu_free_shader_library(compute_shader);
        compute_shader = nullptr;
    }

    // shutdown logging
    skr_log_flush();
    skr_log_finalize_async_worker();
}

void ModelViewerModule::CreateComputePipeline()
{
    auto device = render_device->get_cgpu_device();

    // Create compute shader using scene_raytracing shader
    uint32_t *shader_bytes, shader_length;
    read_shader_bytes(u8"scene_raytracing.cs_main", &shader_bytes, &shader_length, device->adapter->instance->backend);
    CGPUShaderLibraryDescriptor shader_desc = {
        .name = u8"RaytracingComputeShader",
        .code = shader_bytes,
        .code_size = shader_length
    };
    compute_shader = cgpu_create_shader_library(device, &shader_desc);
    free(shader_bytes);

    // Create root signature
    const char8_t* push_constant_name = u8"camera_constants";
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
        .name = u8"RaytracingPipeline"
    };
    compute_pipeline = cgpu_create_compute_pipeline(device, &pipeline_desc);
}
