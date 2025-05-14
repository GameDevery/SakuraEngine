#include <SkrImGui/imgui_backend.hpp>
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

// main window
void ImGuiBackend::create_main_window(const ImGuiWindowCreateInfo& create_info)
{
    SKR_ASSERT(!has_main_window() && "please destroy main window before create");
    _main_window.create(create_info);
}
void ImGuiBackend::destroy_main_window()
{
    SKR_ASSERT(has_main_window() && "please create main window before destroy");
    _main_window.destroy();
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

    ImGui::Render();
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