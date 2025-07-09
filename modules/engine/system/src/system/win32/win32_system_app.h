#pragma once
#include "SkrSystem/system.h"
#include "SkrContainers/vector.hpp"
#include "SkrContainers/map.hpp"
#include <Windows.h>

namespace skr {

class Win32Monitor;
class Win32Window;
class Win32EventSource;

class Win32SystemApp : public SystemApp
{
public:
    Win32SystemApp() SKR_NOEXCEPT;
    ~Win32SystemApp() SKR_NOEXCEPT override;

    // Monitor/Display management
    uint32_t get_monitor_count() const override;
    SystemMonitor* get_monitor(uint32_t index) const override;
    SystemMonitor* get_primary_monitor() const override;
    SystemMonitor* get_monitor_from_point(int32_t x, int32_t y) const override;
    SystemMonitor* get_monitor_from_window(SystemWindow* window) const override;
    void enumerate_monitors(void (*callback)(SystemMonitor* monitor, void* user_data), void* user_data) const override;
    void refresh_monitors() override;
    
    // Event system implementation
    SystemEventQueue* get_event_queue() const override { return event_queue; }
    bool add_event_source(ISystemEventSource* source) override;
    bool remove_event_source(ISystemEventSource* source) override;
    ISystemEventSource* get_platform_event_source() const override;
    bool wait_events(uint32_t timeout_ms) override;

    // Win32 specific - get window from HWND
    Win32Window* get_window_from_hwnd(HWND hwnd) const;
    
    // Get singleton instance (for window proc)
    static Win32SystemApp* get_instance() { return instance_; }

protected:
    // Internal window management
    SystemWindow* create_window_internal(const SystemWindowCreateInfo& create_info) override;
    void destroy_window_internal(SystemWindow* window) override;

private:
    // Monitor enumeration callback
    static BOOL CALLBACK enum_monitors_proc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
    
    // Window class registration
    void register_window_class();
    void unregister_window_class();
    
    // DPI awareness initialization (following SDL3's approach)
    void initialize_dpi_awareness();
    
public:
    // DPI helper functions (matching SDL3's behavior)
    static bool is_dpi_aware();
    static uint32_t get_dpi_for_window(HWND hwnd);
    static bool adjust_window_rect_for_dpi(RECT* rect, DWORD style, DWORD ex_style, uint32_t dpi);
    static bool enable_non_client_dpi_scaling(HWND hwnd);
    
private:
    skr::Map<HMONITOR, Win32Monitor*> monitor_cache;
    skr::Vector<Win32Monitor*> monitor_list;
    skr::Map<HWND, Win32Window*> window_cache;
    
    // Event system (use base type to avoid forward declaration issues)
    Win32EventSource* win32_event_source_ = nullptr;
    
    // Window class atom
    ATOM window_class_atom_ = 0;
    
    // Singleton instance for window proc
    static Win32SystemApp* instance_;
};

} // namespace skr