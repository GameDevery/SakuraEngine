#include "SkrSystem/system_app.h"
#include "sdl3_event_source.h"
#include "sdl3_ime.h"
#include "SkrCore/memory/memory.h"

namespace skr {

// Factory function to create SDL3 event source
ISystemEventSource* CreateSDL3EventSource(SystemApp* app)
{
    return SkrNew<SDL3EventSource>(app);
}

// Helper to connect SDL3 components
void ConnectSDL3Components(ISystemWindowManager* window_manager, ISystemEventSource* event_source, IME* ime)
{
    // We know the types since we just created them
    if (auto* sdl3_source = static_cast<SDL3EventSource*>(event_source))
    {
        if (auto* sdl3_ime = static_cast<SDL3IME*>(ime))
        {
            sdl3_source->set_ime(sdl3_ime);
        }
    }
}

} // namespace skr