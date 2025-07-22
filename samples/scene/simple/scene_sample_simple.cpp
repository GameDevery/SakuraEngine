// The SceneSample_Simple Model 
#
#include <SkrCore/module/module_manager.hpp>
#include <SkrCore/log.h>
#include <SkrScene/actor.h>

struct SceneSampleSimpleModule : public skr::IDynamicModule {
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;
};  

SceneSampleSimpleModule g_scene_sample_simple_module;
IMPLEMENT_DYNAMIC_MODULE(SceneSampleSimpleModule, SceneSample_Simple);
void SceneSampleSimpleModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"Scene Sample Simple Module Loaded");
}

void SceneSampleSimpleModule::on_unload()
{
    SKR_LOG_INFO(u8"Scene Sample Simple Module Unloaded");
}

int SceneSampleSimpleModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("SceneSampleSimpleModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Scene Sample Simple Module");

    auto root = skr::Actor::GetRoot();
    auto actor1 = skr::Actor::CreateActor();
    auto actor2 = skr::Actor::CreateActor();
    actor2.lock()->AttachTo(actor1, skr::EAttachRule::KeepWorldTransform);

    return 0;
}