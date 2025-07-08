#include "sdl3_window.h"
#include "sdl3_monitor.h"
#include "SkrCore/log.h"
#include <SDL3/SDL.h>

namespace skr {

SDL3Window::SDL3Window(SDL_Window* window) SKR_NOEXCEPT
    : sdl_window(window)
    , window_id(SDL_GetWindowID(window))
{
}

SDL3Window::~SDL3Window() SKR_NOEXCEPT
{
    if (sdl_window)
    {
        SDL_DestroyWindow(sdl_window);
        sdl_window = nullptr;
    }
}

// Window properties
void SDL3Window::set_title(const skr::String& title)
{
    SDL_SetWindowTitle(sdl_window, (const char*)title.c_str());
}

skr::String SDL3Window::get_title() const
{
    const char* title = SDL_GetWindowTitle(sdl_window);
    return skr::String((const char8_t*)title);
}

void SDL3Window::set_position(int32_t x, int32_t y)
{
    SDL_SetWindowPosition(sdl_window, x, y);
}

int2 SDL3Window::get_position() const
{
    int x, y;
    SDL_GetWindowPosition(sdl_window, &x, &y);
    return {x, y};
}

void SDL3Window::set_size(uint32_t width, uint32_t height)
{
    SDL_SetWindowSize(sdl_window, (int)width, (int)height);
}

uint2 SDL3Window::get_size() const
{
    int w, h;
    SDL_GetWindowSize(sdl_window, &w, &h);
    return {(uint32_t)w, (uint32_t)h};
}

uint2 SDL3Window::get_physical_size() const
{
    int w, h;
    SDL_GetWindowSizeInPixels(sdl_window, &w, &h);
    return {(uint32_t)w, (uint32_t)h};
}

float SDL3Window::get_pixel_ratio() const
{
    int window_w, window_h;
    int pixel_w, pixel_h;
    SDL_GetWindowSize(sdl_window, &window_w, &window_h);
    SDL_GetWindowSizeInPixels(sdl_window, &pixel_w, &pixel_h);
    
    // Calculate average pixel ratio (handle non-uniform scaling)
    if (window_w > 0 && window_h > 0)
    {
        float ratio_x = (float)pixel_w / window_w;
        float ratio_y = (float)pixel_h / window_h;
        return (ratio_x + ratio_y) / 2.0f;
    }
    
    return 1.0f;
}

// Window state
void SDL3Window::show()
{
    SDL_ShowWindow(sdl_window);
}

void SDL3Window::hide()
{
    SDL_HideWindow(sdl_window);
}

void SDL3Window::minimize()
{
    SDL_MinimizeWindow(sdl_window);
}

void SDL3Window::maximize()
{
    SDL_MaximizeWindow(sdl_window);
}

void SDL3Window::restore()
{
    SDL_RestoreWindow(sdl_window);
}

void SDL3Window::focus()
{
    SDL_RaiseWindow(sdl_window);
}

bool SDL3Window::is_visible() const
{
    Uint32 flags = SDL_GetWindowFlags(sdl_window);
    return !(flags & SDL_WINDOW_HIDDEN);
}

bool SDL3Window::is_minimized() const
{
    Uint32 flags = SDL_GetWindowFlags(sdl_window);
    return (flags & SDL_WINDOW_MINIMIZED) != 0;
}

bool SDL3Window::is_maximized() const
{
    Uint32 flags = SDL_GetWindowFlags(sdl_window);
    return (flags & SDL_WINDOW_MAXIMIZED) != 0;
}

bool SDL3Window::is_focused() const
{
    Uint32 flags = SDL_GetWindowFlags(sdl_window);
    return (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
}

// Opacity
void SDL3Window::set_opacity(float opacity)
{
    SDL_SetWindowOpacity(sdl_window, opacity);
}

float SDL3Window::get_opacity() const
{
    return SDL_GetWindowOpacity(sdl_window);
}

// Fullscreen
void SDL3Window::set_fullscreen(bool fullscreen, SystemMonitor* monitor)
{
    if (fullscreen)
    {
        SDL_DisplayMode mode;
        
        if (monitor)
        {
            // Use specified monitor
            SDL3Monitor* sdl_monitor = static_cast<SDL3Monitor*>(monitor);
            SDL_DisplayID display_id = sdl_monitor->get_display_id();
            
            // Get current display mode
            const SDL_DisplayMode* current_mode = SDL_GetCurrentDisplayMode(display_id);
            if (current_mode)
            {
                mode = *current_mode;
                SDL_SetWindowFullscreenMode(sdl_window, &mode);
                SDL_SetWindowFullscreen(sdl_window, true);
            }
        }
        else
        {
            // Use current monitor
            SDL_SetWindowFullscreen(sdl_window, true);
        }
    }
    else
    {
        SDL_SetWindowFullscreen(sdl_window, false);
    }
}

bool SDL3Window::is_fullscreen() const
{
    Uint32 flags = SDL_GetWindowFlags(sdl_window);
    return (flags & SDL_WINDOW_FULLSCREEN) != 0;
}

// Platform handle
void* SDL3Window::get_native_handle() const
{
    // SDL3 uses properties system for native handles
    SDL_PropertiesID props = SDL_GetWindowProperties(sdl_window);
    
#ifdef _WIN32
    void* hwnd = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
    if (hwnd)
        return hwnd;
#elif defined(__APPLE__)
    void* nswindow = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
    if (nswindow)
        return nswindow;
#elif defined(__linux__)
    // Try X11 first  
    // X11 window is actually a number, need to use GetNumberProperty
    Sint64 xwindow = SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
    if (xwindow)
        return (void*)(uintptr_t)xwindow;
        
    // Try Wayland
    void* wl_surface = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
    if (wl_surface)
        return wl_surface;
#endif
    
    // Fallback: return SDL window as opaque handle
    return sdl_window;
}

void* SDL3Window::get_native_display() const
{
    // SDL3 uses properties system for native handles
    SDL_PropertiesID props = SDL_GetWindowProperties(sdl_window);
    
#ifdef _WIN32
    void* hdc = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HDC_POINTER, nullptr);
    if (hdc)
        return hdc;
#elif defined(__linux__)
    // Try X11 display
    void* x11_display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
    if (x11_display)
        return x11_display;
        
    // Try Wayland display
    void* wl_display = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
    if (wl_display)
        return wl_display;
#endif
    
    return nullptr;
}

} // namespace skr