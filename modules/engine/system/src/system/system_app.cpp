#include "SkrSystem/system.h"
#include "SkrSystem/window_manager.h"
#include "SkrSystem/IME.h"
#include "SkrCore/log.h"
#include "SkrCore/memory/memory.h"
#include <cstring>

namespace skr {
// Platform-specific factory functions
#ifdef _WIN32
extern ISystemEventSource* CreateWin32EventSource(SystemApp* app);
extern void ConnectWin32Components(ISystemWindowManager* window_manager, ISystemEventSource* event_source, IME* ime);
#endif
extern ISystemEventSource* CreateSDL3EventSource(SystemApp* app);
extern void ConnectSDL3Components(ISystemWindowManager* window_manager, ISystemEventSource* event_source, IME* ime);
}

namespace skr {

SystemApp::SystemApp() SKR_NOEXCEPT
{

}

SystemApp::~SystemApp() SKR_NOEXCEPT
{
    shutdown();
}

bool SystemApp::initialize(const char* backend)
{
    // Create event queue
    event_queue = SkrNew<SystemEventQueue>();
    if (!event_queue)
    {
        SKR_LOG_ERROR(u8"Failed to create event queue");
        return false;
    }
    
    // Auto-detect backend if not specified
    if (!backend)
    {
#ifdef _WIN32
        backend = "Win32";
#elif defined(__APPLE__)
        backend = "Cocoa";
#else
        backend = "SDL3";
#endif
    }
    
    // Create window manager
    window_manager = ISystemWindowManager::Create(backend);
    if (!window_manager)
    {
        SKR_LOG_ERROR(u8"Failed to create window manager for backend: %s", backend);
        shutdown();
        return false;
    }
    
    // Create IME
    ime = IME::Create(backend);
    if (!ime)
    {
        SKR_LOG_ERROR(u8"Failed to create IME for backend: %s", backend);
        shutdown();
        return false;
    }
    
    // Create platform event source
#ifdef _WIN32
    if (strcmp(backend, "Win32") == 0)
    {
        platform_event_source = CreateWin32EventSource(this);
        if (platform_event_source)
        {
            ConnectWin32Components(window_manager, platform_event_source, ime);
        }
    }
#endif
    if (strcmp(backend, "SDL3") == 0 || strcmp(backend, "SDL") == 0)
    {
        // Try to create SDL3 event source
        // This will fail at link time if SDL3 is not available
        platform_event_source = CreateSDL3EventSource(this);
        if (platform_event_source)
        {
            ConnectSDL3Components(window_manager, platform_event_source, ime);
        }
    }
    else
    {
        SKR_LOG_ERROR(u8"Unknown backend: %s", backend);
        shutdown();
        return false;
    }
    
    if (!platform_event_source)
    {
        SKR_LOG_ERROR(u8"Failed to create platform event source");
        shutdown();
        return false;
    }
    
    // Add platform event source to queue
    event_queue->add_source(platform_event_source);
    
    SKR_LOG_INFO(u8"SystemApp initialized with backend: %s", backend);
    return true;
}

void SystemApp::shutdown()
{
    // Clean up event source
    if (platform_event_source && event_queue)
    {
        event_queue->remove_source(platform_event_source);
        SkrDelete(platform_event_source);
        platform_event_source = nullptr;
    }
    
    // Clean up window manager
    if (window_manager)
    {
        ISystemWindowManager::Destroy(window_manager);
        window_manager = nullptr;
    }
    
    // Clean up IME
    if (ime)
    {
        IME::Destroy(ime);
        ime = nullptr;
    }
    
    // Clean up event queue
    if (event_queue)
    {
        SkrDelete(event_queue);
        event_queue = nullptr;
    }
}

bool SystemApp::add_event_source(ISystemEventSource* source)
{
    if (event_queue && source)
    {
        return event_queue->add_source(source);
    }
    return false;
}

bool SystemApp::remove_event_source(ISystemEventSource* source)
{
    if (event_queue && source)
    {
        return event_queue->remove_source(source);
    }
    return false;
}

bool SystemApp::wait_events(uint32_t timeout_ms)
{
    if (platform_event_source)
    {
        return platform_event_source->wait_events(timeout_ms);
    }
    return false;
}

SystemApp* SystemApp::Create(const char* backend)
{
    auto* app = SkrNew<SystemApp>();
    if (!app)
    {
        return nullptr;
    }
    
    if (!app->initialize(backend))
    {
        SkrDelete(app);
        return nullptr;
    }
    
    return app;
}

void SystemApp::Destroy(SystemApp* app)
{
    if (app)
    {
        SkrDelete(app);
    }
}

} // namespace skr