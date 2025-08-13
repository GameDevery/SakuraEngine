#include <SkrCore/module/module.hpp>
#include "SkrCore/log.h"

struct AnimSampleCookModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;
};

IMPLEMENT_DYNAMIC_MODULE(AnimSampleCookModule, AnimSampleCook);

void AnimSampleCookModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"Anim Sample Cook Module Loaded");
}

void AnimSampleCookModule::on_unload()
{
    SKR_LOG_INFO(u8"Anim Sample Cook Module Unloaded");
}

int AnimSampleCookModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("AnimSampleCookModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Anim Sample Cook Module");
    return 0;
}