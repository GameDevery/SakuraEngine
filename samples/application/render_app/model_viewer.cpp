#include "SkrOS/filesystem.hpp"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrContainers/hashmap.hpp"
#include "SkrCore/module/module.hpp"
#include "SkrCore/async/thread_job.hpp"
#include "SkrTask/fib_task.hpp"
#include "SkrRT/resource/resource_system.h"
#include "SkrRT/resource/local_resource_registry.hpp"
#include "SkrScene/scene_components.h"
#include "SkrSystem/system_app.h"

#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrRenderer/resources/texture_resource.h"
#include "SkrRenderer/skr_renderer.h"
#include "SkrRenderer/render_app.hpp"
#include "SkrRenderer/gpu_scene.h"

#include "SkrRenderer/resources/mesh_resource.h"
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
                    project_path.c_str(), MetaDatabase.size());
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

    void CreateScene(skr::render_graph::RenderGraph* graph);
    void CreateComputePipeline();
    void render();

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

    struct CloseListener : public skr::ISystemEventHandler
    {
        void handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT 
        {
            if (event.window.type == SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED)
                want_exit = true;
        }
        bool want_exit = false;
    } close_listener;
    render_app->get_event_queue()->add_handler(&close_listener);
    
    // Create compute pipeline for debug rendering
    CreateComputePipeline();

    CreateScene(render_app->render_graph());

    // AsyncResource<> is a handle can be constructed by any resource type & ids
    skr::resource::AsyncResource<MeshResource> mesh_resource;
    mesh_resource = MeshAssetID;

    uint64_t frame_index = 0;
    while (!close_listener.want_exit)
    {
        render_app->get_event_queue()->pump_messages();
        render_app->acquire_frames();

        auto render_graph = render_app->render_graph();
        // Simple render loop using compute shader to fill screen red
        const auto screen_width = static_cast<uint32_t>(window_config.size.x);
        const auto screen_height = static_cast<uint32_t>(window_config.size.y);
        
        // Calculate dispatch groups for 16x16 kernel
        const uint32_t group_count_x = (screen_width + 15) / 16;
        const uint32_t group_count_y = (screen_height + 15) / 16;

        // Update GPUScene
        GPUScene.ExecuteUpload(render_graph);

        // Create output render target texture
        auto render_target_handle = render_graph->create_texture(
            [=](skr::render_graph::RenderGraph& g, skr::render_graph::TextureBuilder& builder) {
                builder.set_name(u8"render_target")
                    .extent(screen_width, screen_height)
                    .format(CGPU_FORMAT_R8G8B8A8_UNORM)
                    .allow_readwrite();
            }
        );
        
        // Add compute pass to fill screen with red
        render_graph->add_compute_pass(
            [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::ComputePassBuilder& builder) {
                builder.set_name(u8"GPUSceneDebugPass")
                    .set_pipeline(compute_pipeline)
                    .readwrite(u8"output_texture", render_target_handle)
                    .read(u8"gpu_scene_buffer", GPUScene.scene_buffer);
            },
            [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::ComputePassContext& ctx) {
                // Push constants
                struct SceneDebugConstants {
                    float screen_width;
                    float screen_height;
                    uint32_t debug_mode;    // 0 = red fill, 3 = GPUScene color debug
                    uint32_t color_segment_offset;
                    uint32_t color_element_size;
                    uint32_t instance_count;
                } constants;
                
                constants.screen_width = static_cast<float>(screen_width);
                constants.screen_height = static_cast<float>(screen_height);
                constants.debug_mode = 3; // Simple GPUScene validation mode
                
                // Get GPUScene color component info
                auto color_type_id = GPUScene.GetComponentTypeID(sugoi_id_of<GPUSceneInstanceColor>::get());
                constants.color_segment_offset = GPUScene.GetCoreComponentSegmentOffset(color_type_id);
                constants.color_element_size = sizeof(GPUSceneInstanceColor);
                constants.instance_count = GPUScene.core_data.allocator.get_instance_count();
                
                cgpu_compute_encoder_push_constants(ctx.encoder, root_signature, u8"debug_constants", &constants);
                cgpu_compute_encoder_dispatch(ctx.encoder, group_count_x, group_count_y, 1);
            }
        );

        // Add copy pass to copy render target to backbuffer
        auto backbuffer_handle = render_graph->get_imported(render_app->get_backbuffer(render_app->get_main_window()));
        render_graph->add_copy_pass(
            [=](skr::render_graph::RenderGraph& g, skr::render_graph::CopyPassBuilder& builder) {
                builder.set_name(u8"CopyToBackbuffer")
                    .texture_to_texture(render_target_handle, backbuffer_handle);
            },
            [=](skr::render_graph::RenderGraph& g, skr::render_graph::CopyPassContext& ctx) {
                // Copy implementation handled by render graph
            }
        );

        frame_index = render_graph->execute();
        if (frame_index >= RG_MAX_FRAME_IN_FLIGHT * 10)
            render_graph->collect_garbage(frame_index - RG_MAX_FRAME_IN_FLIGHT * 10);

        render_app->present_all();
    }
    render_app->close_all_windows();
    render_app->get_event_queue()->remove_handler(&close_listener);
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
            u8"girl.model.meta",                           // virtual uri for this asset in the project
            MeshAssetID,                                   // guid for this asset
            skr::type_id_of<skr::renderer::MeshResource>(),// output resource is a mesh resource 
            skr::type_id_of<skd::asset::MeshCooker>()      // this cooker cooks t he raw mesh data to mesh resource
        );
        // source file
        importer->assetPath = u8"D:/Code/SakuraEngine/samples/application/game/assets/sketchfab/loli/scene.gltf";
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

