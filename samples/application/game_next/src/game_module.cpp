#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrOS/filesystem.hpp"
#include "SkrCore/module/module.hpp"
#include "SkrCore/memory/sp.hpp"
#include "SkrCore/async/thread_job.hpp"
#include "SkrCore/platform/vfs.h"
#include "SkrCore/time.h"

#include "SkrRT/ecs/type_builder.hpp"
#include "SkrRT/resource/resource_system.h"
#include "SkrRT/resource/local_resource_registry.hpp"

#include "SkrProfile/profile.h"
#include "SkrRenderer/shader_map.h"
#include "SkrScene/scene.h"
#include "SkrRenderer/skr_renderer.h"
#include "SkrRenderer/fwd_types.h"
#include "SkrRenderer/primitive_draw.h"
#include "SkrRenderer/render_effect.h"
#include "SkrRenderer/resources/shader_resource.hpp"

#include "SkrImGui/imgui_render_backend.hpp"
#include <SkrImGui/imgui_backend.hpp>

#include "GameRuntime/game_animation.h"

// defined in render
extern void game_initialize_render_effects(SRendererId renderer, skr::render_graph::RenderGraph* renderGraph, skr_vfs_t* resource_vfs);
extern void game_register_render_effects(SRendererId renderer, skr::render_graph::RenderGraph* renderGraph);
extern void game_finalize_render_effects(SRendererId renderer, skr::render_graph::RenderGraph* renderGraph);

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

    // VFS
    skr_vfs_t*             resource_vfs = nullptr;
    skr_io_ram_service_t*  ram_service  = nullptr;
    skr_io_vram_service_t* vram_service = nullptr;

    skr_vfs_t*                             shader_bytes_vfs = nullptr;
    skr_shader_map_t*                      shadermap        = nullptr;
    skr::renderer::SShaderResourceFactory* shaderFactory    = nullptr;

    // Resource System
    skr::resource::SLocalResourceRegistry* registry;

    // install & unsintall resource factories
    void installResourceFactories();
    void uninstallResourceFactories();

    // temp
    void create_test_scene(SRendererId renderer);
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

    // Init Render Effects
    game_initialize_render_effects(game_renderer, render_graph, resource_vfs);
    create_test_scene(game_renderer);

    {
        // TODO: init script context
    }

    // Time
    SHiresTimer tick_timer;
    int64_t     elapsed_us    = 0;
    int64_t     elapsed_frame = 0;
    int64_t     fps           = 60;
    skr_init_hires_timer(&tick_timer);

    // loop
    bool               quit = false;
    skr::task::event_t pSkinCounter(nullptr);
    sugoi_query_t*     initAnimSkinQuery;
    sugoi_query_t*     skinQuery;
    sugoi_query_t*     moveQuery;
    sugoi_query_t*     cameraQuery;
    sugoi_query_t*     animQuery;
    moveQuery         = sugoiQ_from_literal(game_world, u8"[has]skr::MovementComponent, [inout]skr::TranslationComponent, [in]skr::ScaleComponent, [in]skr_index_comp_t,!skr::CameraComponent");
    cameraQuery       = sugoiQ_from_literal(game_world, u8"[has]skr::MovementComponent, [inout]skr::TranslationComponent, [inout]skr::CameraComponent");
    animQuery         = sugoiQ_from_literal(game_world, u8"[in]skr_render_effect_t, [in]game::anim_state_t, [out]<unseq>skr::anim::AnimComponent, [in]<unseq>skr::anim::SkeletonComponent");
    initAnimSkinQuery = sugoiQ_from_literal(game_world, u8"[inout]skr::anim::AnimComponent, [inout]skr::anim::SkinComponent, [in]skr::renderer::MeshComponent, [in]skr::anim::SkeletonComponent");
    skinQuery         = sugoiQ_from_literal(game_world, u8"[in]skr::anim::AnimComponent, [inout]skr::anim::SkinComponent, [in]skr::renderer::MeshComponent, [in]skr::anim::SkeletonComponent");

    uint64_t frame_index      = 0;
    bool     show_demo_window = true;
    while (!imgui_backend.want_exit().comsume())
    {
        SkrZoneScopedN("LoopBody");
        static auto main_thread_id    = skr_current_thread_id();
        auto        current_thread_id = skr_current_thread_id();
        SKR_ASSERT(main_thread_id == current_thread_id && "This is not the main thread");

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
    // release render effects
    game_finalize_render_effects(game_renderer, render_graph);
    // destroy imgui backend
    imgui_backend.destroy();

    return 0;
}

