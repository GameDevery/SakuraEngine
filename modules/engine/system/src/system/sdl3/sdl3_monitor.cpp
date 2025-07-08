#include "sdl3_monitor.h"
#include <SDL3/SDL.h>

namespace skr {

SDL3Monitor::SDL3Monitor(SDL_DisplayID display_id) SKR_NOEXCEPT
    : display_id(display_id)
{
}

SDL3Monitor::~SDL3Monitor() SKR_NOEXCEPT = default;

SystemMonitorInfo SDL3Monitor::get_info() const
{
    SystemMonitorInfo info = {};
    
    // Get display name
    const char* name = SDL_GetDisplayName(display_id);
    info.name = name ? (const char8_t*)name : u8"Unknown Display";
    
    // Get display bounds (position and size in virtual desktop)
    SDL_Rect bounds;
    if (SDL_GetDisplayBounds(display_id, &bounds))
    {
        info.position = {(uint32_t)bounds.x, (uint32_t)bounds.y};
        info.size = {(uint32_t)bounds.w, (uint32_t)bounds.h};
    }
    
    // Get usable bounds (work area excluding taskbar etc.)
    SDL_Rect usable_bounds;
    if (SDL_GetDisplayUsableBounds(display_id, &usable_bounds))
    {
        info.work_area_pos = {(uint32_t)usable_bounds.x, (uint32_t)usable_bounds.y};
        info.work_area_size = {(uint32_t)usable_bounds.w, (uint32_t)usable_bounds.h};
    }
    
    // Get content scale (DPI scale)
    info.dpi_scale = SDL_GetDisplayContentScale(display_id);
    
    // Get orientation
    SDL_DisplayOrientation orientation = SDL_GetCurrentDisplayOrientation(display_id);
    switch (orientation)
    {
        case SDL_ORIENTATION_PORTRAIT:
            info.orientation = ESystemMonitorOrientation::Portrait;
            break;
        case SDL_ORIENTATION_LANDSCAPE:
            info.orientation = ESystemMonitorOrientation::Landscape;
            break;
        case SDL_ORIENTATION_PORTRAIT_FLIPPED:
            info.orientation = ESystemMonitorOrientation::PortraitFlipped;
            break;
        case SDL_ORIENTATION_LANDSCAPE_FLIPPED:
            info.orientation = ESystemMonitorOrientation::LandscapeFlipped;
            break;
        default:
            info.orientation = ESystemMonitorOrientation::Unknown;
            break;
    }
    
    // Check if primary display
    info.is_primary = (display_id == SDL_GetPrimaryDisplay());
    
    // Store the display ID as native handle
    info.native_handle = (void*)(uintptr_t)display_id;
    
    return info;
}

uint32_t SDL3Monitor::get_display_mode_count() const
{
    int count = 0;
    SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(display_id, &count);
    if (modes)
    {
        SDL_free(modes);
    }
    return (uint32_t)count;
}

SystemMonitorDisplayMode SDL3Monitor::get_display_mode(uint32_t index) const
{
    SystemMonitorDisplayMode mode = {};
    
    int count = 0;
    SDL_DisplayMode** modes = SDL_GetFullscreenDisplayModes(display_id, &count);
    
    if (modes && index < (uint32_t)count)
    {
        const SDL_DisplayMode* sdl_mode = modes[index];
        mode.resolution = {(uint32_t)sdl_mode->w, (uint32_t)sdl_mode->h};
        mode.refresh_rate = (uint32_t)sdl_mode->refresh_rate;
        
        // SDL3 uses pixel format, convert to bits per pixel
        const SDL_PixelFormatDetails* format_details = SDL_GetPixelFormatDetails(sdl_mode->format);
        if (format_details)
        {
            mode.bits_per_pixel = format_details->bits_per_pixel;
        }
        
        SDL_free(modes);
    }
    
    return mode;
}

SystemMonitorDisplayMode SDL3Monitor::get_current_display_mode() const
{
    SystemMonitorDisplayMode mode = {};
    
    const SDL_DisplayMode* current = SDL_GetCurrentDisplayMode(display_id);
    if (current)
    {
        mode.resolution = {(uint32_t)current->w, (uint32_t)current->h};
        mode.refresh_rate = (uint32_t)current->refresh_rate;
        
        const SDL_PixelFormatDetails* format_details = SDL_GetPixelFormatDetails(current->format);
        if (format_details)
        {
            mode.bits_per_pixel = format_details->bits_per_pixel;
        }
    }
    
    return mode;
}

} // namespace skr