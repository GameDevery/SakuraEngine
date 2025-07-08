#pragma once
#include "SkrSystem/system.h"
#include "SkrSystem/window.h"
#include "SkrContainers/map.hpp"
#include "SkrContainers/vector.hpp"
#include "SkrBase/memory/memory_ops.hpp"

namespace skr {

// Forward declaration
class WindowManagerHandler;

// Window manager that provides convenient window creation and tracking
class SKR_SYSTEM_API WindowManager
{
public:
    WindowManager(SystemApp* app) SKR_NOEXCEPT;
    ~WindowManager() SKR_NOEXCEPT;
    
    // Non-copyable
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;
    
    // Window management
    SystemWindow* create_window(const SystemWindowCreateInfo& info) SKR_NOEXCEPT;
    void destroy_window(SystemWindow* window) SKR_NOEXCEPT;
    void destroy_all_windows() SKR_NOEXCEPT;
    
    // Window lookup
    SystemWindow* get_window_by_native_handle(void* native_handle) const SKR_NOEXCEPT;
    SystemWindow* get_window_from_event(const SkrSystemEvent& event) const SKR_NOEXCEPT;
    
    // User data association
    void set_window_user_data(SystemWindow* window, void* user_data) SKR_NOEXCEPT;
    void* get_window_user_data(SystemWindow* window) const SKR_NOEXCEPT;
    
    // Get all windows
    void get_all_windows(skr::Vector<SystemWindow*>& out_windows) const SKR_NOEXCEPT;
    uint32_t get_window_count() const SKR_NOEXCEPT { return windows_.size(); }
    
    // Get the app
    SystemApp* get_app() const SKR_NOEXCEPT { return app_; }
    
    // Focus management
    SystemWindow* get_focused_window() const SKR_NOEXCEPT { return focused_window_; }
    void set_focused_window(SystemWindow* window) SKR_NOEXCEPT;
    
    // Window ordering (for future use)
    void bring_window_to_front(SystemWindow* window) SKR_NOEXCEPT;
    void send_window_to_back(SystemWindow* window) SKR_NOEXCEPT;
    
private:
    struct ManagedWindow {
        SystemWindow* window;
        void* user_data = nullptr;
    };
    
    SystemApp* app_;
    skr::Map<void*, ManagedWindow> windows_;  // native_handle -> ManagedWindow
    SystemWindow* focused_window_ = nullptr;   // Currently focused window
    WindowManagerHandler* focus_handler_ = nullptr; // Auto focus tracking
};

} // namespace skr