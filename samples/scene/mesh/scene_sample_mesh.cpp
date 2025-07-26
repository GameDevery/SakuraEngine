#include <SkrCore/module/module_manager.hpp>
#include <SkrCore/log.h>
#include "scene_renderer.hpp"

// The Three-Triangle Example: simple mesh scene hierarchy

struct SceneSampleMeshModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

    static SceneSampleMeshModule* Get() SKR_NOEXCEPT;
};

SceneSampleMeshModule g_scene_sample_mesh_module;
IMPLEMENT_DYNAMIC_MODULE(SceneSampleMeshModule, SceneSample_Mesh);

void SceneSampleMeshModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"Scene Sample Mesh Module Loaded");
}

void SceneSampleMeshModule::on_unload()
{
    SKR_LOG_INFO(u8"Scene Sample Mesh Module Unloaded");
}

SceneSampleMeshModule* SceneSampleMeshModule::Get() SKR_NOEXCEPT
{
    return &g_scene_sample_mesh_module;
}

int SceneSampleMeshModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("SceneSampleMeshModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Scene Sample Mesh Module");

    auto* renderer = skr::SceneRenderer::Create();

    // TODO: Main Window

    skr::SceneRenderer::Destroy(renderer);
    return 0;
}