void SGameModule::installResourceFactories()
{
    SkrZoneScopedN("InstallResourceFactories");
    SKR_LOG_INFO(u8"game runtime installing resource factories!");
    std::error_code ec             = {};
    auto            resourceRoot   = (skr::filesystem::current_path(ec) / "../resources");
    auto            u8ResourceRoot = resourceRoot.u8string();
    skr_vfs_desc_t  vfs_desc       = {};
    vfs_desc.mount_type            = SKR_MOUNT_TYPE_CONTENT;
    vfs_desc.override_mount_dir    = u8ResourceRoot.c_str();
    resource_vfs                   = skr_create_vfs(&vfs_desc);

    auto ioServiceDesc       = make_zeroed<skr_ram_io_service_desc_t>();
    ioServiceDesc.name       = u8"GameRuntime-RAMIOService";
    ioServiceDesc.sleep_time = 1000 / 60;
    ram_service              = skr_io_ram_service_t::create(&ioServiceDesc);
    ram_service->run();

    auto vramServiceDesc               = make_zeroed<skr_vram_io_service_desc_t>();
    vramServiceDesc.name               = u8"GameRuntime-VRAMIOService";
    vramServiceDesc.awake_at_request   = true;
    vramServiceDesc.ram_service        = ram_service;
    vramServiceDesc.callback_job_queue = job_queue.get();
    vramServiceDesc.use_dstorage       = true;
    vramServiceDesc.gpu_device         = game_render_device->get_cgpu_device();
    vram_service                       = skr_io_vram_service_t::create(&vramServiceDesc);
    vram_service->run();

    registry = SkrNew<skr::resource::SLocalResourceRegistry>(resource_vfs);
    skr::resource::GetResourceSystem()->Initialize(registry, ram_service);
    //

    using namespace skr::literals;
    auto resource_system = skr::resource::GetResourceSystem();

    auto gameResourceRoot = resourceRoot / "Game";
    auto u8TextureRoot    = gameResourceRoot.u8string();
    // TODO: texture sampler
    // TODO: texture
    // TODO: mesh
    // TODO: material type
    // TODO: material
    // TODO: anim
    // shader factory
    {
        const auto  backend    = game_render_device->get_backend();
        std::string shaderType = "invalid";
        if (backend == CGPU_BACKEND_D3D12) shaderType = "dxil";
        if (backend == CGPU_BACKEND_VULKAN) shaderType = "spirv";
        auto shaderResourceRoot   = gameResourceRoot / shaderType;
        auto u8ShaderResourceRoot = shaderResourceRoot.u8string();

        // create shader vfs
        skr_vfs_desc_t shader_vfs_desc     = {};
        shader_vfs_desc.mount_type         = SKR_MOUNT_TYPE_CONTENT;
        shader_vfs_desc.override_mount_dir = u8ShaderResourceRoot.c_str();
        shader_bytes_vfs                   = skr_create_vfs(&shader_vfs_desc);

        // create shader map
        skr_shader_map_root_t shadermapRoot = {};
        shadermapRoot.bytecode_vfs          = shader_bytes_vfs;
        shadermapRoot.ram_service           = ram_service;
        shadermapRoot.device                = game_render_device->get_cgpu_device();
        shadermapRoot.job_queue             = job_queue.get();
        shadermap                           = skr_shader_map_create(&shadermapRoot);

        // create shader resource factory
        skr::renderer::SShaderResourceFactory::Root factoryRoot = {};
        factoryRoot.render_device                               = game_render_device;
        factoryRoot.shadermap                                   = shadermap;
        shaderFactory                                           = skr::renderer::SShaderResourceFactory::Create(factoryRoot);
        resource_system->RegisterFactory(shaderFactory);
    }
}

