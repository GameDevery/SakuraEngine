#include "SkrSystem/window_manager.h"
#include "SkrSystem/window_manager_handler.h"
#include "SkrCore/log.h"

namespace skr {

WindowManager::WindowManager(SystemApp* app) SKR_NOEXCEPT
    : app_(app)
{
    // Create and register focus handler
    focus_handler_ = SkrNew<WindowManagerHandler>(this);
    if (app_ && focus_handler_)
    {
        auto* event_queue = app_->get_event_queue();
        if (event_queue)
        {
            event_queue->add_handler(focus_handler_);
        }
    }
    
    SKR_LOG_DEBUG(u8"WindowManager created");
}

WindowManager::~WindowManager() SKR_NOEXCEPT
{
    // Clean up all windows
    destroy_all_windows();
    
    // Unregister and delete focus handler
    if (app_ && focus_handler_)
    {
        auto* event_queue = app_->get_event_queue();
        if (event_queue)
        {
            event_queue->remove_handler(focus_handler_);
        }
    }
    
    if (focus_handler_)
    {
        SkrDelete(focus_handler_);
        focus_handler_ = nullptr;
    }
    
    SKR_LOG_DEBUG(u8"WindowManager destroyed");
}

SystemWindow* WindowManager::create_window(const SystemWindowCreateInfo& info) SKR_NOEXCEPT
{
    if (!app_)
    {
        SKR_LOG_ERROR(u8"WindowManager: app_ is null");
        return nullptr;
    }
    
    // Create window through app's internal method
    SystemWindow* window = app_->create_window_internal(info);
    if (!window)
    {
        SKR_LOG_ERROR(u8"WindowManager: Failed to create window");
        return nullptr;
    }
    
    // Track the window
    void* native_handle = window->get_native_handle();
    windows_.add(native_handle, ManagedWindow{ window, nullptr });
    
    SKR_LOG_DEBUG(u8"WindowManager: Created window with handle %p", native_handle);
    return window;
}

void WindowManager::destroy_window(SystemWindow* window) SKR_NOEXCEPT
{
    if (!window)
    {
        return;
    }
    
    void* native_handle = window->get_native_handle();
    
    // Remove from tracking
    if (windows_.remove(native_handle))
    {
        SKR_LOG_DEBUG(u8"WindowManager: Destroying window with handle %p", native_handle);
        
        // Clear focused window if it's being destroyed
        if (focused_window_ == window)
        {
            focused_window_ = nullptr;
        }
        
        // Destroy through app's internal method
        if (app_)
        {
            app_->destroy_window_internal(window);
        }
    }
    else
    {
        SKR_LOG_WARN(u8"WindowManager: Window with handle %p not found in tracking", native_handle);
    }
}

void WindowManager::destroy_all_windows() SKR_NOEXCEPT
{
    // Collect all windows first to avoid modifying map while iterating
    skr::Vector<SystemWindow*> windows_to_destroy;
    for (const auto& pair : windows_)
    {
        windows_to_destroy.add(pair.value.window);
    }
    
    // Destroy each window
    for (auto* window : windows_to_destroy)
    {
        destroy_window(window);
    }
}

SystemWindow* WindowManager::get_window_by_native_handle(void* native_handle) const SKR_NOEXCEPT
{
    auto found = windows_.find(native_handle);
    return found ? found.value().window : nullptr;
}

SystemWindow* WindowManager::get_window_from_event(const SkrSystemEvent& event) const SKR_NOEXCEPT
{
    void* native_handle = nullptr;
    
    // Extract native handle based on event type
    switch (event.type)
    {
        case SKR_SYSTEM_EVENT_WINDOW_SHOWN:
        case SKR_SYSTEM_EVENT_WINDOW_HIDDEN:
        case SKR_SYSTEM_EVENT_WINDOW_MOVED:
        case SKR_SYSTEM_EVENT_WINDOW_RESIZED:
        case SKR_SYSTEM_EVENT_WINDOW_MINIMIZED:
        case SKR_SYSTEM_EVENT_WINDOW_MAXIMIZED:
        case SKR_SYSTEM_EVENT_WINDOW_RESTORED:
        case SKR_SYSTEM_EVENT_WINDOW_MOUSE_ENTER:
        case SKR_SYSTEM_EVENT_WINDOW_MOUSE_LEAVE:
        case SKR_SYSTEM_EVENT_WINDOW_FOCUS_GAINED:
        case SKR_SYSTEM_EVENT_WINDOW_FOCUS_LOST:
        case SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED:
        case SKR_SYSTEM_EVENT_WINDOW_ENTER_FULLSCREEN:
        case SKR_SYSTEM_EVENT_WINDOW_LEAVE_FULLSCREEN:
            native_handle = reinterpret_cast<void*>(event.window.window_native_handle);
            break;
            
        case SKR_SYSTEM_EVENT_KEY_DOWN:
        case SKR_SYSTEM_EVENT_KEY_UP:
            native_handle = reinterpret_cast<void*>(event.key.window_native_handle);
            break;
            
        case SKR_SYSTEM_EVENT_MOUSE_MOVE:
        case SKR_SYSTEM_EVENT_MOUSE_BUTTON_DOWN:
        case SKR_SYSTEM_EVENT_MOUSE_BUTTON_UP:
        case SKR_SYSTEM_EVENT_MOUSE_WHEEL:
        case SKR_SYSTEM_EVENT_MOUSE_ENTER:
        case SKR_SYSTEM_EVENT_MOUSE_LEAVE:
            native_handle = reinterpret_cast<void*>(event.mouse.window_native_handle);
            break;
            
        default:
            break;
    }
    
    return native_handle ? get_window_by_native_handle(native_handle) : nullptr;
}

void WindowManager::set_window_user_data(SystemWindow* window, void* user_data) SKR_NOEXCEPT
{
    if (!window)
    {
        return;
    }
    
    void* native_handle = window->get_native_handle();
    auto found = windows_.find(native_handle);
    if (found)
    {
        found.value().user_data = user_data;
    }
}

void* WindowManager::get_window_user_data(SystemWindow* window) const SKR_NOEXCEPT
{
    if (!window)
    {
        return nullptr;
    }
    
    void* native_handle = window->get_native_handle();
    auto found = windows_.find(native_handle);
    return found ? found.value().user_data : nullptr;
}

void WindowManager::get_all_windows(skr::Vector<SystemWindow*>& out_windows) const SKR_NOEXCEPT
{
    out_windows.clear();
    out_windows.reserve(windows_.size());
    
    for (const auto& pair : windows_)
    {
        out_windows.add(pair.value.window);
    }
}

void WindowManager::set_focused_window(SystemWindow* window) SKR_NOEXCEPT
{
    if (!window)
    {
        focused_window_ = nullptr;
        return;
    }
    
    // Verify the window is managed by us
    void* native_handle = window->get_native_handle();
    if (windows_.find(native_handle))
    {
        focused_window_ = window;
        SKR_LOG_DEBUG(u8"WindowManager: Set focused window to handle %p", native_handle);
    }
    else
    {
        SKR_LOG_WARN(u8"WindowManager: Attempted to set focus to unmanaged window %p", native_handle);
    }
}

void WindowManager::bring_window_to_front(SystemWindow* window) SKR_NOEXCEPT
{
    if (!window)
    {
        return;
    }
    
    // This is a placeholder for future implementation
    // Different platforms have different ways to manage window z-order
    // For now, just ensure the window has focus
    window->focus();
    set_focused_window(window);
}

void WindowManager::send_window_to_back(SystemWindow* window) SKR_NOEXCEPT
{
    if (!window)
    {
        return;
    }
    
    // This is a placeholder for future implementation
    // Platform-specific code would go here
}

} // namespace skr