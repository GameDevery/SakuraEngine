// a comprehensive example of using the Scene API, displaying a scene tree structure
// The SceneSample_Project Model
#
#include <SkrCore/module/module_manager.hpp>
#include <SkrCore/log.h>
#include <SkrSceneCore/actor.h>

struct SceneSampleProjectModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;
};

IMPLEMENT_DYNAMIC_MODULE(SceneSampleProjectModule, SceneSample_Project);
void SceneSampleProjectModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"Scene Sample Project Module Loaded");
}

void SceneSampleProjectModule::on_unload()
{
    SKR_LOG_INFO(u8"Scene Sample Project Module Unloaded");
}

int SceneSampleProjectModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("SceneSampleProjectModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Scene Sample Project Module");

    return 0;
}