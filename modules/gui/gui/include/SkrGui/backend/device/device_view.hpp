#pragma once
#include "SkrGui/fwd_config.hpp"
#include "SkrGui/math/geometry.hpp"

namespace skr::gui
{
struct ICanvas;
struct IDevice;
struct Layer;
} // namespace skr::gui

namespace skr::gui
{
// 设备视口，某颗 UI 树（或子树）的渲染目标，可能是物理上的窗口、RenderTarget、物理窗口的某一部分、Overlay 等等
// ! 考虑一下事件路由怎么做
// FDirectPolicy: 只找根
// FToLeafmostPolicy: 只找最后的叶子
// FTunnelPolicy: 从根找到叶子
// FBubblePolicy: 从叶子找到根
//
// ! 考虑下 HitTest 怎么做
// ! 考虑下 Focus 怎么管理
struct SKR_GUI_API IDeviceView SKR_GUI_INTERFACE_BASE {
    SKR_GUI_INTERFACE_ROOT(IDeviceView, "ac74082f-42c1-4d39-930e-ee1f022bbda7")
    virtual ~IDeviceView() = default;

    // absolute/relative coordinate
    virtual Offsetf to_absolute(const Offsetf& relative_to_view) SKR_NOEXCEPT = 0;
    virtual Rectf   to_absolute(const Rectf& relative_to_view) SKR_NOEXCEPT = 0;
    virtual Sizef   to_absolute(const Sizef& relative_to_view) SKR_NOEXCEPT = 0;
    virtual Offsetf to_relative(const Offsetf& absolute) SKR_NOEXCEPT = 0;
    virtual Rectf   to_relative(const Rectf& absolute) SKR_NOEXCEPT = 0;
    virtual Sizef   to_relative(const Sizef& absolute) SKR_NOEXCEPT = 0;

    // info
    virtual IDevice* device() SKR_NOEXCEPT = 0;
    virtual Offsetf  absolute_pos() SKR_NOEXCEPT = 0;
    virtual Sizef    absolute_size() SKR_NOEXCEPT = 0;
    virtual Rectf    absolute_work_area() SKR_NOEXCEPT = 0;
    virtual float    pixel_ratio() SKR_NOEXCEPT = 0;      // frame_buffer_pixel_size / logical_pixel_size
    virtual float    text_pixel_ratio() SKR_NOEXCEPT = 0; // text_texture_pixel_size / logical_pixel_size
    virtual bool     invisible() SKR_NOEXCEPT = 0;        // is viewport hidden or minimized or any invisible case
    virtual bool     focused() SKR_NOEXCEPT = 0;          // is viewport taken focus

    // operators
    virtual void set_absolute_pos(Offsetf absolute) SKR_NOEXCEPT = 0;
    virtual void set_absolute_size(Sizef absolute) SKR_NOEXCEPT = 0;
    virtual void set_layer(Layer* layer) SKR_NOEXCEPT = 0;
    virtual void take_focus() SKR_NOEXCEPT = 0;
    virtual void raise() SKR_NOEXCEPT = 0;
};

struct SKR_GUI_API INativeDeviceView : public IDeviceView {
    SKR_GUI_INTERFACE(INativeDeviceView, "47b82f64-2f95-4276-9ffa-0f3d0288894a", IDeviceView)

    // getter
    virtual bool          minimized() SKR_NOEXCEPT = 0;
    virtual bool          maximized() SKR_NOEXCEPT = 0;
    virtual bool          show_in_task_bar() SKR_NOEXCEPT = 0;
    virtual const String& title() SKR_NOEXCEPT = 0;

    // setter
    virtual void set_minimized(bool minimized) SKR_NOEXCEPT = 0;
    virtual void set_maximized(bool maximized) SKR_NOEXCEPT = 0;
    virtual void set_show_in_task_bar(bool show_in_task_bar) SKR_NOEXCEPT = 0;
    virtual void set_title(const String& title) SKR_NOEXCEPT = 0;
};

} // namespace skr::gui