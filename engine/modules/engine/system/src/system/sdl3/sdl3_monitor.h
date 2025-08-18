#pragma once
#include "SkrSystem/window.h"
#include <SDL3/SDL_video.h>

namespace skr {

class SDL3Monitor : public SystemMonitor
{
public:
    SDL3Monitor(SDL_DisplayID display_id) SKR_NOEXCEPT;
    ~SDL3Monitor() SKR_NOEXCEPT override;

    // SystemMonitor interface
    SystemMonitorInfo get_info() const override;
    uint32_t get_display_mode_count() const override;
    SystemMonitorDisplayMode get_display_mode(uint32_t index) const override;
    SystemMonitorDisplayMode get_current_display_mode() const override;
    
    // SDL3 specific
    SDL_DisplayID get_display_id() const { return display_id; }

private:
    SDL_DisplayID display_id;
};

} // namespace skr