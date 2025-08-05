#include "SkrScene/actor.h"
#include "SkrCore/memory/impl/skr_new_delete.hpp"
#include "SkrRT/ecs/world.hpp"
#include "SkrRT/sugoi/sugoi_config.h"
#include "SkrRT/sugoi/sugoi_types.h"
#include "SkrSceneCore/scene_components.h"

namespace skr
{

Actor::Actor(EActorType type) SKR_NOEXCEPT
    : _parent(nullptr),
      attach_rule(EAttachRule::Default),
      actor_type(type),
      spawner{
          [this](skr::ecs::ArchetypeBuilder& Builder) {
              Builder
                  .add_component<skr::scene::ParentComponent>()
                  .add_component<skr::scene::ChildrenComponent>()
                  .add_component<skr::scene::PositionComponent>()
                  .add_component<skr::scene::RotationComponent>()
                  .add_component<skr::scene::ScaleComponent>()
                  .add_component<skr::scene::TransformComponent>();
          },
          [this](skr::ecs::TaskContext& Context) {
              SkrZoneScopedN("Actor::Spawner::run");
              this->scene_entities.resize_zeroed(1);
              this->scene_entities[0] = Context.entities()[0];
              SKR_LOG_INFO(u8"Actor {%s} created with entity: {%d}", this->GetDisplayName().c_str(), this->GetEntity());
          }
      }
{
}

Actor::~Actor() SKR_NOEXCEPT
{
    // Detach from parent if exists
    // detach all children
    for (auto& child : children)
    {
        child->DetachFromParent();
    }
    if (_parent)
    {
        DetachFromParent();
    }
}

skr::RCWeak<Actor> Actor::GetRoot()
{
    return ActorManager::GetInstance().GetRoot();
}

RCWeak<Actor> Actor::CreateActor(EActorType type)
{
    auto root = GetRoot().lock();
    auto actor = ActorManager::GetInstance().CreateActor(type);
    root->children.push_back(actor.lock());
    actor.lock()->_parent = root; // Set the root as parent By Default
    return actor;
}
void Actor::CreateEntity()
{
    skr::ActorManager::GetInstance().CreateActorEntity(this);
}

skr::ecs::Entity Actor::GetEntity() const
{
    if (scene_entities.is_empty())
    {
        SKR_LOG_ERROR(u8"Actor {} has no entity associated", display_name.c_str());
        return skr::ecs::Entity{ SUGOI_NULL_ENTITY };
    }
    return scene_entities[0];
}

void Actor::AttachTo(RCWeak<Actor> parent, EAttachRule rule)
{
    // if _parent is not null, detach from current parent
    if (_parent)
    {
        DetachFromParent();
    }
    parent.lock()->children.emplace(this);
    _parent = parent.lock();
    attach_rule = rule;
    // update hierarchy
    skr::ActorManager::GetInstance().UpdateHierarchy(_parent, this, rule);
}

void Actor::DetachFromParent()
{
    if (_parent)
    {
        // Remove this actor from parent's children list
        auto& siblings = _parent->children;
        auto it = std::find(siblings.begin(), siblings.end(), this);
        if (it != siblings.end())
        {
            siblings.erase(it);
        }
        skr::ActorManager::GetInstance().UpdateHierarchy(_parent, this, attach_rule);
        _parent = nullptr; // Clear parent reference
    }
}

skr::scene::ScaleComponent* Actor::GetScaleComponent() const
{
    auto entity = GetEntity();
    if (entity != skr::ecs::Entity{ SUGOI_NULL_ENTITY })
    {
        return skr::ActorManager::GetInstance().scale_accessor.get(entity);
    }
    SKR_LOG_ERROR(u8"Actor {} has no valid entity to get ScaleComponent", display_name.c_str());
    return nullptr;
}
skr::scene::PositionComponent* Actor::GetPositionComponent() const
{
    auto entity = GetEntity();
    if (entity != skr::ecs::Entity{ SUGOI_NULL_ENTITY })
    {
        return skr::ActorManager::GetInstance().pos_accessor.get(entity);
    }
    SKR_LOG_ERROR(u8"Actor {} has no valid entity to get PositionComponent", display_name.c_str());
    return nullptr;
}
skr::scene::RotationComponent* Actor::GetRotationComponent() const
{
    auto entity = GetEntity();
    if (entity != skr::ecs::Entity{ SUGOI_NULL_ENTITY })
    {
        return skr::ActorManager::GetInstance().rot_accessor.get(entity);
    }
    SKR_LOG_ERROR(u8"Actor {} has no valid entity to get RotationComponent", display_name.c_str());
    return nullptr;
}
skr::scene::TransformComponent* Actor::GetTransformComponent() const
{
    auto entity = GetEntity();
    if (entity != skr::ecs::Entity{ SUGOI_NULL_ENTITY })
    {
        return skr::ActorManager::GetInstance().trans_accessor.get(entity);
    }
    SKR_LOG_ERROR(u8"Actor {} has no valid entity to get TransformComponent", display_name.c_str());
    return nullptr;
}

/////////////////////
// ActorManager Implementation
/////////////////////

void ActorManager::initialize(skr::ecs::World* world)
{
    this->world = world; // Store the ECS world pointer for actor management
    parent_accessor = world->random_readwrite<skr::scene::ParentComponent>();
    children_accessor = world->random_readwrite<skr::scene::ChildrenComponent>();
    pos_accessor = world->random_readwrite<skr::scene::PositionComponent>();
    rot_accessor = world->random_readwrite<skr::scene::RotationComponent>();
    scale_accessor = world->random_readwrite<skr::scene::ScaleComponent>();
    trans_accessor = world->random_readwrite<skr::scene::TransformComponent>();
    mesh_accessor = world->random_readwrite<skr::renderer::MeshComponent>();
}

skr::RC<Actor> ActorManager::CreateActorInstance(EActorType type)
{
    switch (type)
    {
    case EActorType::Mesh:
        return skr::RC<Actor>(new MeshActor());
    case EActorType::SkelMesh:
        return skr::RC<Actor>(new SkelMeshActor());
    case EActorType::Default:
    default:
        return skr::RC<Actor>(new Actor(type));
    }
}

RCWeak<Actor> ActorManager::CreateActor(EActorType type)
{
    auto actor = CreateActorInstance(type);
    actors.add(actor->guid, actor);
    return actor;
}

bool ActorManager::DestroyActor(skr::GUID guid)
{
    auto it = actors.find(guid).value();
    if (!it)
    {
        SKR_LOG_ERROR(u8"Actor with GUID {%s} not found", guid);
        return false;
    }
    DestroyActorEntity(it); // Destroy the actor's entity in ECS world
    actors.remove(guid);    // when ref-counted -> 0, it will call SkrDelete with release()
    return true;
}

void ActorManager::CreateActorEntity(skr::RCWeak<Actor> actor)
{
    // check if actor's scene_entities is empty
    if (actor.lock()->scene_entities.is_empty())
    {
        world->create_entities(actor.lock()->spawner, 1);
    }
    else
    {
        SKR_LOG_WARN(u8"Actor {%s} already has an entity associated", actor.lock()->GetDisplayName().c_str());
    }
}

void ActorManager::DestroyActorEntity(skr::RCWeak<Actor> actor)
{
    if (!actor.lock()->scene_entities.is_empty())
    {
        world->destroy_entities(actor.lock()->scene_entities);
        actor.lock()->scene_entities.clear(); // Clear the entity list after destruction
    }
    else
    {
        SKR_LOG_WARN(u8"Actor {} has no entities to destroy", actor.lock()->GetDisplayName().c_str());
    }
}

void ActorManager::UpdateHierarchy(skr::RCWeak<Actor> parent, skr::RCWeak<Actor> child, EAttachRule rule)
{
    // update the parent/child components
    if (parent && child)
    {
        // child should not be root
        if (child.lock()->guid == skr::Actor::GetRoot().lock()->guid)
        {
            SKR_LOG_ERROR(u8"Cannot attach root actor as child");
            return;
        }
        auto parent_entity = parent.lock()->GetEntity();
        auto child_entity = child.lock()->GetEntity();
        if (parent_entity != skr::ecs::Entity{ SUGOI_NULL_ENTITY } && child_entity != skr::ecs::Entity{ SUGOI_NULL_ENTITY })
        {
            // update parent component
            parent_accessor.write_at(child_entity, skr::scene::ParentComponent{ parent_entity });
            skr::scene::ChildrenArray children;
            for (auto& new_child : parent.lock()->children)
            {
                children.push_back(skr::scene::ChildrenComponent{ .entity = new_child->GetEntity() });
            }
            // update children component
            children_accessor.write_at(parent_entity, children);
        }
        else
        {
            SKR_LOG_ERROR(u8"Parent or Child actor has no valid entity");
        }
    }
    else
    {
        SKR_LOG_ERROR(u8"Parent or Child actor is null");
    }
}

void ActorManager::finalize()
{
    ClearAllActors(); // Clear all actors when finalizing
    world = nullptr;  // Clear the ECS world pointer
}

void ActorManager::ClearAllActors()
{
    for (auto& actor_item : actors)
    {
        DestroyActor(actor_item.key);
    }
}

skr::RCWeak<Actor> ActorManager::GetRoot()
{
    static skr::RCWeak<Actor> root = nullptr;
    if (!root)
    {
        root = CreateActor();
        // override spawner
        root.lock()->spawner = Actor::Spawner{
            [&](skr::ecs::ArchetypeBuilder& Builder) {
                Builder.add_component<skr::scene::ChildrenComponent>()
                    .add_component<skr::scene::PositionComponent>()
                    .add_component<skr::scene::TransformComponent>();
            },
            [&](skr::ecs::TaskContext& Context) {
                SkrZoneScopedN("RootActorSpawner::run");
                auto root_actor = skr::Actor::GetRoot();
                root_actor.lock()->scene_entities.resize_zeroed(1);
                root_actor.lock()->scene_entities[0] = Context.entities()[0];
                root_actor.lock()->SetDisplayName(u8"Root Actor");
                SKR_LOG_INFO(u8"Root actor created with entity: {%d}", root_actor.lock()->GetEntity());
            }
        };
    }
    return root; // Return the root actor reference
}

} // namespace skr
