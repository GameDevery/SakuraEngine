#pragma once
#include <SkrImGui/imgui_system_event_handler.hpp>
#include <SkrContainers/string.hpp>
#include <SkrContainers/optional.hpp>
#include <SkrBase/math.h>
#include <SkrCore/dirty.hpp>
#include <SkrCore/memory/rc.hpp>
#include <imgui.h>
#include <imgui_internal.h>
#include <chrono>

namespace skr
{
struct ImGuiRendererBackend;
struct SystemApp;
struct SystemWindow;

// Window creation info directly in ImGui namespace
struct ImGuiWindowCreateInfo {
    String              title         = {};
    Optional<uint2>     pos           = {}; // default means centered
    uint2               size          = { 1280, 720 };
    SystemWindow*       popup_parent  = nullptr; // if set, means create a popup window
    bool                is_topmost    = false;
    bool                is_tooltip    = false;
    bool                is_borderless = false;
    bool                is_resizable  = true;
};

struct SKR_IMGUI_API ImGuiBackend {
    // ctor & dtor
    ImGuiBackend();
    ~ImGuiBackend();

    // imgui context
    void apply_context();
    void create(
        SystemApp*                     system_app,
        const ImGuiWindowCreateInfo&   main_wnd_create_info,
        RCUnique<ImGuiRendererBackend> backend
    );
    // Legacy overload for backward compatibility
    void create(
        const ImGuiWindowCreateInfo&   main_wnd_create_info,
        RCUnique<ImGuiRendererBackend> backend
    );
    void destroy();

    // frame
    void pump_message();  // Legacy method for compatibility
    void process_events(); // New method using SystemEventQueue
    void begin_frame();
    void end_frame();
    void acquire_next_frame();
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
    inline bool                      is_created() const { return _context != nullptr; }
    inline const Trigger&            want_exit() const { return _event_handler ? _event_handler->want_exit() : _want_exit; }
    inline ImGuiContext*             context() const { return _context; }
    inline SystemApp*                system_app() const { return _system_app; }
    inline SystemWindow*             system_window() const { return _system_window; }

    mutable skr::String _clipboard;
    
private:
    // context
    ImGuiContext*      _context     = nullptr;

    // system integration
    SystemApp*                     _system_app     = nullptr;
    SystemWindow*                  _system_window  = nullptr;
    ImGuiSystemEventHandler*       _event_handler  = nullptr;

    // render backend
    RCUnique<ImGuiRendererBackend> _renderer_backend = nullptr;

    // dirty & trigger (for legacy mode)
    Trigger _pixel_size_changed    = {};
    Trigger _content_scale_changed = {};
    Trigger _want_resize           = {};
    Trigger _want_exit             = {};
    
    // Helper methods
    void UpdateMouseCursor();
    void SetupIMECallbacks();
    void UpdateIMEState();
    
    // Time tracking
    std::chrono::steady_clock::time_point last_frame_time_;
    
    // Track if we own the SystemApp (for legacy mode)
    bool _owns_system_app = false;
    
    // IME integration
    IME* _ime = nullptr;  // Cached IME pointer
    bool _ime_callbacks_set = false;  // Avoid duplicate callback setup
    bool _ime_active_state = false;  // Track our IME activation state
};
} // namespace skr