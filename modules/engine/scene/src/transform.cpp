#include "SkrBase/math.h"
#include "rtm/qvvf.h"
#include "SkrContainers/hashmap.hpp"
#include "SkrRT/ecs/sugoi.h"
#include "SkrRT/ecs/array.hpp"
#include "SkrRT/ecs/storage.hpp"
#include "SkrRT/ecs/type_builder.hpp"
#include "SkrTask/parallel_for.hpp"
#include "SkrScene/scene.h"

namespace skr
{
struct skr::TransformSystem::Impl {
    struct ParallelEntry {
        uint32_t threshold  = 0;
        uint32_t batch_size = 128;
    };
    sugoi_query_t*                                  calculateTransformTree;
    skr::FlatHashMap<sugoi_entity_t, ParallelEntry> parallel_nodes;
};

static void skr_relative_to_world_children(const skr::scene::ChildrenArray* children, skr::TransformF parent, sugoi_storage_t* storage)
{
    auto task = [&](sugoi_chunk_view_t* view) {
        SkrZoneScopedN("CalcTransform(Children)");
        if (auto transforms = sugoi::get_owned<skr::scene::TransformComponent, skr_transform_f_t>(view))
        {
            auto translations = sugoi::get_owned<const skr::scene::TranslationComponent, const skr_float3_t>(view);
            auto rotations    = sugoi::get_owned<const skr::scene::RotationComponent, const skr_rotator_f_t>(view);
            auto scales       = sugoi::get_owned<const skr::scene::ScaleComponent, const skr_float3_t>(view);
            auto childrens    = sugoi::get_owned<const skr::scene::ChildrenComponent, const skr::scene::ChildrenArray>(view);
            for (EIndex i = 0; i < view->count; ++i)
            {
                auto relative = skr::TransformF(
                    skr::QuatF(*(rotations ? &rotations[i] : nullptr)),
                    *(translations ? &translations[i] : nullptr),
                    *(scales ? &scales[i] : nullptr)
                );
                auto transform = TransformF::Identity();
                transform *= parent;   // apply parent transform
                transform *= relative; // apply relative transform

                if (childrens && childrens->size())
                {
                    skr_relative_to_world_children(&childrens[i], transform, storage);
                }
            }
        }
    };
    const auto childrenSize = children->size();
    if (childrenSize > 256) // dispatch recursively
    {
        skr::parallel_for(children->begin(), children->end(), 128, [&](const auto begin, const auto end) {
            sugoiS_batch(storage, (sugoi_entity_t*)&*begin, (EIndex)(end - begin), SUGOI_LAMBDA(task));
        });
    }
    else
    {
        sugoiS_batch(storage, (sugoi_entity_t*)children->data(), (EIndex)children->size(), SUGOI_LAMBDA(task));
    }
}

static void skr_relative_to_world_root(void* u, sugoi_query_t* query, sugoi_chunk_view_t* view, sugoi_type_index_t* localTypes, EIndex entityIndex)
{
    SkrZoneScopedN("CalcTransform");
    auto storage      = sugoiQ_get_storage(query);
    auto transforms   = sugoi::get_owned_local<skr_transform_f_t>(view, localTypes[0]);
    auto children     = sugoi::get_owned_local<const skr::scene::ChildrenArray>(view, localTypes[1]);
    auto translations = sugoi::get_owned_local<const skr_float3_t>(view, localTypes[2]);
    auto rotations    = sugoi::get_owned_local<const skr_rotator_f_t>(view, localTypes[3]);
    auto scales       = sugoi::get_owned_local<const skr_float3_t>(view, localTypes[4]);
    for (EIndex i = 0; i < view->count; ++i)
    {
        transforms[i].position = translations ? translations[i] : skr_float3_t{ 0, 0, 0 };
        transforms[i].rotation = skr::QuatF(rotations ? rotations[i] : skr_rotator_f_t{ 0, 0, 0 });
        transforms[i].scale    = scales ? scales[i] : skr_float3_t{ 1, 1, 1 };
    }
    forloop (i, 0, view->count)
    {
        // const auto transform = rtm::qvv_set(
        //     skr::math::load(transforms[i].rotation),
        //     skr::math::load(transforms[i].translation),
        //     skr::math::load(transforms[i].scale)
        // );
        // skr_relative_to_world_children(&children[i], transform, storage);
    }
}

TransformSystem* TransformSystem::Create(sugoi_storage_t* world) SKR_NOEXCEPT
{
    SkrZoneScopedN("CreateTransformSystem");
    auto memory                          = (uint8_t*)sakura_calloc(1, sizeof(TransformSystem) + sizeof(TransformSystem::Impl));
    auto system                          = new (memory) TransformSystem();
    system->impl                         = new (memory + sizeof(TransformSystem)) TransformSystem::Impl();
    system->impl->calculateTransformTree = world->new_query()
                                               .ReadWriteAll<skr::scene::TransformComponent>()
                                               .ReadAny<skr::scene::ChildrenComponent>()
                                               .ReadAny<skr::scene::TranslationComponent, skr::scene::RotationComponent, skr::scene::ScaleComponent>()
                                               .ReadAll<skr::scene::RootComponent>()
                                               .commit()
                                               .value();
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
    sugoiJ_schedule_ecs(impl->calculateTransformTree, 0, &skr_relative_to_world_root, nullptr, nullptr, nullptr, nullptr, nullptr);
}

void TransformSystem::set_parallel_entry(sugoi_entity_t entity) SKR_NOEXCEPT
{
    SKR_UNIMPLEMENTED_FUNCTION();
}

} // namespace skr

skr::TransformSystem* skr_transform_system_create(sugoi_storage_t* world)
{
    return skr::TransformSystem::Create(world);
}

void skr_transform_system_destroy(skr::TransformSystem* system)
{
    skr::TransformSystem::Destroy(system);
}

void skr_transform_system_set_parallel_entry(skr::TransformSystem* system, sugoi_entity_t entity)
{
    system->set_parallel_entry(entity);
}

void skr_transform_system_update(skr::TransformSystem* system)
{
    system->update();
}

void skr_propagate_transform(sugoi_storage_t* world, sugoi_entity_t* entities, uint32_t count)
{
    SKR_UNIMPLEMENTED_FUNCTION();
}