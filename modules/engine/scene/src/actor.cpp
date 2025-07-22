#include "SkrScene/actor.h"

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
    static skr::RC<Actor> root_actor = skr::RC<Actor>();
    return root_actor;
}

RCWeak<Actor> Actor::CreateActor() {
    auto new_actor = skr::RC<Actor>();
    new_actor->AttachTo(GetRoot(), EAttachRule::Default);
    return new_actor;
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
    // When detaching, the lifecycle of the actor should be destroyed immediately
    if (_parent) {
        auto& siblings = _parent->children;
        auto it = std::find(siblings.begin(), siblings.end(), this);
        if (it != siblings.end()) {
            siblings.erase(it);
        }
        _parent = nullptr;
        attach_rule = EAttachRule::Default;
    }
    this->~Actor(); // Call destructor to clean up resources
}


}// namespace skr