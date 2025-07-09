#pragma once
#include "SkrSystem/window_manager.h"
#include <SDL3/SDL_video.h>

namespace skr {

class SDL3Monitor;
class SDL3Window;

class SDL3WindowManager : public ISystemWindowManager
{
public:
    SDL3WindowManager() SKR_NOEXCEPT;
    ~SDL3WindowManager() SKR_NOEXCEPT override;
    
    // Monitor/Display management implementation
    uint32_t get_monitor_count() const override;
    SystemMonitor* get_monitor(uint32_t index) const override;
    SystemMonitor* get_primary_monitor() const override;
    SystemMonitor* get_monitor_from_point(int32_t x, int32_t y) const override;
    SystemMonitor* get_monitor_from_window(SystemWindow* window) const override;
    void refresh_monitors() override;
    
    // SDL3 specific helpers
    SDL3Window* get_sdl3_window(SDL_WindowID window_id) const;
    void register_sdl_window(SDL_WindowID window_id, SDL3Window* window);
    void unregister_sdl_window(SDL_WindowID window_id);
    
protected:
    // Platform implementation
    SystemWindow* create_window_internal(const SystemWindowCreateInfo& info) override;
    void destroy_window_internal(SystemWindow* window) override;
    void* get_native_handle_from_event(const SkrSystemEvent& event) const override;
    
private:
    // SDL specific window tracking
    skr::Map<SDL_WindowID, SDL3Window*> sdl_window_cache_;
    
    // Monitor tracking
    skr::Map<SDL_DisplayID, SDL3Monitor*> monitor_cache_;
    skr::Vector<SDL3Monitor*> monitor_list_;
    
    // Helper methods
    void update_monitor_cache();
    SDL3Monitor* create_monitor(SDL_DisplayID display_id);
};

} // namespace skr