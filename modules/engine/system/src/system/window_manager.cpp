#include "SkrSystem/window_manager.h"
#include "SkrSystem/window.h"
#include "SkrCore/log.h"
#include <cstring>

namespace skr {

ISystemWindowManager::ISystemWindowManager() SKR_NOEXCEPT
{
}

ISystemWindowManager::~ISystemWindowManager() SKR_NOEXCEPT
{
    destroy_all_windows();
}

SystemWindow* ISystemWindowManager::create_window(const SystemWindowCreateInfo& info) SKR_NOEXCEPT
{
    // Validate input
    if (info.size.x == 0 || info.size.y == 0) {
        SKR_LOG_ERROR(u8"Invalid window size: %dx%d", info.size.x, info.size.y);
        return nullptr;
    }
    
    // Call platform implementation
    SystemWindow* window = create_window_internal(info);
    if (!window) {
        return nullptr;
    }
    
    // Register window
    register_window(window);
    
    return window;
}

void ISystemWindowManager::destroy_window(SystemWindow* window) SKR_NOEXCEPT
{
    if (!window) {
        return;
    }
    
    // Unregister window
    unregister_window(window);
    
    // Call platform implementation
    destroy_window_internal(window);
}

void ISystemWindowManager::destroy_all_windows() SKR_NOEXCEPT
{
    // Collect all windows first to avoid iterator invalidation
    skr::Vector<SystemWindow*> windows_to_destroy;
    get_all_windows(windows_to_destroy);
    
    // Destroy each window
    for (auto* window : windows_to_destroy) {
        destroy_window(window);
    }
}

SystemWindow* ISystemWindowManager::get_window_by_native_handle(void* native_handle) const SKR_NOEXCEPT
{
    if (!native_handle) {
        return nullptr;
    }
    
    auto it = window_data_.find(native_handle);
    if (it) {
        return it.value();
    }
    
    return nullptr;
}

SystemWindow* ISystemWindowManager::get_window_from_event(const SkrSystemEvent& event) const SKR_NOEXCEPT
{
    // Try to get native handle from event
    void* native_handle = get_native_handle_from_event(event);
    if (!native_handle) {
        return nullptr;
    }
    
    return get_window_by_native_handle(native_handle);
}

void ISystemWindowManager::get_all_windows(skr::Vector<SystemWindow*>& out_windows) const SKR_NOEXCEPT
{
    out_windows.clear();
    out_windows.reserve(window_data_.size());
    
    for (const auto& [handle, data] : window_data_) {
        out_windows.add(data);
    }
}

void ISystemWindowManager::enumerate_monitors(void (*callback)(SystemMonitor* monitor, void* user_data), void* user_data) const
{
    if (!callback) {
        return;
    }
    
    uint32_t count = get_monitor_count();
    for (uint32_t i = 0; i < count; ++i) {
        if (SystemMonitor* monitor = get_monitor(i)) {
            callback(monitor, user_data);
        }
    }
}

void ISystemWindowManager::register_window(SystemWindow* window) SKR_NOEXCEPT
{
    if (!window) {
        return;
    }
    
    void* native_handle = window->get_native_handle();
    if (!native_handle) {
        SKR_LOG_ERROR(u8"Window has no native handle");
        return;
    }
    
    window_data_.add(native_handle, window);
}

void ISystemWindowManager::unregister_window(SystemWindow* window) SKR_NOEXCEPT
{
    if (!window) 
        return;
    
    if (void* native_handle = window->get_native_handle())
    {
        window_data_.remove(native_handle);
    } 
}

// Factory implementation
ISystemWindowManager* ISystemWindowManager::Create(const char* backend)
{
    // Auto-detect backend if not specified
    if (!backend) {
#ifdef _WIN32
        backend = "Win32";
#elif defined(__APPLE__)
        backend = "Cocoa";
#else
        backend = "SDL3";  // Default to SDL3 on other platforms
#endif
    }
    
    // Create appropriate implementation
    if (strcmp(backend, "SDL3") == 0) {
        // Forward declaration issue - we'll use a factory function instead
        extern ISystemWindowManager* CreateSDL3WindowManager();
        return CreateSDL3WindowManager();
    }
#ifdef _WIN32
    else if (strcmp(backend, "Win32") == 0) {
        extern ISystemWindowManager* CreateWin32WindowManager();
        return CreateWin32WindowManager();
    }
#endif
#ifdef __APPLE__
    else if (strcmp(backend, "Cocoa") == 0) {
        extern ISystemWindowManager* CreateCocoaWindowManager();
        return CreateCocoaWindowManager();
    }
#endif
    
    SKR_LOG_ERROR(u8"Unknown window manager backend: %s", backend);
    return nullptr;
}

void ISystemWindowManager::Destroy(ISystemWindowManager* manager)
{
    if (manager) {
        SkrDelete(manager);
    }
}

} // namespace skr