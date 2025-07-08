#pragma once
#include "SkrSystem/event.h"
#include "SkrSystem/window.h"
#include "SkrSystem/IME.h"
#include "SkrSystem/window_manager.h"
#include "SkrContainers/function_ref.hpp"

namespace skr {

// Forward declarations
struct IME;
struct ISystemEventHandler;

// Event source interface
struct SKR_SYSTEM_API ISystemEventSource
{
    virtual ~ISystemEventSource() SKR_NOEXCEPT;
    virtual bool poll_event(SkrSystemEvent& event) SKR_NOEXCEPT = 0;
};

// Event handler interface
struct SKR_SYSTEM_API ISystemEventHandler
{
    virtual ~ISystemEventHandler() SKR_NOEXCEPT;
    virtual void handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT = 0;
};

// Event queue implementation
struct SKR_SYSTEM_API SystemEventQueue
{
    SystemEventQueue() SKR_NOEXCEPT;
    ~SystemEventQueue() SKR_NOEXCEPT;

    // Non-copyable
    SystemEventQueue(const SystemEventQueue&) = delete;
    SystemEventQueue& operator=(const SystemEventQueue&) = delete;
    
    // Movable
    SystemEventQueue(SystemEventQueue&&) SKR_NOEXCEPT;
    SystemEventQueue& operator=(SystemEventQueue&&) SKR_NOEXCEPT;

    bool pump_messages() SKR_NOEXCEPT;
    bool add_source(ISystemEventSource* source) SKR_NOEXCEPT;
    bool remove_source(ISystemEventSource* source) SKR_NOEXCEPT;
    bool add_handler(ISystemEventHandler* handler) SKR_NOEXCEPT;
    bool remove_handler(ISystemEventHandler* handler) SKR_NOEXCEPT;
    
    void clear_sources() SKR_NOEXCEPT;
    void clear_handlers() SKR_NOEXCEPT;

private:
    struct Impl;
    Impl* pimpl;
};

struct SKR_SYSTEM_API SystemApp
{
public:
    virtual ~SystemApp() = default;

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
    
    // Window Manager access
    WindowManager* get_window_manager() const { return window_manager; }
    
    // Event system access
    virtual SystemEventQueue* get_event_queue() const = 0;
    
    // Event source management
    virtual bool add_event_source(ISystemEventSource* source) = 0;
    virtual bool remove_event_source(ISystemEventSource* source) = 0;
    virtual ISystemEventSource* get_platform_event_source() const = 0; // Platform-specific source (SDL, Win32, etc)
    
    // Wait for events (blocking)
    virtual bool wait_events(uint32_t timeout_ms = 0) = 0; // 0 = infinite wait
    
    // Factory methods
    static SystemApp* Create(const char* backend = nullptr); // nullptr = auto-detect, "SDL", "Win32", "Cocoa"
    static void Destroy(SystemApp* app);

protected:
    // Internal window management - only accessible by WindowManager
    virtual SystemWindow* create_window_internal(const SystemWindowCreateInfo& create_info) = 0;
    virtual void destroy_window_internal(SystemWindow* window) = 0;
    
    IME* ime = nullptr;
    WindowManager* window_manager = nullptr;
    SystemEventQueue* event_queue = nullptr;
    
    friend class WindowManager;
};

} // namespace skr