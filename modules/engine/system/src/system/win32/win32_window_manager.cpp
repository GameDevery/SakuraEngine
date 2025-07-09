#include "win32_window_manager.h"
#include "win32_monitor.h"
#include "win32_window.h"
#include "SkrCore/log.h"
#include <VersionHelpers.h>
#include <ShellScalingApi.h>

#pragma comment(lib, "Shcore.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Gdi32.lib")

// DPI awareness contexts (Windows 10 version 1607+)
#ifndef DPI_AWARENESS_CONTEXT_UNAWARE
DECLARE_HANDLE(DPI_AWARENESS_CONTEXT);
#define DPI_AWARENESS_CONTEXT_UNAWARE              ((DPI_AWARENESS_CONTEXT)-1)
#define DPI_AWARENESS_CONTEXT_SYSTEM_AWARE         ((DPI_AWARENESS_CONTEXT)-2)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE    ((DPI_AWARENESS_CONTEXT)-3)
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

// Function pointer types for dynamic loading
typedef BOOL (WINAPI *PFN_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
typedef UINT (WINAPI *PFN_GetDpiForWindow)(HWND);
typedef BOOL (WINAPI *PFN_AdjustWindowRectExForDpi)(LPRECT, DWORD, BOOL, DWORD, UINT);
typedef BOOL (WINAPI *PFN_EnableNonClientDpiScaling)(HWND);

namespace skr {

Win32WindowManager* Win32WindowManager::instance_ = nullptr;

// Window class name
static const wchar_t* kWindowClassName = L"SakuraEngineWindow";

// Dynamic loaded DPI functions
static PFN_SetProcessDpiAwarenessContext g_SetProcessDpiAwarenessContext = nullptr;
static PFN_GetDpiForWindow g_GetDpiForWindow = nullptr;
static PFN_AdjustWindowRectExForDpi g_AdjustWindowRectExForDpi = nullptr;
static PFN_EnableNonClientDpiScaling g_EnableNonClientDpiScaling = nullptr;

Win32WindowManager::Win32WindowManager() SKR_NOEXCEPT
{
    instance_ = this;
    
    // Initialize DPI awareness (following SDL3's approach)
    initialize_dpi_awareness();
    
    // Register window class
    register_window_class();
    
    // Cache all monitors
    refresh_monitors();
    
    SKR_LOG_DEBUG(u8"Win32WindowManager created");
}

Win32WindowManager::~Win32WindowManager() SKR_NOEXCEPT
{
    instance_ = nullptr;
    
    // Clean up all windows first
    destroy_all_windows();
    
    // Clean up cached windows
    for (auto& pair : hwnd_window_cache_) {
        SkrDelete(pair.value);
    }
    hwnd_window_cache_.clear();
    
    // Clean up cached monitors
    for (auto& pair : monitor_cache_) {
        SkrDelete(pair.value);
    }
    monitor_cache_.clear();
    monitor_list_.clear();
    
    // Unregister window class
    unregister_window_class();
    
    SKR_LOG_DEBUG(u8"Win32WindowManager destroyed");
}

void Win32WindowManager::register_window_class()
{
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc = window_proc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszClassName = kWindowClassName;
    wc.hIconSm = wc.hIcon;
    
    window_class_atom_ = RegisterClassExW(&wc);
    if (!window_class_atom_)
    {
        SKR_LOG_ERROR(u8"Failed to register window class: %d", GetLastError());
    }
}

void Win32WindowManager::unregister_window_class()
{
    if (window_class_atom_)
    {
        UnregisterClassW(kWindowClassName, GetModuleHandleW(nullptr));
        window_class_atom_ = 0;
    }
}

// Window procedure
LRESULT CALLBACK Win32WindowManager::window_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    // Handle creation
    if (msg == WM_NCCREATE)
    {
        // Store window pointer
        CREATESTRUCTW* cs = reinterpret_cast<CREATESTRUCTW*>(lParam);
        Win32Window* window = reinterpret_cast<Win32Window*>(cs->lpCreateParams);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    }
    
    // Get window pointer
    Win32Window* window = reinterpret_cast<Win32Window*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    
    // Let window handle the message first
    if (window)
    {
        LRESULT result = window->handle_message(msg, wParam, lParam);
        if (result != -1)
            return result;
    }
    
    // Default handling
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

// Monitor enumeration callback
BOOL CALLBACK Win32WindowManager::enum_monitors_proc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    Win32WindowManager* manager = reinterpret_cast<Win32WindowManager*>(dwData);
    
    Win32Monitor* monitor = SkrNew<Win32Monitor>(hMonitor);
    if (monitor)
    {
        manager->monitor_cache_.add(hMonitor, monitor);
        manager->monitor_list_.add(monitor);
    }
    
    return TRUE;  // Continue enumeration
}

// Monitor/Display management
uint32_t Win32WindowManager::get_monitor_count() const
{
    return (uint32_t)monitor_list_.size();
}

SystemMonitor* Win32WindowManager::get_monitor(uint32_t index) const
{
    if (index < monitor_list_.size())
    {
        return monitor_list_[index];
    }
    return nullptr;
}

SystemMonitor* Win32WindowManager::get_primary_monitor() const
{
    const POINT origin = { 0, 0 };
    HMONITOR primary = MonitorFromPoint(origin, MONITOR_DEFAULTTOPRIMARY);
    
    if (auto found = monitor_cache_.find(primary))
    {
        return found.value();
    }
    
    return monitor_list_.is_empty() ? nullptr : monitor_list_[0];
}

SystemMonitor* Win32WindowManager::get_monitor_from_point(int32_t x, int32_t y) const
{
    POINT pt = { x, y };
    HMONITOR hMonitor = MonitorFromPoint(pt, MONITOR_DEFAULTTONULL);
    
    if (hMonitor && monitor_cache_.find(hMonitor))
    {
        return monitor_cache_.find(hMonitor).value();
    }
    
    return nullptr;
}

SystemMonitor* Win32WindowManager::get_monitor_from_window(SystemWindow* window) const
{
    if (!window)
        return nullptr;
        
    HWND hwnd = static_cast<HWND>(window->get_native_handle());
    if (!hwnd)
        return nullptr;
        
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONULL);
    if (!hMonitor)
        return nullptr;
        
    if (auto found = monitor_cache_.find(hMonitor))
    {
        return found.value();
    }
    
    return nullptr;
}

