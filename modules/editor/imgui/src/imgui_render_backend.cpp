#include <SkrImGui/imgui_render_backend.hpp>
#include <filesystem>

// helpers
namespace skr
{
struct ImGuiRendererBackendRGViewportData {
    CGPUSurfaceId   surface          = nullptr;
    CGPUSwapChainId swapchain        = nullptr;
    CGPUFenceId     fence            = nullptr;
    CGPUQueueId     present_queue    = nullptr;
    uint32_t        backbuffer_index = 0;
};
struct ImGuiRendererBackendRGTextureData {
    CGPUTextureId texture = nullptr;
};

inline static Vector<uint8_t> read_shader_bytes(
    String       virtual_path,
    ECGPUBackend backend
)
{
    Vector<uint8_t> result;

    // combine path
    std::filesystem::path shader_path = u8"../resources/shaders/";
    shader_path /= virtual_path.c_str();
    switch (backend)
    {
    case CGPU_BACKEND_VULKAN:
        shader_path += u8".spv";
        break;
    case CGPU_BACKEND_D3D12:
    case CGPU_BACKEND_XBOX_D3D12:
        shader_path += u8".dxil";
        break;
    default:
        break;
    }

    // read file
    if (std::filesystem::exists(shader_path))
    {
        auto f = fopen(shader_path.string().c_str(), "rb");
        fseek(f, 0, SEEK_END);
        result.resize_unsafe(ftell(f));
        fseek(f, 0, SEEK_SET);
        fread(result.data(), result.size(), 1, f);
        fclose(f);
    }
    else
    {
        SKR_LOG_ERROR(u8"shader file not found: %s", shader_path.string().c_str());
    }

    return result;
}

// TODO. update swapchain

} // namespace skr

