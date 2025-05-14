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
} // namespace skr