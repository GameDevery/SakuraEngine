#include "SkrRenderer/render_mesh.h"
#include "SkrSceneCore/scene_components.h"
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

void MeshActor::Initialize()
{
    attach_rule = EAttachRule::Default;
    display_name = u8"MeshActor";
    rttr_type_guid = skr::type_id_of<MeshActor>();
    spawner = skr::UPtr<Spawner>::New(
        [this](skr::ecs::ArchetypeBuilder& Builder) {
            Builder
                .add_component<skr::scene::ParentComponent>()
                .add_component<skr::scene::ChildrenComponent>()
                .add_component<skr::scene::PositionComponent>()
                .add_component<skr::scene::RotationComponent>()
                .add_component<skr::scene::ScaleComponent>()
                .add_component<skr::scene::TransformComponent>()
                .add_component<skr::MeshComponent>();
        },
        [this](skr::ecs::TaskContext& Context) {
            SkrZoneScopedN("MeshActor::Spawner::run");
            this->scene_entities.resize_zeroed(1);
            this->scene_entities[0] = Context.entities()[0];
            SKR_LOG_INFO(u8"MeshActor {%s} created with entity: {%u}", this->GetDisplayName().c_str(), this->GetEntity());
        });
}

} // namespace skr