void Win32WindowManager::refresh_monitors()
{
    // Clear existing monitors
    for (auto& pair : monitor_cache_)
    {
        SkrDelete(pair.value);
    }
    monitor_cache_.clear();
    monitor_list_.clear();
    
    // Enumerate all monitors
    EnumDisplayMonitors(nullptr, nullptr, enum_monitors_proc, reinterpret_cast<LPARAM>(this));
    
    SKR_LOG_INFO(u8"Refreshed monitors: found %d display(s)", monitor_list_.size());
}

// Win32 specific helpers
Win32Window* Win32WindowManager::get_window_from_hwnd(HWND hwnd) const
{
    if (auto found = hwnd_window_cache_.find(hwnd))
    {
        return found.value();
    }
    return nullptr;
}

void Win32WindowManager::register_hwnd(HWND hwnd, Win32Window* window)
{
    hwnd_window_cache_.add(hwnd, window);
}

void Win32WindowManager::unregister_hwnd(HWND hwnd)
{
    hwnd_window_cache_.remove(hwnd);
}

// Platform implementation
SystemWindow* Win32WindowManager::create_window_internal(const SystemWindowCreateInfo& create_info)
{
    // Validate input
    if (create_info.size.x == 0 || create_info.size.y == 0)
    {
        SKR_LOG_ERROR(u8"Invalid window size: %dx%d", create_info.size.x, create_info.size.y);
        return nullptr;
    }
    
    // Window styles
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD ex_style = WS_EX_APPWINDOW;
    
    if (create_info.is_borderless)
    {
        style = WS_POPUP;
        ex_style = WS_EX_APPWINDOW;
    }
    
    if (!create_info.is_resizable)
    {
        style &= ~(WS_SIZEBOX | WS_MAXIMIZEBOX);
    }
    
    if (create_info.is_topmost)
    {
        ex_style |= WS_EX_TOPMOST;
    }
    
    if (create_info.is_tooltip)
    {
        style = WS_POPUP;
        ex_style = WS_EX_TOOLWINDOW | WS_EX_TOPMOST | WS_EX_NOACTIVATE;
    }
    
    // Get DPI for the target monitor (or primary if position not specified)
    uint32_t dpi = 96;  // Default DPI
    if (create_info.pos.has_value())
    {
        POINT pt = { static_cast<LONG>(create_info.pos.value().x), static_cast<LONG>(create_info.pos.value().y) };
        HMONITOR monitor = MonitorFromPoint(pt, MONITOR_DEFAULTTOPRIMARY);
        
        if (monitor && IsWindows8Point1OrGreater())
        {
            UINT dpi_x = 96, dpi_y = 96;
            GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
            dpi = dpi_x;
        }
    }
    else
    {
        // Use primary monitor DPI
        dpi = get_dpi_for_window(nullptr);
    }
    
    // Convert logical size to physical pixels
    float scale = dpi / 96.0f;
    uint32_t physical_width = static_cast<uint32_t>(create_info.size.x * scale);
    uint32_t physical_height = static_cast<uint32_t>(create_info.size.y * scale);
    
    // Calculate window rect with DPI awareness
    RECT rect = { 0, 0, static_cast<LONG>(physical_width), static_cast<LONG>(physical_height) };
    adjust_window_rect_for_dpi(&rect, style, ex_style, dpi);
    
    // Position
    int x = CW_USEDEFAULT;
    int y = CW_USEDEFAULT;
    if (create_info.pos.has_value())
    {
        x = create_info.pos.value().x;
        y = create_info.pos.value().y;
    }
    
    // Convert title to wide string
    int title_len = MultiByteToWideChar(CP_UTF8, 0, (const char*)create_info.title.c_str(), -1, nullptr, 0);
    skr::Vector<wchar_t> wide_title;
    wide_title.resize_default(title_len);
    MultiByteToWideChar(CP_UTF8, 0, (const char*)create_info.title.c_str(), -1, wide_title.data(), title_len);
    
    // Create window - pass window pointer as param
    Win32Window* window_wrapper = nullptr;
    HWND hwnd = CreateWindowExW(
        ex_style,
        kWindowClassName,
        wide_title.data(),
        style,
        x, y,
        rect.right - rect.left,
        rect.bottom - rect.top,
        nullptr,
        nullptr,
        GetModuleHandleW(nullptr),
        window_wrapper  // Will be set in WM_NCCREATE
    );
    
    if (!hwnd)
    {
        SKR_LOG_ERROR(u8"Failed to create window: %d", GetLastError());
        return nullptr;
    }
    
    // Create wrapper
    Win32Window* window = SkrNew<Win32Window>(hwnd, event_source_);
    if (!window)
    {
        DestroyWindow(hwnd);
        SKR_LOG_ERROR(u8"Failed to allocate Win32Window wrapper");
        return nullptr;
    }
    
    // Update window pointer
    SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(window));
    
    // Cache window
    register_hwnd(hwnd, window);
    
    return window;
}

