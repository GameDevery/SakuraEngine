#pragma once
#include <SkrSystem/system_app.h>
#include <SkrCore/dirty.hpp>
#include <imgui.h>

namespace skr
{
// Forward declarations
struct ImGuiBackend;

// ImGui event handler that integrates with SkrSystem event system
struct SKR_IMGUI_API ImGuiSystemEventHandler : public ISystemEventHandler
{
    ImGuiSystemEventHandler(ImGuiBackend* backend);
    ~ImGuiSystemEventHandler() override;

    // ISystemEventHandler interface
    void handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT override;

    // Event processing for ImGui
    void process_keyboard_event(const SkrKeyboardEvent& event);
    void process_mouse_event(const SkrMouseEvent& event);
    void process_window_event(const SkrWindowEvent& event);

    // Helper methods
    ImGuiKey skr_key_to_imgui_key(SInputKeyCode keycode) const;
    ImGuiMouseButton skr_mouse_button_to_imgui(InputMouseButtonFlags button) const;

    // Triggers for backend
    const Trigger& want_exit() const { return _want_exit; }
    const Trigger& want_resize() const { return _want_resize; }
    const Trigger& pixel_size_changed() const { return _pixel_size_changed; }
    const Trigger& content_scale_changed() const { return _content_scale_changed; }

private:
    ImGuiBackend* _backend = nullptr;
    ImGuiContext* _context = nullptr;
    
    // Event triggers
    Trigger _want_exit = {};
    Trigger _want_resize = {};
    Trigger _pixel_size_changed = {};
    Trigger _content_scale_changed = {};
    
    // Track window handle for main window
    void* _main_window_handle = nullptr;
};

} // namespace skr