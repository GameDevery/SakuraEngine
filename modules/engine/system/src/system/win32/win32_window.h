#pragma once
#include "SkrSystem/window.h"
#include <Windows.h>

namespace skr {

class Win32Window : public SystemWindow
{
public:
    Win32Window(HWND hwnd) SKR_NOEXCEPT;
    ~Win32Window() SKR_NOEXCEPT override;

    // SystemWindow interface
    void set_title(const skr::String& title) override;
    skr::String get_title() const override;
    
    void set_position(int32_t x, int32_t y) override;
    int2 get_position() const override;
    
    void set_size(uint32_t width, uint32_t height) override;
    uint2 get_size() const override;
    
    // Physical size and DPI
    uint2 get_physical_size() const override;
    float get_pixel_ratio() const override;
    
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
    
    void set_opacity(float opacity) override;
    float get_opacity() const override;
    
    void set_fullscreen(bool fullscreen, SystemMonitor* monitor = nullptr) override;
    bool is_fullscreen() const override;
    
    void* get_native_handle() const override { return hwnd_; }
    void* get_native_display() const override { return nullptr; }
    
    // Win32 specific
    HWND get_hwnd() const { return hwnd_; }
    
    // Window procedure handler
    LRESULT handle_message(UINT msg, WPARAM wParam, LPARAM lParam);

private:
    void cache_window_info();
    void apply_window_styles(bool fullscreen);
    HMONITOR get_current_monitor() const;
    
private:
    HWND hwnd_;
    
    // Cached window state for fullscreen toggle
    struct WindowState {
        LONG style;
        LONG ex_style;
        RECT rect;
        bool maximized;
    };
    WindowState saved_state_ = {};
    bool is_fullscreen_ = false;
    
    // Track focus state
    bool has_focus_ = false;
    
    // Track first paint for black background
    bool first_paint_ = true;
};

} // namespace skr