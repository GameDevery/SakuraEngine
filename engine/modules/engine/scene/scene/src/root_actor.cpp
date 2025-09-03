#include "SkrScene/actor.h"

namespace skr
{

RootActor::~RootActor() SKR_NOEXCEPT {
    if (world) {
        world->finalize();
    }
}

void RootActor::Initialize()
{
    // standard initialize in system
    attach_rule = EAttachRule::Default;
    rttr_type_guid = skr::type_id_of<RootActor>();
    guid = skr::GUID::Create();
    display_name = u8"RootActor";
    spawner = skr::UPtr<Actor::Spawner>::New(
        [this](skr::ecs::ArchetypeBuilder& Builder) {
            Builder.add_component<skr::scene::ChildrenComponent>()
                .add_component<skr::scene::PositionComponent>()
                .add_component<skr::scene::TransformComponent>();
        },
        [this](skr::ecs::TaskContext& Context) {
            SkrZoneScopedN("RootActorSpawner::run");
            auto root_actor = skr::Actor::GetRoot();
            this->scene_entities.resize_zeroed(1);
            this->scene_entities[0] = Context.entities()[0];
            this->SetDisplayName(u8"Root Actor");
            SKR_LOG_INFO(u8"Root actor created with entity: {%u}", root_actor.lock()->GetEntity());
        });
}

void RootActor::InitWorld()
{
    world = skr::UPtr<skr::ecs::World>::New();
}


} // namespace skr