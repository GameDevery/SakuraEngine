#pragma once
#include "SkrContainersDef/span.hpp"
#include "SkrRenderer/primitive_draw.h"

struct sugoi_storage_t;
namespace skr
{
struct RendererDevice;
}
namespace skr
{
namespace render_graph
{
class RenderGraph;
}
} // namespace skr
namespace skr
{
namespace ecs
{
class World;
}
} // namespace skr

namespace skr
{
struct SCENE_RENDERER_API SceneRenderer
{
    static SceneRenderer* Create();
    static void Destroy(SceneRenderer* renderer);

    virtual ~SceneRenderer();
    virtual void initialize(skr::RendererDevice* render_device, skr::ecs::World* storage, struct skr_vfs_t* resource_vfs) = 0;
    virtual void finalize(skr::RendererDevice* renderer) = 0;
    virtual void create_resource(skr::RendererDevice* renderer) = 0;
    virtual void produce_drawcalls(sugoi_storage_t* storage, skr::render_graph::RenderGraph* render_graph) = 0;
    virtual void draw(skr::render_graph::RenderGraph* render_graph) = 0;
};
} // namespace skr
