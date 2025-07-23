#pragma once
#include "SkrBase/types/impl/guid.hpp"
#include "SkrCore/memory/rc.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/map.hpp"
#include "SkrRT/sugoi/sugoi.h"
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

sreflect_enum_class(guid = "0198367e-d3a5-70ac-b97a-36460d05641a")
EActorType{
    Default = 0,
    Mesh = 1,
    SkelMesh = 2
    // Add more actor types as needed
};

sreflect_struct(
    guid = "4cb20865-0d27-43ee-90b9-7b43ac4c067c")
SKR_SCENE_API Actor
{
    friend class SActorManager;
public:
    SKR_RC_IMPL();
    virtual ~Actor() SKR_NOEXCEPT;
    static RCWeak<Actor> GetRoot();
    static RCWeak<Actor> CreateActor(EActorType type = EActorType::Default);
    void AttachTo(RCWeak<Actor> parent, EAttachRule rule = EAttachRule::Default);
    void DetachFromParent();


    // getters & setters
    inline const skr::String& GetDisplayName() const { return display_name; }
    inline void SetDisplayName(const skr::String& name) { display_name = name; }
    inline EActorType GetActorType() const { return actor_type; }

protected:
    explicit Actor(EActorType type = EActorType::Default) SKR_NOEXCEPT;
    skr::String display_name; // for editor, profiler, and runtime dump
    skr::GUID guid = skr::GUID::Create(); // guid for each actor, used to identify actors in the scene
    skr::InlineVector<sugoi_entity_t, 1> transform_entities;
    skr::Vector<skr::RC<Actor>> children;
    skr::RC<Actor> _parent = nullptr;
    EAttachRule attach_rule = EAttachRule::Default;
    EActorType actor_type = EActorType::Default;
};


class SKR_SCENE_API SActorManager {
public:
    static SActorManager& GetInstance() {
        static SActorManager instance;
        return instance;
    }
    skr::RCWeak<Actor> CreateActor(EActorType type = EActorType::Default);
    bool DestroyActor(skr::GUID guid);
    void ClearAllActors();
    skr::RCWeak<Actor> GetRoot();

protected:
    // Factory method to create specific actor types
    virtual skr::RC<Actor> CreateActorInstance(EActorType type);

private:
    SActorManager() = default;
    ~SActorManager() = default;

    // Disable copy and move semantics
    SActorManager(const SActorManager&) = delete;
    SActorManager& operator=(const SActorManager&) = delete;
    SActorManager(SActorManager&&) = delete;
    SActorManager& operator=(SActorManager&&) = delete;

    // Currently, we only use Map<GUID, Actor*> and cpp new/delete for Actor management.
    // In the future, we can implement a more sophisticated memory management system.
    skr::Map<skr::GUID, skr::RC<Actor>> actors; // Map to manage actors by their GUIDs
};

class SKR_SCENE_API MeshActor : public Actor
{
public:
    friend class SActorManager;
    SKR_RC_IMPL();


protected:
    MeshActor() : Actor(EActorType::Mesh) {}

};


// Actor for Skeletal Meshes
class SKR_SCENE_API SkelMeshActor : public MeshActor
{
public:
    friend class SActorManager;
    SKR_RC_IMPL();
    
protected:
    SkelMeshActor() : MeshActor() {actor_type = EActorType::SkelMesh;}
};

} // namespace skr