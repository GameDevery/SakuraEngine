#include "SkrBase/math.h"
#include "rtm/qvvf.h"
#include "SkrContainers/hashmap.hpp"
#include "SkrRT/ecs/query.hpp"
#include "SkrTask/parallel_for.hpp"
#include "SkrScene/transform_system.h"

namespace skr
{
struct skr::TransformSystem::Impl {
    struct ParallelEntry {
        uint32_t threshold = 0;
        uint32_t batch_size = 128;
    };
    sugoi_query_t* calculateTransformTree;
    skr::FlatHashMap<sugoi_entity_t, ParallelEntry> parallel_nodes;
};

static void skr_relative_to_world_children(const skr::scene::ChildrenArray* children, skr::TransformF parent, sugoi_storage_t* storage)
{
    auto task = [&](sugoi_chunk_view_t* view) {
        SkrZoneScopedN("CalcTransform(Children)");
        if (auto transforms = sugoi::get_owned<skr::scene::TransformComponent, skr_transform_f_t>(view))
        {
            auto translations = sugoi::get_owned<const skr::scene::TranslationComponent, const skr_float3_t>(view);
            auto rotations = sugoi::get_owned<const skr::scene::RotationComponent, const skr_rotator_f_t>(view);
            auto scales = sugoi::get_owned<const skr::scene::ScaleComponent, const skr_float3_t>(view);
            auto childrens = sugoi::get_owned<const skr::scene::ChildrenComponent, const skr::scene::ChildrenArray>(view);
            for (EIndex i = 0; i < view->count; ++i)
            {
                // Safe construction of relative transform with proper defaults
                skr_float3_t translation = translations ? translations[i] : skr_float3_t{ 0.0f, 0.0f, 0.0f };
                skr_rotator_f_t rotation_euler = rotations ? rotations[i] : skr_rotator_f_t{ 0.0f, 0.0f, 0.0f };
                skr_float3_t scale = scales ? scales[i] : skr_float3_t{ 1.0f, 1.0f, 1.0f };

                auto relative = skr::TransformF(
                    skr::QuatF(rotation_euler),
                    translation,
                    scale);

                // Compute world transform: parent * relative
                auto world_transform = parent * relative;

                // Write back to TransformComponent
                transforms[i] = world_transform;

                // Recursively process children if they exist
                if (childrens && childrens[i].size() > 0)
                {
                    skr_relative_to_world_children(&childrens[i], world_transform, storage);
                }
            }
        }
    };
    const auto childrenSize = children->size();
    if (childrenSize > 128) // dispatch recursively
    {
        skr::parallel_for(children->begin(), children->end(), 64, 
        [&](const auto begin, const auto end) {
            sugoiS_batch(storage, (sugoi_entity_t*)&*begin, (EIndex)(end - begin), SUGOI_LAMBDA(task));
        });
    }
    else
    {
        sugoiS_batch(storage, (sugoi_entity_t*)children->data(), (EIndex)children->size(), SUGOI_LAMBDA(task));
    }
}

static void skr_relative_to_world_root(void* u, sugoi_chunk_view_t* view)
{
    SkrZoneScopedN("CalcTransform");
    auto transforms = sugoi::get_components<skr::scene::TranslationComponent, skr_transform_f_t>(view);
    auto children = sugoi::get_components<const skr::scene::ChildrenComponent, const skr::scene::ChildrenArray>(view);
    auto translations = sugoi::get_components<const scene::TranslationComponent>(view);
    auto rotations = sugoi::get_components<const scene::RotationComponent>(view);
    auto scales = sugoi::get_components<const scene::ScaleComponent>(view);
    for (EIndex i = 0; i < view->count; ++i)
    {
        transforms[i].position = !translations.is_empty() ? translations[i].value : skr_float3_t{ 0, 0, 0 };
        transforms[i].rotation = skr::QuatF(!rotations.is_empty() ? rotations[i].euler : skr_rotator_f_t{ 0, 0, 0 });
        transforms[i].scale = !scales.is_empty() ? scales[i].value : skr_float3_t{ 1, 1, 1 };
    }
    forloop (i, 0, view->count)
    {
        // Create TransformF from the assembled components
        skr::TransformF world_transform(
            transforms[i].rotation,
            transforms[i].position,
            transforms[i].scale);

        // Recursively process children if they exist
        if (!children.is_empty() && children[i].size() > 0)
        {
            skr_relative_to_world_children(&children[i], world_transform, sugoiC_get_storage(view->chunk));
        }
    }
}

TransformSystem* TransformSystem::Create(skr::ecs::World* world) SKR_NOEXCEPT
{
    SkrZoneScopedN("CreateTransformSystem");
    auto memory = (uint8_t*)sakura_calloc(1, sizeof(TransformSystem) + sizeof(TransformSystem::Impl));
    auto system = new (memory) TransformSystem();
    system->impl = new (memory + sizeof(TransformSystem)) TransformSystem::Impl();
    auto q = skr::ecs::QueryBuilder(world)
                 .ReadWriteAll<skr::scene::TransformComponent>()
                 .ReadOptional<skr::scene::ChildrenComponent>()
                 .ReadOptional<skr::scene::TranslationComponent, skr::scene::RotationComponent, skr::scene::ScaleComponent>()
                 .ReadAll<skr::scene::RootComponent>()
                 .commit()
                 .value();
    system->impl->calculateTransformTree = q;
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
    SkrZoneScopedN("CalcTransform");
    sugoiQ_get_views(impl->calculateTransformTree, &skr_relative_to_world_root, nullptr);
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