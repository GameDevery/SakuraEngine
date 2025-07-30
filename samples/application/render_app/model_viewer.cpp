#include "SkrOS/filesystem.hpp"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrContainers/hashmap.hpp"
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
        {
            SKR_LOG_INFO(u8"[ModelViewer] Project saved to: %s", project_path.c_str());
        }
        else
        {
            SKR_LOG_ERROR(u8"[ModelViewer] Failed to save project to: %s", project_path.c_str());
        }
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
}

int ModelViewerModule::main_module_exec(int argc, char8_t** argv)
{
    using namespace skr;
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

    // AsyncResource<> is a handle can be constructed by any resource type & ids
    skr::resource::AsyncResource<skr::renderer::MeshResource> mesh_resource;
    mesh_resource = MeshAssetID;

    while (!close_listener.want_exit)
    {
        render_app->get_event_queue()->pump_messages();

        // 'Update' polls resource requests and handles their loading & installing
        // 'load': usually means 'fetch data blocks from disk' & 'deserialize object from these data'
        // 'install': usually means some heavy post-works, like uploading vertices to GPU, build BLAS for meshes, etc.
        // This 'Update' call is able to and should be called from [MULTIPLE] background thread workers.
        // In this sample we call 'Update' here in main eventloop just for ease. 
        resource_system->Update();

        // Resolve issues a install 'request' for mesh_resource, ResourceSystem will load & install it automatically behind background during 'Update' 
        mesh_resource.resolve(true, 0, ESkrRequesterType::SKR_REQUESTER_SYSTEM);
        
        // Resolving the resource is an async progress, thus 'is_resolved' handle will be like an async flag
        // Once the resource is ready in some frame, we can fetch and use the object in AsyncResource<>
        if (mesh_resource.is_resolved())
        {
            auto MeshResource = mesh_resource.get_resolved(true);
            MeshResource = mesh_resource.get_resolved(true);
        }

        // Resource handles are all ref-counted proxies! 'unload' or 'dtor' means [refcount -= 1]
        // So resource will only be actually unloaded if there are no handles still reference to it.
        auto ownershipCopy = mesh_resource;
        ownershipCopy.unload(); // The MeshResource would be safe and unloaded because of the outter 'mesh_resource' handle.
    }
    render_app->close_all_windows();
    render_app->get_event_queue()->remove_handler(&close_listener);
    return 0;
}

void ModelViewerModule::on_unload()
{
    // save imported asset meta to disk
    project.SaveToDisk();

    // shutdown
    DestroyAssetSystem();
    DestroyResourceSystem();
    project.CloseProject();

    // shutdown task system
    scheduler.unbind();

    // shutdown logging
    skr_log_flush();
    skr_log_finalize_async_worker();
}

void ModelViewerModule::CookAndLoadGLTF()
{
    auto& System = *skd::asset::GetCookSystem();
    const bool NeedImport = !project.ExistImportedAsset(u8"girl.gltf");
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
            MeshAssetID, // guid for this asset
            skr::type_id_of<skr::renderer::MeshResource>(),// output resource is a mesh resource 
            skr::type_id_of<skd::asset::MeshCooker>()      // this cooker cooks the raw mesh data to mesh resource
        );
        // source file
        importer->assetPath = u8"D:/Code/SakuraEngine/samples/application/game/assets/sketchfab/loli/scene.gltf";
        System.ImportAssetMeta(&project, asset, importer, metadata);
        
        // save
        System.SaveAssetMeta(&project, asset);
     
        auto event = System.EnsureCooked(asset->GetGUID());
        event.wait(true);
    }
    else // Load imported asset from project...
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