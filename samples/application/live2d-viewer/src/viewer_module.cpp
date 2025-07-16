#include "math.h"
#include "SkrCore/log.h"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrCore/platform/vfs.h"
#include "SkrOS/thread.h"
#include "SkrCore/time.h"
#include "SkrOS/filesystem.hpp"
#include <SkrContainers/string.hpp>
#include "SkrRT/io/ram_io.hpp"
#include "SkrRT/io/vram_io.hpp"
#include <SkrCore/memory/sp.hpp>
#include "SkrCore/async/thread_job.hpp"
#include "SkrCore/module/module_manager.hpp"
#include "SkrRT/runtime_module.h"
#include "SkrSystem/advanced_input.h"
#include <SkrImGui/imgui_backend.hpp>
#include <SkrImGui/imgui_render_backend.hpp>
#include "SkrRenderer/skr_renderer.h"
#include "SkrLive2D/l2d_model_resource.h"
#include "SkrLive2D/l2d_render_model.h"
#include "SkrLive2D/l2d_renderer.hpp"
#include "SkrProfile/profile.h"

#ifdef _WIN32
    #include "SkrImageCoder/extensions/win_dstorage_decompressor.h"
#endif

namespace skr
{
struct JobQueue;
}

class SLive2DViewerModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

public:
    static SLive2DViewerModule* Get();

    bool bUseCVV = true;

    struct sugoi_storage_t* l2d_world = nullptr;
    skr_vfs_t* resource_vfs = nullptr;
    skr_io_ram_service_t* ram_service = nullptr;
    skr_io_vram_service_t* vram_service = nullptr;
    skr::JobQueue* io_job_queue = nullptr;

    // imgui
    skr::UPtr<skr::ImGuiApp> imgui_app = nullptr;
    skr::ImGuiRendererBackendRG* imgui_render_backend = nullptr;
    skr::Live2DRenderer* l2d_renderer = nullptr;
};

IMPLEMENT_DYNAMIC_MODULE(SLive2DViewerModule, Live2DViewer);

SLive2DViewerModule* SLive2DViewerModule::Get()
{
    auto mm = skr_get_module_manager();
    static auto rm = static_cast<SLive2DViewerModule*>(mm->get_module(u8"Live2DViewer"));
    return rm;
}

void SLive2DViewerModule::on_load(int argc, char8_t** argv)
{
    skr_log_set_level(SKR_LOG_LEVEL_INFO);
    skr_log_initialize_async_worker();

    SKR_LOG_INFO(u8"live2d viewer loaded!");

    std::error_code ec = {};
    auto resourceRoot = (skr::filesystem::current_path(ec) / "../resources").u8string();
    skr_vfs_desc_t vfs_desc = {};
    vfs_desc.mount_type = SKR_MOUNT_TYPE_CONTENT;
    vfs_desc.override_mount_dir = resourceRoot.c_str();
    resource_vfs = skr_create_vfs(&vfs_desc);

    l2d_world = sugoiS_create();

    auto render_device = skr_get_default_render_device();

    auto jobQueueDesc = make_zeroed<skr::JobQueueDesc>();
    jobQueueDesc.thread_count = 2;
    jobQueueDesc.priority = SKR_THREAD_ABOVE_NORMAL;
    jobQueueDesc.name = u8"Live2DViewer-RAMIOJobQueue";
    io_job_queue = SkrNew<skr::JobQueue>(jobQueueDesc);

    auto ramServiceDesc = make_zeroed<skr_ram_io_service_desc_t>();
    ramServiceDesc.name = u8"Live2DViewer-RAMIOService";
    ramServiceDesc.sleep_time = 1000 / 100; // tick rate: 100
    ramServiceDesc.io_job_queue = io_job_queue;
    ramServiceDesc.callback_job_queue = io_job_queue;
    ramServiceDesc.awake_at_request = false; // add latency but reduce CPU usage & batch IO requests
    ram_service = skr_io_ram_service_t::create(&ramServiceDesc);
    ram_service->run();

    auto vramServiceDesc = make_zeroed<skr_vram_io_service_desc_t>();
    vramServiceDesc.name = u8"Live2DViewer-VRAMIOService";
    vramServiceDesc.awake_at_request = true;
    vramServiceDesc.ram_service = ram_service;
    vramServiceDesc.callback_job_queue = SLive2DViewerModule::Get()->io_job_queue;
    vramServiceDesc.use_dstorage = true;
    vramServiceDesc.gpu_device = render_device->get_cgpu_device();
    vram_service = skr_io_vram_service_t::create(&vramServiceDesc);
    vram_service->run();

#ifdef _WIN32
    skr_win_dstorage_decompress_desc_t decompress_desc = {};
    decompress_desc.job_queue = io_job_queue;
    if (auto decompress_service = skr_runtime_create_win_dstorage_decompress_service(&decompress_desc))
    {
        skr_win_dstorage_decompress_service_register_callback(decompress_service, SKR_WIN_DSTORAGE_COMPRESSION_TYPE_IMAGE, &skr_image_coder_win_dstorage_decompressor, nullptr);
    }
#endif

    l2d_renderer = skr::Live2DRenderer::Create();
    l2d_renderer->initialize(render_device, l2d_world, resource_vfs);
}

