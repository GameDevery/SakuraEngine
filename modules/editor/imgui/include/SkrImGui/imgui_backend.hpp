#pragma once
#include <SkrImGui/imgui_window_backend.hpp>
#include <SkrContainers/string.hpp>
#include <SkrContainers/optional.hpp>
#include <SkrBase/math.h>
#include <SkrCore/dirty.hpp>
#include <imgui.h>
#include <imgui_internal.h>

namespace skr
{
struct SKR_IMGUI_NG_API ImGuiBackend {
    // ctor & dtor
    ImGuiBackend();
    ~ImGuiBackend();

    // imgui context
    void          apply_context();
    void          attach(ImGuiContext* context);
    ImGuiContext* detach();
    void          create();  // means create a imgui context and attach
    void          destroy(); // means detach and destroy context

    // main window
    void create_main_window(const ImGuiWindowCreateInfo& create_info = {});
    void destroy_main_window();

    // frame
    void pump_message();
    void begin_frame();
    void end_frame();
    void render();

    // imgui functional
    void enable_nav(bool enable = true);
    void enable_docking(bool enable = true);
    void enable_multi_viewport(bool enable = true);
    void enable_ini_file(bool enable = true);
    void enable_log_file(bool enable = true);
    void enable_transparent_docking(bool enable = true);
    void enable_always_tab_bar(bool enable = true);
    void enable_move_window_by_blank_area(bool enable = true);

    // getter
    inline bool                      is_attached() const { return _context != nullptr; }
    inline bool                      has_main_window() const { return _main_window.is_valid(); }
    inline const Trigger&            pixel_size_changed() const { return _pixel_size_changed; }
    inline const Trigger&            content_scale_changed() const { return _content_scale_changed; }
    inline const Trigger&            want_resize() const { return _want_resize; }
    inline const Trigger&            want_exit() const { return _want_exit; }
    inline ImGuiContext*             context() const { return _context; }
    inline const ImGuiWindowBackend& main_window() const { return _main_window; }

private:
    // dirty & trigger
    Trigger _pixel_size_changed    = {};
    Trigger _content_scale_changed = {};
    Trigger _want_resize           = {};
    Trigger _want_exit             = {};

    // context & main window
    ImGuiContext*      _context     = nullptr;
    ImGuiWindowBackend _main_window = {};
};
} // namespace skr