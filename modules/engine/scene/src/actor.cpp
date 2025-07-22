#include "SkrScene/actor.h"
#include "SkrCore/memory/impl/skr_new_delete.hpp"

namespace skr {


Actor::Actor() SKR_NOEXCEPT
    : _parent(nullptr)
    , attach_rule(EAttachRule::Default) {
    // Initialize transform entities if needed
    // transform_entities.reserve(1);
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
    // allocate and construct a new Actor instance into root actor's children
    auto root = GetRoot().lock();
    // Assuming children is a vector-like container of smart pointers (e.g. skr::RC<Actor>)
    // and emplace_back can create a new Actor and manage its lifetime.
    root->children.emplace(new Actor());
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
    // TODO: move the ownership to scheduler to release all components 


    // When No RefCounted (Parent/Child) Exists, the actor will be destroyed
}

void Actor::OnTick(float delta_time) SKR_NOEXCEPT {

}


}// namespace skr