#include "SkrRenderer/render_mesh.h"
#include "SkrScene/actor.h"

namespace skr
{

MeshActor::~MeshActor() SKR_NOEXCEPT
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

MeshActor::MeshActor()
    : Actor(EActorType::Mesh)
{
    // Initialize the actor with default values
    display_name = u8"MeshActor";
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
                .add_component<skr::renderer::MeshComponent>();
        },
        [this](skr::ecs::TaskContext& Context) {
            SkrZoneScopedN("MeshActor::Spawner::run");
            this->scene_entities.resize_zeroed(1);
            this->scene_entities[0] = Context.entities()[0];
            SKR_LOG_INFO(u8"MeshActor {%s} created with entity: {%d}", this->GetDisplayName().c_str(), this->GetEntity());
        }
    };
}

skr::renderer::MeshComponent* MeshActor::GetMeshComponent() const
{
    auto entity = GetEntity();
    if (entity != skr::ecs::Entity{ SUGOI_NULL_ENTITY })
    {
        return skr::ActorManager::GetInstance().mesh_accessor.get(entity);
    }
    SKR_LOG_ERROR(u8"MeshActor {} has no valid entity to get MeshComponent", display_name.c_str());
    return nullptr;
}

} // namespace skr