#include "SkrGui/render_objects/render_stack.hpp"
#include "misc/log.h"

namespace skr
{
namespace gui
{

RenderStack::RenderStack(skr_gdi_device_id gdi_device)
    : RenderBox(gdi_device)
{
    diagnostic_builder.add_properties(
        SkrNew<TextDiagnosticProperty>(u8"type", u8"stack", u8"place children in stack"));
}

void RenderStack::layout(BoxConstraint constraints, bool needSize)
{
    set_size(constraints.max_size);
    float width = get_size().x;
    float height = get_size().y;
    for (int i = 0; i < get_child_count(); i++)
    {
        RenderBox* child = get_child_as_box(i);
        Positional positional = get_position(i);
        BoxConstraint childConstraints;
        childConstraints.min_size = skr_float2_t{ 0, 0 };
        childConstraints.max_size = skr_float2_t{ std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity() };
        if (positional.left && positional.right)
        {
            if (positional.minWidth || positional.maxWidth)
            {
                SKR_GUI_LOG_WARN("Both left and right are set, width will be ignored");
            }
            childConstraints.min_size.x = childConstraints.max_size.x =
                width - positional.left.get_value(width) - positional.right.get_value(width);
        }
        else
        {
            if (positional.minWidth)
            {
                childConstraints.min_size.x = positional.minWidth.get_value(width);
            }
            if (positional.maxWidth)
            {
                childConstraints.max_size.x = positional.maxWidth.get_value(width);
            }
        }
        if (positional.top && positional.bottom)
        {
            if (positional.minHeight || positional.maxHeight)
            {
                SKR_GUI_LOG_WARN("Both top and bottom are set, height will be ignored");
            }
            childConstraints.min_size.y = childConstraints.max_size.y =
                height - positional.top.get_value(height) - positional.bottom.get_value(height);
        }
        else
        {
            if (positional.minHeight)
            {
                childConstraints.min_size.y = positional.minHeight.get_value(height);
            }
            if (positional.maxHeight)
            {
                childConstraints.max_size.y = positional.maxHeight.get_value(height);
            }
        }

        child->layout(childConstraints, true);
        skr_float2_t childPosition = { 0, 0 };
        if (positional.left)
        {
            float pivotX = positional.right ? 0 : positional.pivot.x;
            childPosition.x = positional.left.get_value(width) - child->get_size().x * pivotX;
        }
        else if (positional.right)
        {
            float pivotY = positional.pivot.y;
            childPosition.x = width - positional.right.get_value(width) - child->get_size().x * (1 - pivotY);
        }
        else
        {
            SKR_GUI_LOG_WARN("Both left and right are not set, default to left 0");
            childPosition.x = -child->get_size().x * positional.pivot.x;
        }
        if (positional.top)
        {
            float pivotY = positional.bottom ? 0 : positional.pivot.y;
            childPosition.y = positional.top.get_value(height) - child->get_size().y * pivotY;
        }
        else if (positional.bottom)
        {
            float pivotY = positional.pivot.y;
            childPosition.y = height - positional.bottom.get_value(height) - child->get_size().y * (1 - pivotY);
        }
        else
        {
            SKR_GUI_LOG_WARN("Both top and bottom are not set, default to top 0");
            childPosition.y = -child->get_size().y * positional.pivot.y;
        }
        child->set_position(childPosition);
    }
}

Positional RenderStack::get_position(int index)
{
    SKR_GUI_ASSERT(index >= 0 && index < positionals.size());
    return positionals[index];
}

void RenderStack::add_child(RenderObject* child)
{
    RenderBox::add_child(child);
    positionals.emplace_back(Positional{});
}

void RenderStack::insert_child(RenderObject* child, int index)
{
    RenderBox::insert_child(child, index);
    positionals.insert(positionals.begin() + index, Positional{});
}

void RenderStack::remove_child(RenderObject* child)
{
    positionals.erase(positionals.begin() + get_child_index(child));
    RenderBox::remove_child(child);
}

void RenderStack::set_positional(int index, Positional positional)
{
    SKR_GUI_ASSERT(index >= 0 && index < positionals.size());
    this->positionals[index] = positional;
}

} // namespace gui
} // namespace skr