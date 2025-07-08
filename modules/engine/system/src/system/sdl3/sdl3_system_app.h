#pragma once
#include "SkrSystem/system.h"
#include "SkrContainers/vector.hpp"
#include "SkrContainers/map.hpp"
#include <SDL3/SDL_video.h>

namespace skr {

class SDL3Monitor;
class SDL3Window;

class SDL3SystemApp : public SystemApp
{
public:
    SDL3SystemApp() SKR_NOEXCEPT;
    ~SDL3SystemApp() SKR_NOEXCEPT override;

    // Window management
    SystemWindow* create_window(const SystemWindowCreateInfo& create_info) override;
    void destroy_window(SystemWindow* window) override;

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
    ISystemEventSource* get_platform_event_source() const override { return platform_event_source_; }
    bool wait_events(uint32_t timeout_ms) override;

private:
    skr::Map<SDL_DisplayID, SDL3Monitor*> monitor_cache;
    skr::Vector<SDL3Monitor*> monitor_list;
    skr::Map<SDL_WindowID, SDL3Window*> window_cache;
    
    // Event system
    SDL3EventSource* platform_event_source_ = nullptr;
};

} // namespace skr