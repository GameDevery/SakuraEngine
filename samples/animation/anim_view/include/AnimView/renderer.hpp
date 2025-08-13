#pragma once
#include "SkrBase/config.h"
#include "SkrContainersDef/span.hpp"
#include "SkrRenderer/primitive_draw.h"

struct sugoi_storage_t;

namespace skr
{
struct RendererDevice;
} // namespace skr

namespace skr
{
namespace render_graph
{
class RenderGraph;
} // namespace render_graph
} // namespace skr

namespace skr
{
namespace ecs
{
class World;
} // namespace ecs
} // namespace skr

namespace animd
{

struct ANIM_VIEW_API AnimViewRenderer
{
    static AnimViewRenderer* Create();
    static void Destory(AnimViewRenderer* renderer);

    virtual ~AnimViewRenderer();
    virtual void initialize(skr::RendererDevice* render_device, skr::ecs::World* storage, struct skr_vfs_t* resource_vfs) = 0;
    virtual void finalize(skr::RendererDevice* renderer) = 0;
    virtual void produce_drawcalls(sugoi_storage_t* storage, skr::render_graph::RenderGraph* render_graph) = 0;
    virtual void draw(skr::render_graph::RenderGraph* render_graph) = 0;
};

} // namespace animd