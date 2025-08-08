#include <SkrBase/misc/make_zeroed.hpp>
#include <SkrBase/containers/path/path.hpp>
#include <SkrCore/memory/sp.hpp>
#include <SkrCore/module/module_manager.hpp>
#include <SkrCore/log.h>
#include <SkrCore/platform/vfs.h>
#include <SkrCore/time.h>
#include <SkrCore/async/thread_job.hpp>
#include <SkrRT/io/vram_io.hpp>
#include "SkrOS/thread.h"
#include "SkrProfile/profile.h"
#include "SkrRT/io/ram_io.hpp"
#include "SkrRT/misc/cmd_parser.hpp"

#include <SkrOS/filesystem.hpp>
#include "SkrCore/memory/impl/skr_new_delete.hpp"
#include "SkrSystem/advanced_input.h"
#include <SkrRT/ecs/world.hpp>
#include <SkrRT/resource/resource_system.h>
#include <SkrRT/resource/local_resource_registry.hpp>
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrImGui/imgui_app.hpp"
#include "SkrRenderer/skr_renderer.h"
#include "SkrRenderer/resources/mesh_resource.h"
#include "SkrRenderer/render_mesh.h"
#include "SkrTask/fib_task.hpp"
#include "SkrMeshCore/mesh_processing.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrGLTFTool/mesh_asset.hpp"

#include "SkrScene/actor.h"
#include "SkrSceneCore/transform_system.h"

#include "scene_renderer.hpp"
#include "scene_render_system.h"

#include "helper.hpp"

using namespace skr::literals;
const auto MeshAssetID = u8"01988203-c467-72ef-916b-c8a5db2ec18d"_guid;

// The Three-Triangle Example: simple mesh scene hierarchy
struct SceneSampleSkelMeshModule : public skr::IDynamicModule
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
    skr::resource::LocalResourceRegistry* registry = nullptr;
    skr::renderer::MeshFactory* mesh_factory = nullptr;
    skd::SProject project;
    skr::ActorManager& actor_manager = skr::ActorManager::GetInstance();
    skr::TransformSystem* transform_system = nullptr;
    skr::scene::SceneRenderSystem* scene_render_system = nullptr;
};

IMPLEMENT_DYNAMIC_MODULE(SceneSampleSkelMeshModule, SceneSample_SkelMesh);

void SceneSampleSkelMeshModule::InitializeAssetSystem()
{
    auto& system = *skd::asset::GetCookSystem();
    system.Initialize();
}

void SceneSampleSkelMeshModule::DestroyAssetSystem()
{
    auto& system = *skd::asset::GetCookSystem();
    system.Shutdown();
}

void SceneSampleSkelMeshModule::InitializeResourceSystem()
{
    using namespace skr::literals;
    auto resource_system = skr::resource::GetResourceSystem();
    registry = SkrNew<skr::resource::LocalResourceRegistry>(project.GetResourceVFS());
    resource_system->Initialize(registry, project.GetRamService());
    const auto resource_root = project.GetResourceVFS()->mount_dir;
    {
        skr::String qn = u8"SceneSampleSkelMesh-JobQueue";
        auto job_queueDesc = make_zeroed<skr::JobQueueDesc>();
        job_queueDesc.thread_count = 2;
        job_queueDesc.priority = SKR_THREAD_NORMAL;
        job_queueDesc.name = qn.u8_str();
        job_queue = skr::SP<skr::JobQueue>::New(job_queueDesc);

        auto ioServiceDesc = make_zeroed<skr_ram_io_service_desc_t>();

        ioServiceDesc.name = u8"S012-3456-789A-BCDE-FGHI-JKLM-N"; // 31 byte, pass
        //ioServiceDesc.name = u8"S012-3456-789A-BCDE-FGHI-JKLM-NO"; // 32 byte, fail

        ioServiceDesc.sleep_time = 1000 / 60;
        ram_service = skr_io_ram_service_t::create(&ioServiceDesc);
        ram_service->run();

        auto vramServiceDesc = make_zeroed<skr_vram_io_service_desc_t>();
        vramServiceDesc.name = u8"SkelMesh-VRAMIOService";
        vramServiceDesc.awake_at_request = true;
        vramServiceDesc.ram_service = ram_service;
        vramServiceDesc.callback_job_queue = job_queue.get();
        vramServiceDesc.use_dstorage = true;
        vramServiceDesc.gpu_device = render_device->get_cgpu_device();
        vram_service = skr_io_vram_service_t::create(&vramServiceDesc);
        vram_service->run();
    }
    // mesh factory
    {
        skr::renderer::MeshFactory::Root factoryRoot = {};
        factoryRoot.dstorage_root = resource_root;
        factoryRoot.vfs = project.GetResourceVFS();
        factoryRoot.ram_service = ram_service;
        factoryRoot.vram_service = vram_service;
        factoryRoot.render_device = render_device;
        mesh_factory = skr::renderer::MeshFactory::Create(factoryRoot);
        resource_system->RegisterFactory(mesh_factory);
    }
}

void SceneSampleSkelMeshModule::DestroyResourceSystem()
{
    auto resource_system = skr::resource::GetResourceSystem();
    resource_system->Shutdown();

    skr::renderer::MeshFactory::Destroy(mesh_factory);

    skr_io_ram_service_t::destroy(ram_service);
    skr_io_vram_service_t::destroy(vram_service);
    job_queue.reset();
}

