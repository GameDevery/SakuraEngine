#pragma once
#include "system/event_loop.h"
#include "system/IME.h"
#include "window.h"

namespace skr {

struct SKR_SYSTEM_API SystemApp
{
public:
    virtual ~SystemApp() = default;

    // Window management
    virtual SystemWindow* create_window(const SystemWindowCreateInfo& create_info) = 0;
    virtual void destroy_window(SystemWindow* window) = 0;

    // Monitor/Display management
    virtual uint32_t get_monitor_count() const = 0;
    virtual SystemMonitor* get_monitor(uint32_t index) const = 0;
    virtual SystemMonitor* get_primary_monitor() const = 0;
    virtual SystemMonitor* get_monitor_from_point(int32_t x, int32_t y) const = 0;
    virtual SystemMonitor* get_monitor_from_window(SystemWindow* window) const = 0;
    
    // Monitor change notifications
    virtual void enumerate_monitors(void (*callback)(SystemMonitor* monitor, void* user_data), void* user_data) const = 0;
    
    // Refresh all monitor information (useful after display changes)
    virtual void refresh_monitors() = 0;
    
    // Input Method Editor access
    IME* get_ime() const { return ime; }
    
    // Factory methods
    static SystemApp* Create(const char* backend = nullptr); // nullptr = auto-detect, "SDL", "Win32", "Cocoa"
    static void Destroy(SystemApp* app);

protected:
    IME* ime = nullptr;
};

} // namespace skr