#include "win32_event_source.h"
#include "win32_window.h"
#include "win32_window_manager.h"
#include "win32_monitor.h"
#include "SkrCore/log.h"
#include <Dwmapi.h>
#include <VersionHelpers.h>

#pragma comment(lib, "Dwmapi.lib")
#pragma comment(lib, "UxTheme.lib")

// DWM window attributes for dark mode
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE
#define DWMWA_USE_IMMERSIVE_DARK_MODE 20
#endif

// Windows 10 1903+ (build 18362)
#ifndef DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1
#define DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1 19
#endif

// Undocumented Windows APIs for dark mode
typedef enum {
    UXTHEME_APPMODE_DEFAULT,
    UXTHEME_APPMODE_ALLOW_DARK,
    UXTHEME_APPMODE_FORCE_DARK,
    UXTHEME_APPMODE_FORCE_LIGHT,
    UXTHEME_APPMODE_MAX
} UxthemePreferredAppMode;

typedef void (WINAPI *fnSetPreferredAppMode)(UxthemePreferredAppMode appMode);
typedef bool (WINAPI *fnShouldAppsUseDarkMode)(void);
typedef bool (WINAPI *fnAllowDarkModeForWindow)(HWND hWnd, bool allow);
typedef void (WINAPI *fnRefreshImmersiveColorPolicyState)(void);
typedef bool (WINAPI *fnIsDarkModeAllowedForWindow)(HWND hWnd);

// Ordinals for Windows 10 1809
#define UXTHEME_SHOULDAPPSUSEDARKMODE_ORDINAL 132
#define UXTHEME_ALLOWDARKMODE_FORWINDOW_ORDINAL 133
#define UXTHEME_SETPREFERREDAPPMODE_ORDINAL 135
#define UXTHEME_ISDARKMODE_ALLOWED_FORWINDOW_ORDINAL 137
#define UXTHEME_REFRESHIMMERSIVECOLORPOLICYSTATE_ORDINAL 104

namespace skr {

// Static function pointers for dark mode
static fnSetPreferredAppMode g_SetPreferredAppMode = nullptr;
static fnShouldAppsUseDarkMode g_ShouldAppsUseDarkMode = nullptr;
static fnAllowDarkModeForWindow g_AllowDarkModeForWindow = nullptr;
static fnRefreshImmersiveColorPolicyState g_RefreshImmersiveColorPolicyState = nullptr;
static fnIsDarkModeAllowedForWindow g_IsDarkModeAllowedForWindow = nullptr;
static bool g_dark_mode_initialized = false;
static bool g_dark_mode_supported = false;

static void initialize_dark_mode_support()
{
    if (g_dark_mode_initialized)
        return;
        
    g_dark_mode_initialized = true;
    
    // Load uxtheme.dll
    HMODULE uxtheme = LoadLibraryW(L"uxtheme.dll");
    if (!uxtheme)
    {
        SKR_LOG_WARN(u8"Failed to load uxtheme.dll for dark mode support");
        return;
    }
    
    // Load undocumented functions by ordinal
    g_SetPreferredAppMode = (fnSetPreferredAppMode)GetProcAddress(uxtheme, MAKEINTRESOURCEA(UXTHEME_SETPREFERREDAPPMODE_ORDINAL));
    g_ShouldAppsUseDarkMode = (fnShouldAppsUseDarkMode)GetProcAddress(uxtheme, MAKEINTRESOURCEA(UXTHEME_SHOULDAPPSUSEDARKMODE_ORDINAL));
    g_AllowDarkModeForWindow = (fnAllowDarkModeForWindow)GetProcAddress(uxtheme, MAKEINTRESOURCEA(UXTHEME_ALLOWDARKMODE_FORWINDOW_ORDINAL));
    g_RefreshImmersiveColorPolicyState = (fnRefreshImmersiveColorPolicyState)GetProcAddress(uxtheme, MAKEINTRESOURCEA(UXTHEME_REFRESHIMMERSIVECOLORPOLICYSTATE_ORDINAL));
    g_IsDarkModeAllowedForWindow = (fnIsDarkModeAllowedForWindow)GetProcAddress(uxtheme, MAKEINTRESOURCEA(UXTHEME_ISDARKMODE_ALLOWED_FORWINDOW_ORDINAL));
    
    // Check if we have the minimum required functions
    if (g_SetPreferredAppMode && g_AllowDarkModeForWindow && g_RefreshImmersiveColorPolicyState)
    {
        // Enable dark mode for the app
        g_SetPreferredAppMode(UXTHEME_APPMODE_ALLOW_DARK);
        g_RefreshImmersiveColorPolicyState();
        g_dark_mode_supported = true;
        
        SKR_LOG_INFO(u8"Dark mode support initialized successfully");
    }
    else
    {
        SKR_LOG_INFO(u8"Dark mode not fully supported on this Windows version");
    }
}

static void apply_dark_mode_to_window(HWND hwnd)
{
    if (!g_dark_mode_supported || !hwnd)
        return;
        
    // Check if system is in dark mode
    bool use_dark_mode = false;
    if (g_ShouldAppsUseDarkMode)
    {
        use_dark_mode = g_ShouldAppsUseDarkMode();
    }
    
    if (!use_dark_mode)
        return;
        
    // Apply dark mode to the window
    if (g_AllowDarkModeForWindow)
    {
        g_AllowDarkModeForWindow(hwnd, true);
    }
    
    // Try to set window attribute for Windows 10 1903+
    BOOL value = TRUE;
    HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
    
    if (FAILED(hr))
    {
        // Try older attribute for Windows 10 1809-1903
        hr = DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1, &value, sizeof(value));
    }
    
