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

#include "SkrRenderer/resources/mesh_resource.h"
#include "SkrGLTFTool/mesh_asset.hpp"

struct ModelViewerModule : public skr::IDynamicModule
{
public:
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

protected:
    void InitializeAssetSystem();
    void DestroyAssetSystem();
    void InitializeReosurceSystem();
    void DestroyResourceSystem();

    skr::task::scheduler_t scheduler;
    skd::SProject* project = nullptr;
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

    const auto current_path = skr::filesystem::current_path();
    skd::SProjectConfig projectConfig = {
        .assetDirectory = (current_path / "assets").u8string().c_str(),
        .resourceDirectory = (current_path / "resources").u8string().c_str(),
        .artifactsDirectory = (current_path / "artifacts").u8string().c_str()
    };
    skr::String projectName = u8"ModelViewer";
    skr::String rootPath = skr::filesystem::current_path().u8string().c_str();
    project = skd::SProject::OpenProject(u8"ModelViewer", rootPath.c_str(), projectConfig);

    InitializeReosurceSystem();
    InitializeAssetSystem();
}

int ModelViewerModule::main_module_exec(int argc, char8_t** argv)
{

    return 0;
}

void ModelViewerModule::on_unload()
{
    DestroyAssetSystem();
    DestroyResourceSystem();

    skd::SProject::CloseProject(project);

    scheduler.unbind();

    skr_log_flush();
    skr_log_finalize_async_worker();
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
    registry = SkrNew<skr::resource::LocalResourceRegistry>(project->GetResourceVFS());
    resource_system->Initialize(registry, project->GetRamService());
    const auto resource_root = project->GetResourceVFS()->mount_dir;
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
        factoryRoot.vfs = project->GetResourceVFS();
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
        factoryRoot.vfs = project->GetResourceVFS();
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