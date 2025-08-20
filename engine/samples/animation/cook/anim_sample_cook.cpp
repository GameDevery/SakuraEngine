#include <SkrCore/module/module.hpp>
#include <SkrBase/misc/make_zeroed.hpp>
#include <SkrCore/memory/sp.hpp>

#include "SkrCore/log.h"
#include "SkrCore/async/thread_job.hpp"
#include "SkrCore/memory/rc.hpp"
#include "SkrSerde/json_serde.hpp"
#include <SkrOS/filesystem.hpp>

#include "SkrRT/io/ram_io.hpp"
#include "SkrRT/io/vram_io.hpp"
#include <SkrRT/resource/resource_system.h>
#include "SkrRT/resource/local_resource_registry.hpp"
#include <SkrRT/ecs/world.hpp>

#include "SkrAnim/resources/animation_resource.hpp"
#include "SkrAnim/resources/skeleton_resource.hpp"

#include "SkrToolCore/project/project.hpp"

#include "SkrToolCore/cook_system/cook_system.hpp"

#include "SkrAnimTool/skeleton_asset.h"
#include "SkrAnimTool/animation_asset.h"

struct AnimSampleCookModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

    skr::task::scheduler_t scheduler;
    skr::ecs::World world{ scheduler };
    skr_vfs_t* resource_vfs = nullptr;
    skr::io::IRAMService* ram_service = nullptr;
    skr::SP<skr::JobQueue> job_queue = nullptr;
    skr::LocalResourceRegistry* registry = nullptr;

    skr::SkelFactory* skelFactory = nullptr;
    skr::AnimFactory* animFactory = nullptr;
    skd::SProject project;

    void InitializeResourceSystem();
    void InitializeAssetSystem();
    void DestroyAssetSystem();
    void DestroyResourceSystem();
};

IMPLEMENT_DYNAMIC_MODULE(AnimSampleCookModule, AnimSampleCook);

using namespace skr::literals;
const auto SkelAssetID = u8"0198a7d5-6819-76e2-88c3-fad2f6c3d5d5"_guid;
const auto AnimAssetID = u8"0198a890-b5f8-750e-8e4d-cb200eb53b0e"_guid;

void AnimSampleCookModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"Anim Sample Cook Module Loaded");
    // TODO: parse cmdline

    scheduler.initialize({});
    scheduler.bind();
    world.initialize();
    auto resourceRoot = (skr::fs::current_directory() / u8"../resources");
    skr_vfs_desc_t vfs_desc = {};
    vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
    vfs_desc.override_mount_dir = resourceRoot.c_str();
    resource_vfs = skr_create_vfs(&vfs_desc);

    auto projectRoot = skr::fs::current_directory() / u8"../resources/anim/sample_cook";
    if (!skr::fs::Directory::exists(projectRoot))
    {
        skr::fs::Directory::create(projectRoot, true);
    }
    skd::SProjectConfig projectConfig = {
        .assetDirectory = (projectRoot / u8"assets").string().c_str(),
        .resourceDirectory = (projectRoot / u8"resources").string().c_str(),
        .artifactsDirectory = (projectRoot / u8"artifacts").string().c_str()
    };
    skr::String projectName = u8"AnimSampleCook";
    skr::String rootPath = projectRoot.string().c_str();
    project.OpenProject(projectName.c_str(), rootPath.c_str(), projectConfig);

    {
        InitializeResourceSystem();
        InitializeAssetSystem();
    }
}

void AnimSampleCookModule::on_unload()
{
    project.CloseProject();
    // stop all ram_service
    {
        DestroyAssetSystem();
        DestroyResourceSystem();
    }
    world.finalize();
    scheduler.unbind();
    SKR_LOG_INFO(u8"Anim Sample Cook Module Unloaded");
}

void AnimSampleCookModule::InitializeResourceSystem()
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

        auto ramServiceDesc = make_zeroed<skr_ram_io_service_desc_t>();
        ramServiceDesc.name = u8"SceneSampleMesh-RAMIOService";
        ramServiceDesc.sleep_time = 1000 / 60;
        ram_service = skr_io_ram_service_t::create(&ramServiceDesc);
        ram_service->run();
    }
    // skel factory
    {
        skelFactory = SkrNew<skr::SkelFactory>();
        resource_system->RegisterFactory(skelFactory);
    }
    {
        animFactory = SkrNew<skr::AnimFactory>();
        resource_system->RegisterFactory(animFactory);
    }
}

void AnimSampleCookModule::InitializeAssetSystem()
{
    auto& system = *skd::asset::GetCookSystem();
    system.Initialize();
}

void AnimSampleCookModule::DestroyAssetSystem()
{
    auto& system = *skd::asset::GetCookSystem();
    system.Shutdown();
}

void AnimSampleCookModule::DestroyResourceSystem()
{
    auto resource_system = skr::GetResourceSystem();
    resource_system->Shutdown();

    SkrDelete(skelFactory);
    SkrDelete(animFactory);

    skr_io_ram_service_t::destroy(ram_service);
    job_queue.reset();
}

int AnimSampleCookModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("AnimSampleCookModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Anim Sample Cook Module");

    auto& system = *skd::asset::GetCookSystem();
    // import skeleton
    auto source_gltf = u8"D:/ws/repos/SakuraEngine/samples/assets/sketchfab/ruby/scene.gltf";
    auto skelImporter = skd::asset::GltfSkelImporter::Create<skd::asset::GltfSkelImporter>();
    auto meshdata = skd::asset::SkeletonAsset::Create<skd::asset::SkeletonAsset>();
    auto skel_asset = skr::RC<skd::asset::AssetMetaFile>::New(
        u8"test_skeleton.gltf.meta",
        SkelAssetID,
        skr::type_id_of<skr::SkeletonResource>(),
        skr::type_id_of<skd::asset::SkelCooker>());
    skelImporter->assetPath = source_gltf;
    system.ImportAssetMeta(&project, skel_asset, skelImporter, meshdata);

    auto animImporter = skd::asset::GltfAnimImporter::Create<skd::asset::GltfAnimImporter>();
    auto animdata = skd::asset::AnimAsset::Create<skd::asset::AnimAsset>();

    animdata->skeletonAsset = skel_asset->GetGUID();
    auto anim_asset = skr::RC<skd::asset::AssetMetaFile>::New(
        u8"test_animation.gltf.meta",
        AnimAssetID,
        skr::type_id_of<skr::AnimResource>(),
        skr::type_id_of<skd::asset::AnimCooker>());
    animImporter->assetPath = source_gltf;
    animImporter->animationName = u8"Take 001";

    system.ImportAssetMeta(&project, anim_asset, animImporter, animdata);

    {
        system.ParallelForEachAsset(1,
            [&](skr::span<skr::RC<skd::asset::AssetMetaFile>> assets) {
                SkrZoneScopedN("Cook");
                for (auto asset : assets)
                {
                    system.EnsureCooked(asset->GetGUID());
                }
            });
    }

    auto resource_system = skr::GetResourceSystem();
    skr::task::schedule([&] {
        system.WaitForAll();
        // resource_system->Quit();
    },
        nullptr);
    resource_system->Update();
    //----- wait
    skr::AsyncResource<skr::SkeletonResource> skel_resource = SkelAssetID;
    skr::AsyncResource<skr::AnimResource> anim_resource = AnimAssetID;

    // while (!system.AllCompleted() && resource_system->WaitRequest())
    // {
    //     resource_system->Update();
    // }

    return 0;
}
