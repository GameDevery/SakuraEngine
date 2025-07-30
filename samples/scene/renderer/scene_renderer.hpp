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

// TODO: use official CameraComponent to replace this
namespace temp
{
struct Camera
{
    skr_float3_t position = { 0.0f, 0.0f, -1.0f }; // camera position
    skr_float3_t front = skr_float3_t::forward();  // camera front vector
    skr_float3_t up = skr_float3_t::up();          // camera up vector
    skr_float3_t right = skr_float3_t::right();    // camera right vector

    float fov = 3.1415926f / 2.f; // fov_x
    float aspect = 1.0;           // aspect ratio
    float near_plane = 0.1;       // near plane distance
    float far_plane = 1000.0;     // far plane distance
};
} // namespace temp

namespace skr
{

struct SCENE_RENDERER_API SceneRenderer
{
    static SceneRenderer* Create();
    static void Destroy(SceneRenderer* renderer);

    virtual ~SceneRenderer();
    virtual void initialize(skr::RendererDevice* render_device, skr::ecs::World* storage, struct skr_vfs_t* resource_vfs) = 0;
    virtual void finalize(skr::RendererDevice* renderer) = 0;
    virtual void draw_primitives(skr::render_graph::RenderGraph* render_graph, const skr::span<skr::renderer::PrimitiveCommand> cmds) = 0;

    virtual void temp_set_camera(temp::Camera* camera) = 0;
};
} // namespace skr