void SLive2DViewerModule::on_unload()
{
    SKR_LOG_INFO(u8"live2d viewer unloaded!");

    l2d_renderer->finalize(skr_get_default_render_device());
    skr::Live2DRenderer::Destroy(l2d_renderer);

    skr_io_vram_service_t::destroy(vram_service);
    skr_io_ram_service_t::destroy(ram_service);
    skr_free_vfs(resource_vfs);

    sugoiS_release(l2d_world);

    SkrDelete(io_job_queue);

    skr_log_finalize_async_worker();
}

#include "SkrRT/ecs/sugoi.h"

#include "SkrRT/ecs/type_builder.hpp"

void create_test_scene(skr_vfs_t* resource_vfs, skr_io_ram_service_t* ram_service, bool bUseCVV)
{
    auto storage = SLive2DViewerModule::Get()->l2d_world;
    auto type_builder = make_zeroed<sugoi::TypeSetBuilder>();
    type_builder
        .with<skr_live2d_render_model_comp_t>();
    // allocate renderable
    auto live2d_type = make_zeroed<sugoi_entity_type_t>();
    live2d_type.type = type_builder.build();
    // deallocate existed
    {
        auto filter = make_zeroed<sugoi_filter_t>();
        filter.all = live2d_type.type;
        auto meta = make_zeroed<sugoi_meta_filter_t>();
        skr::Vector<sugoi_entity_t> to_destroy;
        auto freeFunc = [&](sugoi_chunk_view_t* view) {
            auto mesh_comps = sugoi::get_owned_rw<skr_live2d_render_model_comp_t>(view);
            for (uint32_t i = 0; i < view->count; i++)
            {
                while (!mesh_comps[i].vram_future.is_ready()) {}
                skr_live2d_render_model_free(mesh_comps[i].vram_future.render_model);
                while (!mesh_comps[i].ram_future.is_ready()) {}
                skr_live2d_model_free(mesh_comps[i].ram_future.model_resource);
            }
            auto pents = sugoiV_get_entities(view);
            to_destroy.append(pents, view->count);
        };
        sugoiS_filter(storage, &filter, &meta, SUGOI_LAMBDA(freeFunc));
        sugoiS_destroy_entities(storage, to_destroy.data(), to_destroy.size());
    }

    // allocate new
    auto live2dEntSetup = [&](sugoi_chunk_view_t* view) {
        auto render_device = skr_get_default_render_device();
        auto mesh_comps = sugoi::get_owned_rw<skr_live2d_render_model_comp_t>(view);
        for (uint32_t i = 0; i < view->count; i++)
        {
            auto& vram_request = mesh_comps[i].vram_future;
            auto& ram_request = mesh_comps[i].ram_future;
            vram_request.vfs_override = resource_vfs;
            vram_request.queue_override = render_device->get_gfx_queue();
            ram_request.vfs_override = resource_vfs;
            ram_request.callback_data = &vram_request;
            vram_request.use_dynamic_buffer = bUseCVV;
            ram_request.finish_callback = +[](skr_live2d_ram_io_future_t* request, void* data) {
                auto pRendermodelFuture = (skr_live2d_render_model_future_t*)data;
                auto ram_service = SLive2DViewerModule::Get()->ram_service;
                auto vram_service = SLive2DViewerModule::Get()->vram_service;
                auto render_device = skr_get_default_render_device();
                auto cgpu_device = render_device->get_cgpu_device();
                skr_live2d_render_model_create_from_raw(ram_service, vram_service, cgpu_device, request->model_resource, pRendermodelFuture);
            };
            // skr_live2d_model_create_from_json(ram_service, u8"Live2DViewer/Mao/mao_pro_t02.model3.json", &ram_request);
            skr_live2d_model_create_from_json(ram_service, u8"Live2DViewer/Hiyori/Hiyori.model3.json", &ram_request);
        }
    };
    sugoiS_allocate_type(storage, &live2d_type, 1, SUGOI_LAMBDA(live2dEntSetup));
}

