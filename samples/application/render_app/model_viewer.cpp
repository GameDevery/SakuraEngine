#include "SkrOS/filesystem.hpp"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrCore/module/module.hpp"
#include "SkrCore/async/thread_job.hpp"
#include "SkrTask/fib_task.hpp"
#include "SkrRT/resource/resource_system.h"
#include "SkrRT/resource/local_resource_registry.hpp"
#include "SkrSystem/system_app.h"

#include "SkrToolCore/cook_system/cook_system.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrRenderer/resources/texture_resource.h"
#include "SkrRenderer/skr_renderer.h"
#include "SkrRenderer/render_app.hpp"

#include "SkrRenderer/resources/mesh_resource.h"
#include "SkrMeshCore/mesh_processing.hpp"
#include "SkrGLTFTool/mesh_asset.hpp"

struct VirtualProject : skd::SProject
{
    VirtualProject()
    {
        /*
        auto meta = u8R"_({
            "guid": "18db1369-ba32-4e91-aa52-b2ed1556f576",
            "type": "3b8ca511-33d1-4db4-b805-00eea6a8d5e1",
            "importer": {
                "importer_type": "D72E2056-3C12-402A-A8B8-148CB8EAB922",
                "assetPath": "D:/Code/SakuraEngine/samples/application/game/assets/sketchfab/loli/scene.gltf"
            },
            "vertexType": "C35BD99A-B0A8-4602-AFCC-6BBEACC90321"
        })_";
        MetaDatabase.emplace(u8"girl.gltf", meta);
        */
    }
    bool LoadAssetMeta(skr::StringView uri, skr::String& content) noexcept override
    {
        if (MetaDatabase.contains(uri))
        {
            content = MetaDatabase[uri];   
            return true;
        }
        return false;
    }
    skr::ParallelFlatHashMap<skr::String, skr::String, skr::Hash<skr::String>> MetaDatabase;  
};

struct ModelViewerModule : public skr::IDynamicModule
{
public:
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

protected:
    void CookAndLoadGLTF();
    void InitializeAssetSystem();
    void DestroyAssetSystem();
    void InitializeReosurceSystem();
    void DestroyResourceSystem();

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

    InitializeReosurceSystem();
    InitializeAssetSystem();

    CookAndLoadGLTF();
}

int ModelViewerModule::main_module_exec(int argc, char8_t** argv)
{
    using namespace skr;
    auto device = render_device->get_cgpu_device();
    auto gfx_queue = render_device->get_gfx_queue();
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
            {
                want_exit = true;
            }
        }
        bool want_exit = false;
    } close_listener;
    render_app->get_event_queue()->add_handler(&close_listener);

    skr::resource::AsyncResource<skr::renderer::MeshResource> mesh_resource;
    mesh_resource = u8"18db1369-ba32-4e91-aa52-b2ed1556f576"_guid;

    while (!close_listener.want_exit)
    {
        render_app->get_event_queue()->pump_messages();

        auto resource_system = skr::resource::GetResourceSystem();
        resource_system->Update();

        mesh_resource.resolve(true, 0, ESkrRequesterType::SKR_REQUESTER_SYSTEM);
        if (mesh_resource.is_resolved())
        {
            auto MeshResource = mesh_resource.get_resolved(true);
            MeshResource = mesh_resource.get_resolved(true);
        }
    }
    render_app->close_all_windows();
    render_app->get_event_queue()->remove_handler(&close_listener);
    return 0;
}

void ModelViewerModule::on_unload()
{
    DestroyAssetSystem();
    DestroyResourceSystem();

    project.CloseProject();

    scheduler.unbind();

    skr_log_flush();
    skr_log_finalize_async_worker();
}

void ModelViewerModule::CookAndLoadGLTF()
{
    auto& System = *skd::asset::GetCookSystem();
    const bool NeedImport = true;
    if (NeedImport) // Import from disk!
    {
        using namespace skr::literals;
        // source file is a GLTF so we create a gltf importer, if it is a fbx, then just use a fbx importer
        auto importer = skr::RC<skd::asset::GltfMeshImporter>::New();
        importer->importer_type = skr::type_id_of<skd::asset::GltfMeshImporter>();

        // static mesh has some additional meta data to append
        auto metadata = skr::RC<skd::asset::MeshAsset>::New();
        metadata->vertexType = u8"C35BD99A-B0A8-4602-AFCC-6BBEACC90321"_guid;

        auto asset = skr::RC<skd::asset::AssetMetaFile>::New(
            u8"girl.gltf",                                 // virtual uri for this asset in the project
            u8"18db1369-ba32-4e91-aa52-b2ed1556f576"_guid, // guid for this asset
            skr::type_id_of<skr::renderer::MeshResource>(),// output resource is a mesh resource 
            skr::type_id_of<skd::asset::MeshCooker>()      // this cooker cooks the raw mesh data to mesh resource
        );
        // source file
        importer->assetPath = u8"D:/Code/SakuraEngine/samples/application/game/assets/sketchfab/loli/scene.gltf";
        System.ImportAsset(&project, asset, importer, metadata);
     
        auto event = System.EnsureCooked(asset->GetGUID());
        event.wait(true);
    }
    else // Load imported asset from asset...
    {
        auto asset = System.LoadAssetMeta(&project, u8"girl.gltf");
        auto event = System.EnsureCooked(asset->GetGUID());
        event.wait(true);
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