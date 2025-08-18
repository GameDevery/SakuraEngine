#pragma once

#include "SkrRT/ecs/world.hpp"
#include "SkrRenderer/primitive_draw.h"
#include "scene_renderer.hpp"

namespace skr::ecs
{
struct World;
}

namespace skr
{
namespace render_graph
{
class RenderGraph;
}
} // namespace skr

namespace skr::scene
{

struct SCENE_RENDERER_API AnimRenderSystem
{
public:
    static AnimRenderSystem* Create(skr::ecs::World* world) SKR_NOEXCEPT;
    static void Destroy(AnimRenderSystem* system) SKR_NOEXCEPT;
    void bind_renderer(skr::SceneRenderer* renderer) SKR_NOEXCEPT;
    void update() SKR_NOEXCEPT;
    skr::span<skr_primitive_draw_t> get_drawcalls() const SKR_NOEXCEPT;

private:
private:
    AnimRenderSystem() SKR_NOEXCEPT = default;
    ~AnimRenderSystem() SKR_NOEXCEPT = default;
    struct Impl;
    Impl* impl;
    struct AnimRenderJob;
};

} // namespace skr::scene