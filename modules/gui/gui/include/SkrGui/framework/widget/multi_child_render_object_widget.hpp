#pragma once
#include "SkrGui/framework/widget/render_object_widget.hpp"
#include "SkrGui/framework/fwd_framework.hpp"
#ifndef __meta__
    #include "SkrGui/framework/widget/multi_child_render_object_widget.generated.h"
#endif

namespace skr::gui
{
sreflect_struct(guid = "21ff85e4-9f05-48a8-80bf-7eec604de7f4")
SKR_GUI_API MultiChildRenderObjectWidget : public RenderObjectWidget {
    SKR_GENERATE_BODY()

    NotNull<Element*> create_element() SKR_NOEXCEPT override;

    Array<Widget*> children = {};
};
} // namespace skr::gui