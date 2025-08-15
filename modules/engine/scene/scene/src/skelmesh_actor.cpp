#include "SkrAnim/components/skeleton_component.hpp"
#include "SkrAnim/components/skin_component.hpp"
#include "SkrScene/actor.h"

namespace skr
{

SkelMeshActor::~SkelMeshActor() SKR_NOEXCEPT
{
    for (auto& child : children)
    {
        child->DetachFromParent();
    }
    if (_parent)
    {
        DetachFromParent();
    }
}

SkelMeshActor::SkelMeshActor()
{
    display_name = u8"SkelMeshActor";
    // override spawner
    spawner = Spawner{
        [this](skr::ecs::ArchetypeBuilder& Builder) {
            Builder
                .add_component<skr::scene::ParentComponent>()
                .add_component<skr::scene::ChildrenComponent>()
                .add_component<skr::scene::PositionComponent>()
                .add_component<skr::scene::RotationComponent>()
                .add_component<skr::scene::ScaleComponent>()
                .add_component<skr::scene::TransformComponent>()
                .add_component<skr::renderer::MeshComponent>()
                .add_component<skr::anim::SkeletonComponent>() // runtime skeleton component
                .add_component<skr::anim::AnimComponent>()
                .add_component<skr::anim::SkinComponent>();
        },
        [this](skr::ecs::TaskContext& Context) {
            SkrZoneScopedN("SkelMeshActor::Spawner::run");
            this->scene_entities.resize_zeroed(1);
            this->scene_entities[0] = Context.entities()[0];
            SKR_LOG_INFO(u8"SkelMeshActor {%s} created with entity: {%u}", this->GetDisplayName().c_str(), this->GetEntity());
        }
    };
}

} // namespace skr