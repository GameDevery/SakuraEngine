#include "win32_window.h"
#include "win32_system_app.h"
#include "win32_monitor.h"
#include "SkrCore/log.h"
#include <Dwmapi.h>

#pragma comment(lib, "Dwmapi.lib")

namespace skr {

Win32Window::Win32Window(HWND hwnd) SKR_NOEXCEPT
    : hwnd_(hwnd)
{
    cache_window_info();
}

Win32Window::~Win32Window() SKR_NOEXCEPT
{
    if (hwnd_)
    {
        DestroyWindow(hwnd_);
        hwnd_ = nullptr;
    }
}

void Win32Window::set_title(const skr::String& title)
{
    if (!hwnd_)
        return;
        
    // Convert UTF-8 to wide string
    int len = MultiByteToWideChar(CP_UTF8, 0, (const char*)title.c_str(), -1, nullptr, 0);
    skr::Vector<wchar_t> wide_title;
    wide_title.resize_default(len);
    MultiByteToWideChar(CP_UTF8, 0, (const char*)title.c_str(), -1, wide_title.data(), len);
    
    SetWindowTextW(hwnd_, wide_title.data());
}

skr::String Win32Window::get_title() const
{
    if (!hwnd_)
        return skr::String();
        
    int len = GetWindowTextLengthW(hwnd_);
    if (len == 0)
        return skr::String();
        
    skr::Vector<wchar_t> wide_title;
    wide_title.resize_default(len + 1);
    GetWindowTextW(hwnd_, wide_title.data(), len + 1);
    
    // Convert wide string to UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wide_title.data(), -1, nullptr, 0, nullptr, nullptr);
    skr::Vector<char> utf8_title;
    utf8_title.resize_default(utf8_len);
    WideCharToMultiByte(CP_UTF8, 0, wide_title.data(), -1, utf8_title.data(), utf8_len, nullptr, nullptr);
    
    return skr::String((const char8_t*)utf8_title.data());
}

void Win32Window::set_position(int32_t x, int32_t y)
{
    if (!hwnd_)
        return;
        
    SetWindowPos(hwnd_, nullptr, x, y, 0, 0, 
        SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}

void Win32Window::set_size(uint32_t width, uint32_t height)
{
    if (!hwnd_)
        return;
        
    // Convert logical size to physical pixels
    uint32_t dpi = Win32SystemApp::get_dpi_for_window(hwnd_);
    float scale = dpi / 96.0f;
    
    uint32_t physical_width = static_cast<uint32_t>(width * scale);
    uint32_t physical_height = static_cast<uint32_t>(height * scale);
    
    // Get window style to calculate proper size including borders
    DWORD style = GetWindowLongW(hwnd_, GWL_STYLE);
    DWORD ex_style = GetWindowLongW(hwnd_, GWL_EXSTYLE);
    
    RECT rect = { 0, 0, static_cast<LONG>(physical_width), static_cast<LONG>(physical_height) };
    Win32SystemApp::adjust_window_rect_for_dpi(&rect, style, ex_style, dpi);
    
    SetWindowPos(hwnd_, nullptr, 0, 0, 
        rect.right - rect.left, rect.bottom - rect.top,
        SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
}

int2 Win32Window::get_position() const
{
    if (!hwnd_)
        return {0, 0};
    
    RECT rect;
    GetWindowRect(hwnd_, &rect);
    return {rect.left, rect.top};
}

uint2 Win32Window::get_size() const
{
    if (!hwnd_)
        return {0, 0};
    
    // Return logical size (DPI-scaled)
    RECT rect;
    GetClientRect(hwnd_, &rect);
    
    // Get DPI and calculate logical size
    uint32_t dpi = Win32SystemApp::get_dpi_for_window(hwnd_);
    float scale = dpi / 96.0f;
    
    uint32_t logical_width = static_cast<uint32_t>((rect.right - rect.left) / scale);
    uint32_t logical_height = static_cast<uint32_t>((rect.bottom - rect.top) / scale);
    
    return {logical_width, logical_height};
}

uint2 Win32Window::get_physical_size() const
{
    if (!hwnd_)
        return {0, 0};
        
    // Return physical pixel size
    RECT rect;
    GetClientRect(hwnd_, &rect);
    return {static_cast<uint32_t>(rect.right - rect.left), 
            static_cast<uint32_t>(rect.bottom - rect.top)};
}

float Win32Window::get_pixel_ratio() const
{
    if (!hwnd_)
        return 1.0f;
        
    uint32_t dpi = Win32SystemApp::get_dpi_for_window(hwnd_);
    return dpi / 96.0f;
}

void Win32Window::show()
{
    if (hwnd_)
    {
        ShowWindow(hwnd_, SW_SHOW);
    }
}

void Win32Window::hide()
{
    if (hwnd_)
    {
        ShowWindow(hwnd_, SW_HIDE);
    }
}

void Win32Window::minimize()
{
    if (hwnd_)
    {
        ShowWindow(hwnd_, SW_MINIMIZE);
    }
}

void Win32Window::maximize()
{
    if (hwnd_)
    {
        ShowWindow(hwnd_, SW_MAXIMIZE);
    }
}

void Win32Window::restore()
{
    if (hwnd_)
    {
        ShowWindow(hwnd_, SW_RESTORE);
    }
}

void Win32Window::focus()
{
    if (hwnd_)
    {
        SetForegroundWindow(hwnd_);
        SetFocus(hwnd_);
    }
}

bool Win32Window::is_visible() const
{
    return hwnd_ && IsWindowVisible(hwnd_);
}

bool Win32Window::is_minimized() const
{
    return hwnd_ && IsIconic(hwnd_);
}

bool Win32Window::is_maximized() const
{
    return hwnd_ && IsZoomed(hwnd_);
}

bool Win32Window::is_focused() const
{
    return has_focus_;
}

void Win32Window::set_opacity(float opacity)
{
    if (!hwnd_)
        return;
        
    // Clamp opacity
    opacity = (opacity < 0.0f) ? 0.0f : (opacity > 1.0f) ? 1.0f : opacity;
    BYTE alpha = static_cast<BYTE>(opacity * 255);
    
    // Enable layered window
    LONG ex_style = GetWindowLongW(hwnd_, GWL_EXSTYLE);
    if (!(ex_style & WS_EX_LAYERED))
    {
        SetWindowLongW(hwnd_, GWL_EXSTYLE, ex_style | WS_EX_LAYERED);
    }
    
    SetLayeredWindowAttributes(hwnd_, 0, alpha, LWA_ALPHA);
}

float Win32Window::get_opacity() const
{
    if (!hwnd_)
        return 1.0f;
        
    LONG ex_style = GetWindowLongW(hwnd_, GWL_EXSTYLE);
    if (!(ex_style & WS_EX_LAYERED))
        return 1.0f;
        
    BYTE alpha;
    DWORD flags;
    COLORREF color;
    if (GetLayeredWindowAttributes(hwnd_, &color, &alpha, &flags) && (flags & LWA_ALPHA))
    {
        return alpha / 255.0f;
    }
    
    return 1.0f;
}

void Win32Window::set_fullscreen(bool fullscreen, SystemMonitor* monitor)
{
    if (!hwnd_ || is_fullscreen_ == fullscreen)
        return;
        
    if (fullscreen)
    {
        // Save current window state
        saved_state_.style = GetWindowLongW(hwnd_, GWL_STYLE);
        saved_state_.ex_style = GetWindowLongW(hwnd_, GWL_EXSTYLE);
        GetWindowRect(hwnd_, &saved_state_.rect);
        saved_state_.maximized = IsZoomed(hwnd_);
        
        // Get target monitor
        HMONITOR hMonitor = monitor ? 
            static_cast<Win32Monitor*>(monitor)->get_handle() : 
            get_current_monitor();
            
        // Get monitor rect
        MONITORINFO mi = {};
        mi.cbSize = sizeof(mi);
        GetMonitorInfoW(hMonitor, &mi);
        
        // Apply fullscreen styles
        apply_window_styles(true);
        
        // Set fullscreen window position
        SetWindowPos(hwnd_, HWND_TOP,
            mi.rcMonitor.left, mi.rcMonitor.top,
            mi.rcMonitor.right - mi.rcMonitor.left,
            mi.rcMonitor.bottom - mi.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);
            
        is_fullscreen_ = true;
    }
    else
    {
        // Restore window styles
        apply_window_styles(false);
        
        // Restore window position
        SetWindowPos(hwnd_, nullptr,
            saved_state_.rect.left, saved_state_.rect.top,
            saved_state_.rect.right - saved_state_.rect.left,
            saved_state_.rect.bottom - saved_state_.rect.top,
            SWP_FRAMECHANGED | SWP_NOZORDER | SWP_NOACTIVATE);
            
        // Restore maximized state if needed
        if (saved_state_.maximized)
        {
            ShowWindow(hwnd_, SW_MAXIMIZE);
        }
        
        is_fullscreen_ = false;
    }
}

bool Win32Window::is_fullscreen() const
{
    return is_fullscreen_;
}

void Win32Window::cache_window_info()
{
    if (!hwnd_)
        return;
        
    // Cache initial focus state
    has_focus_ = (GetFocus() == hwnd_);
}

void Win32Window::apply_window_styles(bool fullscreen)
{
    if (!hwnd_)
        return;
        
    if (fullscreen)
    {
        // Remove window decorations
        SetWindowLongW(hwnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE);
        SetWindowLongW(hwnd_, GWL_EXSTYLE, WS_EX_APPWINDOW);
    }
    else
    {
        // Restore saved styles
        SetWindowLongW(hwnd_, GWL_STYLE, saved_state_.style);
        SetWindowLongW(hwnd_, GWL_EXSTYLE, saved_state_.ex_style);
    }
}

HMONITOR Win32Window::get_current_monitor() const
{
    if (!hwnd_)
        return nullptr;
        
    return MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
}

LRESULT Win32Window::handle_message(UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Handle specific messages that need special processing
    switch (msg)
    {
        case WM_NCCREATE:
        {
            // Enable non-client DPI scaling for proper title bar scaling
            Win32SystemApp::enable_non_client_dpi_scaling(hwnd_);
            break;
        }
        
        case WM_CLOSE:
        {
            // Don't destroy window, let app handle it through event
            return 0;
        }
        
        case WM_SETFOCUS:
            has_focus_ = true;
            break;
            
        case WM_KILLFOCUS:
            has_focus_ = false;
            break;
            
        case WM_ERASEBKGND:
            // Prevent flicker
            return 1;
    }
    
    return -1;  // Use default processing
}

} // namespace skr