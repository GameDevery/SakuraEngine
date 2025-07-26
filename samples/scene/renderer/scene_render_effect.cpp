#include "SkrProfile/profile.h"
#include "SkrBase/misc/make_zeroed.hpp"
#include "SkrCore/time.h"
#include "SkrCore/platform/vfs.h"
#include "SkrRT/ecs/query.hpp"
#include "SkrRenderer/render_device.h"

#include "scene_renderer.hpp"

struct SceneRendererImpl : public skr::SceneRenderer
{
    const char8_t* push_constants_name = u8"push_constants";
    skr_vfs_t* resource_vfs;

    uint64_t frame_count;

    CGPURenderPipelineId pipeline = nullptr;

    void initialize(skr::RendererDevice* render_device, skr::ecs::World* world, struct skr_vfs_t* resource_vfs) override
    {
        this->resource_vfs = resource_vfs;
        // TODO: effect_query

        // TODO: prepare_pipeline_settings()
        // TODO: prepare_pipeline
        // TODO: reset_render_view
    }

    void finalize(skr::RendererDevice* renderer) override
    {
        // TODO: free_pipeline
    }

    void draw(skr::render_graph::RenderGraph* render_graph) override
    {
        // TODO: create_frame_resources
        // TODO: push drawcalls to rendergraph
    }

    void produce_drawcalls(sugoi_storage_t* storage, skr::render_graph::RenderGraph* render_graph) override
    {
        frame_count++;
        produce_mesh_drawcall(storage, render_graph);
    }

    void produce_mesh_drawcall(sugoi_storage_t* storage, skr::render_graph::RenderGraph* render_graph)
    {
        // TODO: Camera -> View Matrix
        // TODO: Query Storage to iterate render model and produce drawcalls
    }

    // helpers
    uint32_t* read_shader_bytes(skr::RendererDevice* render_device, const char8_t* name, uint32_t* out_length);
    CGPUShaderLibraryId create_shader_library(skr::RendererDevice* render_device, const char8_t* name, ECGPUShaderStage stage);
    void prepare_pipeline_settings();
    void prepare_pipeline(skr::RendererDevice* render_device);
    void free_pipeline(skr::RendererDevice* renderer);
};

// SceneRendererImpl* scene_effect = SkrNew<SceneRendererImpl>();
skr::SceneRenderer* skr::SceneRenderer::Create()
{
    return SkrNew<SceneRendererImpl>();
}

void skr::SceneRenderer::Destroy(skr::SceneRenderer* renderer)
{
    SkrDelete(renderer);
}

skr::SceneRenderer::~SceneRenderer() {}

// render_view_reset
// render_view_set_screen
// render_view_set_transform_screen
// render_view_transform_view