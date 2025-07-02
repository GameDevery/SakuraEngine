#include "SkrCore/module/module.hpp"
#include "SkrCore/log.h"
#include "SkrRenderer/skr_renderer.h"
// #include "SkrGraphics/api.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrImGui/imgui_render_backend.hpp"
#include <SkrImGui/imgui_backend.hpp>
#include "AnimDebugRuntime/renderer.h"
#include <SDL3/SDL_video.h>
#include "common/utils.h"
#include "imgui_sail.h"

class SAnimDebugModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int  main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;
};

static SAnimDebugModule* g_anim_debug_module = nullptr;

///////
/// Module entry point
///////
IMPLEMENT_DYNAMIC_MODULE(SAnimDebugModule, AnimDebug);

void SAnimDebugModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"anim debug runtime loaded!");
}

void SAnimDebugModule::on_unload()
{
    g_anim_debug_module = nullptr;
    SKR_LOG_INFO(u8"anim debug runtime unloaded!");
}

int SAnimDebugModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("AnimDebugExecution");
    SKR_LOG_INFO(u8"anim debug runtime executed as main module!");
    animd::Renderer renderer;
    SKR_LOG_INFO(u8"anim debug renderer created with backend: %s", gCGPUBackendNames[renderer.get_backend()]);
    renderer.create_api_objects();
    // create render graph
    auto device       = renderer.get_device();
    auto gfx_queue    = renderer.get_gfx_queue();
    auto render_graph = skr::render_graph::RenderGraph::create(
        [&device, &gfx_queue](skr::render_graph::RenderGraphBuilder& builder) {
            builder.with_device(device)
                .with_gfx_queue(gfx_queue)
                .enable_memory_aliasing();
        }
    );

    // init imgui
    skr::ImGuiBackend            imgui_backend;
    skr::ImGuiRendererBackendRG* render_backend_rg = nullptr;
    {
        auto render_backend = skr::RCUnique<skr::ImGuiRendererBackendRG>::New();
        render_backend_rg   = render_backend.get();
        skr::ImGuiRendererBackendRGConfig config{};
        config.render_graph = render_graph;
        config.queue        = renderer.get_gfx_queue();
        render_backend->init(config);
        imgui_backend.create(
            {
                .title = skr::format(u8"Anim Debug Runtime Inner [{}]", gCGPUBackendNames[renderer.get_backend()]),
                .size  = { 1500, 1500 },
            },
            std::move(render_backend)
        );
        imgui_backend.main_window().show();
        imgui_backend.enable_docking();
        imgui_backend.enable_high_dpi();
        // imgui_backend.enable_multi_viewport();
        // Apply Sail Style
        ImGui::Sail::LoadFont(12.0f);
        ImGui::Sail::StyleColorsSail();
    }

    {
        // Init Your Scene Here
        auto viewport = ImGui::GetMainViewport();
        renderer.set_swapchain(render_backend_rg->get_swapchain(viewport));
        renderer.create_render_pipeline();
    }
    auto pipeline = renderer.get_pipeline();
    // draw loop
    bool     show_demo_window = true;
    uint64_t frame_index      = 0;
    while (!imgui_backend.want_exit().comsume())
    {
        SkrZoneScopedN("LoopBody");
        {
            SkrZoneScopedN("PumpMessage");
            // Pump messages
            imgui_backend.pump_message();
        }

        {
            SkrZoneScopedN("ImGUINewFrame");
            imgui_backend.begin_frame();
        }

        {
            ImGui::ShowDemoWindow(&show_demo_window);
        }
        {
            SkrZoneScopedN("ImGuiEndFrame");
            imgui_backend.end_frame();
        }
        {
            SkrZoneScopedN("ImGuiRender");
            imgui_backend.render();
        }

        {
            SkrZoneScopedN("Viewport Render");
            auto          viewport          = ImGui::GetMainViewport();
            CGPUTextureId native_backbuffer = render_backend_rg->get_backbuffer(viewport);
            auto          back_buffer       = render_graph->create_texture(
                [=](skr::render_graph::RenderGraph& g, skr::render_graph::TextureBuilder& builder) {
                    skr::String buf_name = skr::format(u8"backbuffer", viewport->ID);
                    builder.set_name((const char8_t*)buf_name.c_str())
                        .import(native_backbuffer, CGPU_RESOURCE_STATE_UNDEFINED)
                        .allow_render_target();
                }
            );

            render_graph->add_render_pass(
                [=](skr::render_graph::RenderGraph& g, skr::render_graph::RenderPassBuilder& builder) {
                    builder.set_name(SKR_UTF8("color_pass"))
                        .set_pipeline(pipeline)
                        .write(0, back_buffer, CGPU_LOAD_ACTION_LOAD);
                },
                [=](skr::render_graph::RenderGraph& g, skr::render_graph::RenderPassContext& stack) {
                    cgpu_render_encoder_set_viewport(stack.encoder, 0.0f, 0.0f, (float)native_backbuffer->info->width / 3, (float)native_backbuffer->info->height, 0.f, 1.f);
                    cgpu_render_encoder_set_scissor(stack.encoder, 0, 0, native_backbuffer->info->width, native_backbuffer->info->height);
                    cgpu_render_encoder_draw(stack.encoder, 3, 0);
                }
            );
        }
        // draw render graph
        render_graph->compile();
        render_graph->execute();
        if (frame_index >= RG_MAX_FRAME_IN_FLIGHT * 10)
            render_graph->collect_garbage(frame_index - RG_MAX_FRAME_IN_FLIGHT * 10);

        // present
        render_backend_rg->present_all();
        ++frame_index;
    }

    // wait for rendering done
    cgpu_wait_queue_idle(renderer.get_gfx_queue());

    // destroy render graph
    skr::render_graph::RenderGraph::destroy(render_graph);

    // destroy imgui
    imgui_backend.destroy();
    renderer.destroy();

    return 0; // Return 0 to indicate success
}