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
    bool use_gltf = false;

    skd::SProject project;
    skr::ActorManager& actor_manager = skr::ActorManager::GetInstance();
    skr::TransformSystem* transform_system = nullptr;
    skr::scene::SceneRenderSystem* scene_render_system = nullptr;
};

IMPLEMENT_DYNAMIC_MODULE(SceneSampleSkelMeshModule, SceneSample_SkelMesh);

void SceneSampleSkelMeshModule::InitializeAssetSystem()
{
}

void SceneSampleSkelMeshModule::DestroyAssetSystem()
{
}

void SceneSampleSkelMeshModule::InitializeResourceSystem()
{
}

void SceneSampleSkelMeshModule::DestroyResourceSystem()
{
}

void SceneSampleSkelMeshModule::on_load(int argc, char8_t** argv)
{
}

void SceneSampleSkelMeshModule::on_unload()
{
}

int SceneSampleSkelMeshModule::main_module_exec(int argc, char8_t** argv)
{

    SkrZoneScopedN("SceneSampleSkelMeshModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Scene Sample SkelMesh Module");
    return 0;
}