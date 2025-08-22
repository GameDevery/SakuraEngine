#include <SkrBase/misc/make_zeroed.hpp>
#include <SkrCore/memory/sp.hpp>
#include <SkrCore/module/module_manager.hpp>
#include <SkrCore/log.h>
#include <SkrCore/platform/vfs.h>
#include <SkrCore/time.h>
#include <SkrCore/async/thread_job.hpp>
#include "SkrCore/memory/impl/skr_new_delete.hpp"
#include <SkrRT/io/vram_io.hpp>
#include "SkrOS/thread.h"
#include "SkrProfile/profile.h"
#include "SkrRT/ecs/scheduler.hpp"
#include "SkrRT/io/ram_io.hpp"
#include "SkrRT/misc/cmd_parser.hpp"
#include "SkrRTTR/rttr_traits.hpp"

#include <SkrOS/filesystem.hpp>
#include "SkrSystem/advanced_input.h"
#include <SkrRT/ecs/world.hpp>
#include <SkrRT/resource/resource_system.h>
#include <SkrRT/resource/local_resource_registry.hpp>
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrImGui/imgui_app.hpp"

#include "SkrRenderer/skr_renderer.h"
#include "SkrRenderer/resources/mesh_resource.h"
#include "SkrRenderer/resources/texture_resource.h"
#include "SkrRenderer/resources/material_resource.hpp"
#include "SkrRenderer/render_mesh.h"

#include "SkrTask/fib_task.hpp"
#include "SkrMeshCore/mesh_processing.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrToolCore/cook_system/cook_system.hpp"

#include "SkrTextureCompiler/texture_compiler.hpp"
#include "SkrTextureCompiler/texture_sampler_asset.hpp"

#include "SkrGLTFTool/mesh_asset.hpp"
#include "SkrMeshCore/builtin_mesh_asset.hpp"
#include "SkrMeshCore/builtin_mesh.hpp"

#include "SkrScene/actor.h"
#include "SkrSceneCore/transform_system.h"

#include "scene_renderer.hpp"
#include "scene_render_system.h"

#include "helper.hpp"

using namespace skr::literals;
const auto MeshAssetID = u8"01985f1f-8286-773f-8bcc-7d7451b0265d"_guid;
const auto TextureID = u8"0198cb9d-35ab-7342-bd41-21f61e1d0d8e"_guid;
const auto BuiltinMeshID = u8"0198cfc4-9bfd-77fd-a9a0-553938b10314"_guid;

// The Three-Triangle Example: simple mesh scene hierarchy
struct SceneSampleMeshModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

    void InitializeResourceSystem();
    void InitializeAssetSystem();
    void DestroyAssetSystem();
    void DestroyResourceSystem();

    void CookAndLoadGLTF();

    skr::task::scheduler_t scheduler;
    skr::ecs::World world{ scheduler };
    skr_vfs_t* resource_vfs = nullptr;
    skr::io::IRAMService* ram_service = nullptr;
    skr_io_vram_service_t* vram_service = nullptr;
    skr::SP<skr::JobQueue> job_queue = nullptr;
    SRenderDeviceId render_device = nullptr;

    skr::UPtr<skr::ImGuiApp> imgui_app = nullptr;
    skr::SceneRenderer* scene_renderer = nullptr;

    skr::String gltf_path = u8"";
    skr::LocalResourceRegistry* registry = nullptr;
    skr::MeshFactory* mesh_factory = nullptr;
    skr::TextureSamplerFactory* TextureSamplerFactory = nullptr;
    skr::TextureFactory* TextureFactory = nullptr;
    skr::MaterialFactory* matFactory = nullptr;

    skd::SProject project;
    skr::ActorManager& actor_manager = skr::ActorManager::GetInstance();
    skr::TransformSystem* transform_system = nullptr;
    skr::scene::SceneRenderSystem* scene_render_system = nullptr;
};

