// a comprehensive example of using the Scene API, displaying a scene tree structure
// The SceneSample_Tree Model 
#
#include <SkrCore/module/module_manager.hpp>
#include <SkrCore/log.h>
#include <SkrScene/actor.h>

struct SceneSampleTreeModule : public skr::IDynamicModule {
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;
};  

SceneSampleTreeModule g_scene_sample_Tree_module;
IMPLEMENT_DYNAMIC_MODULE(SceneSampleTreeModule, SceneSample_Tree);
void SceneSampleTreeModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"Scene Sample Tree Module Loaded");
}

void SceneSampleTreeModule::on_unload()
{
    SKR_LOG_INFO(u8"Scene Sample Tree Module Unloaded");
}

int SceneSampleTreeModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("SceneSampleTreeModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Scene Sample Tree Module");



    return 0;
}