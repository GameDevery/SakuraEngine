#include "SkrGui/render_objects/render_stack.hpp"
#include "SkrGui/framework/painting_context.hpp"

namespace skr::gui
{
struct _StackHelper {
    inline static BoxConstraints _fit_constraints(const RenderStack& self, const BoxConstraints& constraints) SKR_NOEXCEPT
    {
        switch (self._child_fit)
        {
            case EPositionalFit::Loose:
                return constraints.loosen();
            case EPositionalFit::Expand:
                return BoxConstraints::Tight(constraints.biggest());
            case EPositionalFit::PassThrough:
                return constraints;
        }
    }

    template <typename TLayoutFunc>
    inline static Size _compute_size(const RenderStack&    self,
                                     const BoxConstraints& constraints,
                                     TLayoutFunc&&         layout_func) SKR_NOEXCEPT
    {
        if (self._stack_slots.size() == 0) return constraints.biggest().is_finite() ? constraints.biggest() : constraints.smallest();
        if (self._stack_size == EStackSize::Expand) return constraints.biggest();

        BoxConstraints fit_constraints = _fit_constraints(self, constraints);
        float          width = constraints.min_width;
        float          height = constraints.min_height;
        for (const auto& slot : self._stack_slots)
        {
            Size child_size = layout_func(slot.child, fit_constraints);
            width = std::max(width, child_size.width);
            height = std::max(height, child_size.height);
        }
        return { width, height };
    }

    template <typename TIntrinsicFunc>
    inline static float _get_intrinsic(
    const RenderStack& self,
    TIntrinsicFunc&&   intrinsic_func) SKR_NOEXCEPT
    {
        float result = 0.0f;
        for (const auto& slot : self._stack_slots)
        {
            result = std::max(result, intrinsic_func(slot.child));
        }
        return result;
    }
};
} // namespace skr::gui

namespace skr::gui
{
// intrinsic size
float RenderStack::compute_min_intrinsic_width(float height) const SKR_NOEXCEPT
{
    return _StackHelper::_get_intrinsic(
    *this,
    [height](const RenderBox* child) { return child->get_min_intrinsic_width(height); });
}
float RenderStack::compute_max_intrinsic_width(float height) const SKR_NOEXCEPT
{
    return _StackHelper::_get_intrinsic(
    *this,
    [height](const RenderBox* child) { return child->get_max_intrinsic_width(height); });
}
float RenderStack::compute_min_intrinsic_height(float width) const SKR_NOEXCEPT
{
    return _StackHelper::_get_intrinsic(
    *this,
    [width](const RenderBox* child) { return child->get_min_intrinsic_height(width); });
}
float RenderStack::compute_max_intrinsic_height(float width) const SKR_NOEXCEPT
{
    return _StackHelper::_get_intrinsic(
    *this,
    [width](const RenderBox* child) { return child->get_max_intrinsic_height(width); });
}

// dry layout
Size RenderStack::compute_dry_layout(BoxConstraints constraints) const SKR_NOEXCEPT
{
    return _StackHelper::_compute_size(
    *this,
    constraints,
    [](const RenderBox* child, const BoxConstraints& constraints) {
        return child->get_dry_layout(constraints);
    });
}

// layout
void RenderStack::perform_layout() SKR_NOEXCEPT
{
    set_size(_StackHelper::_compute_size(
    *this,
    constraints(),
    [](RenderBox* child, const BoxConstraints& constraints) {
        child->set_constraints(constraints);
        child->layout(true);
        return child->size();
    }));

    for (auto& slot : _stack_slots)
    {
        slot.offset = _stack_alignment.along_size(size() - slot.child->size());
    }
}

// paint
void RenderStack::paint(NotNull<PaintingContext*> context, Offset offset) SKR_NOEXCEPT
{
    // TODO. clip behaviour
    for (const auto& slot : _stack_slots)
    {
        if (slot.child)
        {
            context->paint_child(make_not_null(slot.child), slot.offset + offset);
        }
        else
        {
            SKR_GUI_LOG_ERROR("RenderFlex::paint: child is nullptr.");
        }
    }
}
} // namespace skr::gui