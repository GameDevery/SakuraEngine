#pragma once
#include "SkrBase/types/impl/guid.hpp"
#include "SkrCore/memory/rc.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/map.hpp"
#include "SkrRT/ecs/component.hpp"
#include "SkrRT/resource/resource_handle.h"
#include "SkrRT/ecs/world.hpp"
#include "SkrSceneCore/scene_components.h"

#if !defined(__meta__)
    #include "SkrScene/actor.generated.h"
#endif

namespace skr
{

sreflect_enum_class(guid = "a1ebd9b1-900c-44f4-b381-0dd48014718d")
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
    friend class ActorManager;

public:
    SKR_RC_IMPL();
    virtual ~Actor() SKR_NOEXCEPT;
    static RCWeak<Actor> GetRoot();
    static RCWeak<Actor> CreateActor(EActorType type = EActorType::Default);

    void CreateEntity();
    skr::ecs::Entity GetEntity() const;
    void AttachTo(RCWeak<Actor> parent, EAttachRule rule = EAttachRule::Default);
    void DetachFromParent();

    struct Spawner
    {
        using BuildF = std::function<void(skr::ecs::ArchetypeBuilder&)>;
        using RunF = std::function<void(skr::ecs::TaskContext&)>;
        Spawner(BuildF build_func, RunF func)
            : build_func(build_func)
            , f(func)
        {
        }

        void build(skr::ecs::ArchetypeBuilder& Builder)
        {
            build_func(Builder);
        }
        void run(skr::ecs::TaskContext& Context)
        {
            SkrZoneScopedN("Spawner");
            f(Context);
        }
        BuildF build_func;
        RunF f;
    };
    Spawner spawner;

    // getters & setters
    inline const skr::String& GetDisplayName() const { return display_name; }
    inline void SetDisplayName(const skr::String& name) { display_name = name; }
    inline EActorType GetActorType() const { return actor_type; }
    skr::scene::ScaleComponent* GetScaleComponent() const;
    skr::scene::PositionComponent* GetPositionComponent() const;
    skr::scene::RotationComponent* GetRotationComponent() const;
    skr::scene::TransformComponent* GetTransformComponent() const;
    skr::GUID GetGUID() const { return guid; }

protected:
    explicit Actor(EActorType type = EActorType::Default) SKR_NOEXCEPT;

    skr::String display_name;             // for editor, profiler, and runtime dump
    skr::GUID guid = skr::GUID::Create(); // guid for each actor, used to identify actors in the scene
    skr::InlineVector<skr::ecs::Entity, 1> scene_entities;
    skr::Vector<skr::RC<Actor>> children;
    skr::RC<Actor> _parent = nullptr;
    EAttachRule attach_rule = EAttachRule::Default;
    EActorType actor_type = EActorType::Default;
};

class SKR_SCENE_API ActorManager
{
public:
    static ActorManager& GetInstance()
    {
        static ActorManager instance;
        return instance;
    }
    void initialize(skr::ecs::World* world);
    void finalize();
    skr::RCWeak<Actor> CreateActor(EActorType type = EActorType::Default);
    bool DestroyActor(skr::GUID guid);
    void CreateActorEntity(skr::RCWeak<Actor> actor);
    void DestroyActorEntity(skr::RCWeak<Actor> actor);
    void UpdateHierarchy(skr::RCWeak<Actor> parent, skr::RCWeak<Actor> child, EAttachRule rule = EAttachRule::Default);

    void ClearAllActors();
    skr::RCWeak<Actor> GetRoot();
    // accessors
    skr::ecs::RandomComponentReadWrite<skr::scene::ParentComponent> parent_accessor;
    skr::ecs::RandomComponentReadWrite<skr::scene::ChildrenComponent> children_accessor;
    skr::ecs::RandomComponentReadWrite<skr::scene::PositionComponent> pos_accessor;
    skr::ecs::RandomComponentReadWrite<skr::scene::RotationComponent> rot_accessor;
    skr::ecs::RandomComponentReadWrite<skr::scene::ScaleComponent> scale_accessor;
    skr::ecs::RandomComponentReadWrite<skr::scene::TransformComponent> trans_accessor;

protected:
    // Factory method to create specific actor types
    virtual skr::RC<Actor> CreateActorInstance(EActorType type);

private:
    ActorManager() = default;
    ~ActorManager() = default;
    skr::ecs::World* world = nullptr; // Pointer to the ECS world for actor management

    // Disable copy and move semantics
    ActorManager(const ActorManager&) = delete;
    ActorManager& operator=(const ActorManager&) = delete;
    ActorManager(ActorManager&&) = delete;
    ActorManager& operator=(ActorManager&&) = delete;

    // Currently, we only use Map<GUID, Actor*> and cpp new/delete for Actor management.
    // In the future, we can implement a more sophisticated memory management system.
    skr::Map<skr::GUID, skr::RC<Actor>> actors; // Map to manage actors by their GUIDs
};

class SKR_SCENE_API MeshActor : public Actor
{
    friend class ActorManager;

public:
    SKR_RC_IMPL();

    ~MeshActor() SKR_NOEXCEPT;

protected:
    MeshActor();
};

// Actor for Skeletal Meshes
class SKR_SCENE_API SkelMeshActor : public MeshActor
{
    friend class ActorManager;

public:
    SKR_RC_IMPL();

    ~SkelMeshActor() SKR_NOEXCEPT override;

protected:
    SkelMeshActor()
        : MeshActor()
    {
        actor_type = EActorType::SkelMesh;
    }
};

} // namespace skr