#pragma once
#include <SkrContainers/string.hpp>
#include <SkrContainers/optional.hpp>
#include <SkrBase/math.h>
#include <SkrCore/memory/rc.hpp>
#include <imgui.h>
#include <imgui_internal.h>

namespace skr
{
struct ImGuiWindowBackend;

struct SKR_IMGUI_NG_API ImGuiRendererBackend {
    SKR_RC_IMPL();

    // ctor & dtor
    ImGuiRendererBackend()          = default;
    virtual ~ImGuiRendererBackend() = default;

    // main window api
    virtual void create_main_window(ImGuiWindowBackend* wnd)                   = 0;
    virtual void destroy_main_window(ImGuiWindowBackend* wnd)                  = 0;
    virtual void resize_main_window(ImGuiWindowBackend* wnd, ImVec2 size)      = 0;
    virtual void render_main_window(ImGuiWindowBackend* wnd, ImDrawData* data) = 0;

    // multi viewport api
    virtual void create_window(ImGuiViewport* viewport)              = 0;
    virtual void destroy_window(ImGuiViewport* viewport)             = 0;
    virtual void resize_window(ImGuiViewport* viewport, ImVec2 size) = 0;
    virtual void render_window(ImGuiViewport* viewport, void*)       = 0;

    // font api
    virtual void create_texture(ImTextureData* tex_data)  = 0;
    virtual void destroy_texture(ImTextureData* tex_data) = 0;
    virtual void update_texture(ImTextureData* tex_data)  = 0;
};

struct SKR_IMGUI_NG_API ImGuiRendererBackendRG : ImGuiRendererBackend {
};

} // namespace skr