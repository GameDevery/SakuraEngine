
#include "SkrCore/module/module.hpp"
#include "SkrCore/log.h"
#include "SkrCore/time.h"
#include "SkrCore/memory/sp.hpp"

#include "SkrSystem/advanced_input.h"

#include "SkrRenderer/skr_renderer.h"
// #include "SkrGraphics/api.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include <SkrImGui/imgui_backend.hpp>
#include "AnimDebugRuntime/renderer.h"
#include <SDL3/SDL_video.h>
#include "common/utils.h"
#include "imgui_sail.h"
#include "SkrRT/runtime_module.h" // for dpi_aware
#include "SkrRT/misc/cmd_parser.hpp"

#include <SkrAnim/ozz/skeleton.h>
#include <SkrAnim/ozz/animation.h>
#include <SkrAnim/ozz/base/io/archive.h>
#include <SkrAnim/ozz/skeleton_utils.h>
#include <SkrAnim/ozz/local_to_model_job.h>
#include <SkrAnim/ozz/sampling_job.h>
#include "SkrAnim/ozz/base/maths/soa_transform.h"
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
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

private:
    skr::String m_skel_file = u8"D:/ws/data/assets/media/bin/ruby_skeleton.ozz";
    skr::String m_anim_file = u8"D:/ws/data/assets/media/bin/ruby_animation.ozz";
    ozz::animation::Animation m_animation;
    float current_time = 0.0f;
    bool is_playing = false;
    void DisplayAnimationInfo(const ozz::animation::Animation& animation);
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
    parser.add(u8"skeleton", u8"ozz skeleton file path", u8"-s", false);
    parser.add(u8"animation", u8"ozz animation file path", u8"-a", false);
    if (!parser.parse())
    {
        SKR_LOG_ERROR(u8"Failed to parse command line arguments.");
        return;
    }
    auto skel_path = parser.get_optional<skr::String>(u8"skeleton");
    if (skel_path)
    {
        m_skel_file = *skel_path;
        SKR_LOG_INFO(u8"ozz skeleton set to: %s", m_skel_file.c_str());
    }
    else
    {
        SKR_LOG_INFO(u8"No skeleton file specified, using default: %s", m_skel_file.c_str());
    }

    auto anim_path = parser.get_optional<skr::String>(u8"animation");
    if (anim_path)
    {
        m_anim_file = *anim_path;
        SKR_LOG_INFO(u8"ozz animation set to: %s", m_anim_file.c_str());
    }
    else
    {
        SKR_LOG_INFO(u8"No animation file specified.");
    }
}

void SAnimDebugModule::on_unload()
{
    g_anim_debug_module = nullptr;
    SKR_LOG_INFO(u8"anim debug runtime unloaded!");
}

void SAnimDebugModule::DisplayAnimationInfo(const ozz::animation::Animation& animation)
{
    ImGui::Text("Animation Name: %s", animation.name());
    ImGui::Text("Duration: %.2f", animation.duration());
    ImGui::Text("Number of Tracks: %d", animation.num_tracks());
}

