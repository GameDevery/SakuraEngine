#include "SkrCore/module/module.hpp"
#include "SkrCore/log.h"

class SAnimDebugModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;
};

static SAnimDebugModule* g_anim_debug_module = nullptr;
///////
/// Module entry point
///////
IMPLEMENT_DYNAMIC_MODULE(SAnimDebugModule, AnimDebugNext);

void SAnimDebugModule::on_load(int argc, char8_t** argv)
{
}

void SAnimDebugModule::on_unload()
{
    g_anim_debug_module = nullptr;
    SKR_LOG_INFO(u8"anim debug runtime unloaded!");
}

int SAnimDebugModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("AnimDebugExecution");
    SKR_LOG_INFO(u8"anim debug runtime executed as main module!");

    return 0;
}