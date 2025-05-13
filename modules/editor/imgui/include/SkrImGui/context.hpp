#pragma once
#include <SkrImGui/window.hpp>
#include <SkrContainers/string.hpp>
#include <SkrContainers/optional.hpp>
#include <SkrBase/math.h>
#include <imgui.h>

namespace skr
{
struct SKR_IMGUI_NG_API ImGuiBackend {
    // ctor & dtor
    ImGuiBackend();
    ~ImGuiBackend();

    // imgui context
    void          attach(ImGuiContext* context);
    ImGuiContext* detach();
    void          create();  // means create a imgui context and attach
    void          destroy(); // means detach and destroy context

    // getter
    ImGuiContext* context() const;

    // main window
    const ImGuiWindowBackend& main_window() const;
    void                      create_main_window(const ImGuiWindowCreateInfo& create_info = {});

    // frame
    void pump_message();
    void begin_frame();
    void end_frame();

private:
    ImGuiContext*      _context     = nullptr;
    ImGuiWindowBackend _main_window = {};
};
} // namespace skr