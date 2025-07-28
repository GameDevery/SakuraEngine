#include <SkrBase/misc/make_zeroed.hpp>
#include <SkrCore/memory/sp.hpp>
#include <SkrCore/module/module_manager.hpp>
#include <SkrCore/log.h>
#include <SkrCore/platform/vfs.h>
#include <SkrCore/time.h>
#include <SkrCore/async/thread_job.hpp>

#include <SkrOS/filesystem.hpp>
#include "SkrCore/memory/impl/skr_new_delete.hpp"
#include "SkrSystem/advanced_input.h"
#include <SkrRT/ecs/world.hpp>
#include <SkrRT/resource/resource_system.h>
#include <SkrRT/resource/local_resource_registry.hpp>

#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrImGui/imgui_backend.hpp"
#include "SkrImGui/imgui_render_backend.hpp"
#include "SkrRenderer/skr_renderer.h"
#include "SkrRenderer/resources/mesh_resource.h"
#include "SkrTask/fib_task.hpp"
#include "SkrGLTFTool/mesh_processing.hpp"
#include "scene_renderer.hpp"
#include "cgltf/cgltf.h"

// The Three-Triangle Example: simple mesh scene hierarchy

struct SceneSampleMeshModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

    void installResourceFactories();
    void uninstallResourceFactories();

    skr::task::scheduler_t scheduler;
    skr::ecs::World world{ scheduler };
    skr_vfs_t* resource_vfs = nullptr;
    skr::io::IRAMService* ram_service = nullptr;
    skr_io_vram_service_t* vram_service = nullptr;

    skr::JobQueue* io_job_queue = nullptr;
    skr::renderer::MeshFactory* mesh_factory = nullptr;
    SRenderDeviceId render_device = nullptr;

    skr::UPtr<skr::ImGuiApp> imgui_app = nullptr;
    skr::ImGuiRendererBackendRG* imgui_render_backend = nullptr;
    skr::SceneRenderer* scene_renderer = nullptr;
};

IMPLEMENT_DYNAMIC_MODULE(SceneSampleMeshModule, SceneSample_Mesh);

void SceneSampleMeshModule::on_load(int argc, char8_t** argv)
{
    skr_log_set_level(SKR_LOG_LEVEL_INFO);
    SKR_LOG_INFO(u8"Scene Sample Mesh Module Loaded");

    render_device = skr_get_default_render_device();
    scheduler.initialize({});
    scheduler.bind();
    world.initialize();

    auto jobQueueDesc = make_zeroed<skr::JobQueueDesc>();
    jobQueueDesc.thread_count = 2;
    jobQueueDesc.priority = SKR_THREAD_ABOVE_NORMAL;
    jobQueueDesc.name = u8"SceneSample-RAMIOJobQueue";
    io_job_queue = SkrNew<skr::JobQueue>(jobQueueDesc);

    std::error_code ec = {};
    auto resourceRoot = (skr::filesystem::current_path(ec) / "../resources").u8string();
    skr_vfs_desc_t vfs_desc = {};
    vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
    vfs_desc.override_mount_dir = resourceRoot.c_str();
    resource_vfs = skr_create_vfs(&vfs_desc);

    auto ioServiceDesc = make_zeroed<skr_ram_io_service_desc_t>();
    ioServiceDesc.name = u8"SceneSample-RAMIOService";
    ioServiceDesc.sleep_time = 1000 / 60;
    ioServiceDesc.io_job_queue = io_job_queue;
    ioServiceDesc.callback_job_queue = io_job_queue;
    ram_service = skr_io_ram_service_t::create(&ioServiceDesc);
    ram_service->run();

    auto vramServiceDesc = make_zeroed<skr_vram_io_service_desc_t>();
    vramServiceDesc.name = u8"SceneSample-VRAMIOService";
    vramServiceDesc.awake_at_request = true;
    vramServiceDesc.ram_service = ram_service;
    vramServiceDesc.callback_job_queue = io_job_queue;
    vramServiceDesc.use_dstorage = true;
    vramServiceDesc.gpu_device = render_device->get_cgpu_device();
    vram_service = skr_io_vram_service_t::create(&vramServiceDesc);
    vram_service->run();

    scene_renderer = skr::SceneRenderer::Create();
    render_device = SkrRendererModule::Get()->get_render_device();
    scene_renderer->initialize(render_device, &world, resource_vfs);

    // installResourceFactories();
}

void SceneSampleMeshModule::on_unload()
{
    uninstallResourceFactories();
    scene_renderer->finalize(SkrRendererModule::Get()->get_render_device());
    skr::SceneRenderer::Destroy(scene_renderer);
    skr_free_vfs(resource_vfs);
    world.finalize();
    scheduler.unbind();
    SKR_LOG_INFO(u8"Scene Sample Mesh Module Unloaded");
}

void SceneSampleMeshModule::installResourceFactories()
{
}

void SceneSampleMeshModule::uninstallResourceFactories()
{
}

int SceneSampleMeshModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("SceneSampleMeshModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Scene Sample Mesh Module");

    std::error_code ec = {};
    // auto gltf_path = (skr::filesystem::current_path(ec) / "../resources/scene/Cube.gltf").u8string();
    // SKR_LOG_INFO(u8"gltf file path: {%s}", gltf_path.c_str());
    // auto* gltf_data = skd::asset::ImportGLTFWithData(gltf_path.c_str(), ram_service, resource_vfs);
    // if (!gltf_data)
    // {
    //     SKR_LOG_ERROR(u8"Failed to load glTF data");
    //     return 1;
    // }
    // SKR_LOG_INFO(u8"Successfully loaded glTF data");
    // SKR_LOG_INFO(u8"Number of Nodes: %d", gltf_data->nodes_count);
    // SKR_LOG_INFO(u8"Buffer Count: %d", gltf_data->buffers_count);
    // if (gltf_data->buffers_count > 0)
    // {
    //     SKR_LOG_INFO(u8"Buffer 0 Size: %zu bytes", gltf_data->buffers[0].size);
    //     SKR_LOG_INFO(u8"Buffer 0 Data: %p", gltf_data->buffers[0].data);
    //     // First 10 bytes of the first buffer
    //     if (gltf_data->buffers[0].data && gltf_data->buffers[0].size > 10)
    //     {
    //         SKR_LOG_INFO(u8"First 10 bytes of Buffer 0: ");
    //         for (size_t i = 0; i < 10; ++i)
    //         {
    //             SKR_LOG_INFO(u8"%02x ", ((uint8_t*)gltf_data->buffers[0].data)[i]);
    //         }
    //         SKR_LOG_INFO(u8"");
    //     }
    // }


    auto render_device = SkrRendererModule::Get()->get_render_device();
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
            .title = skr::format(u8"Scene Viewer [{}]", gCGPUBackendNames[cgpu_device->adapter->instance->backend]),
            .size = { 1500, 1500 },
        };

        imgui_app = skr::UPtr<skr::ImGuiApp>::New(main_window_info, render_device, std::move(render_backend));
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

        // Update resources
        auto resource_system = skr::resource::GetResourceSystem();
        resource_system->Update();

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
            scene_renderer->produce_drawcalls(world.get_storage(), render_graph);
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