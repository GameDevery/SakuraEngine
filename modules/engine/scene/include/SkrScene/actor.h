#pragma once
#include "SkrCore/memory/rc.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrRT/ecs/sugoi.h"
#if !defined(__meta__)
    #include "SkrScene/actor.generated.h"
#endif

namespace skr
{

sreflect_enum(guid = "a1ebd9b1-900c-44f4-b381-0dd48014718d")
EAttachRule{
    Default = 0,
    KeepWorldTransform = 0x01
};

sreflect_struct(
    guid = "4cb20865-0d27-43ee-90b9-7b43ac4c067c")
SKR_SCENE_API Actor
{
public:
    SKR_RC_IMPL();

    static RCWeak<Actor> CreateActor();
    void AttachTo(RCWeak<Actor> parent, EAttachRule rule = EAttachRule::Default);
    void DetachFromParent();

protected:
    Actor() SKR_NOEXCEPT;
    virtual ~Actor() SKR_NOEXCEPT;
    skr::InlineVector<sugoi_entity_t, 1> transform_entities;
    skr::Vector<skr::RC<Actor>> children;
};

class MeshActor : public Actor
{
public:
    SKR_RC_IMPL();

protected:
    // skr::InlineVector<>
};

} // namespace skr