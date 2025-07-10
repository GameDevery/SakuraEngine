#include "sdl3_window_manager.h"
#include "sdl3_window.h"
#include "sdl3_monitor.h"
#include "SkrCore/log.h"
#include <SDL3/SDL.h>

namespace skr {

SDL3WindowManager::SDL3WindowManager() SKR_NOEXCEPT
{
    // Initialize monitors on creation
    refresh_monitors();
    
    SKR_LOG_DEBUG(u8"SDL3WindowManager created");
}

SDL3WindowManager::~SDL3WindowManager() SKR_NOEXCEPT
{
    // Clean up all windows first
    destroy_all_windows();
    
    // Clean up cached windows
    for (auto& pair : sdl_window_cache_) {
        SkrDelete(pair.value);
    }
    sdl_window_cache_.clear();
    
    // Clean up cached monitors
    for (auto& pair : monitor_cache_) {
        SkrDelete(pair.value);
    }
    monitor_cache_.clear();
    monitor_list_.clear();
    
    SKR_LOG_DEBUG(u8"SDL3WindowManager destroyed");
}

// Monitor management implementation
uint32_t SDL3WindowManager::get_monitor_count() const
{
    return static_cast<uint32_t>(monitor_list_.size());
}

SystemMonitor* SDL3WindowManager::get_monitor(uint32_t index) const
{
    if (index < monitor_list_.size()) {
        return monitor_list_[index];
    }
    return nullptr;
}

SystemMonitor* SDL3WindowManager::get_primary_monitor() const
{
    SDL_DisplayID primary = SDL_GetPrimaryDisplay();
    if (auto found = monitor_cache_.find(primary)) {
        return found.value();
    }
    return nullptr;
}

SystemMonitor* SDL3WindowManager::get_monitor_from_point(int32_t x, int32_t y) const
{
    SDL_Point point = {x, y};
    SDL_DisplayID display = SDL_GetDisplayForPoint(&point);
    
    if (display != 0) {
        if (auto found = monitor_cache_.find(display)) {
            return found.value();
        }
    }
    
    return nullptr;
}

SystemMonitor* SDL3WindowManager::get_monitor_from_window(SystemWindow* window) const
{
    if (!window) {
        return nullptr;
    }
    
    // Get SDL window from native handle
    auto* sdl_window = static_cast<SDL_Window*>(window->get_native_handle());
    if (!sdl_window) {
        return nullptr;
    }
    
    SDL_DisplayID display = SDL_GetDisplayForWindow(sdl_window);
    if (display == 0) {
        return nullptr;
    }
    
    if (auto found = monitor_cache_.find(display)) {
        return found.value();
    }
    
    return nullptr;
}

void SDL3WindowManager::refresh_monitors()
{
    // Clear existing monitors
    for (auto& pair : monitor_cache_) {
        SkrDelete(pair.value);
    }
    monitor_cache_.clear();
    monitor_list_.clear();
    
    // Get all displays
    int count = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    
    if (!displays) {
        SKR_LOG_ERROR(u8"Failed to get displays: %s", SDL_GetError());
        return;
    }
    
    monitor_list_.reserve(count);
    
    for (int i = 0; i < count; ++i) {
        SDL3Monitor* monitor = create_monitor(displays[i]);
        if (monitor) {
            monitor_cache_.add(displays[i], monitor);
            monitor_list_.add(monitor);
        }
    }
    
    SDL_free(displays);
    
    SKR_LOG_INFO(u8"Refreshed monitors: found %d display(s)", count);
}

// SDL3 specific helpers
SDL3Window* SDL3WindowManager::get_sdl3_window(SDL_WindowID window_id) const
{
    if (auto found = sdl_window_cache_.find(window_id)) {
        return found.value();
    }
    return nullptr;
}

void SDL3WindowManager::register_sdl_window(SDL_WindowID window_id, SDL3Window* window)
{
    sdl_window_cache_.add(window_id, window);
}

void SDL3WindowManager::unregister_sdl_window(SDL_WindowID window_id)
{
    sdl_window_cache_.remove(window_id);
}

// Platform implementation
SystemWindow* SDL3WindowManager::create_window_internal(const SystemWindowCreateInfo& info)
{
    // Validate input
    if (info.size.x == 0 || info.size.y == 0) {
        SKR_LOG_ERROR(u8"Invalid window size: %dx%d", info.size.x, info.size.y);
        return nullptr;
    }
    
    // Convert window flags
    Uint32 flags = SDL_WINDOW_VULKAN; // Default to Vulkan-capable
    
    if (info.is_borderless) {
        flags |= SDL_WINDOW_BORDERLESS;
    }
    if (info.is_resizable) {
        flags |= SDL_WINDOW_RESIZABLE;
    }
    if (info.is_topmost) {
        flags |= SDL_WINDOW_ALWAYS_ON_TOP;
    }
    if (info.is_tooltip) {
        flags |= SDL_WINDOW_TOOLTIP;
    }
    
    // Create SDL window
    SDL_Window* sdl_window = SDL_CreateWindow(
        (const char*)info.title.c_str(),
        static_cast<int>(info.size.x),
        static_cast<int>(info.size.y),
        flags
    );
    
    if (!sdl_window) {
        SKR_LOG_ERROR(u8"Failed to create SDL window: %s", SDL_GetError());
        return nullptr;
    }
    
    // Set position if specified
    if (info.pos.has_value()) {
        SDL_SetWindowPosition(sdl_window, 
            static_cast<int>(info.pos.value().x), 
            static_cast<int>(info.pos.value().y));
    }
    
    // Create wrapper
    SDL3Window* window = SkrNew<SDL3Window>(sdl_window);
    if (!window) {
        SDL_DestroyWindow(sdl_window);
        SKR_LOG_ERROR(u8"Failed to allocate SDL3Window wrapper");
        return nullptr;
    }
    
    // Register in SDL-specific cache
    SDL_WindowID window_id = SDL_GetWindowID(sdl_window);
    register_sdl_window(window_id, window);
    
    return window;
}

void SDL3WindowManager::destroy_window_internal(SystemWindow* window)
{
    if (!window) {
        return;
    }
    
    SDL3Window* sdl3_window = static_cast<SDL3Window*>(window);
    SDL_WindowID window_id = sdl3_window->get_window_id();
    
    // Remove from SDL cache
    unregister_sdl_window(window_id);
    
    // Delete window (destructor handles SDL_DestroyWindow)
    SkrDelete(sdl3_window);
}

// Helper methods
void SDL3WindowManager::update_monitor_cache()
{
    refresh_monitors();
}

SDL3Monitor* SDL3WindowManager::create_monitor(SDL_DisplayID display_id)
{
    return SkrNew<SDL3Monitor>(display_id);
}

// Factory function for WindowManager
ISystemWindowManager* CreateSDL3WindowManager()
{
    SDL_Init(SDL_INIT_EVENTS | SDL_INIT_VIDEO);
    return SkrNew<SDL3WindowManager>();
}

} // namespace skr