// impl ImGuiRendererBackendRG
namespace skr
{
// ctor & dtor
ImGuiRendererBackendRG::ImGuiRendererBackendRG()
{
}
ImGuiRendererBackendRG::~ImGuiRendererBackendRG()
{
    if (has_init())
    {
        shutdown();
    }
}

// init & shutdown
bool ImGuiRendererBackendRG::has_init() const
{
    return _gfx_queue;
}
void ImGuiRendererBackendRG::init(const ImGuiRendererBackendRGConfig& config)
{
    SKR_ASSERT(!has_init() && "multiple init");
    SKR_ASSERT(config.ps.has_value() == config.vs.has_value() && "only one shader is setted");

    // setup draw data
    _gfx_queue              = config.queue;
    _render_graph           = config.render_graph;
    _concurrent_frame_count = config.concurrent_frame_count;
    _backbuffer_format      = config.format;

    // load shaders
    CGPUShaderEntryDescriptor ppl_shaders[2] = {};
    if (config.vs.has_value())
    {
        ppl_shaders[0] = config.vs.value();
        ppl_shaders[1] = config.ps.value();
    }
    else
    {
        auto vs_bytes = read_shader_bytes(
            u8"imgui_vertex",
            _gfx_queue->device->adapter->instance->backend
        );
        auto ps_bytes = read_shader_bytes(
            u8"imgui_fragment",
            _gfx_queue->device->adapter->instance->backend
        );

        // create lib
        CGPUShaderLibraryDescriptor vs_desc{};
        vs_desc.name      = SKR_UTF8("imgui_vertex_shader");
        vs_desc.stage     = CGPU_SHADER_STAGE_VERT;
        vs_desc.code      = reinterpret_cast<uint32_t*>(vs_bytes.data());
        vs_desc.code_size = vs_bytes.size();
        CGPUShaderLibraryDescriptor ps_desc{};
        ps_desc.name      = SKR_UTF8("imgui_fragment_shader");
        ps_desc.stage     = CGPU_SHADER_STAGE_FRAG;
        ps_desc.code      = reinterpret_cast<uint32_t*>(ps_bytes.data());
        ps_desc.code_size = ps_bytes.size();
        auto vs_lib       = cgpu_create_shader_library(
            _gfx_queue->device,
            &vs_desc
        );
        auto fs_lib = cgpu_create_shader_library(
            _gfx_queue->device,
            &ps_desc
        );

        // fill desc
        ppl_shaders[0].library = vs_lib;
        ppl_shaders[0].stage   = CGPU_SHADER_STAGE_VERT;
        ppl_shaders[0].entry   = SKR_UTF8("main");
        ppl_shaders[1].library = fs_lib;
        ppl_shaders[1].stage   = CGPU_SHADER_STAGE_FRAG;
        ppl_shaders[1].entry   = SKR_UTF8("main");
    }

    // load static sampler
    CGPUSamplerId static_sampler = nullptr;
    if (config.static_sampler.has_value())
    {
        static_sampler = config.static_sampler.value();
    }
    else
    {
        CGPUSamplerDescriptor sampler_desc{};
        sampler_desc.address_u    = CGPU_ADDRESS_MODE_REPEAT;
        sampler_desc.address_v    = CGPU_ADDRESS_MODE_REPEAT;
        sampler_desc.address_w    = CGPU_ADDRESS_MODE_REPEAT;
        sampler_desc.mipmap_mode  = CGPU_MIPMAP_MODE_LINEAR;
        sampler_desc.min_filter   = CGPU_FILTER_TYPE_LINEAR;
        sampler_desc.mag_filter   = CGPU_FILTER_TYPE_LINEAR;
        sampler_desc.compare_func = CGPU_CMP_NEVER;
        static_sampler            = cgpu_create_sampler(
            _gfx_queue->device,
            &sampler_desc
        );
        _static_sampler = static_sampler;
    }

    // create root signature
    {
        const char8_t*              push_constant_name = u8"push_constants";
        const char8_t*              sampler_name       = u8"sampler0";
        CGPURootSignatureDescriptor rs_desc{};
        rs_desc.shaders              = ppl_shaders;
        rs_desc.shader_count         = 2;
        rs_desc.push_constant_names  = &push_constant_name;
        rs_desc.push_constant_count  = 1;
        rs_desc.static_sampler_names = &sampler_name;
        rs_desc.static_sampler_count = 1;
        rs_desc.static_samplers      = &static_sampler;
        _root_sig                    = cgpu_create_root_signature(
            _gfx_queue->device,
            &rs_desc
        );
    }

    // create pipeline
    {
        CGPUVertexLayout vertex_layout{};
        vertex_layout.attribute_count = 3;
        vertex_layout.attributes[0]   = {
            u8"POSITION",
            1,
            CGPU_FORMAT_R32G32_SFLOAT,
            0,
            0,
            sizeof(float) * 2,
            CGPU_INPUT_RATE_VERTEX
        };
        vertex_layout.attributes[1] = {
            u8"TEXCOORD",
            1,
            CGPU_FORMAT_R32G32_SFLOAT,
            0,
            sizeof(float) * 2,
            sizeof(float) * 2,
            CGPU_INPUT_RATE_VERTEX
        };
        vertex_layout.attributes[2] = {
            u8"COLOR",
            1,
            CGPU_FORMAT_R8G8B8A8_UNORM,
            0, sizeof(float) * 4,
            sizeof(uint32_t),
            CGPU_INPUT_RATE_VERTEX
        };

        CGPURasterizerStateDescriptor rs_state{};
        rs_state.cull_mode               = CGPU_CULL_MODE_NONE;
        rs_state.fill_mode               = CGPU_FILL_MODE_SOLID;
        rs_state.front_face              = CGPU_FRONT_FACE_CW;
        rs_state.slope_scaled_depth_bias = 0.f;
        rs_state.enable_depth_clamp      = false;
        rs_state.enable_scissor          = true;
        rs_state.enable_multi_sample     = false;
        rs_state.depth_bias              = 0;

        CGPUBlendStateDescriptor blend_state{};
        blend_state.blend_modes[0]       = CGPU_BLEND_MODE_ADD;
        blend_state.src_factors[0]       = CGPU_BLEND_CONST_SRC_ALPHA;
        blend_state.dst_factors[0]       = CGPU_BLEND_CONST_ONE_MINUS_SRC_ALPHA;
        blend_state.blend_alpha_modes[0] = CGPU_BLEND_MODE_ADD;
        blend_state.src_alpha_factors[0] = CGPU_BLEND_CONST_ONE;
        blend_state.dst_alpha_factors[0] = CGPU_BLEND_CONST_ONE_MINUS_SRC_ALPHA;
        blend_state.masks[0]             = CGPU_COLOR_MASK_ALL;
        blend_state.independent_blend    = false;

        CGPURenderPipelineDescriptor rp_desc{};
        rp_desc.root_signature      = _root_sig;
        rp_desc.prim_topology       = CGPU_PRIM_TOPO_TRI_LIST;
        rp_desc.vertex_layout       = &vertex_layout;
        rp_desc.vertex_shader       = &ppl_shaders[0];
        rp_desc.fragment_shader     = &ppl_shaders[1];
        rp_desc.render_target_count = 1;
        rp_desc.rasterizer_state    = &rs_state;
        rp_desc.blend_state         = &blend_state;
        rp_desc.color_formats       = &_backbuffer_format;

        _render_pipeline = cgpu_create_render_pipeline(
            _gfx_queue->device,
            &rp_desc
        );
    }

    // cleanup shader library
    if (!config.vs.has_value())
    {
        cgpu_free_shader_library(ppl_shaders[0].library);
        cgpu_free_shader_library(ppl_shaders[1].library);
    }
}
void ImGuiRendererBackendRG::shutdown()
{
    SKR_ASSERT(has_init() && "not init");

    // destroy render pipeline & root signature
    cgpu_free_render_pipeline(_render_pipeline);
    cgpu_free_root_signature(_root_sig);
    _render_pipeline = nullptr;
    _root_sig        = nullptr;

    // destroy sampler
    if (_static_sampler)
    {
        cgpu_free_sampler(_static_sampler);
        _static_sampler = nullptr;
    }

    // reset config
    _gfx_queue              = nullptr;
    _render_graph           = nullptr;
    _concurrent_frame_count = 1;
    _backbuffer_format      = CGPU_FORMAT_R8G8B8A8_UNORM;
}

// real present
void ImGuiRendererBackendRG::present_sub_viewports()
{
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    for (ImGuiViewport* viewport : platform_io.Viewports)
    {
        if (viewport->Flags & ImGuiViewportFlags_IsMinimized) { continue; }
        if (viewport == ImGui::GetMainViewport()) { continue; }

        // get render data
        auto rdata = (ImGuiRendererBackendRGViewportData*)viewport->RendererUserData;
        SKR_ASSERT(rdata != nullptr);

        // do present
        CGPUQueuePresentDescriptor present_desc{};
        present_desc.index     = rdata->backbuffer_index;
        present_desc.swapchain = rdata->swapchain;
        cgpu_queue_present(rdata->present_queue, &present_desc);
    }
}

// setup io
void ImGuiRendererBackendRG::setup_io(ImGuiIO& io)
{
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;
    io.BackendFlags |= ImGuiBackendFlags_RendererHasViewports;
}

// main window api
void ImGuiRendererBackendRG::create_main_window(ImGuiWindowBackend* wnd)
{
}
void ImGuiRendererBackendRG::destroy_main_window(ImGuiWindowBackend* wnd)
{
}
void ImGuiRendererBackendRG::resize_main_window(ImGuiWindowBackend* wnd, uint2 size)
{
}
void ImGuiRendererBackendRG::render_main_window(ImGuiWindowBackend* wnd, ImDrawData* data)
{
}

// multi viewport api
void ImGuiRendererBackendRG::create_window(ImGuiViewport* viewport)
{
}
void ImGuiRendererBackendRG::destroy_window(ImGuiViewport* viewport)
{
}
void ImGuiRendererBackendRG::resize_window(ImGuiViewport* viewport, ImVec2 size)
{
}
void ImGuiRendererBackendRG::render_window(ImGuiViewport* viewport, void*)
{
}

// texture api
void ImGuiRendererBackendRG::create_texture(ImTextureData* tex_data)
{
}
void ImGuiRendererBackendRG::destroy_texture(ImTextureData* tex_data)
{
}
void ImGuiRendererBackendRG::update_texture(ImTextureData* tex_data)
{
}

} // namespace skr