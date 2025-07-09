#pragma once
#include "SkrSystem/window.h"
#include "SkrSystem/event.h"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/map.hpp"

namespace skr {

// System window manager base class - provides common window and monitor management
struct SKR_SYSTEM_API ISystemWindowManager
{
public:
    ISystemWindowManager() SKR_NOEXCEPT;
    virtual ~ISystemWindowManager() SKR_NOEXCEPT;
    
    // Window management
    SystemWindow* create_window(const SystemWindowCreateInfo& info) SKR_NOEXCEPT;
    void destroy_window(SystemWindow* window) SKR_NOEXCEPT;
    void destroy_all_windows() SKR_NOEXCEPT;
    
    // Window lookup
    SystemWindow* get_window_by_native_handle(void* native_handle) const SKR_NOEXCEPT;
    SystemWindow* get_window_from_event(const SkrSystemEvent& event) const SKR_NOEXCEPT;
    
    // Window enumeration
    void get_all_windows(skr::Vector<SystemWindow*>& out_windows) const SKR_NOEXCEPT;
    uint32_t get_window_count() const SKR_NOEXCEPT { return window_data_.size(); }
    
    // Monitor/Display management - must be implemented by platform
    virtual uint32_t get_monitor_count() const = 0;
    virtual SystemMonitor* get_monitor(uint32_t index) const = 0;
    virtual SystemMonitor* get_primary_monitor() const = 0;
    virtual SystemMonitor* get_monitor_from_point(int32_t x, int32_t y) const = 0;
    virtual SystemMonitor* get_monitor_from_window(SystemWindow* window) const = 0;
    virtual void refresh_monitors() = 0;

    // Monitor enumeration with default implementation
    virtual void enumerate_monitors(void (*callback)(SystemMonitor* monitor, void* user_data), void* user_data) const;
    
    // Factory method
    static ISystemWindowManager* Create(const char* backend = nullptr);
    static void Destroy(ISystemWindowManager* manager);
    
protected:
    // Platform implementations must implement these
    virtual SystemWindow* create_window_internal(const SystemWindowCreateInfo& info) = 0;
    virtual void destroy_window_internal(SystemWindow* window) = 0;
    
    // Platform implementations should implement native handle extraction
    virtual void* get_native_handle_from_event(const SkrSystemEvent& event) const { return nullptr; }
    
    // Window tracking helpers for subclasses
    void register_window(SystemWindow* window) SKR_NOEXCEPT;
    void unregister_window(SystemWindow* window) SKR_NOEXCEPT;
    
private:
    // Common data management
    skr::Map<void*, SystemWindow*> window_data_;  // native_handle -> ManagedWindow
};

} // namespace skr