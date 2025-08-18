#pragma once
#include "SkrSystem/window_manager.h"
#include <Windows.h>

namespace skr {

class Win32Monitor;
class Win32Window;
class Win32EventSource;

class Win32WindowManager : public ISystemWindowManager
{
public:
    Win32WindowManager() SKR_NOEXCEPT;
    ~Win32WindowManager() SKR_NOEXCEPT override;
    
    // Monitor/Display management implementation
    uint32_t get_monitor_count() const override;
    SystemMonitor* get_monitor(uint32_t index) const override;
    SystemMonitor* get_primary_monitor() const override;
    SystemMonitor* get_monitor_from_point(int32_t x, int32_t y) const override;
    SystemMonitor* get_monitor_from_window(SystemWindow* window) const override;
    void refresh_monitors() override;
    
    // Win32 specific helpers
    Win32Window* get_window_from_hwnd(HWND hwnd) const;
    void register_hwnd(HWND hwnd, Win32Window* window);
    void unregister_hwnd(HWND hwnd);
    
    // DPI helper functions (matching SDL3's behavior)
    static bool is_dpi_aware();
    static uint32_t get_dpi_for_window(HWND hwnd);
    static bool adjust_window_rect_for_dpi(RECT* rect, DWORD style, DWORD ex_style, uint32_t dpi);
    static bool enable_non_client_dpi_scaling(HWND hwnd);
    
    // Get singleton instance (for window proc)
    static Win32WindowManager* get_instance() { return instance_; }
    
    // Set event source (called by Win32SystemApp)
    void set_event_source(Win32EventSource* event_source) { event_source_ = event_source; }
    
    // Window procedure
    static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
    
protected:
    // Platform implementation
    SystemWindow* create_window_internal(const SystemWindowCreateInfo& info) override;
    void destroy_window_internal(SystemWindow* window) override;
    
private:
    // Monitor enumeration callback
    static BOOL CALLBACK enum_monitors_proc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData);
    
    // Window class management
    void register_window_class();
    void unregister_window_class();
    
    // DPI awareness initialization
    void initialize_dpi_awareness();
    
    // Monitor tracking
    skr::Map<HMONITOR, Win32Monitor*> monitor_cache_;
    skr::Vector<Win32Monitor*> monitor_list_;
    
    // Window tracking by HWND
    skr::Map<HWND, Win32Window*> hwnd_window_cache_;
    
    // Window class atom
    ATOM window_class_atom_ = 0;
    
    // Singleton instance for window proc
    static Win32WindowManager* instance_;
    
    // Event source reference (not owned)
    Win32EventSource* event_source_ = nullptr;
    
    // Helper structure for monitor enumeration
    struct MonitorEnumData {
        Win32WindowManager* manager;
        void (*callback)(SystemMonitor*, void*);
        void* user_data;
    };
};

} // namespace skr