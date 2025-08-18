#pragma once
#include "SkrGui/framework/widget/slot_widget.hpp"
#include "SkrGui/math/layout.hpp"
#ifndef __meta__
    #include "SkrGui/widgets/flex_slot.generated.h"
#endif

namespace skr::gui
{
sreflect_struct(guid = "32dc9243-8d16-4854-9ce8-3039a47991bf")
SKR_GUI_API FlexSlot : public SlotWidget {
    SKR_GENERATE_BODY(FlexSlot)

    float    flex     = 1;               // determines how much the child should grow or shrink relative to other flex items
    EFlexFit flex_fit = EFlexFit::Loose; // determines how much the child should be allowed to shrink relative to its own size
};
} // namespace skr::gui