#include "SkrScene/actor.h"
#include "SkrCore/memory/impl/skr_new_delete.hpp"

namespace skr {


Actor::Actor(EActorType type) SKR_NOEXCEPT
    : _parent(nullptr)
    , attach_rule(EAttachRule::Default)
    , actor_type(type) {
}   

Actor::~Actor() SKR_NOEXCEPT {
    // Detach from parent if exists
    // detach all children
    for (auto& child : children) {
        child->DetachFromParent();
    }
    if (_parent) {
        DetachFromParent();
    }
}

skr::RCWeak<Actor> Actor::GetRoot()
{
    return SActorManager::GetInstance().GetRoot();
}

RCWeak<Actor> Actor::CreateActor(EActorType type) {
    auto root = GetRoot().lock();
    auto actor = SActorManager::GetInstance().CreateActor(type);
    root->children.push_back(actor.lock());
    actor.lock()->_parent = root; // Set the root as parent By Default
    return actor;
}

void Actor::AttachTo(RCWeak<Actor> parent, EAttachRule rule) {
    // if _parent is not null, detach from current parent
    if (_parent) {
        DetachFromParent();
    }
    parent.lock()->children.emplace(this);
    _parent = parent.lock();
    attach_rule = rule;
}

void Actor::DetachFromParent() {
    if (_parent) {
        // Remove this actor from parent's children list
        auto& siblings = _parent->children;
        auto it = std::find(siblings.begin(), siblings.end(), this);
        if (it != siblings.end()) {
            siblings.erase(it);
        }
        _parent = nullptr; // Clear parent reference
    }
}

skr::RC<Actor> SActorManager::CreateActorInstance(EActorType type) {
    switch (type) {
        case EActorType::Mesh:
            return skr::RC<Actor>(new MeshActor());
        case EActorType::SkelMesh:
            return skr::RC<Actor>(new SkelMeshActor());
        case EActorType::Default:
        default:
            return skr::RC<Actor>(new Actor(type));
    }
}

RCWeak<Actor> SActorManager::CreateActor(EActorType type) {
    auto actor = CreateActorInstance(type);
    actors.add(actor->guid, actor);
    return actor;
}

bool SActorManager::DestroyActor(skr::GUID guid) {
    auto it = actors.find(guid).value();
    actors.remove(guid);
    return true;
}

void SActorManager::ClearAllActors() {
    for (auto& actor_item : actors) {
        DestroyActor(actor_item.key);
    }
}

skr::RCWeak<Actor> SActorManager::GetRoot()
{
    static skr::RCWeak<Actor> root = nullptr;
    if (!root) {
        root = CreateActor();
        root.lock()->SetDisplayName(u8"Root Actor");
    }
    return root; // Return the root actor reference
}


}// namespace skr