int SAnimDebugModule::main_module_exec(int argc, char8_t** argv)
{
    static constexpr float kPi = 3.14159265358979323846f;
    namespace rg = skr::render_graph;
    SkrZoneScopedN("AnimDebugExecution");
    SKR_LOG_INFO(u8"anim debug runtime executed as main module!");
    auto render_device = SkrRendererModule::Get()->get_render_device();
    animd::Renderer renderer;
    SKR_LOG_INFO(u8"anim debug renderer created with backend: %s", gCGPUBackendNames[render_device->get_backend()]);
    renderer.set_aware_DPI(skr_runtime_is_dpi_aware());

    // geometry
    ozz::animation::Skeleton skeleton;

    ozz::io::File file(m_skel_file.data_raw(), "rb");
    if (!file.opened())
    {
        SKR_LOG_ERROR(u8"Cannot open file %s.", m_skel_file.c_str());
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
    job.input = skeleton.joint_rest_poses();
    job.output = ozz::make_span(prealloc_models_);
    job.skeleton = &skeleton;
    if (!job.Run())
    {
        SKR_LOG_ERROR(u8"Failed to run LocalToModelJob.");
    }
    // renderer.read_anim(skeleton);
    renderer.create_skeleton(skeleton);
    renderer.update_anim(skeleton, ozz::make_span(prealloc_models_));

    // Load animation
    if (!m_anim_file.is_empty())
    {
        ozz::io::File anim_file(m_anim_file.data_raw(), "rb");
        if (!anim_file.opened())
        {
            SKR_LOG_ERROR(u8"Cannot open animation file %s.", m_anim_file.c_str());
        }
        ozz::io::IArchive anim_archive(&anim_file);
        if (!anim_archive.TestTag<ozz::animation::Animation>())
        {
            SKR_LOG_ERROR(u8"Archive doesn't contain the expected animation object type.");
        }
        anim_archive >> m_animation;
        SKR_LOG_INFO(u8"Animation loaded with %d tracks.", m_animation.num_tracks());
    }

    ozz::animation::SamplingJob::Context context_;
    context_.Resize(skeleton.num_joints());
    ozz::vector<ozz::math::SoaTransform> locals_;
    locals_.resize(skeleton.num_soa_joints());
    
    renderer.create_render_pipeline();
    renderer.create_resources();

    auto device = render_device->get_cgpu_device();
    auto gfx_queue = render_device->get_gfx_queue();

    // TODO: init profiler
    skr::UPtr<skr::ImGuiApp> imgui_app = nullptr;
    {
        skr::render_graph::RenderGraphBuilder graph_builder;
        graph_builder.with_device(device)
            .with_gfx_queue(gfx_queue)
            .enable_memory_aliasing();

        skr::SystemWindowCreateInfo main_window_info = {
            .title = skr::format(u8"Live2D Viewer Inner [{}]", gCGPUBackendNames[device->adapter->instance->backend]),
            .size = { 1500, 1500 },
        };

        imgui_app = skr::UPtr<skr::ImGuiApp>::New(main_window_info, renderer.get_render_device(), graph_builder);
        imgui_app->initialize();
        imgui_app->enable_docking();
        // Apply Sail Style
        ImGui::Sail::LoadFont(12.0f);
        ImGui::Sail::StyleColorsSail();
    }
    auto render_graph = imgui_app->render_graph();

    animd::Camera camera;
    renderer.set_pcamera(&camera);
    camera.position = skr::float3(0.1f, 2.1f, 3.1f);         // eye position
    skr_float3_t target(0.0f, 1.0f, 0.0f);                   // look at position
    camera.front = skr::normalize(target - camera.position); // look at direction

    bool show_demo_window = true;
    uint64_t frame_index = 0;

    // Time
    SHiresTimer tick_timer;
    int64_t elapsed_us = 0;
    int64_t elapsed_frame = 0;
    int64_t fps = 60;
    skr_init_hires_timer(&tick_timer);

    skr::input::Input::Initialize();

    while (!imgui_app->want_exit().comsume())
    {
        SkrZoneScopedN("LoopBody");
        {
            SkrZoneScopedN("PumpMessage");
            // Pump messages
            imgui_app->pump_message();
        }

        // Camera control
        {
            ImGuiIO& io = ImGui::GetIO();
            const float camera_speed = 2.5f * io.DeltaTime;
            const float camera_pan_speed = 0.5f * io.DeltaTime;
            const float camera_sensitivity = 0.1f;

            skr_float3_t world_up = { 0.0f, 1.0f, 0.0f };
            skr_float3_t camera_right = skr::normalize(skr::cross(camera.front, world_up));
            skr_float3_t camera_up = skr::normalize(skr::cross(camera_right, camera.front));

            // Movement
            if (ImGui::IsKeyDown(ImGuiKey_W)) camera.position += camera.front * camera_speed;
            if (ImGui::IsKeyDown(ImGuiKey_S)) camera.position -= camera.front * camera_speed;
            if (ImGui::IsKeyDown(ImGuiKey_A)) camera.position -= camera_right * camera_speed;
            if (ImGui::IsKeyDown(ImGuiKey_D)) camera.position += camera_right * camera_speed;

            // Rotation
            if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
            {
                static float yaw = -90.0f;
                static float pitch = 0.0f;
                yaw += io.MouseDelta.x * camera_sensitivity;
                pitch -= io.MouseDelta.y * camera_sensitivity;
                if (pitch > 89.0f) pitch = 89.0f;
                if (pitch < -89.0f) pitch = -89.0f;

                skr_float3_t direction;
                direction.x = cosf(yaw * (float)kPi / 180.f) * cosf(pitch * (float)kPi / 180.f);
                direction.y = sinf(pitch * (float)kPi / 180.f);
                direction.z = sinf(yaw * (float)kPi / 180.f) * cosf(pitch * (float)kPi / 180.f);
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
            ImGui::Begin("Animation Control");
            if (!m_anim_file.is_empty())
            {
                DisplayAnimationInfo(m_animation);

                if (ImGui::Button(is_playing ? "Stop" : "Play"))
                {
                    is_playing = !is_playing;
                }
                ImGui::SameLine();
                if (ImGui::Button("Resume"))
                {
                    current_time = 0.0f;
                    is_playing = true;
                }
            }
            else
            {
                ImGui::Text("No animation loaded.");
            }
            ImGui::End();
        }
        {
            // update viewport
            SkrZoneScopedN("Viewport Render");
            imgui_app->acquire_frames();

            auto main_window = imgui_app->main_window();
            const auto size = main_window->get_physical_size();
            renderer.set_width(size.x);
            renderer.set_height(size.y);
            camera.aspect = (float)size.x / (float)size.y;

            const auto backbuffer_index = imgui_app->backbuffer_index(main_window);
            const auto swapchain = imgui_app->get_swapchain(main_window);
            const auto backbuffer = render_graph->get_imported(swapchain->back_buffers[backbuffer_index]);
            renderer.build_render_graph(render_graph, backbuffer);
        }
        {
            SkrZoneScopedN("ImGuiRender");
            imgui_app->set_load_action(CGPU_LOAD_ACTION_LOAD);
            imgui_app->render_imgui();
        }
        {
            frame_index = render_graph->execute();
            if (frame_index >= RG_MAX_FRAME_IN_FLIGHT * 10)
                render_graph->collect_garbage(frame_index - RG_MAX_FRAME_IN_FLIGHT * 10);
        }
        // present
        imgui_app->present_all();

        // Update animation
        if (!m_anim_file.is_empty() && is_playing)
        {
            /// TODO: sample animation
            int64_t us = skr_hires_timer_get_usec(&tick_timer, true);
            double deltaTime = (double)us / 1000 / 1000; // in seconds
            current_time += deltaTime;
            if (current_time > m_animation.duration())
            {
                current_time = 0.0f;
            }
            float ratio = current_time / m_animation.duration();

            ozz::animation::SamplingJob sampling_job;
            sampling_job.animation = &m_animation;
            sampling_job.context = &context_;
            sampling_job.ratio = ratio;
            sampling_job.output = ozz::make_span(locals_);

            if (sampling_job.Run())
            {
                ozz::animation::LocalToModelJob local_to_model_job;
                local_to_model_job.input = ozz::make_span(locals_);
                local_to_model_job.output = ozz::make_span(prealloc_models_);
                local_to_model_job.skeleton = &skeleton;
                if (!local_to_model_job.Run())
                {
                    SKR_LOG_ERROR(u8"Failed to run local to model job.");
                }

                renderer.update_anim(skeleton, ozz::make_span(prealloc_models_));
                renderer.create_resources(); // update resource buffer and upload
            }
            else
            {
                SKR_LOG_ERROR(u8"Failed to run sampling job.");
            }
        }
    }

    // wait for rendering done
    cgpu_wait_queue_idle(render_device->get_gfx_queue());

    // destroy imgui
    imgui_app->shutdown();
    renderer.finalize();

    skr::input::Input::Finalize();

    return 0; // Return 0 to indicate success
}