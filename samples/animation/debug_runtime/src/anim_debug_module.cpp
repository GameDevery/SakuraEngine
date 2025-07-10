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
#include "SkrRT/runtime_module.h" // for dpi_aware
#include "SkrRT/misc/cmd_parser.hpp"

#include <SkrAnim/ozz/skeleton.h>
#include <SkrAnim/ozz/base/io/archive.h>
#include <SkrAnim/ozz/skeleton_utils.h>
#include <SkrAnim/ozz/local_to_model_job.h>
#include <AnimDebugRuntime/util.h>
#include <SkrAnim/ozz/base/containers/vector.h>

void DisplayBoneHierarchy(const ozz::animation::Skeleton& skeleton, int bone_index)
{
    const char* bone_name = skeleton.joint_names()[bone_index];

    // Check if this bone has children to determine if it's a leaf or a node
    bool has_children = false;
    for (int i = 0; i < skeleton.num_joints(); ++i)
    {
        if (skeleton.joint_parents()[i] == bone_index)
        {
            has_children = true;
            break;
        }
    }

    ImGuiTreeNodeFlags node_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
    if (!has_children)
    {
        node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    }

    bool node_open = ImGui::TreeNodeEx((void*)(intptr_t)bone_index, node_flags, "%s", bone_name);

    if (node_open)
    {
        // If the node is open, recurse for all children
        for (int i = 0; i < skeleton.num_joints(); ++i)
        {
            if (skeleton.joint_parents()[i] == bone_index)
            {
                DisplayBoneHierarchy(skeleton, i);
            }
        }
        ImGui::TreePop();
    }
}

class SAnimDebugModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int  main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

private:
    skr::String m_anim_file = u8"D:/ws/data/assets/media/bin/ruby_skeleton.ozz"; // default anim file
};

static SAnimDebugModule* g_anim_debug_module = nullptr;
///////
/// Module entry point
///////
IMPLEMENT_DYNAMIC_MODULE(SAnimDebugModule, AnimDebug);

void SAnimDebugModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"anim debug runtime loaded!");
    // parse command line arguments
    skr::cmd::parser parser(argc, (char**)argv);
    parser.add(u8"anim", u8"animation file path", u8"-a", true);
    if (!parser.parse())
    {
        SKR_LOG_ERROR(u8"Failed to parse command line arguments.");
        return;
    }
    auto anim_path = parser.get_optional<skr::String>(u8"anim");
    if (anim_path)
    {
        m_anim_file = *anim_path;
        SKR_LOG_INFO(u8"Animation file set to: %s", m_anim_file.c_str());
    }
    else
    {
        SKR_LOG_INFO(u8"No animation file specified, using default: %s", m_anim_file.c_str());
    }
}

void SAnimDebugModule::on_unload()
{
    g_anim_debug_module = nullptr;
    SKR_LOG_INFO(u8"anim debug runtime unloaded!");
}