IMPLEMENT_DYNAMIC_MODULE(SceneSampleMeshModule, SceneSample_Mesh);

void SceneSampleMeshModule::InitializeAssetSystem()
{
    auto& system = *skd::asset::GetCookSystem();
    system.Initialize();
}

void SceneSampleMeshModule::DestroyAssetSystem()
{
    auto& system = *skd::asset::GetCookSystem();
    system.Shutdown();
}

void SceneSampleMeshModule::InitializeResourceSystem()
{
    using namespace skr::literals;
    auto resource_system = skr::GetResourceSystem();
    registry = SkrNew<skr::LocalResourceRegistry>(project.GetResourceVFS());
    resource_system->Initialize(registry, project.GetRamService());

    const auto resource_root = project.GetResourceVFS()->mount_dir;
    {
        skr::String qn = u8"SceneSampleMesh-JobQueue";
        auto job_queueDesc = make_zeroed<skr::JobQueueDesc>();
        job_queueDesc.thread_count = 2;
        job_queueDesc.priority = SKR_THREAD_NORMAL;
        job_queueDesc.name = qn.u8_str();
        job_queue = skr::SP<skr::JobQueue>::New(job_queueDesc);

        auto ioServiceDesc = make_zeroed<skr_ram_io_service_desc_t>();
        ioServiceDesc.name = u8"SceneSampleMesh-RAMIOService";
        ioServiceDesc.sleep_time = 1000 / 60;
        ram_service = skr_io_ram_service_t::create(&ioServiceDesc);
        ram_service->run();

        auto vramServiceDesc = make_zeroed<skr_vram_io_service_desc_t>();
        vramServiceDesc.name = u8"SceneSampleMesh-VRAMIOService";
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
        skr::TextureSamplerFactory::Root factoryRoot = {};
        factoryRoot.device = render_device->get_cgpu_device();
        TextureSamplerFactory = skr::TextureSamplerFactory::Create(factoryRoot);
        resource_system->RegisterFactory(TextureSamplerFactory);
    }
    // texture factory
    {
        skr::TextureFactory::Root factoryRoot = {};
        factoryRoot.dstorage_root = resource_root;
        factoryRoot.vfs = project.GetResourceVFS();
        factoryRoot.ram_service = ram_service;
        factoryRoot.vram_service = vram_service;
        factoryRoot.render_device = render_device;
        TextureFactory = skr::TextureFactory::Create(factoryRoot);
        resource_system->RegisterFactory(TextureFactory);
    }
    // mesh factory
    {
        skr::MeshFactory::Root factoryRoot = {};
        factoryRoot.dstorage_root = resource_root;
        factoryRoot.vfs = project.GetResourceVFS();
        factoryRoot.ram_service = ram_service;
        factoryRoot.vram_service = vram_service;
        factoryRoot.render_device = render_device;
        mesh_factory = skr::MeshFactory::Create(factoryRoot);
        resource_system->RegisterFactory(mesh_factory);
    }
}

void SceneSampleMeshModule::DestroyResourceSystem()
{
    auto resource_system = skr::GetResourceSystem();
    resource_system->Shutdown();

    skr::MeshFactory::Destroy(mesh_factory);
    skr::TextureFactory::Destroy(TextureFactory);
    skr::TextureSamplerFactory::Destroy(TextureSamplerFactory);

    skr_io_ram_service_t::destroy(ram_service);
    skr_io_vram_service_t::destroy(vram_service);
    job_queue.reset();
}

