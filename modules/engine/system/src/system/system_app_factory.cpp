#include "SkrSystem/system.h"
#include "SkrCore/log.h"
#include "SkrCore/memory/memory.h"

// Platform implementations
#ifdef _WIN32
    #include "win32/win32_system_app.h"
#endif
#include "sdl3/sdl3_system_app.h"

namespace skr {

SystemApp* SystemApp::Create(const char* backend)
{
    // Auto-detect if no backend specified
    if (!backend)
    {
        #ifdef _WIN32
            // Prefer native Win32 on Windows
            backend = "Win32";
        #else
            // Use SDL3 on other platforms
            backend = "SDL3";
        #endif
    }
    
    // Win32 backend
    #ifdef _WIN32
    if (strcmp(backend, "Win32") == 0 || strcmp(backend, "Windows") == 0)
    {
        Win32SystemApp* win32_app = SkrNew<Win32SystemApp>();
        SKR_LOG_INFO(u8"Created Win32 system backend");
        return static_cast<SystemApp*>(win32_app);
    }
    #endif
    
    // SDL3 backend
    if (strcmp(backend, "SDL") == 0 || strcmp(backend, "SDL3") == 0)
    {
        SDL3SystemApp* sdl_app = SkrNew<SDL3SystemApp>();
        SKR_LOG_INFO(u8"Created SDL3 system backend");
        return static_cast<SystemApp*>(sdl_app);
    }
    
    SKR_LOG_ERROR(u8"Unknown system backend: %s", backend);
    return nullptr;
}

void SystemApp::Destroy(SystemApp* app)
{
    if (app)
    {
        SkrDelete(app);
    }
}

} // namespace skr