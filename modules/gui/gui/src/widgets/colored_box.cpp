#include "SkrGui/widgets/colored_box.hpp"
#include "SkrGui/render_objects/render_colored_box.hpp"

namespace skr::gui
{
NotNull<RenderObject*> ColoredBox::create_render_object() SKR_NOEXCEPT
{
    auto result = SkrNew<RenderColoredBox>();
    result->set_color(color);
    return result;
}
void ColoredBox::update_render_object(NotNull<IBuildContext*> context, NotNull<RenderObject*> render_object) SKR_NOEXCEPT
{
    auto r_obj = render_object->type_cast_fast<RenderColoredBox>();

    r_obj->set_color(color);
}
} // namespace skr::gui