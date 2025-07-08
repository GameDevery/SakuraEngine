#include "SkrSystem/window_manager_handler.h"
#include "SkrCore/log.h"

namespace skr {

WindowManagerHandler::WindowManagerHandler(WindowManager* manager) SKR_NOEXCEPT
    : window_manager_(manager)
{
}

void WindowManagerHandler::handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT
{
    if (!window_manager_)
    {
        return;
    }
    
    // Handle window focus events
    switch (event.type)
    {
        case SKR_SYSTEM_EVENT_WINDOW_FOCUS_GAINED:
        {
            SystemWindow* window = window_manager_->get_window_from_event(event);
            if (window)
            {
                window_manager_->set_focused_window(window);
                SKR_LOG_DEBUG(u8"WindowManagerHandler: Window gained focus");
            }
            break;
        }
        
        case SKR_SYSTEM_EVENT_WINDOW_FOCUS_LOST:
        {
            SystemWindow* window = window_manager_->get_window_from_event(event);
            if (window && window == window_manager_->get_focused_window())
            {
                // Only clear focus if the losing window was actually focused
                window_manager_->set_focused_window(nullptr);
                SKR_LOG_DEBUG(u8"WindowManagerHandler: Window lost focus");
            }
            break;
        }
        
        default:
            // Not interested in other events
            break;
    }
}

} // namespace skr