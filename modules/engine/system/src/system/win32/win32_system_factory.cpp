#include "SkrSystem/system.h"
#include "win32_event_source.h"
#include "win32_window_manager.h"
#include "SkrCore/memory/memory.h"

namespace skr {

// Factory function to create Win32 event source
ISystemEventSource* CreateWin32EventSource(SystemApp* app)
{
    return SkrNew<Win32EventSource>(app);
}

// Helper to connect Win32 components
void ConnectWin32Components(ISystemWindowManager* window_manager, ISystemEventSource* event_source, IME* ime)
{
    // We know the types since we just created them
    if (auto* win32_source = static_cast<Win32EventSource*>(event_source))
    {
        win32_source->set_ime(ime);
        
        if (auto* win32_wm = static_cast<Win32WindowManager*>(window_manager))
        {
            win32_wm->set_event_source(win32_source);
        }
    }
}

} // namespace skr