int SLive2DViewerModule::main_module_exec(int argc, char8_t** argv)
{
    namespace render_graph = skr::render_graph;

    SKR_LOG_INFO(u8"live2d viewer executed!");

    // get rendering context
    auto render_device = skr_get_default_render_device();
    auto cgpu_device = render_device->get_cgpu_device();
    auto gfx_queue = render_device->get_gfx_queue();
    auto adapter_detail = cgpu_query_adapter_detail(cgpu_device->adapter);
    auto ram_service = SLive2DViewerModule::Get()->ram_service;

    // init rendering context
    auto renderGraph = render_graph::RenderGraph::create(
        [=](skr::render_graph::RenderGraphBuilder& builder) {
            builder.with_device(cgpu_device)
                .with_gfx_queue(gfx_queue)
                .enable_memory_aliasing();
        });

    // init imgui
    {
        using namespace skr;
        
        SystemWindowCreateInfo main_window_info = 
        {
            .title = skr::format(u8"Live2D Viewer Inner [{}]", gCGPUBackendNames[cgpu_device->adapter->instance->backend]),
            .size = { 1500, 1500 },
        };
        auto render_backend = skr::RCUnique<skr::ImGuiRendererBackendRG>::New();
        ImGuiRendererBackendRGConfig config = {};
        config.render_graph = renderGraph;
        config.queue = gfx_queue;
        render_backend->init(config);
        imgui_render_backend = render_backend.get();

        imgui_app = UPtr<ImGuiApp>::New(main_window_info, std::move(render_backend));
        imgui_app->initialize();
        imgui_app->enable_docking();
    }

    // init live2d
    create_test_scene(resource_vfs, ram_service, bUseCVV);
    uint64_t frame_index = 0;
    SHiresTimer tick_timer;
    int64_t elapsed_us = 0;
    int64_t elapsed_frame = 0;
    uint32_t fps = 60;
    skr_init_hires_timer(&tick_timer);

    // init input system
    skr::input::Input::Initialize();

    while (!imgui_app->want_exit().comsume())
    {
        FrameMark;

        // LoopBody
        SkrZoneScopedN("LoopBody");

        // get frame time
        int64_t us = skr_hires_timer_get_usec(&tick_timer, true);
        elapsed_us += us;
        elapsed_frame += 1;
        if (elapsed_us > (1000 * 1000))
        {
            fps = (uint32_t)elapsed_frame;
            elapsed_frame = 0;
            elapsed_us = 0;
        }
        // pump messages
        {
            SkrZoneScopedN("PollEvent");
            imgui_app->pump_message();
            skr::input::Input::GetInstance()->Tick();
        }

        // imgui begin frame
        {
            SkrZoneScopedN("ImGUINewFrame");
            imgui_app->begin_frame();
        }

        // config
        static uint32_t sample_count = 0;
        bool bPrevUseCVV = bUseCVV;

        // update imgui
        {
            SkrZoneScopedN("ImGUIUpdate");
            ImGui::Begin("Live2DViewer");
#if !SKR_SHIPPING
            ImGui::Text("Debug Build");
#else
            ImGui::Text("Shipping Build");
#endif
            ImGui::Text("Graphics: %s", adapter_detail->vendor_preset.gpu_name);
            auto res = ImGui::GetMainViewport()->Size;
            ImGui::Text("Resolution: %dx%d", (int)res.x, (int)res.y);
            ImGui::Text("MotionEvalFPS(Fixed): %d", 240);
            ImGui::Text("PhysicsEvalFPS(Fixed): %d", 240);
            ImGui::Text("RenderFPS: %d", (uint32_t)fps);
            ImGui::Text("UseCVV");
            ImGui::SameLine();
            ImGui::Checkbox("##UseCVV", &bUseCVV);

            static char buf[1024];
            ImGui::InputText("INPUT", buf, sizeof(buf));

            bool bShowMetrics = true;
            ImGui::ShowMetricsWindow(&bShowMetrics);
            /*
            {
                const char* items[] = { "DirectStorage(File)", "DirectStorage(Memory)", "UploadBuffer" };
                ImGui::Text("UploadMethod");
                ImGui::SameLine();
                const char* combo_preview_value = items[upload_method];  // Pass in the preview value visible before opening the combo (it could be anything)
                if (ImGui::BeginCombo("##UploadMethod", combo_preview_value, ImGuiComboFlags_PopupAlignLeft))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                    {
                        const bool is_selected = (upload_method == n);
                        if (ImGui::Selectable(items[n], is_selected))
                            upload_method = static_cast<DemoUploadMethod>(n);

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
            }
            */
            {
                static int sample_index = 0;
                const char* items[] = { "1x", "2x", "4x", "8x" };
                ImGui::Text("MSAA");
                ImGui::SameLine();
                const char* combo_preview_value = items[sample_index]; // Pass in the preview value visible before opening the combo (it could be anything)
                if (ImGui::BeginCombo("##MSAA", combo_preview_value, ImGuiComboFlags_PopupAlignLeft))
                {
                    for (int n = 0; n < IM_ARRAYSIZE(items); n++)
                    {
                        const bool is_selected = (sample_index == n);
                        if (ImGui::Selectable(items[n], is_selected))
                            sample_index = n;

                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                        if (is_selected)
                            ImGui::SetItemDefaultFocus();
                    }
                    ImGui::EndCombo();
                }
                sample_count = static_cast<uint32_t>(::pow(2, sample_index));
            }
            ImGui::End();
        }

        // imgui end frame
        {
            SkrZoneScopedN("ImGUIEndFrame");
            imgui_app->end_frame();
        }

        // restart test scene
        if (bPrevUseCVV != bUseCVV)
        {
            cgpu_wait_queue_idle(gfx_queue);
            create_test_scene(
                resource_vfs,
                ram_service,
                bUseCVV);
        }

        // acquire backbuffer
        CGPUTextureId native_backbuffer;
        {
            SkrZoneScopedN("AcquireFrame");

            // get backbuffer
            native_backbuffer = imgui_render_backend->get_backbuffer(
                ImGui::GetMainViewport());

            // register backbuffer
            renderGraph->create_texture(
                [=](render_graph::RenderGraph& g, render_graph::TextureBuilder& builder) {
                    builder.set_name(u8"backbuffer")
                        .import(native_backbuffer, CGPU_RESOURCE_STATE_UNDEFINED)
                        .allow_render_target();
                });
        }

        // render live2d scene
        {
            SkrZoneScopedN("RenderScene");
            l2d_renderer->produce_drawcalls(l2d_world, renderGraph);
            l2d_renderer->draw(renderGraph);
        }

        // render imgui
        {
            SkrZoneScopedN("RenderIMGUI");
            imgui_render_backend->set_load_action(
                ImGui::GetMainViewport(),
                CGPU_LOAD_ACTION_LOAD);
            imgui_app->render();
        }

        // execute render graph
        {
            SkrZoneScopedN("ExecuteRenderGraph");
            frame_index = renderGraph->execute();
            {
                SkrZoneScopedN("CollectGarbage");
                if (frame_index >= RG_MAX_FRAME_IN_FLIGHT * 10)
                    renderGraph->collect_garbage(frame_index - RG_MAX_FRAME_IN_FLIGHT * 10);
            }
        }

        // do present
        imgui_render_backend->present_all();
    }
    cgpu_wait_queue_idle(gfx_queue);
    render_graph::RenderGraph::destroy(renderGraph);
    imgui_app->shutdown();

    skr::input::Input::Finalize();

    return 0;
}