void SceneSampleMeshModule::on_load(int argc, char8_t** argv)
{
    skr_log_set_level(SKR_LOG_LEVEL_INFO);
    SKR_LOG_INFO(u8"Scene Sample Mesh Module Loaded");

    skr::cmd::parser parser(argc, (char**)argv);
    parser.add(u8"gltf", u8"use gltf", u8"-g", false);

    if (!parser.parse())
    {
        SKR_LOG_ERROR(u8"Failed to parse command line arguments.");
        return;
    }
    auto gltf_path_opt = parser.get_optional<skr::String>(u8"gltf");
    if (gltf_path_opt)
    {
        gltf_path = *gltf_path_opt;
        SKR_LOG_INFO(u8"gltf file path set to: %s", gltf_path.c_str());
    }
    else
    {
        SKR_LOG_INFO(u8"No gltf file specified");
    }
    scheduler.initialize({});
    scheduler.bind();
    world.initialize();
    actor_manager.initialize(&world);
    transform_system = skr_transform_system_create(&world);
    scene_render_system = skr::scene::SceneRenderSystem::Create(&world);
    render_device = SkrRendererModule::Get()->get_render_device();

    auto resourceRoot = (skr::fs::current_directory() / u8"../resources");
    skr_vfs_desc_t vfs_desc = {};
    vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
    vfs_desc.override_mount_dir = resourceRoot.c_str();
    resource_vfs = skr_create_vfs(&vfs_desc);

    auto projectRoot = skr::fs::current_directory() / u8"../resources/scene/sample_mesh";
    // if not exists, create the project root directory
    if (!skr::fs::Directory::exists(projectRoot))
    {
        skr::fs::Directory::create(projectRoot, true);
    }
    skd::SProjectConfig projectConfig = {
        .assetDirectory = (projectRoot / u8"assets").string().c_str(),
        .resourceDirectory = (projectRoot / u8"resources").string().c_str(),
        .artifactsDirectory = (projectRoot / u8"artifacts").string().c_str()
    };

    skr::String projectName = u8"SceneSampleMesh";
    skr::String rootPath = projectRoot.string().c_str();
    project.OpenProject(projectName.c_str(), rootPath.c_str(), projectConfig);
    {
        InitializeResourceSystem();
        InitializeAssetSystem();
    }
    scene_renderer = skr::SceneRenderer::Create();
    scene_renderer->initialize(render_device, &world, resource_vfs);
    scene_render_system->bind_renderer(scene_renderer);
}

void SceneSampleMeshModule::on_unload()
{
    project.CloseProject();
    scene_renderer->finalize(SkrRendererModule::Get()->get_render_device());
    skr::SceneRenderer::Destroy(scene_renderer);
    // stop all ram_service
    {
        DestroyAssetSystem();
        DestroyResourceSystem();
    }
    skr_transform_system_destroy(transform_system);
    skr::scene::SceneRenderSystem::Destroy(scene_render_system);

    actor_manager.finalize();
    world.finalize();
    scheduler.unbind();
    SKR_LOG_INFO(u8"Scene Sample Mesh Module Unloaded");
}

void SceneSampleMeshModule::CookAndLoadGLTF()
{
    auto& System = *skd::asset::GetCookSystem();

    // auto textureImporter = skd::asset::TextureImporter::Create<skd::asset::TextureImporter>();
    // auto textureAsset = skr::RC<skd::asset::AssetMetaFile>::New(
    //     u8"test_texture.meta",
    //     TextureID,
    //     skr::type_id_of<skr::TextureResource>(),
    //     skr::type_id_of<skd::asset::TextureCooker>());
    // textureImporter->assetPath = u8"D:/ws/ext/glTFSample/media/Cauldron-Media/buster_drone/Assets/Models/Public/BusterDrone/Materials/body_albedo2048.png";
    // System.ImportAssetMeta(&project, textureAsset, textureImporter);
    auto metadata = skd::asset::MeshAsset::Create<skd::asset::MeshAsset>();
    metadata->vertexType = u8"C35BD99A-B0A8-4602-AFCC-6BBEACC90321"_guid; //    GLTFVertexLayoutWithJointId

    auto builtin_importer = skd::asset::BuiltinMeshImporter::Create<skd::asset::BuiltinMeshImporter>();
    builtin_importer->built_in_mesh_tid = skr::type_id_of<skd::asset::SimpleTriangleMesh>();
    auto builtin_asset = skr::RC<skd::asset::AssetMetaFile>::New(
        u8"simple_triangle.meta",
        BuiltinMeshID,
        skr::type_id_of<skr::MeshResource>(),
        skr::type_id_of<skd::asset::BuiltinMeshCooker>());
    System.ImportAssetMeta(&project, builtin_asset, builtin_importer, metadata);
    auto builtin_event = System.EnsureCooked(BuiltinMeshID);
    builtin_event.wait(true);

    auto importer = skd::asset::GltfMeshImporter::Create<skd::asset::GltfMeshImporter>();

    // metadata->materials.push_back(TextureID);                             // Use the texture we just imported
    auto asset = skr::RC<skd::asset::AssetMetaFile>::New(
        u8"test.gltf.meta",
        MeshAssetID,
        skr::type_id_of<skr::MeshResource>(),
        skr::type_id_of<skd::asset::MeshCooker>());
    importer->assetPath = gltf_path.c_str();

    System.ImportAssetMeta(&project, asset, importer, metadata);

    auto event = System.EnsureCooked(asset->GetGUID());
    event.wait(true);
}