void ModelViewerModule::CreateScene(skr::render_graph::RenderGraph* graph)
{
    using namespace skr::ecs;
    using namespace skr::renderer;

    GPUSceneConfig cfg = {
        .world = &world,
        .render_device = render_device,
        .core_data_types = {
            { sugoi_id_of<GPUSceneObjectToWorld>::get(), 0 },
            { sugoi_id_of<GPUSceneInstanceColor>::get(), 1 }
        },
        .additional_data_types = {
            { sugoi_id_of<GPUSceneInstanceEmission>::get(), 2 }
        }
    };
    GPUScene.Initialize(render_device->get_cgpu_device(), cfg);

    struct Spawner
    {
        void build(skr::ecs::ArchetypeBuilder& Builder)
        {
            Builder.add_component<skr::scene::TransformComponent>()
                .add_component(&Spawner::instances)
                .add_component(&Spawner::colors)
                .add_component(&Spawner::transforms)

                .add_component(&Spawner::translations)
                .add_component(&Spawner::rotations)
                .add_component(&Spawner::scales)
                .add_component(&Spawner::indices);
        }

        void run(skr::ecs::TaskContext& Context)
        {
            auto cnt = Context.size();
            auto entities = Context.entities();
            for (uint32_t i = 0; i < cnt; i++)
            {
                colors[i].color = skr::float4(1.f, 0.f, 1.f, 1.f);

                pScene->RequireUpload(entities[i], sugoi_id_of<GPUSceneObjectToWorld>::get());
                pScene->RequireUpload(entities[i], sugoi_id_of<GPUSceneInstanceColor>::get());
                pScene->RequireUpload(entities[i], sugoi_id_of<GPUSceneInstanceEmission>::get());
            }
        }

        skr::renderer::GPUScene* pScene = nullptr;
        ComponentView<GPUSceneInstance> instances;
        ComponentView<GPUSceneInstanceColor> colors;
        ComponentView<GPUSceneObjectToWorld> transforms;
        ComponentView<skr::scene::PositionComponent> translations;
        ComponentView<skr::scene::RotationComponent> rotations;
        ComponentView<skr::scene::ScaleComponent> scales;
        ComponentView<skr::scene::IndexComponent> indices;
    } spawner;
    spawner.pScene = &GPUScene;
    world.create_entities(spawner, 10'000);
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
    
    // Create compute shader using scene_debug shader
    uint32_t *shader_bytes, shader_length;
    read_shader_bytes(u8"scene_debug.scene_debug", &shader_bytes, &shader_length, device->adapter->instance->backend);
    CGPUShaderLibraryDescriptor shader_desc = {
        .name = u8"DebugFillComputeShader",
        .code = shader_bytes,
        .code_size = shader_length
    };
    compute_shader = cgpu_create_shader_library(device, &shader_desc);
    free(shader_bytes);

    // Create root signature
    const char8_t* push_constant_name = u8"debug_constants";
    CGPUShaderEntryDescriptor compute_shader_entry = {
        .library = compute_shader,
        .entry = u8"scene_debug_main",
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
}
