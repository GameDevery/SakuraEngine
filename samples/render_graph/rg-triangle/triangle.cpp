#include "common/utils.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"

thread_local SDL_Window*     sdl_window;
thread_local CGPUSurfaceId   surface;
thread_local CGPUSwapChainId swapchain;
thread_local uint32_t        backbuffer_index;
thread_local CGPUFenceId     present_fence;

#if _WIN32
thread_local ECGPUBackend backend = CGPU_BACKEND_D3D12;
#else
thread_local ECGPUBackend backend = CGPU_BACKEND_VULKAN;
#endif

thread_local CGPUInstanceId instance;
thread_local CGPUAdapterId  adapter;
thread_local CGPUDeviceId   device;
thread_local CGPUQueueId    gfx_queue;

thread_local CGPURootSignatureId  root_sig;
thread_local CGPURenderPipelineId pipeline;

void create_api_objects()
{
    // Create instance
    CGPUInstanceDescriptor instance_desc      = {};
    instance_desc.backend                     = backend;
    instance_desc.enable_debug_layer          = true;
    instance_desc.enable_gpu_based_validation = true;
    instance_desc.enable_set_name             = true;
    instance                                  = cgpu_create_instance(&instance_desc);

    // Filter adapters
    uint32_t adapters_count = 0;
    cgpu_enum_adapters(instance, CGPU_NULLPTR, &adapters_count);
    CGPUAdapterId adapters[64];
    cgpu_enum_adapters(instance, adapters, &adapters_count);
    adapter = adapters[0];

    // Create device
    CGPUQueueGroupDescriptor queue_group_desc = {};
    queue_group_desc.queue_type               = CGPU_QUEUE_TYPE_GRAPHICS;
    queue_group_desc.queue_count              = 1;
    CGPUDeviceDescriptor device_desc          = {};
    device_desc.queue_groups                  = &queue_group_desc;
    device_desc.queue_group_count             = 1;
    device                                    = cgpu_create_device(adapter, &device_desc);
    gfx_queue                                 = cgpu_get_queue(device, CGPU_QUEUE_TYPE_GRAPHICS, 0);
    present_fence                             = cgpu_create_fence(device);

    // Create swapchain
#if defined(_WIN32) || defined(_WIN64)
    surface = cgpu_surface_from_hwnd(device, (HWND)SDLGetNativeWindowHandle(sdl_window));
#elif defined(__APPLE__)
    struct CGPUNSView* ns_view = (struct CGPUNSView*)nswindow_get_content_view(SDLGetNativeWindowHandle(sdl_window));
    surface                    = cgpu_surface_from_ns_view(device, ns_view);
#endif
    CGPUSwapChainDescriptor chain_desc = {};
    chain_desc.present_queues          = &gfx_queue;
    chain_desc.present_queues_count    = 1;
    chain_desc.width                   = BACK_BUFFER_WIDTH;
    chain_desc.height                  = BACK_BUFFER_HEIGHT;
    chain_desc.surface                 = surface;
    chain_desc.image_count             = 3;
    chain_desc.format                  = CGPU_FORMAT_R8G8B8A8_UNORM;
    chain_desc.enable_vsync            = true;
    swapchain                          = cgpu_create_swapchain(device, &chain_desc);
}

