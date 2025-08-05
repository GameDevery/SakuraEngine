#pragma once
// The SceneRenderSystem: A render system for rendering scene using SceneRenderer
#include "SkrRT/ecs/world.hpp"

namespace skr::scene
{
struct SceneRenderJob
{
    void build(skr::ecs::AccessBuilder& builder)
    {
    }
    void run(skr::ecs::TaskContext& context)
    {
        SkrZoneScopedN("SceneRenderJob::run");
    }
};

} // namespace skr::scene