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

namespace skr
{

struct ANIM_VIEW_API AnimViewRenderer
{
    static AnimViewRenderer* Create();
};

} // namespace skr