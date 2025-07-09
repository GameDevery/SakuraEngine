#pragma once
#include "SkrSystem/system.h"
#include <SDL3/SDL.h>

namespace skr {

class SDL3WindowManager;
class SDL3IME;

class SDL3EventSource : public ISystemEventSource
{
public:
    SDL3EventSource(SystemApp* app) SKR_NOEXCEPT;
    ~SDL3EventSource() SKR_NOEXCEPT override;

    bool poll_event(SkrSystemEvent& event) SKR_NOEXCEPT override;
    bool wait_events(uint32_t timeout_ms) SKR_NOEXCEPT override;
    
    // Set IME for event forwarding
    void set_ime(SDL3IME* ime) { ime_ = ime; }

private:
    SystemApp* app_ = nullptr;
    SDL3IME* ime_ = nullptr;
    
    // Helper functions
    SkrSystemEvent translate_sdl_event(const SDL_Event& sdl_event) const;
    bool should_forward_to_ime(const SDL_Event& sdl_event) const;
};

} // namespace skr