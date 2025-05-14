#include <SkrImGui/imgui_backend.hpp>

namespace skr
{
// ctor & dtor
ImGuiBackend::ImGuiBackend()
{
}
ImGuiBackend::~ImGuiBackend()
{
    destroy();
}

// imgui context
void ImGuiBackend::attach(ImGuiContext* context)
{
}
ImGuiContext* ImGuiBackend::detach()
{
    return nullptr;
}
void ImGuiBackend::create()
{
}
void ImGuiBackend::destroy()
{
}

} // namespace skr