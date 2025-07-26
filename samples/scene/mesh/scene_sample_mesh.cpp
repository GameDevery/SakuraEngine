#include <SkrCore/module/module_manager.hpp>
#include <SkrOS/filesystem.hpp>
#include <SkrCore/log.h>
#include <SkrCore/platform/vfs.h>
#include <SkrCore/time.h>
#include "SkrSystem/advanced_input.h"
#include <SkrRT/ecs/world.hpp>
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrImGui/imgui_backend.hpp"
#include "SkrImGui/imgui_render_backend.hpp"
#include "SkrRenderer/skr_renderer.h"
#include "SkrTask/fib_task.hpp"
#include "scene_renderer.hpp"

// The Three-Triangle Example: simple mesh scene hierarchy

struct SceneSampleMeshModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

    skr::task::scheduler_t scheduler;
    skr::ecs::World world{ scheduler };
    skr_vfs_t* resource_vfs = nullptr;
    skr_io_ram_service_t* ram_service = nullptr;
    skr_io_vram_service_t* vram_service = nullptr;
    skr::JobQueue* io_job_queue = nullptr;

    skr::UPtr<skr::ImGuiApp> imgui_app = nullptr;
    skr::ImGuiRendererBackendRG* imgui_render_backend = nullptr;
    skr::SceneRenderer* scene_renderer = nullptr;
};

IMPLEMENT_DYNAMIC_MODULE(SceneSampleMeshModule, SceneSample_Mesh);

void SceneSampleMeshModule::on_load(int argc, char8_t** argv)
{
    skr_log_set_level(SKR_LOG_LEVEL_INFO);
    SKR_LOG_INFO(u8"Scene Sample Mesh Module Loaded");
    scheduler.initialize({});
    scheduler.bind();
    world.initialize();

    std::error_code ec = {};
    auto resourceRoot = (skr::filesystem::current_path(ec) / "../resources").u8string();
    skr_vfs_desc_t vfs_desc = {};
    vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
    vfs_desc.override_mount_dir = resourceRoot.c_str();
    resource_vfs = skr_create_vfs(&vfs_desc);

    scene_renderer = skr::SceneRenderer::Create();
    scene_renderer->initialize(skr_get_default_render_device(), &world, resource_vfs);
}

void SceneSampleMeshModule::on_unload()
{
    scene_renderer->finalize(skr_get_default_render_device());
    skr::SceneRenderer::Destroy(scene_renderer);
    skr_free_vfs(resource_vfs);
    world.finalize();
    scheduler.unbind();
    SKR_LOG_INFO(u8"Scene Sample Mesh Module Unloaded");
}

int SceneSampleMeshModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("SceneSampleMeshModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Scene Sample Mesh Module");

    auto render_device = skr_get_default_render_device();
    auto cgpu_device = render_device->get_cgpu_device();
    auto gfx_queue = render_device->get_gfx_queue();
    auto render_graph = skr::render_graph::RenderGraph::create(
        [&cgpu_device, &gfx_queue](skr::render_graph::RenderGraphBuilder& builder) {
            builder.with_device(cgpu_device)
                .with_gfx_queue(gfx_queue)
                .enable_memory_aliasing();
        });

    {
        auto render_backend = skr::RCUnique<skr::ImGuiRendererBackendRG>::New();
        skr::ImGuiRendererBackendRGConfig config = {};
        config.render_graph = render_graph;
        config.queue = gfx_queue;
        render_backend->init(config);
        imgui_render_backend = render_backend.get();

        skr::SystemWindowCreateInfo main_window_info = {
            .title = skr::format(u8"Live2D Viewer Inner [{}]", gCGPUBackendNames[cgpu_device->adapter->instance->backend]),
            .size = { 1500, 1500 },
        };

        imgui_app = skr::UPtr<skr::ImGuiApp>::New(main_window_info, std::move(render_backend));
        imgui_app->initialize();
        imgui_app->enable_docking();
    }
    // Time
    SHiresTimer tick_timer;
    int64_t elapsed_us = 0;
    int64_t elapsed_frame = 0;
    int64_t fps = 60;
    skr_init_hires_timer(&tick_timer);
    uint64_t frame_index = 0;

    skr::input::Input::Initialize();
    while (!imgui_app->want_exit().comsume())
    {
        SkrZoneScopedN("LoopBody");
        {
            SkrZoneScopedN("PumpMessage");
            imgui_app->pump_message();
        }
        {
            SkrZoneScopedN("ImGUINewFrame");
            imgui_app->begin_frame();
        }
        {
            ImGui::Begin("Scene Sample Mesh");
            ImGui::Text("Some Text");
            ImGui::End();
        }
        {
            SkrZoneScopedN("ImGuiEndFrame");
            imgui_app->end_frame();
        }
        {
            // update viewport
            SkrZoneScopedN("Viewport Render");
            auto viewport = ImGui::GetMainViewport();
            CGPUTextureId native_backbuffer = imgui_render_backend->get_backbuffer(viewport);
            auto back_buffer = render_graph->create_texture(
                [=](skr::render_graph::RenderGraph& g, skr::render_graph::TextureBuilder& builder) {
                    skr::String buf_name = skr::format(u8"backbuffer");
                    builder.set_name((const char8_t*)buf_name.c_str())
                        .import(native_backbuffer, CGPU_RESOURCE_STATE_UNDEFINED)
                        .allow_render_target();
                });
        };
        {
            SkrZoneScopedN("ImGuiRender");
            imgui_render_backend->set_load_action(
                ImGui::GetMainViewport(),
                CGPU_LOAD_ACTION_LOAD);
            imgui_app->render();
        }
        {
            frame_index = render_graph->execute();
            if (frame_index >= RG_MAX_FRAME_IN_FLIGHT * 10)
                render_graph->collect_garbage(frame_index - RG_MAX_FRAME_IN_FLIGHT * 10);
        }
        // present
        imgui_render_backend->present_all();
    }

    cgpu_wait_queue_idle(gfx_queue);
    skr::render_graph::RenderGraph::destroy(render_graph);
    imgui_app->shutdown();
    skr::input::Input::Finalize();
    return 0;
}