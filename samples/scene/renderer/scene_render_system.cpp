#include "scene_render_system.h"
#include "SkrBase/math.h"
#include "SkrRenderer/primitive_draw.h"
#include "SkrSceneCore/scene_components.h"
#include "SkrRenderer/render_mesh.h"
#include "helper.hpp"
#include "scene_renderer.hpp"

namespace skr
{
namespace render_graph
{
class RenderGraph;
}
} // namespace skr

namespace skr::scene
{
struct SceneRenderJob
{
    using RenderF = std::function<void(const skr::span<skr::renderer::PrimitiveCommand>, skr_float4x4_t)>;
    SceneRenderJob(RenderF render_callback = nullptr)
        : render_callback(render_callback)
    {
    }

    void build(skr::ecs::AccessBuilder& builder)
    {
        builder.has<renderer::MeshComponent>()
            .has<scene::TransformComponent>();

        builder.access(&SceneRenderJob::mesh_accessor)
            .access(&SceneRenderJob::transform_accessor);
    }
    void run(skr::ecs::TaskContext& context)
    {
        SkrZoneScopedN("SceneRenderJob::run");
        for (auto i = 0; i < context.size(); i++)
        {
            const auto entity = context.entities()[i];
            // SKR_LOG_INFO(u8"Rendering entity: {%u}", entity);
            auto* mesh_component = mesh_accessor.get(entity);
            auto* transform_component = transform_accessor.get(entity);
            // auto transform = transform_component->get();
            // SKR_LOG_INFO(u8"Transform Position: ({%f}, {%f}, {%f})", transform.position.x, transform.position.y, transform.position.z);
            // SKR_LOG_INFO(u8"Transform Rotation: ({%f}, {%f}, {%f}, {%f})", transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
            // SKR_LOG_INFO(u8"Transform Scale: ({%f}, {%f}, {%f})", transform.scale.x, transform.scale.y, transform.scale.z);
            if (!mesh_component || !transform_component)
            {
                continue;
            }

            mesh_component->mesh_resource.resolve(true, 0);
            if (mesh_component->mesh_resource.is_resolved())
            {
                auto* mesh_resource = mesh_component->mesh_resource.get_resolved(true);

                if (mesh_resource && mesh_resource->render_mesh)
                {
                    render_callback(mesh_resource->render_mesh->primitive_commands, transform_component->get().to_matrix());
                }
                else
                {
                    continue; // skip if mesh resource is not valid
                }
            }
        }
    }

    skr::ecs::RandomComponentReadWrite<skr::renderer::MeshComponent> mesh_accessor;
    skr::ecs::RandomComponentReader<const skr::scene::TransformComponent> transform_accessor;
    RenderF render_callback = nullptr;
};

struct SceneRenderSystem::Impl
{
    skr::ecs::World* mp_world = nullptr;
    sugoi_query_t* m_render_job_query = nullptr;
    skr::SceneRenderer* mp_renderer = nullptr;
    struct PushConstants
    {
        skr_float4x4_t model = skr_float4x4_t::identity();
        skr_float4x4_t view_proj = skr_float4x4_t::identity();
    };
    skr::Vector<skr_primitive_draw_t> drawcalls;
    skr::Vector<PushConstants> push_constants_list;
};

SceneRenderSystem* SceneRenderSystem::Create(skr::ecs::World* world) SKR_NOEXCEPT
{
    SkrZoneScopedN("CreateSceneRenderSystem");
    auto memory = (uint8_t*)sakura_calloc(1, sizeof(SceneRenderSystem) + sizeof(SceneRenderSystem::Impl));
    // placement new for constructing the SceneRenderSystem and its Impl compactly
    auto system = new (memory) SceneRenderSystem();
    system->impl = new (memory + sizeof(SceneRenderSystem)) SceneRenderSystem::Impl();
    system->impl->mp_world = world;
    return system;
};

void SceneRenderSystem::Destroy(SceneRenderSystem* system) SKR_NOEXCEPT
{
    SkrZoneScopedN("DestroySceneRenderSystem");
    system->impl->~Impl();
    system->~SceneRenderSystem();
    sakura_free(system);
}

void SceneRenderSystem::bind_renderer(skr::SceneRenderer* renderer) SKR_NOEXCEPT
{
    impl->mp_renderer = renderer;
}

skr::span<skr_primitive_draw_t> SceneRenderSystem::get_drawcalls() const SKR_NOEXCEPT
{
    return impl->drawcalls;
}

void SceneRenderSystem::update() SKR_NOEXCEPT
{
    SkrZoneScopedN("SceneRenderSystem::update");
    impl->drawcalls.clear();
    impl->push_constants_list.clear();

    auto render_func = impl->mp_renderer != nullptr ?
        scene::SceneRenderJob::RenderF([this](const skr::span<skr::renderer::PrimitiveCommand> cmds, skr_float4x4_t model) {
            // impl->mp_renderer->draw_primitives(render_graph, cmds, model);
            auto& push_constants_data = impl->push_constants_list.emplace().ref();
            push_constants_data.model = skr::transpose(model);
            utils::Camera* camera = impl->mp_renderer->get_camera();

            auto view = skr::float4x4::view_at(
                camera->position,
                camera->position + camera->front,
                camera->up);

            auto proj = skr::float4x4::perspective_fov(
                skr::camera_fov_y_from_x(camera->fov, camera->aspect),
                camera->aspect,
                camera->near_plane,
                camera->far_plane);
            auto _view_proj = skr::mul(view, proj);
            push_constants_data.view_proj = skr::transpose(_view_proj);

            for (const auto& cmd : cmds)
            {
                skr_primitive_draw_t& drawcall = impl->drawcalls.emplace().ref();
                // fill the drawcall with data
                // wait for the render effect to fill the pipeline data
                drawcall.push_const = (const uint8_t*)(&push_constants_data);
                drawcall.vertex_buffer_count = (uint32_t)cmd.vbvs.size();
                drawcall.vertex_buffers = cmd.vbvs.data();
                drawcall.index_buffer = *cmd.ibv;
            }
        }) :
        scene::SceneRenderJob::RenderF([](const skr::span<skr::renderer::PrimitiveCommand> cmds, skr_float4x4_t model) {
            // do nothing
        });
    scene::SceneRenderJob job{ render_func };
    impl->m_render_job_query = impl->mp_world->dispatch_task(job, UINT32_MAX, impl->m_render_job_query);
}

} // namespace skr::scene

// C interface
skr::scene::SceneRenderSystem* skr_scene_render_system_create(skr::ecs::World* world)
{
    return skr::scene::SceneRenderSystem::Create(world);
}

void skr_scene_render_system_destroy(skr::scene::SceneRenderSystem* system)
{
    skr::scene::SceneRenderSystem::Destroy(system);
}

// void skr_scene_render_system_update(skr::scene::SceneRenderSystem* system, skr::render_graph::RenderGraph* render_graph)
// {
//     system->update(render_graph);
// }
