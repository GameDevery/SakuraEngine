#pragma once
#include <SkrGraphics/api.h>
#include <SkrRenderGraph/frontend/render_graph.hpp>
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

    // setup io
    virtual void setup_io(ImGuiIO& io) = 0;

    // main window api
    // TODO. 携带一个 main viewport 会比较好
    virtual void create_main_window(ImGuiWindowBackend* wnd)                   = 0;
    virtual void destroy_main_window(ImGuiWindowBackend* wnd)                  = 0;
    virtual void resize_main_window(ImGuiWindowBackend* wnd, uint2 size)      = 0;
    virtual void render_main_window(ImGuiWindowBackend* wnd, ImDrawData* data) = 0;

    // multi viewport api
    virtual void create_window(ImGuiViewport* viewport)              = 0;
    virtual void destroy_window(ImGuiViewport* viewport)             = 0;
    virtual void resize_window(ImGuiViewport* viewport, ImVec2 size) = 0;
    virtual void render_window(ImGuiViewport* viewport, void*)       = 0;

    // texture api
    virtual void create_texture(ImTextureData* tex_data)  = 0;
    virtual void destroy_texture(ImTextureData* tex_data) = 0;
    virtual void update_texture(ImTextureData* tex_data)  = 0;
};

struct ImGuiRendererBackendRGConfig {
    skr::render_graph::RenderGraph* render_graph           = nullptr;
    CGPUQueueId                     queue                  = nullptr;
    ECGPUFormat                     format                 = CGPU_FORMAT_R8G8B8A8_UNORM;
    uint32_t                        concurrent_frame_count = 1; // for destroy texture

    Optional<CGPUSamplerId>             static_sampler = {}; // default: linear
    Optional<CGPUShaderEntryDescriptor> vs             = {}; // default: shaders/imgui_vertex
    Optional<CGPUShaderEntryDescriptor> ps             = {}; // default: shaders/imgui_fragment
};

struct SKR_IMGUI_NG_API ImGuiRendererBackendRG : ImGuiRendererBackend {
    // ctor & dtor
    ImGuiRendererBackendRG();
    ~ImGuiRendererBackendRG();

    // init & shutdown
    bool has_init() const;
    void init(const ImGuiRendererBackendRGConfig& config);
    void shutdown();

    // real present
    void present_sub_viewports();

    //==> ImGuiRendererBackend API
    // setup io
    void setup_io(ImGuiIO& io) override;

    // main window api
    void create_main_window(ImGuiWindowBackend* wnd) override;
    void destroy_main_window(ImGuiWindowBackend* wnd) override;
    void resize_main_window(ImGuiWindowBackend* wnd, uint2 size) override;
    void render_main_window(ImGuiWindowBackend* wnd, ImDrawData* data) override;

    // multi viewport api
    void create_window(ImGuiViewport* viewport) override;
    void destroy_window(ImGuiViewport* viewport) override;
    void resize_window(ImGuiViewport* viewport, ImVec2 size) override;
    void render_window(ImGuiViewport* viewport, void*) override;

    // texture api
    void create_texture(ImTextureData* tex_data) override;
    void destroy_texture(ImTextureData* tex_data) override;
    void update_texture(ImTextureData* tex_data) override;
    //==> ImGuiRendererBackend API

private:
    // config
    CGPUQueueId                     _gfx_queue              = nullptr;
    skr::render_graph::RenderGraph* _render_graph           = nullptr;
    uint32_t                        _concurrent_frame_count = 1; // for destroy texture
    ECGPUFormat                     _backbuffer_format      = CGPU_FORMAT_R8G8B8A8_UNORM;

    // state
    CGPURootSignatureId  _root_sig        = nullptr;
    CGPURenderPipelineId _render_pipeline = nullptr;
    CGPUSamplerId        _static_sampler  = nullptr;
};

} // namespace skr