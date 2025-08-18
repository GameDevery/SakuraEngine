#pragma once
#include "SkrGui/framework/element/component_element.hpp"
#include "SkrGui/framework/fwd_framework.hpp"
#ifndef __meta__
    #include "SkrGui/framework/element/proxy_element.generated.h"
#endif

namespace skr::gui
{

sreflect_struct(guid = "5b6fca8a-7558-4301-a00a-749b63be5aab")
SKR_GUI_API ProxyElement : public ComponentElement {
    SKR_GENERATE_BODY(ProxyElement)
    using Super = ComponentElement;
    using Super::Super;

    // build & update
    void update(NotNull<Widget*> new_widget) SKR_NOEXCEPT override;

    Widget* build() SKR_NOEXCEPT override;

    virtual void updated(NotNull<ProxyWidget*> old_widget) = 0;
};
} // namespace skr::gui