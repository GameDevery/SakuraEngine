#include <SkrBase/misc/make_zeroed.hpp>
#include <SkrCore/memory/sp.hpp>
#include <SkrCore/module/module_manager.hpp>
#include <SkrCore/log.h>
#include <SkrCore/platform/vfs.h>
#include <SkrCore/time.h>
#include <SkrCore/async/thread_job.hpp>
#include <SkrRT/io/vram_io.hpp>
#include "SkrOS/thread.h"
#include "SkrRT/io/ram_io.hpp"
#include "SkrRT/misc/cmd_parser.hpp"

#include <SkrOS/filesystem.hpp>
#include "SkrCore/memory/impl/skr_new_delete.hpp"
#include "SkrSystem/advanced_input.h"
#include <SkrRT/ecs/world.hpp>
#include <SkrRT/resource/resource_system.h>
#include <SkrRT/resource/local_resource_registry.hpp>

#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderer/skr_renderer.h"
#include "SkrRenderer/resources/mesh_resource.h"
#include "SkrRenderer/render_mesh.h"
#include "SkrTask/fib_task.hpp"
#include "SkrGLTFTool/mesh_processing.hpp"

#include "SkrImGui/imgui_app.hpp"
#include "scene_renderer.hpp"
#include "helper.hpp"
#include "cgltf/cgltf.h"
#include <cstdio>

struct SceneSampleSkelMeshModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

    skr::task::scheduler_t scheduler;
    skr::ecs::World world{ scheduler };
    skr_vfs_t* resource_vfs = nullptr;
    skr::io::IRAMService* ram_service = nullptr;
    skr_io_vram_service_t* vram_service = nullptr;
    skr::JobQueue* io_job_queue = nullptr;
    SRenderDeviceId render_device = nullptr;

    skr::UPtr<skr::ImGuiApp> imgui_app = nullptr;
    skr::SceneRenderer* scene_renderer = nullptr;

    skr::String gltf_path = u8"";
    bool use_gltf = false;
};

IMPLEMENT_DYNAMIC_MODULE(SceneSampleSkelMeshModule, SceneSample_SkelMesh);

void SceneSampleSkelMeshModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"Scene Sample SkelMesh Module Loaded");
}

void SceneSampleSkelMeshModule::on_unload()
{
    SKR_LOG_INFO(u8"Scene Sample SkelMesh Module Unloaded");
}

int SceneSampleSkelMeshModule::main_module_exec(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"Running Scene Sample SkelMesh Module");
    return 0;
}