void create_render_pipeline()
{
    uint32_t *vs_bytes, vs_length;
    uint32_t *fs_bytes, fs_length;
    read_shader_bytes(SKR_UTF8("rg-triangle/vertex_shader"), &vs_bytes, &vs_length, backend);
    read_shader_bytes(SKR_UTF8("rg-triangle/fragment_shader"), &fs_bytes, &fs_length, backend);
    CGPUShaderLibraryDescriptor vs_desc = {};
    vs_desc.name                        = SKR_UTF8("VertexShaderLibrary");
    vs_desc.code                        = vs_bytes;
    vs_desc.code_size                   = vs_length;
    CGPUShaderLibraryDescriptor ps_desc = {};
    ps_desc.name                        = SKR_UTF8("FragmentShaderLibrary");
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

void finalize()
{
    // Free cgpu objects
    cgpu_wait_queue_idle(gfx_queue);
    cgpu_wait_fences(&present_fence, 1);
    cgpu_free_fence(present_fence);
    cgpu_free_swapchain(swapchain);
    cgpu_free_surface(device, surface);
    cgpu_free_render_pipeline(pipeline);
    cgpu_free_root_signature(root_sig);
    cgpu_free_queue(gfx_queue);
    cgpu_free_device(device);
    cgpu_free_instance(instance);
}

int main(int argc, char* argv[])
{
    sdl_window = SDL_CreateWindow(
        (const char*)gCGPUBackendNames[backend],
        BACK_BUFFER_WIDTH,
        BACK_BUFFER_HEIGHT,
        SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE
    );
    create_api_objects();
    create_render_pipeline();
    // initialize
    namespace render_graph = skr::render_graph;
    auto graph             = render_graph::RenderGraph::create(
        [=](render_graph::RenderGraphBuilder& builder) {
            builder.with_device(device)
                .with_gfx_queue(gfx_queue);
        }
    );
    // loop
    bool quit = false;
    while (!quit)
    {
        SDL_Event event;
        while (SDL_PollEvent(&event))
        {
            if (SDL_GetWindowID(sdl_window) == event.window.windowID)
            {
                if (!SDLEventHandler(&event, sdl_window))
                {
                    quit = true;
                }
            }
            if (event.type == SDL_EVENT_WINDOW_RESIZED)
            {
                cgpu_wait_queue_idle(gfx_queue);
                cgpu_free_swapchain(swapchain);
                int width = 0, height = 0;
                SDL_GetWindowSize(sdl_window, &width, &height);
                CGPUSwapChainDescriptor chain_desc = {};
                chain_desc.present_queues          = &gfx_queue;
                chain_desc.present_queues_count    = 1;
                chain_desc.width                   = width;
                chain_desc.height                  = height;
                chain_desc.surface                 = surface;
                chain_desc.image_count             = 3;
                chain_desc.format                  = CGPU_FORMAT_R8G8B8A8_UNORM;
                chain_desc.enable_vsync            = true;
                swapchain                          = cgpu_create_swapchain(device, &chain_desc);
            }
        }
        // acquire frame
        cgpu_wait_fences(&present_fence, 1);
        CGPUAcquireNextDescriptor acquire_desc = {};
        acquire_desc.fence                     = present_fence;
        backbuffer_index                       = cgpu_acquire_next_image(swapchain, &acquire_desc);
        // render graph setup & compile & exec
        CGPUTextureId to_import   = swapchain->back_buffers[backbuffer_index];
        auto          back_buffer = graph->create_texture(
            [=](render_graph::RenderGraph& g, render_graph::TextureBuilder& builder) {
                builder.set_name(SKR_UTF8("backbuffer"))
                    .import(to_import, CGPU_RESOURCE_STATE_PRESENT)
                    .allow_render_target();
            }
        );
        graph->add_render_pass(
            [=](render_graph::RenderGraph& g, render_graph::RenderPassBuilder& builder) {
                builder.set_name(SKR_UTF8("color_pass"))
                    .set_pipeline(pipeline)
                    .write(0, back_buffer, CGPU_LOAD_ACTION_CLEAR);
            },
            [=](render_graph::RenderGraph& g, render_graph::RenderPassContext& stack) {
                cgpu_render_encoder_set_viewport(stack.encoder, 0.0f, 0.0f, (float)to_import->info->width / 3, (float)to_import->info->height, 0.f, 1.f);
                cgpu_render_encoder_set_scissor(stack.encoder, 0, 0, to_import->info->width, to_import->info->height);
                cgpu_render_encoder_draw(stack.encoder, 3, 0);
            }
        );
        graph->add_render_pass(
            [=](render_graph::RenderGraph& g, render_graph::RenderPassBuilder& builder) {
                builder.set_name(SKR_UTF8("color_pass2"))
                    .set_pipeline(pipeline)
                    .write(0, back_buffer, CGPU_LOAD_ACTION_LOAD);
            },
            [=](render_graph::RenderGraph& g, render_graph::RenderPassContext& stack) {
                cgpu_render_encoder_set_viewport(stack.encoder, 2 * (float)to_import->info->width / 3, 0.0f, (float)to_import->info->width / 3, (float)to_import->info->height, 0.f, 1.f);
                cgpu_render_encoder_set_scissor(stack.encoder, 0, 0, to_import->info->width, to_import->info->height);
                cgpu_render_encoder_draw(stack.encoder, 3, 0);
            }
        );
        graph->add_present_pass(
            [=](render_graph::RenderGraph& g, render_graph::PresentPassBuilder& builder) {
                builder.set_name(SKR_UTF8("present"))
                    .swapchain(swapchain, backbuffer_index)
                    .texture(back_buffer, true);
            }
        );
        graph->compile();
        const auto frame_index = graph->execute();
        // present
        cgpu_wait_queue_idle(gfx_queue);
        CGPUQueuePresentDescriptor present_desc = {};
        present_desc.index                      = backbuffer_index;
        present_desc.swapchain                  = swapchain;
        cgpu_queue_present(gfx_queue, &present_desc);
        if (frame_index == 0)
            render_graph::RenderGraphViz::write_graphviz(*graph, "render_graph_demo.gv");
    }
    render_graph::RenderGraph::destroy(graph);
    // clean up
    finalize();
    SDL_DestroyWindow(sdl_window);
    return 0;
}