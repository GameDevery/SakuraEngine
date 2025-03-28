#pragma once
#include "SkrBase/config.h"
#include "SkrGui/backend/device/device.hpp"
#include "SkrGui/backend/device/display.hpp"
#ifndef __meta__
    #include "SkrGuiRenderer/device/skr_native_device.generated.h"
#endif

namespace skr::gui
{
struct SkrRenderDevice;
struct SkrResourceDevice;
struct SkrNativeWindow;

sreflect_struct(guid = "e3c81438-70e6-4727-8133-7a32a23b62c4")
SKR_GUI_RENDERER_API SkrNativeDevice final : public INativeDevice {
    SKR_GENERATE_BODY()

    void init();
    void shutdown();

    // view
    NotNull<INativeWindow*> create_window() override;
    void                    destroy_window(NotNull<INativeWindow*> view) override;

    // sub device
    inline NotNull<SkrRenderDevice*>   render_device() const SKR_NOEXCEPT { return _render_device; }
    inline NotNull<SkrResourceDevice*> resource_device() const SKR_NOEXCEPT { return _resource_device; }

    void render_all_windows() SKR_NOEXCEPT;

    // display info
    const DisplayMetrics& display_metrics() const override;

    // resource management
    NotNull<IUpdatableImage*> create_updatable_image() override;

    // canvas management
    NotNull<ICanvas*> create_canvas() override;
    void              destroy_canvas(NotNull<ICanvas*> canvas) override;

    // text management
    NotNull<IParagraph*> create_paragraph() override;
    void                 destroy_paragraph(NotNull<IParagraph*> paragraph) override;

private:
    // sub devices
    SkrRenderDevice*   _render_device   = nullptr;
    SkrResourceDevice* _resource_device = nullptr;

    DisplayMetrics          _display_metrics = {};
    Array<SkrNativeWindow*> _all_windows     = {};

    Array<IUpdatableImage*> _all_updatable_images = {};
};
} // namespace skr::gui