#include "SkrProfile/profile.h"
#include "scene_renderer.h"

namespace skr::scene
{

void Renderer::create_api_objects()
{
    SkrZoneScopedN("Renderer::create_api_objects");
}

void Renderer::create_resources()
{
    SkrZoneScopedN("Renderer::create_resources");
}
void Renderer::create_debug_pipeline()
{
    SkrZoneScopedN("Renderer::create_debug_pipeline");
}
void Renderer::build_render_graph(skr::render_graph::RenderGraph* graph, skr::render_graph::TextureHandle back_buffer)
{
    SkrZoneScopedN("Renderer::build_render_graph");
    // Implement the render graph building logic here
}

void Renderer::finalize()
{
    SkrZoneScopedN("Renderer::finalize");
    // Free cgpu objects
    cgpu_wait_queue_idle(_gfx_queue);
    cgpu_free_render_pipeline(_debug_pipeline);
    cgpu_free_buffer(_index_buffer);
    cgpu_free_buffer(_vertex_buffer);
    cgpu_free_buffer(_instance_buffer);
    cgpu_free_sampler(_static_sampler);
    cgpu_free_queue(_gfx_queue);
    cgpu_free_device(_device);
    cgpu_free_instance(_instance);
}

} // namespace skr::scene