void Win32WindowManager::destroy_window_internal(SystemWindow* window)
{
    if (!window)
        return;
        
    Win32Window* win32_window = static_cast<Win32Window*>(window);
    HWND hwnd = win32_window->get_hwnd();
    
    // Remove from cache
    unregister_hwnd(hwnd);
    
    // Delete window (destructor handles DestroyWindow)
    SkrDelete(win32_window);
}

void* Win32WindowManager::get_native_handle_from_event(const SkrSystemEvent& event) const
{
    void* native_handle = nullptr;
    
    // Extract native handle based on event type
    switch (event.type) {
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
    
    return native_handle;
}

void Win32WindowManager::initialize_dpi_awareness()
{
    // Dynamic load DPI functions
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32)
    {
        g_SetProcessDpiAwarenessContext = (PFN_SetProcessDpiAwarenessContext)GetProcAddress(user32, "SetProcessDpiAwarenessContext");
        g_GetDpiForWindow = (PFN_GetDpiForWindow)GetProcAddress(user32, "GetDpiForWindow");
        g_AdjustWindowRectExForDpi = (PFN_AdjustWindowRectExForDpi)GetProcAddress(user32, "AdjustWindowRectExForDpi");
        g_EnableNonClientDpiScaling = (PFN_EnableNonClientDpiScaling)GetProcAddress(user32, "EnableNonClientDpiScaling");
    }
    
    // Try to set DPI awareness (following SDL3's approach)
    bool dpi_set = false;
    
    // First try SetProcessDpiAwarenessContext for Per-Monitor V2 (Windows 10 1607+)
    if (g_SetProcessDpiAwarenessContext)
    {
        // Try Per-Monitor V2 first (best option)
        if (g_SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
        {
            SKR_LOG_INFO(u8"Set DPI awareness to Per-Monitor V2");
            dpi_set = true;
        }
        // Fall back to Per-Monitor V1
        else if (g_SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE))
        {
            SKR_LOG_INFO(u8"Set DPI awareness to Per-Monitor");
            dpi_set = true;
        }
    }
    
    // Fall back to SetProcessDpiAwareness (Windows 8.1+)
    if (!dpi_set && IsWindows8Point1OrGreater())
    {
        HMODULE shcore = LoadLibraryW(L"Shcore.dll");
        if (shcore)
        {
            typedef HRESULT(WINAPI *PFN_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);
            PFN_SetProcessDpiAwareness pSetProcessDpiAwareness = 
                (PFN_SetProcessDpiAwareness)GetProcAddress(shcore, "SetProcessDpiAwareness");
            
            if (pSetProcessDpiAwareness)
            {
                HRESULT hr = pSetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
                if (SUCCEEDED(hr))
                {
                    SKR_LOG_INFO(u8"Set DPI awareness via SetProcessDpiAwareness");
                    dpi_set = true;
                }
            }
            FreeLibrary(shcore);
        }
    }
    
    // Final fallback to SetProcessDPIAware (Vista+)
    if (!dpi_set)
    {
        if (SetProcessDPIAware())
        {
            SKR_LOG_INFO(u8"Set DPI awareness via SetProcessDPIAware (legacy)");
            dpi_set = true;
        }
    }
    
    if (!dpi_set)
    {
        SKR_LOG_WARN(u8"Failed to set DPI awareness - application may be blurry on high-DPI displays");
    }
}

