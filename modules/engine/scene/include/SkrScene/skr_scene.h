#pragma once
#include "SkrBase/config.h"
#include "transform_system.h"

#ifdef __cplusplus
    #include "SkrCore/module/module.hpp"
    
class SKR_SCENE_API SkrSceneModule : public skr::IDynamicModule
{
public:
    virtual void on_load(int argc, char8_t** argv) override;
    virtual void on_unload() override;

    static SkrSceneModule* Get();
};
#endif