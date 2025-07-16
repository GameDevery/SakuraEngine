#include "cocoa_system_factory.h"
#include "cocoa_event_source.h"
#include "cocoa_window_manager.h"
#include "cocoa_ime.h"
#include "SkrSystem/system_app.h"
#include "SkrCore/memory/memory.h"

namespace skr {

ISystemEventSource* CreateCocoaEventSource(SystemApp* app)
{
    return SkrNew<CocoaEventSource>(app);
}

void ConnectCocoaComponents(ISystemWindowManager* window_manager, ISystemEventSource* event_source, IME* ime)
{
    // Connect event source with IME for text event processing
    if (event_source && ime) {
        CocoaEventSource* cocoa_event_source = static_cast<CocoaEventSource*>(event_source);
        CocoaIME* cocoa_ime = static_cast<CocoaIME*>(ime);
        
        cocoa_event_source->set_ime(cocoa_ime);
        cocoa_ime->set_event_source(cocoa_event_source);
    }
    
    // Connect event source with window manager
    if (window_manager && event_source) {
        CocoaWindowManager* cocoa_window_manager = static_cast<CocoaWindowManager*>(window_manager);
        CocoaEventSource* cocoa_event_source = static_cast<CocoaEventSource*>(event_source);
        
        cocoa_window_manager->connect_event_source(cocoa_event_source);
    }
}

} // namespace skr