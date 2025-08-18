#pragma once
// The SceneRenderSystem: A render system for rendering scene using SceneRenderer
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

struct SCENE_RENDERER_API SceneRenderSystem
{
public:
    static SceneRenderSystem* Create(skr::ecs::World* world) SKR_NOEXCEPT;
    static void Destroy(SceneRenderSystem* system) SKR_NOEXCEPT;
    void bind_renderer(skr::SceneRenderer* renderer) SKR_NOEXCEPT;
    void update() SKR_NOEXCEPT;
    skr::span<skr_primitive_draw_t> get_drawcalls() const SKR_NOEXCEPT;

private:
private:
    SceneRenderSystem() SKR_NOEXCEPT = default;
    ~SceneRenderSystem() SKR_NOEXCEPT = default;
    struct Impl;
    Impl* impl;

    // The render job for rendering meshes in the scene
    struct SceneRenderJob;
};

} // namespace skr::scene

SKR_EXTERN_C SCENE_RENDERER_API skr::scene::SceneRenderSystem* skr_scene_render_system_create(skr::ecs::World* world);
SKR_EXTERN_C SCENE_RENDERER_API void skr_scene_render_system_destroy(skr::scene::SceneRenderSystem* system);
// SKR_EXTERN_C SCENE_RENDERER_API void skr_scene_render_system_update(skr::scene::SceneRenderSystem* system);
