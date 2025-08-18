#include "SkrSceneCore/module.h"
#include "SkrCore/module/module_manager.hpp"
#include "SkrCore/log.h"

IMPLEMENT_DYNAMIC_MODULE(SkrSceneCoreModule, SkrSceneCore);

void SkrSceneCoreModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_TRACE(u8"skr scene loaded!");
}

void SkrSceneCoreModule::on_unload()
{
    SKR_LOG_TRACE(u8"skr scene unloaded!");
}