int SceneSampleMeshModule::main_module_exec(int argc, char8_t** argv)
{
    using namespace skr;

    SkrZoneScopedN("SceneSampleMeshModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Scene Sample Mesh Module");

    SKR_LOG_INFO(u8"gltf file path: {%s}", gltf_path.c_str());
    if (gltf_path.is_empty())
    {
        SKR_LOG_ERROR(u8"gltf file path is empty, please specify a valid gltf file path.");
        return 1;
    }

    utils::Camera camera;
    scene_renderer->set_camera(&camera);
    utils::CameraController cam_ctrl;
    cam_ctrl.set_camera(&camera);

    auto cgpu_device = render_device->get_cgpu_device();
    auto gfx_queue = render_device->get_gfx_queue();

    // skr::Vector<skr::RCWeak<skr::MeshActor>> hierarchy_actors;
    // constexpr int hierarchy_count = 3; // Number of actors in the hierarchy

    auto root = skr::Actor::GetRoot();
    auto actor1 = actor_manager.CreateActor<skr::MeshActor>().cast_static<skr::MeshActor>();
    auto actor2 = actor_manager.CreateActor<skr::MeshActor>().cast_static<skr::MeshActor>();

    actor1.lock()->SetDisplayName(u8"Actor 1");
    actor2.lock()->SetDisplayName(u8"Actor 2");

    root.lock()->CreateEntity();
    actor1.lock()->CreateEntity();
    actor2.lock()->CreateEntity();

    actor1.lock()->AttachTo(root);
    actor2.lock()->AttachTo(root);

    root.lock()->GetComponent<skr::scene::PositionComponent>()->set({ 0.0f, 0.0f, 0.0f });

    actor1.lock()->GetComponent<skr::scene::PositionComponent>()->set({ 0.0f, 1.0f, 0.0f });
    actor1.lock()->GetComponent<skr::scene::ScaleComponent>()->set({ .1f, .1f, .1f });
    actor1.lock()->GetComponent<skr::scene::RotationComponent>()->set({ 0.0f, 0.0f, 0.0f });

    actor2.lock()->GetComponent<skr::scene::PositionComponent>()->set({ 5.0f, 5.0f, 1.0f });
    actor2.lock()->GetComponent<skr::scene::ScaleComponent>()->set({ 8.f, 8.f, 2.0f });
    actor2.lock()->GetComponent<skr::scene::RotationComponent>()->set({ 0.0f, 0.0f, 0.0f });

    // for (auto i = 0; i < hierarchy_count; ++i)
    // {
    //     auto actor = actor_manager.CreateActor<skr::MeshActor>().cast_static<skr::MeshActor>();
    //     hierarchy_actors.push_back(actor);

    //     actor.lock()->SetDisplayName(skr::format(u8"HActor {}", i).c_str());
    //     actor.lock()->CreateEntity();

    //     if (i == 0)
    //     {
    //         actor.lock()->AttachTo(actor1);
    //     }
    //     else
    //     {
    //         actor.lock()->AttachTo(hierarchy_actors[i - 1]);
    //     }

    //     actor.lock()->GetComponent<skr::scene::PositionComponent>()->set({ 0.0f, 0.0f, (float)(i + 1) * 5.0f });
    //     actor.lock()->GetComponent<skr::scene::ScaleComponent>()->set({ .8f, .8f, .8f });
    // }

    transform_system->update();
    transform_system->get_context()->update_finish.wait(true);

    actor1.lock()->GetComponent<skr::MeshComponent>()->mesh_resource = MeshAssetID;
    actor2.lock()->GetComponent<skr::MeshComponent>()->mesh_resource = BuiltinMeshID;
    // for (auto& actor : hierarchy_actors)
    // {
    //     actor.lock()->GetComponent<skr::MeshComponent>()->mesh_resource = MeshAssetID;
    // }

    CookAndLoadGLTF();

    {
        skr::render_graph::RenderGraphBuilder graph_builder;
        graph_builder.with_device(cgpu_device)
            .with_gfx_queue(gfx_queue)
            .enable_memory_aliasing();

        skr::SystemWindowCreateInfo main_window_info = {
            .title = skr::format(u8"Scene Viewer [{}]", gCGPUBackendNames[cgpu_device->adapter->instance->backend]),
            .size = { 1024, 768 },
        };

        imgui_app = skr::UPtr<skr::ImGuiApp>::New(main_window_info, render_device, graph_builder);
        imgui_app->initialize();
        imgui_app->enable_docking();
    }
    auto render_graph = imgui_app->render_graph();
    // Time
    SHiresTimer tick_timer;
    int64_t elapsed_us = 0;
    int64_t elapsed_frame = 0;
    int64_t fps = 60;
    skr_init_hires_timer(&tick_timer);
    uint64_t frame_index = 0;

    skr::input::Input::Initialize();

    auto resource_system = skr::GetResourceSystem();

    while (!imgui_app->want_exit().comsume())
    {
        SkrZoneScopedN("LoopBody");
        {
            SkrZoneScopedN("PumpMessage");
            imgui_app->pump_message();
        }
        {
            // ResourceSystem Update
            resource_system->Update();
        }
        {
            // Update Actor Transform
            transform_system->update();
        }

        // Camera control
        {
            SkrZoneScopedN("CameraControl");
            cam_ctrl.imgui_control_frame();
        }

        {
            SkrZoneScopedN("ViewportRender");
            imgui_app->acquire_frames();
            auto main_window = imgui_app->main_window();
            const auto size = main_window->get_physical_size();
            camera.aspect = (float)size.x / (float)size.y;
        };
        {
            transform_system->get_context()->update_finish.wait(true);
            scene_render_system->update();
            scene_render_system->get_context()->update_finish.wait(true);
            scene_renderer->draw_primitives(
                render_graph,
                scene_render_system->get_drawcalls());
        }

        {
            SkrZoneScopedN("ImGuiRender");
            imgui_app->set_load_action(CGPU_LOAD_ACTION_LOAD);
            imgui_app->render_imgui();
        }
        {
            SkrZoneScopedN("RenderGraphExecute");
            frame_index = render_graph->execute();
            if (frame_index >= RG_MAX_FRAME_IN_FLIGHT * 10)
                render_graph->collect_garbage(frame_index - RG_MAX_FRAME_IN_FLIGHT * 10);
        }
        // present
        imgui_app->present_all();
    }

    cgpu_wait_queue_idle(gfx_queue);
    imgui_app->shutdown();
    skr::input::Input::Finalize();
    return 0;
}