void SGameModule::uninstallResourceFactories()
{
    SkrZoneScopedN("UninstallResourceFactories");
    SKR_LOG_INFO(u8"game runtime uninstalling resource factories!");
    sugoiS_release(game_world);
    auto resource_system = skr::resource::GetResourceSystem();
    resource_system->Shutdown();

    skr_io_ram_service_t::destroy(ram_service);
    skr_io_vram_service_t::destroy(vram_service);
    skr_free_vfs(resource_vfs);
}

void SGameModule::create_test_scene(SRendererId renderer)
{
    auto renderableT_builder = make_zeroed<sugoi::TypeSetBuilder>();
    renderableT_builder
        .with<skr::TranslationComponent, skr::RotationComponent, skr::ScaleComponent>()
        .with<skr::IndexComponent, skr::MovementComponent>()
        .with<skr_render_effect_t>()
        .with(SUGOI_COMPONENT_GUID);
    auto renderableT  = make_zeroed<sugoi_entity_type_t>();
    renderableT.type  = renderableT_builder.build();
    uint32_t init_idx = 0;

    auto primSetup = [&](sugoi_chunk_view_t* view) {
        auto translations = sugoi::get_owned_rw<skr::TranslationComponent>(view);
        auto rotations    = sugoi::get_owned_rw<skr::RotationComponent>(view);
        auto scales       = sugoi::get_owned_rw<skr::ScaleComponent>(view);
        auto indices      = sugoi::get_owned_rw<skr::IndexComponent>(view);
        auto movements    = sugoi::get_owned_rw<skr::MovementComponent>(view);
        auto states       = sugoi::get_owned_rw<game::anim_state_t>(view);
        auto guids        = (skr_guid_t*)sugoiV_get_owned_ro(view, SUGOI_COMPONENT_GUID);
        for (uint32_t i = 0; i < view->count; i++)
        {
            if (guids)
                sugoi_make_guid(&guids[i]);
            if (movements)
            {
                translations[i].value = { 0.f, 0.f, 0.f };
                rotations[i].euler    = { 0.f, 0.f, 0.f };
                scales[i].value       = { 8.f, 8.f, 8.f };
                if (indices) indices[i].value = init_idx++;
            }
            else
            {
                translations[i].value = { 0.f, 30.f, -10.f };
                rotations[i].euler    = { 90.f, 0.f, 0.f };
                scales[i].value       = { .25f, .25f, .25f };
            }
            if (states)
            {
                using namespace skr::literals;
                states[i].animation_resource = u8"83c0db0b-08cd-4951-b1c3-65c2008d0113"_guid;
                states[i].animation_resource.resolve(true, renderer->get_sugoi_storage());
            }
        }
        if (auto feature_arrs = sugoiV_get_owned_rw(view, sugoi_id_of<skr_render_effect_t>::get()))
        {
            if (movements)
                skr_render_effect_attach(renderer, view, u8"ForwardEffect");
            else
                skr_render_effect_attach(renderer, view, u8"ForwardEffectSkin");
        }
    };

    sugoiS_allocate_type(renderer->get_sugoi_storage(), &renderableT, 512, SUGOI_LAMBDA(primSetup));

    SKR_LOG_DEBUG(u8"Create Scene 0!");
    auto playerT_builder = make_zeroed<sugoi::TypeSetBuilder>();
    playerT_builder
        .with<skr::TranslationComponent, skr::RotationComponent, skr::ScaleComponent>()
        .with<skr::MovementComponent>()
        .with<skr::CameraComponent>();
    auto playerT = make_zeroed<sugoi_entity_type_t>();
    playerT.type = playerT_builder.build();
    sugoiS_allocate_type(renderer->get_sugoi_storage(), &playerT, 1, SUGOI_LAMBDA(primSetup));

    SKR_LOG_DEBUG(u8"Create Scene 1!");

    auto static_renderableT_builderT = make_zeroed<sugoi::TypeSetBuilder>();
    static_renderableT_builderT
        .with<skr::TranslationComponent, skr::RotationComponent, skr::ScaleComponent>()
        .with<skr_render_effect_t, game::anim_state_t>();
    auto static_renderableT = make_zeroed<sugoi_entity_type_t>();
    static_renderableT.type = static_renderableT_builderT.build();
    sugoiS_allocate_type(renderer->get_sugoi_storage(), &static_renderableT, 1, SUGOI_LAMBDA(primSetup));

    SKR_LOG_DEBUG(u8"Create Scene 2!");
}
