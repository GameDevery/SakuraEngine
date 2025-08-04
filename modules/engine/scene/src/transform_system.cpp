#include "SkrBase/math.h"
#include "rtm/qvvf.h"
#include "SkrContainers/hashmap.hpp"
#include "SkrRT/ecs/world.hpp"
#include "SkrTask/parallel_for.hpp"
#include "SkrScene/transform_system.h"

namespace skr::scene
{
struct TransformJob
{
    void build(skr::ecs::AccessBuilder& builder)
    {
        builder.none<scene::ParentComponent>()
            .has<scene::PositionComponent>()
            .has<scene::TransformComponent>();

        builder.access(&TransformJob::children_accessor)
            .access(&TransformJob::postion_accessor)
            .access(&TransformJob::scale_accessor)
            .access(&TransformJob::rotation_accessor)
            .access(&TransformJob::transform_accessor);
    }

    void calculate(skr::ecs::Entity entity, const skr::scene::Transform& prev_transform)
    {
        SKR_LOG_INFO(u8"Calculating transform for entity: {%d}", entity);

        auto pOptionalRotation = rotation_accessor.get(entity);
        auto pOptionalScale = scale_accessor.get(entity);
        const auto& Position = postion_accessor[entity];

        const bool position_dirty = Position.get_dirty();
        const bool rotation_dirty = pOptionalRotation ? pOptionalRotation->get_dirty() : false;
        const bool scale_dirty = pOptionalScale ? pOptionalScale->get_dirty() : false;
        const bool dirty = position_dirty | rotation_dirty | scale_dirty;
        auto& transform = transform_accessor[entity];

        // TODO: 当前的更新逻辑存在问题
        // 1. 对于根节点，需要手动设置Position来触发更新，否则会默认初始化为0
        // 2. 对于某些希望保持世界变换的节点更新逻辑没有照顾到
        if (dirty)
        {
            transform.set(
                Position.get(),
                pOptionalRotation ? skr::QuatF(pOptionalRotation->get()) : skr::QuatF(0, 0, 0, 1),
                pOptionalScale ? pOptionalScale->get() : skr_float3_t{ 1, 1, 1 });
            transform.set(prev_transform * transform.get());

            Position.dirty = false;
            if (pOptionalRotation)
                pOptionalRotation->dirty = false;
            if (pOptionalScale)
                pOptionalScale->dirty = false;
        }

        for (const auto& child : children_accessor[entity])
        {
            calculate(child.entity, transform.get());
        }
    }

    void run(skr::ecs::TaskContext& Context)
    {
        SkrZoneScopedN("UpdateTransforms");
        for (auto i = 0; i < Context.size(); i++)
        {
            const auto entity = Context.entities()[i];
            calculate(entity, skr::scene::Transform::Identity());
        }
    }

    skr::ecs::RandomComponentReader<const skr::scene::ChildrenComponent> children_accessor;
    skr::ecs::RandomComponentReader<const skr::scene::PositionComponent> postion_accessor;
    skr::ecs::RandomComponentReader<const skr::scene::ScaleComponent> scale_accessor;
    skr::ecs::RandomComponentReader<const skr::scene::RotationComponent> rotation_accessor;
    skr::ecs::RandomComponentReadWrite<skr::scene::TransformComponent> transform_accessor;
};

} // namespace skr::scene

namespace skr
{

struct skr::TransformSystem::Impl
{
    skr::ecs::World* pWorld = nullptr;
    sugoi_query_t* rootJobQuery = nullptr;
};

TransformSystem* TransformSystem::Create(skr::ecs::World* world) SKR_NOEXCEPT
{
    SkrZoneScopedN("CreateTransformSystem");
    auto memory = (uint8_t*)sakura_calloc(1, sizeof(TransformSystem) + sizeof(TransformSystem::Impl));
    auto system = new (memory) TransformSystem();
    system->impl = new (memory + sizeof(TransformSystem)) TransformSystem::Impl();
    system->impl->pWorld = world;
    return system;
}

void TransformSystem::Destroy(TransformSystem* system) SKR_NOEXCEPT
{
    SkrZoneScopedN("FinalizeTransformSystem");
    system->impl->~Impl();
    system->~TransformSystem();
    sakura_free(system);
}

void TransformSystem::update() SKR_NOEXCEPT
{
    scene::TransformJob job;
    impl->rootJobQuery = impl->pWorld->dispatch_task(job, UINT32_MAX, impl->rootJobQuery);
}

} // namespace skr

skr::TransformSystem* skr_transform_system_create(skr::ecs::World* world)
{
    return skr::TransformSystem::Create(world);
}

void skr_transform_system_destroy(skr::TransformSystem* system)
{
    skr::TransformSystem::Destroy(system);
}

void skr_transform_system_update(skr::TransformSystem* system)
{
    system->update();
}