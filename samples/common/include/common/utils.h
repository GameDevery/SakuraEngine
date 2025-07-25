#pragma once
#include "SkrBase/config.h"
#include "SkrGraphics/api.h"
#include "stdbool.h"
#include "stdio.h"
#include "stdlib.h"
#ifdef TARGET_MACOS
    #include "SkrCore/platform/apple/macos/window.h"
#endif
#include "SkrGraphics/api.h"
#include <SDL3/SDL.h>

#define BACK_BUFFER_WIDTH 900
#define BACK_BUFFER_HEIGHT 900

inline static bool SDLEventHandler(const SDL_Event* event, SDL_Window* window)
{
    switch (event->type)
    {
    case SDL_EVENT_WINDOW_RESIZED: {
        const int32_t ResizeWidth  = event->window.data1;
        const int32_t ResizeHeight = event->window.data2;
        (void)ResizeWidth;
        (void)ResizeHeight;
    }
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        return false;
    default:
        return true;
    }
}

inline static void* SDLGetNativeWindowHandle(SDL_Window* wnd)
{
    SDL_PropertiesID prop_id = SDL_GetWindowProperties(wnd);
#if SKR_PLAT_WINDOWS
    return SDL_GetPointerProperty(prop_id, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#elif SKR_PLAT_MACOSX
    return SDL_GetPointerProperty(prop_id, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL);
#else
    return NULL;
#endif
}

inline static void read_bytes(const char8_t* file_name, uint32_t** bytes, uint32_t* length)
{
    FILE* f = fopen((const char*)file_name, "rb");
    fseek(f, 0, SEEK_END);
    *length = ftell(f);
    fseek(f, 0, SEEK_SET);
    *bytes = (uint32_t*)malloc(*length);
    fread(*bytes, *length, 1, f);
    fclose(f);
}

inline static void read_shader_bytes(const char8_t* virtual_path, uint32_t** bytes, uint32_t* length, ECGPUBackend backend)
{
    char8_t        shader_file[256];
    const char8_t* shader_path = SKR_UTF8("./../resources/shaders/");
    strcpy((char*)shader_file, (const char*)shader_path);
    strcat((char*)shader_file, (const char*)virtual_path);
    switch (backend)
    {
    case CGPU_BACKEND_VULKAN:
        strcat((char*)shader_file, (const char*)SKR_UTF8(".spv"));
        break;
    case CGPU_BACKEND_D3D12:
    case CGPU_BACKEND_XBOX_D3D12:
        strcat((char*)shader_file, (const char*)SKR_UTF8(".dxil"));
        break;
    default:
        break;
    }
    read_bytes(shader_file, bytes, length);
}

#ifdef __cplusplus
    #include <type_traits>

template <typename Pipeline>
inline static void free_pipeline_and_signature(Pipeline* pipeline)
{
    auto sig = pipeline->root_signature;
    if constexpr (std::is_same_v<Pipeline*, CGPURenderPipelineId>)
    {
        cgpu_free_render_pipeline(pipeline);
    }
    else
    {
        cgpu_free_compute_pipeline(pipeline);
    }
    cgpu_free_root_signature(sig);
}
#endif