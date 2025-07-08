#pragma once
#include "SkrSystem/system.h"
#include "SkrSystem/window_manager.h"

namespace skr {

// WindowManagerHandler - automatically tracks window focus changes
class SKR_SYSTEM_API WindowManagerHandler : public ISystemEventHandler
{
public:
    WindowManagerHandler(WindowManager* manager) SKR_NOEXCEPT;
    ~WindowManagerHandler() SKR_NOEXCEPT override = default;
    
    void handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT override;
    
private:
    WindowManager* window_manager_;
};

} // namespace skr