int SAnimDebugModule::main_module_exec(int argc, char8_t** argv)
{
    constexpr float M_PI = 3.14159265358979323846f;
    namespace rg         = skr::render_graph;
    SkrZoneScopedN("AnimDebugExecution");
    SKR_LOG_INFO(u8"anim debug runtime executed as main module!");
    animd::Renderer renderer;
    SKR_LOG_INFO(u8"anim debug renderer created with backend: %s", gCGPUBackendNames[renderer.get_backend()]);
    renderer.set_aware_DPI(skr_runtime_is_dpi_aware());

    // geometry
    ozz::animation::Skeleton skeleton;

    ozz::io::File file(m_anim_file.data_raw(), "rb");
    if (!file.opened())
    {
        SKR_LOG_ERROR(u8"Cannot open file %s.", m_anim_file.c_str());
    }

    // deserialize
    ozz::io::IArchive archive(&file);
    // test archive
    if (!archive.TestTag<ozz::animation::Skeleton>())
    {
        SKR_LOG_ERROR(u8"Archive doesn't contain the expected object type.");
    }
    // Create Runtime Skeleton
    archive >> skeleton;
    SKR_LOG_INFO(u8"Skeleton loaded with %d joints.", skeleton.num_joints());

    ozz::vector<ozz::math::Float4x4> prealloc_models_;
    prealloc_models_.resize(skeleton.num_joints());
    ozz::animation::LocalToModelJob job;
    job.input    = skeleton.joint_rest_poses();
    job.output   = ozz::make_span(prealloc_models_);
    job.skeleton = &skeleton;
    if (!job.Run())
    {
        SKR_LOG_ERROR(u8"Failed to run LocalToModelJob.");
    }
    // renderer.read_anim(skeleton);
    renderer.create_skeleton(skeleton);
    renderer.update_anim(skeleton, ozz::make_span(prealloc_models_));

    renderer.create_api_objects();
    renderer.create_render_pipeline();
    renderer.create_resources();

    auto device    = renderer.get_device();
    auto gfx_queue = renderer.get_gfx_queue();

    auto render_graph = skr::render_graph::RenderGraph::create(
        [&device, &gfx_queue](skr::render_graph::RenderGraphBuilder& builder) {
            builder.with_device(device)
                .with_gfx_queue(gfx_queue)
                .enable_memory_aliasing();
        }
    );
    // TODO: init profiler
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
                .size  = { 1024, 768 },
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

    animd::Camera camera;
    renderer.set_pcamera(&camera);
    camera.position = skr::float3(0.1f, 2.1f, 3.1f);         // eye position
    skr_float3_t target(0.0f, 1.0f, 0.0f);                   // look at position
    camera.front = skr::normalize(target - camera.position); // look at direction

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

        // Camera control
        {
            ImGuiIO&    io                 = ImGui::GetIO();
            const float camera_speed       = 2.5f * io.DeltaTime;
            const float camera_pan_speed   = 0.5f * io.DeltaTime;
            const float camera_sensitivity = 0.1f;

            skr_float3_t world_up     = { 0.0f, 1.0f, 0.0f };
            skr_float3_t camera_right = skr::normalize(skr::cross(camera.front, world_up));
            skr_float3_t camera_up    = skr::normalize(skr::cross(camera_right, camera.front));

            // Movement
            if (ImGui::IsKeyDown(ImGuiKey_W)) camera.position += camera.front * camera_speed;
            if (ImGui::IsKeyDown(ImGuiKey_S)) camera.position -= camera.front * camera_speed;
            if (ImGui::IsKeyDown(ImGuiKey_A)) camera.position -= camera_right * camera_speed;
            if (ImGui::IsKeyDown(ImGuiKey_D)) camera.position += camera_right * camera_speed;

            // Rotation
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                static float yaw   = -90.0f;
                static float pitch = 0.0f;
                yaw += io.MouseDelta.x * camera_sensitivity;
                pitch -= io.MouseDelta.y * camera_sensitivity;
                if (pitch > 89.0f) pitch = 89.0f;
                if (pitch < -89.0f) pitch = -89.0f;

                skr_float3_t direction;
                direction.x  = cosf(yaw * (float)M_PI / 180.f) * cosf(pitch * (float)M_PI / 180.f);
                direction.y  = sinf(pitch * (float)M_PI / 180.f);
                direction.z  = sinf(yaw * (float)M_PI / 180.f) * cosf(pitch * (float)M_PI / 180.f);
                camera.front = skr::normalize(direction);
            }

            // Panning
            if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
            {
                camera.position -= camera_right * io.MouseDelta.x * camera_pan_speed;
                camera.position += camera_up * io.MouseDelta.y * camera_pan_speed;
            }
        }

        {
            // ImGui::ShowDemoWindow(&show_demo_window);
            ImGui::Begin("Skeleton Hierarchy");
            if (skeleton.num_joints() > 0)
            {
                // Find and display root bones (those with no parent)
                for (int i = 0; i < skeleton.num_joints(); ++i)
                {
                    if (skeleton.joint_parents()[i] == ozz::animation::Skeleton::kNoParent)
                    {
                        DisplayBoneHierarchy(skeleton, i);
                    }
                }
            }
            else
            {
                ImGui::Text("No skeleton loaded.");
            }
            ImGui::End();
        }
        {
            ImGui::Begin("Camera Info");
            ImGui::Text("Position: (%.2f, %.2f, %.2f)", camera.position.x, camera.position.y, camera.position.z);
            ImGui::Text("Front:    (%.2f, %.2f, %.2f)", camera.front.x, camera.front.y, camera.front.z);
            ImGui::End();
        }
        {
            SkrZoneScopedN("ImGuiEndFrame");
            imgui_backend.end_frame();
        }
        imgui_backend.collect(); // contact @zihuang.zhu for any issue
        {
            // update viewport
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
            renderer.set_width(native_backbuffer->info->width);
            renderer.set_height(native_backbuffer->info->height);

            camera.aspect = (float)native_backbuffer->info->width / (float)native_backbuffer->info->height;

            renderer.build_render_graph(render_graph, back_buffer);
        }
        {
            SkrZoneScopedN("ImGuiRender");
            imgui_backend.render(); // add present pass
        }
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
    renderer.finalize();

    return 0; // Return 0 to indicate success
}