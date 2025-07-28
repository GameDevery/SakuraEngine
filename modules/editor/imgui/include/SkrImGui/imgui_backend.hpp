#pragma once
#include "SkrImGui/imgui_system_event_handler.hpp"
#include "SkrCore/dirty.hpp"
#include "SkrCore/memory/rc.hpp"
#include "SkrRenderer/render_app.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <chrono>

namespace skr
{
struct ImGuiRendererBackend;
struct SystemApp;
struct SystemWindow;

struct SKR_IMGUI_API ImGuiApp : public skr::RenderApp
{
    // ctor & dtor
    ImGuiApp(const SystemWindowCreateInfo& main_wnd_create_info, SRenderDeviceId render_device, RCUnique<ImGuiRendererBackend> backend);
    ~ImGuiApp();

    // imgui context
    void apply_context();

    virtual bool initialize(const char* backend = nullptr) override;
    virtual void shutdown() override;

    // frame
    void pump_message(); // Legacy method for compatibility
    void begin_frame();
    void end_frame();
    void acquire_next_frame();
    // for resize and render issue
    // render() -> collect() + render()
    // temp change, should be limited to dev_anim branch only
    // contact @zihuang.zhu for any issue
    void collect();
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
    void enable_high_dpi(bool enable = true);

    // getter
    inline bool is_created() const { return _context != nullptr; }
    inline const Trigger& want_exit() const { return _event_handler ? _event_handler->want_exit() : _want_exit; }
    inline ImGuiContext* context() const { return _context; }
    inline SystemWindow* main_window() const { return _main_window; }

    mutable skr::String _clipboard;

private:
    void _collect();
    // context
    ImGuiContext* _context = nullptr;

    // system integration
    SystemWindowCreateInfo _main_window_info;
    SystemWindow* _main_window = nullptr;
    ImGuiSystemEventHandler* _event_handler = nullptr;

    // render backend
    RCUnique<ImGuiRendererBackend> _renderer_backend = nullptr;

    // dirty & trigger (for legacy mode)
    Trigger _pixel_size_changed = {};
    Trigger _content_scale_changed = {};
    Trigger _want_resize = {};
    Trigger _want_exit = {};

    // Helper methods
    void UpdateMouseCursor();
    void SetupIMECallbacks();
    void UpdateIMEState();
    bool _ime_active_state = false;

    // Time tracking
    std::chrono::steady_clock::time_point last_frame_time_;
};
} // namespace skr