bool Win32WindowManager::is_dpi_aware()
{
    // Check if we have DPI awareness set
    if (g_GetDpiForWindow)
    {
        // If we can get DPI for window, we're DPI aware
        return true;
    }
    
    // Legacy check
    HDC hdc = GetDC(nullptr);
    int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
    ReleaseDC(nullptr, hdc);
    
    return dpi != 96;  // 96 is the default DPI for non-aware apps
}

uint32_t Win32WindowManager::get_dpi_for_window(HWND hwnd)
{
    // Use GetDpiForWindow if available (Windows 10 1607+)
    if (g_GetDpiForWindow && hwnd)
    {
        return g_GetDpiForWindow(hwnd);
    }
    
    // Fall back to monitor DPI
    HMONITOR monitor = MonitorFromWindow(hwnd ? hwnd : GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
    UINT dpi_x = 96, dpi_y = 96;
    
    // Try GetDpiForMonitor (Windows 8.1+)
    if (IsWindows8Point1OrGreater())
    {
        HMODULE shcore = GetModuleHandleW(L"Shcore.dll");
        if (shcore)
        {
            typedef HRESULT(WINAPI *PFN_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
            PFN_GetDpiForMonitor pGetDpiForMonitor = 
                (PFN_GetDpiForMonitor)GetProcAddress(shcore, "GetDpiForMonitor");
            
            if (pGetDpiForMonitor)
            {
                pGetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &dpi_x, &dpi_y);
            }
        }
    }
    else
    {
        // Legacy: Get system DPI
        HDC hdc = GetDC(nullptr);
        dpi_x = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
    }
    
    return dpi_x;
}

bool Win32WindowManager::adjust_window_rect_for_dpi(RECT* rect, DWORD style, DWORD ex_style, uint32_t dpi)
{
    // Use AdjustWindowRectExForDpi if available (Windows 10 1607+)
    if (g_AdjustWindowRectExForDpi)
    {
        return g_AdjustWindowRectExForDpi(rect, style, FALSE, ex_style, dpi) != FALSE;
    }
    
    // Fall back to regular AdjustWindowRectEx
    return AdjustWindowRectEx(rect, style, FALSE, ex_style) != FALSE;
}

bool Win32WindowManager::enable_non_client_dpi_scaling(HWND hwnd)
{
    // Use EnableNonClientDpiScaling if available (Windows 10 1607+)
    if (g_EnableNonClientDpiScaling && hwnd)
    {
        return g_EnableNonClientDpiScaling(hwnd) != FALSE;
    }
    
    // Not available on older Windows versions
    return false;
}

// Factory function for WindowManager
ISystemWindowManager* CreateWin32WindowManager()
{
    return static_cast<ISystemWindowManager*>(SkrNew<Win32WindowManager>());
}

} // namespace skr