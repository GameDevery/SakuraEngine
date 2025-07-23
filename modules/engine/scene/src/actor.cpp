#include "SkrScene/actor.h"
#include "SkrCore/memory/impl/skr_new_delete.hpp"

namespace skr {


Actor::Actor() SKR_NOEXCEPT
    : _parent(nullptr)
    , attach_rule(EAttachRule::Default) {
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
    static Actor root_actor;
    return RCWeak<Actor>(&root_actor);
}

RCWeak<Actor> Actor::CreateActor() {
    auto root = GetRoot().lock();
    auto actor = SActorManager::GetInstance().CreateActor();
    root->children.push_back(actor.lock());
    return root->children.back(); // Return the newly created actor reference
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


RCWeak<Actor> SActorManager::CreateActor() {
    auto actor = new Actor(); // TODO: currently we use cpp new/delete for Actor management 
    actors.add(actor->guid, skr::RC<Actor>(actor));
    return skr::RC<Actor>(actor);
}

bool SActorManager::DestroyActor(skr::GUID guid) {
    auto it = actors.find(guid).value();
    delete it.get(); // TODO: currently we use cpp new/delete for Actor management 
    actors.remove(guid);
    return true;
}

void SActorManager::ClearAllActors() {
    for (auto& actor_item : actors) {
        DestroyActor(actor_item.key);
    }
}

}// namespace skr