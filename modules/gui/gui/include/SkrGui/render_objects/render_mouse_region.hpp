#pragma once
#include "SkrGui/framework/render_object/render_proxy_box_with_hit_test_behavior.hpp"
#ifndef __meta__
    #include "SkrGui/render_objects/render_mouse_region.generated.h"
#endif

namespace skr::gui
{
struct PointerEnterEvent;
struct PointerExitEvent;
struct PointerMoveEvent;
struct PointerDownEvent;
struct PointerUpEvent;

sreflect_struct(guid = "0de9790c-5a02-470f-92bd-81ca6fb282f2")
SKR_GUI_API RenderMouseRegion : public RenderProxyBoxWithHitTestBehavior {
    using Super = RenderProxyBoxWithHitTestBehavior;
    SKR_GENERATE_BODY()

    bool hit_test(HitTestResult* result, Offsetf local_position) const SKR_NOEXCEPT override;
    bool handle_event(NotNull<PointerEvent*> event, NotNull<HitTestEntry*> entry) override;

public:
    Function<bool(PointerEnterEvent*)> on_enter = {};
    Function<bool(PointerExitEvent*)>  on_exit  = {};
    Function<bool(PointerMoveEvent*)>  on_hover = {};
    Function<bool(PointerDownEvent*)>  on_down  = {};
    Function<bool(PointerUpEvent*)>    on_up    = {};
};
} // namespace skr::gui