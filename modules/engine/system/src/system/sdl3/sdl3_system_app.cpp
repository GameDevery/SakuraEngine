#include "sdl3_system_app.h"
#include "sdl3_monitor.h"
#include "sdl3_window.h"
#include "sdl3_ime.h"
#include "SkrCore/log.h"
#include <SDL3/SDL.h>

namespace skr {

SDL3SystemApp::SDL3SystemApp() SKR_NOEXCEPT
{
    // Initialize SDL video subsystem
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        SKR_LOG_ERROR(u8"Failed to initialize SDL3: %s", SDL_GetError());
    }
    
    // Set DPI awareness on Windows
    #ifdef _WIN32
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
    #endif
    
    // Cache all monitors
    refresh_monitors();
    
    // Create IME
    ime = IME::Create(this);
}

SDL3SystemApp::~SDL3SystemApp() SKR_NOEXCEPT
{
    // Clean up IME
    if (ime)
    {
        IME::Destroy(ime);
        ime = nullptr;
    }
    
    // Clean up cached windows
    for (auto& pair : window_cache)
    {
        SkrDelete(pair.value);
    }
    window_cache.clear();
    
    // Clean up cached monitors
    for (auto& pair : monitor_cache)
    {
        SkrDelete(pair.value);
    }
    monitor_cache.clear();
    monitor_list.clear();
    
    // Quit SDL
    SDL_Quit();
}

// Window management
SystemWindow* SDL3SystemApp::create_window(const SystemWindowCreateInfo& create_info)
{
    // Validate input
    if (create_info.size.x == 0 || create_info.size.y == 0)
    {
        SKR_LOG_ERROR(u8"Invalid window size: %dx%d", create_info.size.x, create_info.size.y);
        return nullptr;
    }
    
    // Convert window flags
    Uint32 flags = SDL_WINDOW_VULKAN; // Default to Vulkan-capable
    
    if (create_info.is_borderless)
        flags |= SDL_WINDOW_BORDERLESS;
    if (create_info.is_resizable)
        flags |= SDL_WINDOW_RESIZABLE;
    if (create_info.is_topmost)
        flags |= SDL_WINDOW_ALWAYS_ON_TOP;
    if (create_info.is_tooltip)
        flags |= SDL_WINDOW_TOOLTIP;
    
    // Create SDL window
    SDL_Window* sdl_window = SDL_CreateWindow(
        (const char*)create_info.title.c_str(),
        static_cast<int>(create_info.size.x),
        static_cast<int>(create_info.size.y),
        flags
    );
    
    if (!sdl_window)
    {
        SKR_LOG_ERROR(u8"Failed to create SDL window: %s", SDL_GetError());
        return nullptr;
    }
    
    // Set position if specified
    if (create_info.pos.has_value())
    {
        SDL_SetWindowPosition(sdl_window, 
            static_cast<int>(create_info.pos.value().x), 
            static_cast<int>(create_info.pos.value().y));
    }
    
    // Create wrapper
    SDL3Window* window = SkrNew<SDL3Window>(sdl_window);
    if (!window)
    {
        SDL_DestroyWindow(sdl_window);
        SKR_LOG_ERROR(u8"Failed to allocate SDL3Window wrapper");
        return nullptr;
    }
    
    SDL_WindowID window_id = SDL_GetWindowID(sdl_window);
    window_cache.add(window_id, window);
    
    return window;
}

void SDL3SystemApp::destroy_window(SystemWindow* window)
{
    if (!window)
        return;
        
    SDL3Window* sdl3_window = static_cast<SDL3Window*>(window);
    SDL_WindowID window_id = sdl3_window->get_window_id();
    
    // Remove from cache
    window_cache.remove(window_id);
    
    // Delete window (destructor handles SDL_DestroyWindow)
    SkrDelete(sdl3_window);
}

// Monitor/Display management
uint32_t SDL3SystemApp::get_monitor_count() const
{
    return (uint32_t)monitor_list.size();
}

SystemMonitor* SDL3SystemApp::get_monitor(uint32_t index) const
{
    if (index < monitor_list.size())
    {
        return monitor_list[index];
    }
    return nullptr;
}

SystemMonitor* SDL3SystemApp::get_primary_monitor() const
{
    SDL_DisplayID primary = SDL_GetPrimaryDisplay();
    if (auto found = monitor_cache.find(primary))
    {
        return found.value();
    }
    return nullptr;
}

SystemMonitor* SDL3SystemApp::get_monitor_from_point(int32_t x, int32_t y) const
{
    SDL_Point point = {x, y};
    SDL_DisplayID display = SDL_GetDisplayForPoint(&point);
    
    if (display != 0)
    {
        if (auto found = monitor_cache.find(display))
        {
            return found.value();
        }
    }
    
    return nullptr;
}

SystemMonitor* SDL3SystemApp::get_monitor_from_window(SystemWindow* window) const
{
    if (!window)
        return nullptr;
        
    // Get SDL window from native handle
    auto* sdl_window = static_cast<SDL_Window*>(window->get_native_handle());
    if (!sdl_window)
        return nullptr;
        
    SDL_DisplayID display = SDL_GetDisplayForWindow(sdl_window);
    if (display == 0)
        return nullptr;
        
    if (auto found = monitor_cache.find(display))
    {
        return found.value();
    }
    
    return nullptr;
}

void SDL3SystemApp::enumerate_monitors(void (*callback)(SystemMonitor* monitor, void* user_data), void* user_data) const
{
    if (!callback)
        return;
        
    for (auto* monitor : monitor_list)
    {
        callback(monitor, user_data);
    }
}

void SDL3SystemApp::refresh_monitors()
{
    // Clear existing monitors
    for (auto& pair : monitor_cache)
    {
        SkrDelete(pair.value);
    }
    monitor_cache.clear();
    monitor_list.clear();
    
    // Get all displays
    int count = 0;
    SDL_DisplayID* displays = SDL_GetDisplays(&count);
    
    if (!displays)
    {
        SKR_LOG_ERROR(u8"Failed to get displays: %s", SDL_GetError());
        return;
    }
    
    monitor_list.reserve(count);
    
    for (int i = 0; i < count; ++i)
    {
        SDL3Monitor* monitor = SkrNew<SDL3Monitor>(displays[i]);
        if (monitor)
        {
            monitor_cache.add(displays[i], monitor);
            monitor_list.push_back(monitor);
        }
    }
    
    SDL_free(displays);
    
    SKR_LOG_INFO(u8"Refreshed monitors: found %d display(s)", count);
}

// Factory methods implementation
SystemApp* SystemApp::Create(const char* backend)
{
    if (!backend || strcmp(backend, "SDL") == 0 || strcmp(backend, "SDL3") == 0)
    {
        SDL3SystemApp* sdl_app = SkrNew<SDL3SystemApp>();
        return static_cast<SystemApp*>(sdl_app);
    }
    
    // TODO: Add other backends (Win32, Cocoa)
    return nullptr;
}

void SystemApp::Destroy(SystemApp* app)
{
    if (app)
    {
        SkrDelete(app);
    }
}

} // namespace skr