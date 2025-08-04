#include "SkrSystem/IME.h"
#include "SkrCore/log.h"
#include "SkrCore/memory/memory.h"
#include <cstring>

#ifdef _WIN32
#include "win32/win32_ime.h"
#endif
#ifdef __APPLE__
#include "cocoa/cocoa_ime.h"
#endif
#include "sdl3/sdl3_ime.h"

namespace skr {

IME* IME::Create(const char* backend)
{
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
    
#ifdef _WIN32
    if (strcmp(backend, "Win32") == 0)
    {
        return SkrNew<Win32IME>();
    }
#endif
    
#ifdef __APPLE__
    if (strcmp(backend, "Cocoa") == 0)
    {
        return SkrNew<CocoaIME>();
    }
#endif
    
    if (strcmp(backend, "SDL3") == 0 || strcmp(backend, "SDL") == 0)
    {
        return SkrNew<SDL3IME>();
    }
    
    SKR_LOG_ERROR(u8"Unknown IME backend: %s", backend);
    return nullptr;
}

void IME::Destroy(IME* ime)
{
    if (ime)
    {
        SkrDelete(ime);
    }
}

} // namespace skr