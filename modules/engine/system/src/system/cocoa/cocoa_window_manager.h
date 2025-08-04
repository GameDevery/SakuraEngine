#pragma once
#include "SkrSystem/window_manager.h"
#include "SkrContainers/map.hpp"
#include "SkrContainers/vector.hpp"

#ifdef __OBJC__
#import <Cocoa/Cocoa.h>
#else
// Forward declarations for C++
typedef struct objc_object NSWindow;
typedef struct objc_object NSScreen;
#endif

class CocoaWindow;
class CocoaMonitor;
class CocoaEventSource;

class CocoaWindowManager : public skr::ISystemWindowManager {
public:
    CocoaWindowManager() SKR_NOEXCEPT;
    ~CocoaWindowManager() SKR_NOEXCEPT override;

    // ISystemWindowManager implementation
    uint32_t get_monitor_count() const SKR_NOEXCEPT override;
    skr::SystemMonitor* get_monitor(uint32_t index) const SKR_NOEXCEPT override;
    skr::SystemMonitor* get_primary_monitor() const SKR_NOEXCEPT override;
    skr::SystemMonitor* get_monitor_from_point(int32_t x, int32_t y) const SKR_NOEXCEPT override;
    skr::SystemMonitor* get_monitor_from_window(skr::SystemWindow* window) const SKR_NOEXCEPT override;
    void refresh_monitors() SKR_NOEXCEPT override;

    // Factory function
    static CocoaWindowManager* instance;
    
    // Connect event source to all windows
    void connect_event_source(CocoaEventSource* source) SKR_NOEXCEPT;

protected:
    skr::SystemWindow* create_window_internal(const skr::SystemWindowCreateInfo& info) SKR_NOEXCEPT override;
    void destroy_window_internal(skr::SystemWindow* window) SKR_NOEXCEPT override;

private:
    void initialize_monitors() SKR_NOEXCEPT;
    NSScreen* get_nsscreen_from_monitor(skr::SystemMonitor* monitor) const SKR_NOEXCEPT;

    skr::Vector<CocoaMonitor*> monitors_;
    skr::Map<NSWindow*, CocoaWindow*> window_map_;  // For fast lookups by NSWindow
    CocoaEventSource* event_source_ = nullptr;  // Connected event source
};