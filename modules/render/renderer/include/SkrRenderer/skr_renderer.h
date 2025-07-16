#pragma once
#include "SkrBase/config.h"
#include "SkrRenderer/fwd_types.h"

struct sugoi_storage_t;
struct SViewportManager;

#ifdef __cplusplus
    #include "SkrCore/module/module.hpp"
    #include "render_device.h"

class SKR_RENDERER_API SkrRendererModule : public skr::IDynamicModule
{
public:
    virtual void on_load(int argc, char8_t** argv) override;
    virtual void on_unload() override;

    SRenderDeviceId get_render_device();

    static SkrRendererModule* Get();
protected:
    skr::RendererDevice* render_device;
};
#endif

SKR_EXTERN_C SKR_RENDERER_API 
SRenderDeviceId skr_get_default_render_device();