void SceneSampleSkelMeshModule::on_load(int argc, char8_t** argv)
{
    skr_log_set_level(SKR_LOG_LEVEL_INFO);
    SKR_LOG_INFO(u8"Scene Sample Mesh Module Loaded");

    skr::cmd::parser parser(argc, (char**)argv);
    parser.add(u8"gltf", u8"gltf file path", u8"-g", false);
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
    scene_render_system = skr_scene_render_system_create(&world);
    render_device = SkrRendererModule::Get()->get_render_device();

    auto resourceRoot = (skr::fs::current_directory() / u8"../resources");
    skr_vfs_desc_t vfs_desc = {};
    vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
    vfs_desc.override_mount_dir = resourceRoot.c_str();
    resource_vfs = skr_create_vfs(&vfs_desc);

    auto projectRoot = skr::fs::current_directory() / u8"../resources/scene/sample_skelmesh";
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

    skr::String projectName = u8"SceneSampleSkelMesh";
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

void SceneSampleSkelMeshModule::on_unload()
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
    skr_scene_render_system_destroy(scene_render_system);

    actor_manager.finalize();
    world.finalize();
    scheduler.unbind();
    SKR_LOG_INFO(u8"Scene Sample Mesh Module Unloaded");
}

void SceneSampleSkelMeshModule::CookAndLoadGLTF()
{
    auto& System = *skd::asset::GetCookSystem();
    auto importer = skd::asset::GltfMeshImporter::Create<skd::asset::GltfMeshImporter>();
    auto metadata = skd::asset::MeshAsset::Create<skd::asset::MeshAsset>();
    metadata->vertexType = u8"1b357a40-83ff-471c-8903-23e99d95b273"_guid; // GLTFVertexLayoutWithoutTangentId
    auto asset = skr::RC<skd::asset::AssetMetaFile>::New(
        u8"girl.gltf.meta",
        MeshAssetID,
        skr::type_id_of<skr::renderer::MeshResource>(),
        skr::type_id_of<skd::asset::MeshCooker>());
    importer->assetPath = gltf_path.c_str();
    System.ImportAssetMeta(&project, asset, importer, metadata);
    auto event = System.EnsureCooked(asset->GetGUID());
    event.wait(true);
}

int SceneSampleSkelMeshModule::main_module_exec(int argc, char8_t** argv)
{

    SkrZoneScopedN("SceneSampleSkelMeshModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Scene Sample SkelMesh Module");

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

    skr::Vector<skr::RCWeak<skr::MeshActor>> hierarchy_actors;
    constexpr int hierarchy_count = 3; // Number of actors in the hierarchy

    auto root = skr::Actor::GetRoot();
    auto actor1 = skr::MeshActor::CreateActor(skr::EActorType::Mesh).cast_static<skr::MeshActor>();

    actor1.lock()->SetDisplayName(u8"Actor 1");

    root.lock()->CreateEntity();
    actor1.lock()->CreateEntity();

    actor1.lock()->AttachTo(root);

    root.lock()->GetPositionComponent()->set({ 0.0f, 0.0f, 0.0f });

    actor1.lock()->GetPositionComponent()->set({ 0.0f, 1.0f, 0.0f });
    actor1.lock()->GetScaleComponent()->set({ .1f, .1f, .1f });
    actor1.lock()->GetRotationComponent()->set({ 0.0f, 0.0f, 0.0f });
    for (auto i = 0; i < hierarchy_count; ++i)
    {
        auto actor = skr::MeshActor::CreateActor(skr::EActorType::Mesh).cast_static<skr::MeshActor>();
        hierarchy_actors.push_back(actor);

        actor.lock()->SetDisplayName(skr::format(u8"Actor {}", i + 2).c_str());
        actor.lock()->CreateEntity();
        if (i == 0)
        {
            actor.lock()->AttachTo(actor1);
        }
        else
        {
            actor.lock()->AttachTo(hierarchy_actors[i - 1]);
        }

        actor.lock()->GetPositionComponent()->set({ 0.0f, 0.0f, (float)(i + 1) * 5.0f });
        actor.lock()->GetScaleComponent()->set({ .8f, .8f, .8f });
    }

    transform_system->update();
    skr::ecs::TaskScheduler::Get()->sync_all();

    skr_mesh_resource_t* mesh_resource = nullptr;
    skr_render_mesh_id render_mesh = SkrNew<skr_render_mesh_t>();

    actor1.lock()->GetMeshComponent()->mesh_resource = MeshAssetID;
    // actor2.lock()->GetMeshComponent()->mesh_resource = MeshAssetID;
    for (auto& actor : hierarchy_actors)
    {
        actor.lock()->GetMeshComponent()->mesh_resource = MeshAssetID;
    }

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

    auto resource_system = skr::resource::GetResourceSystem();

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
            scene_render_system->update();
            skr::ecs::TaskScheduler::Get()->sync_all();
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
    skr_render_mesh_free(render_mesh);
    mesh_resource->bins.clear();
    mesh_resource->render_mesh = nullptr;
    SkrDelete(mesh_resource);
    return 0;
}