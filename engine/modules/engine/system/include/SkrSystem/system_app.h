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
    
    // Lifecycle management
    virtual bool initialize(const char* backend);
    virtual void shutdown();

    // System components
    SystemEventQueue* get_event_queue() const { return event_queue; }
    ISystemWindowManager* get_window_manager() const { return window_manager; }
    IME* get_ime() const { return ime; }
    
    // Event source management
    ISystemEventSource* get_platform_event_source() const { return platform_event_source; }
    
    // Wait for events (blocking) - delegates to platform event source
    bool wait_events(uint32_t timeout_ms = 0);

protected:
    IME* ime = nullptr;
    ISystemWindowManager* window_manager = nullptr;
    SystemEventQueue* event_queue = nullptr;
    ISystemEventSource* platform_event_source = nullptr;
};

} // namespace skr