    if (SUCCEEDED(hr))
    {
        SKR_LOG_DEBUG(u8"Applied dark mode to window");
    }
}

Win32Window::Win32Window(HWND hwnd, Win32EventSource* src) SKR_NOEXCEPT
    : hwnd_(hwnd), win32_event_source_(src)
{
    cache_window_info();
    
    // Apply dark mode to the window if supported
    initialize_dark_mode_support();
    apply_dark_mode_to_window(hwnd_);
    
    // Try to set black background
    SetClassLongPtrW(hwnd_, GCLP_HBRBACKGROUND, (LONG_PTR)GetStockObject(BLACK_BRUSH));
    
    // Force a repaint to apply black background
    InvalidateRect(hwnd_, nullptr, TRUE);
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
    uint32_t dpi = Win32WindowManager::get_dpi_for_window(hwnd_);
    float scale = dpi / 96.0f;
    
    uint32_t physical_width = static_cast<uint32_t>(width * scale);
    uint32_t physical_height = static_cast<uint32_t>(height * scale);
    
    // Get window style to calculate proper size including borders
    DWORD style = GetWindowLongW(hwnd_, GWL_STYLE);
    DWORD ex_style = GetWindowLongW(hwnd_, GWL_EXSTYLE);
    
    RECT rect = { 0, 0, static_cast<LONG>(physical_width), static_cast<LONG>(physical_height) };
    Win32WindowManager::adjust_window_rect_for_dpi(&rect, style, ex_style, dpi);
    
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
    uint32_t dpi = Win32WindowManager::get_dpi_for_window(hwnd_);
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
        
    uint32_t dpi = Win32WindowManager::get_dpi_for_window(hwnd_);
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
    win32_event_source_->window_messages_.enqueue({hwnd_, msg, wParam, lParam});
    switch (msg)
    {
        case WM_NCCREATE:
        {
            // Enable non-client DPI scaling for proper title bar scaling
            Win32WindowManager::enable_non_client_dpi_scaling(hwnd_);
            
            // Apply dark mode early for proper title bar theming
            initialize_dark_mode_support();
            apply_dark_mode_to_window(hwnd_);
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
        {
            // Handle the first paint to show black background
            if (first_paint_)
            {
                first_paint_ = false;
                
                PAINTSTRUCT ps;
                HDC hdc = BeginPaint(hwnd_, &ps);
                
                // Fill with black
                RECT rect;
                GetClientRect(hwnd_, &rect);
                HBRUSH blackBrush = (HBRUSH)GetStockObject(BLACK_BRUSH);
                FillRect(hdc, &rect, blackBrush);
                
                EndPaint(hwnd_, &ps);
                return 0;
            }
            break;
        }
    }
    
    return -1;  // Use default processing
}

} // namespace skr