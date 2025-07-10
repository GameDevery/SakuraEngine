#pragma once
#include "SkrSystem/window.h"
#include <SDL3/SDL_video.h>

namespace skr {

class SDL3Window : public SystemWindow
{
public:
    SDL3Window(SDL_Window* window) SKR_NOEXCEPT;
    ~SDL3Window() SKR_NOEXCEPT override;

    // Window properties
    void set_title(const skr::String& title) override;
    skr::String get_title() const override;
    
    void set_position(int32_t x, int32_t y) override;
    int2 get_position() const override;
    
    void set_size(uint32_t width, uint32_t height) override;
    uint2 get_size() const override;
    
    // Physical size and DPI
    uint2 get_physical_size() const override;
    float get_pixel_ratio() const override;
    
    // Window state
    void show() override;
    void hide() override;
    void minimize() override;
    void maximize() override;
    void restore() override;
    void focus() override;
    
    bool is_visible() const override;
    bool is_minimized() const override;
    bool is_maximized() const override;
    bool is_focused() const override;
    
    // Opacity
    void set_opacity(float opacity) override;
    float get_opacity() const override;
    
    // Fullscreen
    void set_fullscreen(bool fullscreen, SystemMonitor* monitor = nullptr) override;
    bool is_fullscreen() const override;
    
    // Platform handle
    void* get_native_handle() const override;
    void* get_native_view() const override;
    
    // SDL3 specific
    SDL_Window* get_sdl_window() const { return sdl_window; }
    SDL_WindowID get_window_id() const { return window_id; }

private:
    SDL_Window* sdl_window;
    SDL_WindowID window_id;
};

} // namespace skr