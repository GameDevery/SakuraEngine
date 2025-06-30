#include "SkrCore/module/module.hpp"
#include "SkrCore/log.h"
#include "SkrRenderer/skr_renderer.h"
// #include "SkrGraphics/api.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrImGui/imgui_render_backend.hpp"
#include <SkrImGui/imgui_backend.hpp>
#include <SDL3/SDL_video.h>
#include "common/utils.h"

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

void create_dummy_render_pipeline(CGPUDeviceId device, CGPURootSignatureId& root_sig, CGPUSwapChainId swapchain, CGPURenderPipelineId& pipeline)
{
    uint32_t *vs_bytes, vs_length;
    uint32_t *fs_bytes, fs_length;
    read_shader_bytes(SKR_UTF8("AnimDebug/vertex_shader"), &vs_bytes, &vs_length, CGPU_BACKEND_D3D12);
    read_shader_bytes(SKR_UTF8("AnimDebug/fragment_shader"), &fs_bytes, &fs_length, CGPU_BACKEND_D3D12);
    CGPUShaderLibraryDescriptor vs_desc = {};
    vs_desc.stage                       = CGPU_SHADER_STAGE_VERT;
    vs_desc.name                        = SKR_UTF8("VertexShaderLibrary");
    vs_desc.code                        = vs_bytes;
    vs_desc.code_size                   = vs_length;
    CGPUShaderLibraryDescriptor ps_desc = {};
    ps_desc.name                        = SKR_UTF8("FragmentShaderLibrary");
    ps_desc.stage                       = CGPU_SHADER_STAGE_FRAG;
    ps_desc.code                        = fs_bytes;
    ps_desc.code_size                   = fs_length;
    CGPUShaderLibraryId vertex_shader   = cgpu_create_shader_library(device, &vs_desc);
    CGPUShaderLibraryId fragment_shader = cgpu_create_shader_library(device, &ps_desc);
    free(vs_bytes);
    free(fs_bytes);
    CGPUShaderEntryDescriptor ppl_shaders[2];
    ppl_shaders[0].stage                 = CGPU_SHADER_STAGE_VERT;
    ppl_shaders[0].entry                 = SKR_UTF8("main");
    ppl_shaders[0].library               = vertex_shader;
    ppl_shaders[1].stage                 = CGPU_SHADER_STAGE_FRAG;
    ppl_shaders[1].entry                 = SKR_UTF8("main");
    ppl_shaders[1].library               = fragment_shader;
    CGPURootSignatureDescriptor rs_desc  = {};
    rs_desc.shaders                      = ppl_shaders;
    rs_desc.shader_count                 = 2;
    root_sig                             = cgpu_create_root_signature(device, &rs_desc);
    CGPUVertexLayout vertex_layout       = {};
    vertex_layout.attribute_count        = 0;
    CGPURenderPipelineDescriptor rp_desc = {};
    rp_desc.root_signature               = root_sig;
    rp_desc.prim_topology                = CGPU_PRIM_TOPO_TRI_LIST;
    rp_desc.vertex_layout                = &vertex_layout;
    rp_desc.vertex_shader                = &ppl_shaders[0];
    rp_desc.fragment_shader              = &ppl_shaders[1];
    rp_desc.render_target_count          = 1;
    auto backend_format                  = (ECGPUFormat)swapchain->back_buffers[0]->info->format;
    rp_desc.color_formats                = &backend_format;
    pipeline                             = cgpu_create_render_pipeline(device, &rp_desc);
    cgpu_free_shader_library(vertex_shader);
    cgpu_free_shader_library(fragment_shader);
}

int SAnimDebugModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("AnimDebugExecution");
    SKR_LOG_INFO(u8"anim debug runtime executed as main module!");

    // Create instance
    CGPUInstanceDescriptor instance_desc{};
    instance_desc.backend                     = CGPU_BACKEND_D3D12;
    instance_desc.enable_debug_layer          = true;
    instance_desc.enable_gpu_based_validation = false;
    instance_desc.enable_set_name             = true;
    CGPUInstanceId instance                   = cgpu_create_instance(&instance_desc);
    // Filter adapters
    uint32_t adapters_count = 0;
    cgpu_enum_adapters(instance, CGPU_NULLPTR, &adapters_count);
    CGPUAdapterId adapters[64];
    cgpu_enum_adapters(instance, adapters, &adapters_count);
    auto adapter = adapters[0];

    // Create device
    CGPUQueueGroupDescriptor queue_group_desc = {};
    queue_group_desc.queue_type               = CGPU_QUEUE_TYPE_GRAPHICS;
    queue_group_desc.queue_count              = 1;
    CGPUDeviceDescriptor device_desc          = {};
    device_desc.queue_groups                  = &queue_group_desc;
    device_desc.queue_group_count             = 1;
    auto device                               = cgpu_create_device(adapter, &device_desc);
    auto gfx_queue                            = cgpu_get_queue(device, CGPU_QUEUE_TYPE_GRAPHICS, 0);
    // create render graph
    auto render_graph = skr::render_graph::RenderGraph::create(
        [=](skr::render_graph::RenderGraphBuilder& builder) {
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
        config.queue        = gfx_queue;
        render_backend->init(config);
        imgui_backend.create({}, std::move(render_backend));
        imgui_backend.main_window().show();
        imgui_backend.enable_docking();
        imgui_backend.enable_high_dpi();
        // imgui_backend.enable_multi_viewport();
    }

    CGPURootSignatureId  root_sig = nullptr;
    CGPURenderPipelineId pipeline = nullptr;
    {
        // Init Your Scene Here
        auto viewport = ImGui::GetMainViewport();
        auto wnd_id   = (SDL_WindowID) reinterpret_cast<size_t>(viewport->PlatformHandle);
        auto wnd      = SDL_GetWindowFromID(wnd_id);
        SKR_ASSERT(wnd);

        create_dummy_render_pipeline(
            device,
            root_sig,
            render_backend_rg->get_swapchain(viewport),
            pipeline
        );
    }
    // draw loop
    bool     show_demo_window    = true;
    bool     show_another_window = true;
    ImVec4   clear_color         = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
    uint64_t frame_index         = 0;
    while (!imgui_backend.want_exit().comsume())
    {
        imgui_backend.pump_message();

        {
            auto viewport = ImGui::GetMainViewport();
            render_backend_rg->acquire_next_frame(viewport);
            CGPUTextureId to_import   = render_backend_rg->get_backbuffer(viewport);
            auto          back_buffer = render_graph->create_texture(
                [=](skr::render_graph::RenderGraph& g, skr::render_graph::TextureBuilder& builder) {
                    builder.set_name(SKR_UTF8("backbuffer"))
                        .import(to_import, CGPU_RESOURCE_STATE_PRESENT)
                        .allow_render_target();
                }
            );
            render_graph->add_render_pass(
                [=](skr::render_graph::RenderGraph& g, skr::render_graph::RenderPassBuilder& builder) {
                    builder.set_name(SKR_UTF8("color_pass"))
                        .set_pipeline(pipeline)
                        .write(0, back_buffer, CGPU_LOAD_ACTION_CLEAR);
                },
                [=](skr::render_graph::RenderGraph& g, skr::render_graph::RenderPassContext& stack) {
                    cgpu_render_encoder_set_viewport(stack.encoder, 0.0f, 0.0f, (float)to_import->info->width / 3, (float)to_import->info->height, 0.f, 1.f);
                    cgpu_render_encoder_set_scissor(stack.encoder, 0, 0, to_import->info->width, to_import->info->height);
                    cgpu_render_encoder_draw(stack.encoder, 3, 0);
                }
            );
        }

        imgui_backend.begin_frame(); // ImGui::NewFrame();

        {
            ImGuiIO& io = ImGui::GetIO();

            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            if (show_demo_window)
                ImGui::ShowDemoWindow(&show_demo_window);

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
            {
                static float f       = 0.0f;
                static int   counter = 0;

                ImGui::Begin("Hello, world!"); // Create a window called "Hello, world!" and append into it.

                ImGui::Text("This is some useful text.");          // Display some text (you can use a format strings too)
                ImGui::Checkbox("Demo Window", &show_demo_window); // Edit bools storing our window open/close state
                ImGui::Checkbox("Another Window", &show_another_window);

                ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
                ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                if (ImGui::Button("Button")) // Buttons return true when clicked (most widgets return true when edited/activated)
                    counter++;
                ImGui::SameLine();
                ImGui::Text("counter = %d", counter);

                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::End();
            }

            // 3. Show another simple window.
            if (show_another_window)
            {
                ImGui::Begin("Another Window", &show_another_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
                ImGui::Text("Hello from another window!");
                if (ImGui::Button("Close Me"))
                    show_another_window = false;
                ImGui::End();
            }
        }

        imgui_backend.end_frame();

        // add render passes
        imgui_backend.render();

        // draw render graph
        render_graph->compile();
        render_graph->execute();
        if (frame_index >= RG_MAX_FRAME_IN_FLIGHT * 10)
            render_graph->collect_garbage(frame_index - RG_MAX_FRAME_IN_FLIGHT * 10);

        // present
        render_backend_rg->present_main_viewport();
        render_backend_rg->present_sub_viewports();
        ++frame_index;
    }

    // wait for rendering done
    cgpu_wait_queue_idle(gfx_queue);

    // destroy render graph
    skr::render_graph::RenderGraph::destroy(render_graph);

    // destroy imgui
    imgui_backend.destroy();

    // shutdown cgpu
    cgpu_free_queue(gfx_queue);
    cgpu_free_device(device);
    cgpu_free_instance(instance);
    return 0; // Return 0 to indicate success
}