#include <SkrImGui/imgui_backend.hpp>
#include <SkrImGui/imgui_render_backend.hpp>
#include <SDL3/SDL.h>
#include <SkrCore/log.hpp>
#include "./private/imgui_impl_sdl3.h"

namespace skr
{
// ctor & dtor
ImGuiBackend::ImGuiBackend()
{
}
ImGuiBackend::~ImGuiBackend()
{
    if (is_attached())
    {
        destroy();
    }

    if (has_main_window())
    {
        destroy_main_window();
    }
}

// imgui context
void ImGuiBackend::apply_context()
{
    SKR_ASSERT(is_attached() && "please attach context before apply");
    SKR_ASSERT(!_renderer_backend.is_empty() && "please set render backend before apply");

    ImGui::SetCurrentContext(_context);
}
void ImGuiBackend::attach(ImGuiContext* context)
{
    // set context
    SKR_ASSERT(!is_attached() && "please detach context before attach");
    _context = context;

    // check main window status
    SKR_ASSERT(has_main_window() && "please create main window before attach");

    // init platform backend
    {
        ImGuiContext* cache = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(context);
        ImGui_ImplSDL3_InitForOther(reinterpret_cast<SDL_Window*>(_main_window.payload()));
        ImGui::SetCurrentContext(cache);
    }

    // init render backend
    {
        _context->IO.BackendRendererUserData = _renderer_backend.get();
        _context->IO.BackendRendererName     = "Sakura ImGui Renderer";

        _context->PlatformIO.Renderer_CreateWindow = +[](ImGuiViewport* vp) {
            auto backend = static_cast<ImGuiRendererBackend*>(
                ImGui::GetCurrentContext()->IO.BackendRendererUserData
            );
            backend->create_window(vp);
        };
        _context->PlatformIO.Renderer_DestroyWindow = +[](ImGuiViewport* vp) {
            auto backend = static_cast<ImGuiRendererBackend*>(
                ImGui::GetCurrentContext()->IO.BackendRendererUserData
            );
            backend->destroy_window(vp);
        };
        _context->PlatformIO.Renderer_SetWindowSize = +[](ImGuiViewport* vp, ImVec2 size) {
            auto backend = static_cast<ImGuiRendererBackend*>(
                ImGui::GetCurrentContext()->IO.BackendRendererUserData
            );
            backend->resize_window(vp, size);
        };
        _context->PlatformIO.Renderer_RenderWindow = +[](ImGuiViewport* vp, void*) {
            auto backend = static_cast<ImGuiRendererBackend*>(
                ImGui::GetCurrentContext()->IO.BackendRendererUserData
            );
            backend->render_window(vp, nullptr);
        };
    }
}
ImGuiContext* ImGuiBackend::detach()
{
    // reset context
    SKR_ASSERT(is_attached() && "detach context before attach");
    auto old = _context;
    _context = nullptr;

    // destroy platform backend
    {
        ImGuiContext* cache = ImGui::GetCurrentContext();
        ImGui::SetCurrentContext(old);
        ImGui_ImplSDL3_Shutdown();
        ImGui::SetCurrentContext(cache);
    }

    // destroy all textures
    {
        for (ImTextureData* tex : old->PlatformIO.Textures)
        {
            if (tex->RefCount == 1)
            {
                _renderer_backend->destroy_texture(tex);
                tex->TexID  = 0;
                tex->Status = ImTextureStatus_Destroyed;
            }
        }
    }

    // reset render backend
    {
        old->IO.BackendRendererUserData = nullptr;
        old->IO.BackendRendererName     = nullptr;

        old->PlatformIO.Renderer_CreateWindow  = nullptr;
        old->PlatformIO.Renderer_DestroyWindow = nullptr;
        old->PlatformIO.Renderer_SetWindowSize = nullptr;
        old->PlatformIO.Renderer_RenderWindow  = nullptr;
    }

    return old;
}
void ImGuiBackend::create()
{
    SKR_ASSERT(!is_attached() && "please detach context before create");
    ImGuiContext* new_context = ImGui::CreateContext();
    attach(new_context);
}
void ImGuiBackend::destroy()
{
    SKR_ASSERT(is_attached() && "please attach context before destroy");
    ImGuiContext* old = detach();
    ImGui::DestroyContext(old);
}

// render backend
void ImGuiBackend::set_renderer_backend(RCUnique<ImGuiRendererBackend> backend)
{
    SKR_ASSERT(!is_attached() && "cannot set render backend after attach");
    _renderer_backend = std::move(backend);
}

// main window
void ImGuiBackend::create_main_window(const ImGuiWindowCreateInfo& create_info)
{
    SKR_ASSERT(!has_main_window() && "please destroy main window before create");
    SKR_ASSERT(!_renderer_backend.is_empty() && "please set render backend before create");

    _main_window.create(create_info);
    _renderer_backend->create_main_window(&_main_window);
}
void ImGuiBackend::destroy_main_window()
{
    SKR_ASSERT(has_main_window() && "please create main window before destroy");
    SKR_ASSERT(!_renderer_backend.is_empty() && "please set render backend before destroy");

    _main_window.destroy();
    _renderer_backend->destroy_main_window(&_main_window);
}

// frame
void ImGuiBackend::pump_message()
{
    SKR_ASSERT(is_attached() && "please attach context before pump message");
    SDL_Event   e;
    SDL_Window* main_wnd = reinterpret_cast<SDL_Window*>(_main_window.payload());
    while (SDL_PollEvent(&e))
    {
        ImGui_ImplSDL3_ProcessEvent(&e);

        // quit message
        if (e.type == SDL_EVENT_QUIT)
        {
            _want_exit.trigger();
        }
        else if (e.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED &&
                 e.window.windowID == SDL_GetWindowID(main_wnd))
        {
            _want_exit.trigger();
        }

        // resize message
        if (e.window.windowID == SDL_GetWindowID(main_wnd))
        {
            _want_resize.trigger_if(e.type == SDL_EVENT_WINDOW_RESIZED);
            _pixel_size_changed.trigger_if(e.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED);
            _content_scale_changed.trigger_if(e.type == SDL_EVENT_DISPLAY_CONTENT_SCALE_CHANGED);
        }
    }
}
void ImGuiBackend::begin_frame()
{
    SKR_ASSERT(is_attached() && "please attach context before begin frame");
    SKR_ASSERT(ImGui::GetCurrentContext() == _context && "context mismatch");

    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}
void ImGuiBackend::end_frame()
{
    SKR_ASSERT(is_attached() && "please attach context before end frame");
    SKR_ASSERT(ImGui::GetCurrentContext() == _context && "context mismatch");

    ImGui::EndFrame();
    if (_context->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
    }
}
void ImGuiBackend::render()
{
    SKR_ASSERT(is_attached() && "please attach context before render");
    SKR_ASSERT(ImGui::GetCurrentContext() == _context && "context mismatch");

    // update textures
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
    {
        switch (tex->Status)
        {
        case ImTextureStatus_WantCreate:
            _renderer_backend->create_texture(tex);
            break;
        case ImTextureStatus_WantUpdates:
            _renderer_backend->update_texture(tex);
            break;
        case ImTextureStatus_WantDestroy:
            _renderer_backend->destroy_texture(tex);
            break;
        }
    }

    // render main window
    ImGui::Render();
    _renderer_backend->render_main_window(&_main_window, ImGui::GetDrawData());

    // render other viewports
    if (_context->IO.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::RenderPlatformWindowsDefault();
    }
}

// imgui functional
void ImGuiBackend::enable_nav(bool enable)
{
    SKR_ASSERT(is_attached() && "please attach context before set feature");
    if (enable)
    {
        _context->IO.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        _context->IO.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    }
    else
    {
        _context->IO.ConfigFlags &= ~ImGuiConfigFlags_NavEnableKeyboard;
        _context->IO.ConfigFlags &= ~ImGuiConfigFlags_NavEnableGamepad;
    }
}
void ImGuiBackend::enable_docking(bool enable)
{
    SKR_ASSERT(is_attached() && "please attach context before set feature");
    if (enable)
    {
        _context->IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }
    else
    {
        _context->IO.ConfigFlags &= ~ImGuiConfigFlags_DockingEnable;
    }
}
void ImGuiBackend::enable_multi_viewport(bool enable)
{
    SKR_ASSERT(is_attached() && "please attach context before set feature");
    if (enable)
    {
        _context->IO.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }
    else
    {
        _context->IO.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;
    }
}
void ImGuiBackend::enable_ini_file(bool enable)
{
    SKR_ASSERT(is_attached() && "please attach context before set feature");
    if (enable)
    {
        _context->IO.IniFilename = "imgui.ini";
    }
    else
    {
        _context->IO.IniFilename = nullptr;
    }
}
void ImGuiBackend::enable_log_file(bool enable)
{
    SKR_ASSERT(is_attached() && "please attach context before set feature");
    if (enable)
    {
        _context->IO.LogFilename = "imgui.log";
    }
    else
    {
        _context->IO.LogFilename = nullptr;
    }
}
void ImGuiBackend::enable_transparent_docking(bool enable)
{
    SKR_ASSERT(is_attached() && "please attach context before set feature");
    _context->IO.ConfigDockingTransparentPayload = enable;
}
void ImGuiBackend::enable_always_tab_bar(bool enable)
{
    SKR_ASSERT(is_attached() && "please attach context before set feature");
    _context->IO.ConfigDockingAlwaysTabBar = enable;
}
void ImGuiBackend::enable_move_window_by_blank_area(bool enable)
{
    SKR_ASSERT(is_attached() && "please attach context before set feature");
    _context->IO.ConfigWindowsMoveFromTitleBarOnly = !enable;
}

} // namespace skr