#include "anim_render_system.h"
#include "SkrAnim/components/skin_component.hpp"
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
struct AnimRenderJob
{
    using RenderF = std::function<void(const skr::span<skr::renderer::PrimitiveCommand>, skr_float4x4_t, skr::Vector<skr::anim::SkinPrimitive>)>;
    AnimRenderJob(RenderF render_callback = nullptr)
        : render_callback(render_callback)
    {
    }

    void build(skr::ecs::AccessBuilder& builder)
    {
        builder.has<renderer::MeshComponent>()
            .has<scene::TransformComponent>()
            .has<anim::AnimComponent>();

        builder.access(&AnimRenderJob::mesh_accessor)
            .access(&AnimRenderJob::transform_accessor)
            .access(&AnimRenderJob::anim_accessor);
    }
    void run(skr::ecs::TaskContext& context)
    {
        SkrZoneScopedN("AnimRenderJob::run");
        for (auto i = 0; i < context.size(); i++)
        {
            const auto entity = context.entities()[i];
            // SKR_LOG_INFO(u8"Rendering entity: {%u}", entity);
            auto* mesh_component = mesh_accessor.get(entity);
            auto* transform_component = transform_accessor.get(entity);
            auto* anim_component = anim_accessor.get(entity);

            if (!mesh_component || !transform_component)
            {
                continue;
            }

            if (!anim_component)
            {
                continue; // skip if no anim component
            }

            mesh_component->mesh_resource.resolve(true, 0);

            if (mesh_component->mesh_resource.is_resolved())
            {
                auto* mesh_resource = mesh_component->mesh_resource.get_resolved(true);

                if (mesh_resource && mesh_resource->render_mesh && !anim_component->vbs.is_empty())
                {
                    render_callback(mesh_resource->render_mesh->primitive_commands, transform_component->get().to_matrix(), anim_component->primitives);
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
    skr::ecs::RandomComponentReader<const skr::anim::AnimComponent> anim_accessor;
    RenderF render_callback = nullptr;
};

struct AnimRenderSystem::Impl
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

AnimRenderSystem* AnimRenderSystem::Create(skr::ecs::World* world) SKR_NOEXCEPT
{
    SkrZoneScopedN("CreateAnimRenderSystem");
    auto memory = (uint8_t*)sakura_calloc(1, sizeof(AnimRenderSystem) + sizeof(AnimRenderSystem::Impl));
    // placement new for constructing the AnimRenderSystem and its Impl compactly
    auto system = new (memory) AnimRenderSystem();
    system->impl = new (memory + sizeof(AnimRenderSystem)) AnimRenderSystem::Impl();
    system->impl->mp_world = world;
    return system;
};

void AnimRenderSystem::Destroy(AnimRenderSystem* system) SKR_NOEXCEPT
{
    SkrZoneScopedN("DestroyAnimRenderSystem");
    system->impl->~Impl();
    system->~AnimRenderSystem();
    sakura_free(system);
}

void AnimRenderSystem::bind_renderer(skr::SceneRenderer* renderer) SKR_NOEXCEPT
{
    impl->mp_renderer = renderer;
}

skr::span<skr_primitive_draw_t> AnimRenderSystem::get_drawcalls() const SKR_NOEXCEPT
{
    return impl->drawcalls;
}

void AnimRenderSystem::update() SKR_NOEXCEPT
{
    SkrZoneScopedN("AnimRenderSystem::update");
    impl->drawcalls.clear();
    impl->push_constants_list.clear();

    auto render_func = impl->mp_renderer != nullptr ?
        scene::AnimRenderJob::RenderF([this](const skr::span<skr::renderer::PrimitiveCommand> cmds, skr_float4x4_t model, skr::Vector<skr::anim::SkinPrimitive> skin_primitives) {
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

            for (auto i = 0; i < cmds.size(); i++)
            {
                skr_primitive_draw_t& drawcall = impl->drawcalls.emplace().ref();
                auto& cmd = cmds[i];
                auto& skin_prim = skin_primitives[i];

                drawcall.push_const = (const uint8_t*)(&push_constants_data);
                drawcall.index_buffer = *cmd.ibv;
                drawcall.vertex_buffer_count = (uint32_t)skin_prim.views.size();
                drawcall.vertex_buffers = skin_prim.views.data();
            }
        }) :
        scene::AnimRenderJob::RenderF([](const skr::span<skr::renderer::PrimitiveCommand> cmds, skr_float4x4_t model, skr::Vector<skr::anim::SkinPrimitive> skin_primitives) {
            // do nothing
        });
    scene::AnimRenderJob job{ render_func };
    impl->m_render_job_query = impl->mp_world->dispatch_task(job, UINT32_MAX, impl->m_render_job_query);
}

} // namespace skr::scene
