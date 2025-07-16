#pragma once
#include "SkrContainersDef/span.hpp"
#include "SkrRenderer/primitive_draw.h"

namespace skr { namespace render_graph { class RenderGraph; } }

namespace skr
{
 
struct SKR_LIVE2D_API Live2DRenderer
{
    static bool Initialize();
    
    virtual void produce_drawcalls(sugoi_storage_t* storage, skr::render_graph::RenderGraph* render_graph) = 0;
    virtual void draw(skr::render_graph::RenderGraph* render_graph) = 0;
};

} // namespace skr