#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrCore/module/module.hpp"
#include "SkrCore/memory/sp.hpp"
#include "SkrCore/async/thread_job.hpp"

#include "SkrProfile/profile.h"
#include "SkrScene/resources/scene_resource.h"
#include "SkrRenderer/skr_renderer.h"
#include "SkrRenderer/fwd_types.h"
#include "SkrRenderer/primitive_draw.h"

#include "SkrImGui/imgui_render_backend.hpp"
#include <SkrImGui/imgui_backend.hpp>

const bool bUseJob = true;
class SGameModule : public skr::IDynamicModule
{
    // DynamicModule interface
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int  main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;
    // ECS
    struct sugoi_storage_t* game_world = nullptr;
    // Task Scheduler
    skr::task::scheduler_t scheduler;
    skr::SP<skr::JobQueue> job_queue = nullptr;

    // Renderer
    SRenderDeviceId game_render_device = nullptr;
    SRendererId     game_renderer      = nullptr;
    CGPUSwapChainId swapchain          = nullptr;
    CGPUFenceId     present_fence      = nullptr;

    // install & unsintall resource factories
    void installResourceFactories();
    void uninstallResourceFactories();
};
static SGameModule* g_game_module = nullptr;
///////
/// Module entry point
///////
IMPLEMENT_DYNAMIC_MODULE(SGameModule, Game);
void SGameModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"game runtime loaded!");
    if (!job_queue)
    {
        skr::String qn             = u8"GameJobQueue";
        auto        job_queueDesc  = make_zeroed<skr::JobQueueDesc>();
        job_queueDesc.thread_count = 2;
        job_queueDesc.priority     = SKR_THREAD_NORMAL;
        job_queueDesc.name         = qn.u8_str();
        job_queue                  = skr::SP<skr::JobQueue>::New(job_queueDesc);
    }
    SKR_ASSERT(job_queue);
    {
        // create ECS game world
        game_world         = sugoiS_create();
        game_render_device = skr_get_default_render_device();
        game_renderer      = skr_create_renderer(game_render_device, game_world);
    }

    if (bUseJob)
    {
        scheduler.initialize(skr::task::scheudler_config_t{});
        scheduler.bind();
        sugoiJ_bind_storage(game_world);
    }

    installResourceFactories();
    g_game_module = this;
}
void SGameModule::on_unload()
{
    SKR_LOG_INFO(u8"game unloaded!");
    g_game_module = nullptr;
    if (bUseJob)
    {
        sugoiJ_unbind_storage(game_world);
        scheduler.unbind();
    }
    uninstallResourceFactories();
}

int SGameModule::main_module_exec(int argc, char8_t** argv)
{
    namespace rg = skr::render_graph;

    SkrZoneScopedN("GameExecution");
    SKR_LOG_INFO(u8"game executed as main module!");
    auto render_device = skr_get_default_render_device();
    auto cgpu_device   = render_device->get_cgpu_device();
    auto gfx_queue     = render_device->get_gfx_queue();

    auto render_graph = skr::render_graph::RenderGraph::create(
        [&cgpu_device, &gfx_queue](skr::render_graph::RenderGraphBuilder& builder) {
            builder.with_device(cgpu_device)
                .with_gfx_queue(gfx_queue)
                .enable_memory_aliasing();
        }
    );
    skr::ImGuiBackend            imgui_backend;
    skr::ImGuiRendererBackendRG* render_backend_rg = nullptr;
    {
        auto render_backend = skr::RCUnique<skr::ImGuiRendererBackendRG>::New();
        render_backend_rg   = render_backend.get();
        skr::ImGuiRendererBackendRGConfig config{};
        config.render_graph = render_graph;
        config.queue        = gfx_queue;
        ;
        render_backend->init(config);
        imgui_backend.create(
            {
                .title = skr::format(u8"Anim Debug Runtime Inner [{}]", gCGPUBackendNames[render_device->get_backend()]),
                .size  = { 1500, 1500 },
            },
            std::move(render_backend)
        );
        imgui_backend.main_window().show();
        imgui_backend.enable_docking();
        imgui_backend.enable_high_dpi();
        // imgui_backend.enable_multi_viewport();

        // Apply Sail Style
        // ImGui::Sail::LoadFont(12.0f);
        // ImGui::Sail::StyleColorsSail();
    }
    uint64_t frame_index      = 0;
    bool     show_demo_window = true;
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
        imgui_backend.collect(); // contact @zihuang.zhu for any issue
        {

            SkrZoneScopedN("Viewport Render");
            auto          viewport          = ImGui::GetMainViewport();
            CGPUTextureId native_backbuffer = render_backend_rg->get_backbuffer(viewport);
            // acquire next frame buffer
            render_backend_rg->set_load_action(viewport, CGPU_LOAD_ACTION_LOAD); // append not clear
            auto back_buffer = render_graph->create_texture(
                [=](rg::RenderGraph& g, skr::render_graph::TextureBuilder& builder) {
                    skr::String buf_name = skr::format(u8"backbuffer");
                    builder.set_name((const char8_t*)buf_name.c_str())
                        .import(native_backbuffer, CGPU_RESOURCE_STATE_UNDEFINED)
                        .allow_render_target();
                }
            );
        }
        {
            SkrZoneScopedN("ImGuiRender");
            imgui_backend.render(); // add present pass
        }
        {
            SkrZoneScopedN("FrameEnd: compile render graph and execute");
            render_graph->compile();
            render_graph->execute();
            if (frame_index >= RG_MAX_FRAME_IN_FLIGHT * 10)
                render_graph->collect_garbage(frame_index - RG_MAX_FRAME_IN_FLIGHT * 10);

            // present
            render_backend_rg->present_all();
            ++frame_index;
        }
    }
    // wait for rendering done
    cgpu_wait_queue_idle(gfx_queue);

    // destroy render graph
    skr::render_graph::RenderGraph::destroy(render_graph);

    // destroy imgui
    imgui_backend.destroy();

    return 0;
}

void SGameModule::installResourceFactories()
{
    SkrZoneScopedN("InstallResourceFactories");
    SKR_LOG_INFO(u8"game runtime installing resource factories!");
}

void SGameModule::uninstallResourceFactories()
{
    SkrZoneScopedN("UninstallResourceFactories");
    SKR_LOG_INFO(u8"game runtime uninstalling resource factories!");
}