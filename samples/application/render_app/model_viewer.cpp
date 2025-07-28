#include "SkrCore/module/module.hpp"
#include "SkrTask/fib_task.hpp"
#include "SkrRT/resource/resource_system.h"
#include "SkrRT/resource/local_resource_registry.hpp"
#include "SkrSystem/system_app.h"
#include "SkrToolCore/asset/cook_system.hpp"
#include "SkrToolCore/project/project.hpp"
#include "SkrGLTFTool/mesh_asset.hpp"
#include "SkrRenderer/resources/texture_resource.h"
#include "SkrRenderer/skr_renderer.h"

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
    skr::resource::LocalResourceRegistry* registry = nullptr;
    
    SRenderDeviceId render_device = nullptr;
    skr::resource::TextureSamplerFactory* TextureSamplerFactory = nullptr;
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
    // texture sampler factory
    /*
    {
        skr::resource::TextureSamplerFactory::Root factoryRoot = {};
        factoryRoot.device = render_device->get_cgpu_device();
        TextureSamplerFactory = skr::resource::TextureSamplerFactory::Create(factoryRoot);
        resource_system->RegisterFactory(TextureSamplerFactory);
    }
    // texture factory
    {
        skr_vfs_desc_t tex_vfs_desc = {};
        tex_vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
        tex_vfs_desc.override_mount_dir = u8TextureRoot.c_str();
        tex_resource_vfs = skr_create_vfs(&tex_vfs_desc);

        skr::resource::TextureFactory::Root factoryRoot = {};
        auto RootStr = gameResourceRoot.u8string();
        factoryRoot.dstorage_root = RootStr.c_str();
        factoryRoot.vfs = tex_resource_vfs;
        factoryRoot.ram_service = ram_service;
        factoryRoot.vram_service = vram_service;
        factoryRoot.render_device = game_render_device;
        textureFactory = skr::resource::TextureFactory::Create(factoryRoot);
        resource_system->RegisterFactory(textureFactory);
    }
    // mesh factory
    {
        skr::renderer::MeshFactory::Root factoryRoot = {};
        auto RootStr = gameResourceRoot.u8string();
        factoryRoot.dstorage_root = RootStr.c_str();
        factoryRoot.vfs = tex_resource_vfs;
        factoryRoot.ram_service = ram_service;
        factoryRoot.vram_service = vram_service;
        factoryRoot.render_device = game_render_device;
        meshFactory = skr::renderer::MeshFactory::Create(factoryRoot);
        resource_system->RegisterFactory(meshFactory);
    }
    */
}

void ModelViewerModule::DestroyResourceSystem()
{

}