#pragma once
#include "SkrGui/framework/render_object/render_box.hpp"
#include "SkrGui/framework/render_object/single_child_render_object.hpp"
#ifndef __meta__
    #include "SkrGui/framework/render_object/render_shifted_box.generated.h"
#endif

namespace skr::gui
{
// 会对 child 施加布局偏移的 RenderBox
sreflect_struct(guid = "357e11e8-dbcd-4830-9256-869198ca7bed")
RenderShiftedBox : public RenderBox,
                   public ISingleChildRenderObject {
    SKR_GENERATE_BODY()

    inline Offsetf offset() const SKR_NOEXCEPT { return _offset; }
    inline void    set_offset(Offsetf offset) SKR_NOEXCEPT { _offset = offset; }

protected:
    // intrinsic size
    float compute_min_intrinsic_width(float height) const SKR_NOEXCEPT override;
    float compute_max_intrinsic_width(float height) const SKR_NOEXCEPT override;
    float compute_min_intrinsic_height(float width) const SKR_NOEXCEPT override;
    float compute_max_intrinsic_height(float width) const SKR_NOEXCEPT override;

    // paint
    void paint(NotNull<PaintingContext*> context, Offsetf offset) SKR_NOEXCEPT override;

    // hit test
    bool hit_test(HitTestResult* result, Offsetf local_position) const SKR_NOEXCEPT override;

    // transform
    void apply_paint_transform(NotNull<const RenderObject*> child, Matrix4& transform) const SKR_NOEXCEPT override;

private:
    Offsetf _offset = {};

    SKR_GUI_SINGLE_CHILD_RENDER_OBJECT_MIXIN(RenderShiftedBox, RenderBox)
};
} // namespace skr::gui