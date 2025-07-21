#include "AnimView/renderer.hpp"
#include "SkrRT/ecs/query.hpp"
#include "AnimView/render_model.h"

namespace animd
{

struct AnimViewRendererImpl : public animd::AnimViewRenderer
{
    skr::Vector<skr_primitive_draw_t> model_drawcalls;
    skr_vfs_t* resource_vfs = nullptr;
    skr::ecs::EntityQuery* effect_query = nullptr;

    AnimViewRendererImpl() {}
    ~AnimViewRendererImpl() override {}
    void initialize(skr::RendererDevice* render_device, skr::ecs::World* storage, struct skr_vfs_t* resource_vfs) override
    {
        this->resource_vfs = resource_vfs;

        // prepare pipeline settings
        // prepare pipeline
        // reset render view
    }
    void finalize(skr::RendererDevice* renderer) override
    {
        // free pipeline
    }
    void produce_drawcalls(sugoi_storage_t* storage, skr::render_graph::RenderGraph* render_graph) override {}
    void draw(skr::render_graph::RenderGraph* render_graph) override
    {
        // pass -> create frame resources
        // pass -> execute
    }
};

AnimViewRenderer* AnimViewRenderer::Create()
{
    return SkrNew<AnimViewRendererImpl>();
}

void AnimViewRenderer::Destory(AnimViewRenderer* renderer)
{
    SkrDelete(renderer);
}

AnimViewRenderer::~AnimViewRenderer()
{
}

} // namespace animd
