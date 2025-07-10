#pragma once
#include "SkrSystem/event.h"
#include "SkrSystem/IME.h"
#include "SkrSystem/window_manager.h"

namespace skr {

// Forward declarations
struct IME;

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
    SystemApp() SKR_NOEXCEPT;
    ~SystemApp() SKR_NOEXCEPT;
    
    // Non-copyable
    SystemApp(const SystemApp&) = delete;
    SystemApp& operator=(const SystemApp&) = delete;
    
    // Input Method Editor access
    IME* get_ime() const { return ime; }
    
    // Window Manager access
    ISystemWindowManager* get_window_manager() const { return window_manager; }
    
    // Event system access
    SystemEventQueue* get_event_queue() const { return event_queue; }
    
    // Event source management
    bool add_event_source(ISystemEventSource* source);
    bool remove_event_source(ISystemEventSource* source);
    ISystemEventSource* get_platform_event_source() const { return platform_event_source; }
    
    // Wait for events (blocking) - delegates to platform event source
    bool wait_events(uint32_t timeout_ms = 0);
    
    // Factory methods
    static SystemApp* Create(const char* backend = nullptr); // nullptr = auto-detect, "SDL", "Win32", "Cocoa"
    static void Destroy(SystemApp* app);

private:
    bool initialize(const char* backend);
    void shutdown();

private:
    IME* ime = nullptr;
    ISystemWindowManager* window_manager = nullptr;
    SystemEventQueue* event_queue = nullptr;
    ISystemEventSource* platform_event_source = nullptr;
};

} // namespace skr