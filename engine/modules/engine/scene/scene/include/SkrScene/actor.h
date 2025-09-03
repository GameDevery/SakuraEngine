#pragma once
#include "SkrBase/types/impl/guid.hpp"
#include "SkrContainersDef/string.hpp"
#include "SkrCore/memory/rc.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/map.hpp"
#include "SkrRT/ecs/component.hpp"
#include "SkrRT/resource/resource_handle.h"
#include "SkrRT/ecs/world.hpp"
#include "SkrRTTR/rttr_traits.hpp"
#include "SkrRTTR/type.hpp"
#include "SkrRTTR/type_registry.hpp"
#include "SkrSceneCore/scene_components.h"
#include "SkrRenderer/render_mesh.h"
#include "SkrSerde/json_serde.hpp"

#if !defined(__meta__)
    #include "SkrScene/actor.generated.h"
#endif

namespace skr
{

sreflect_enum_class(
    guid = "a1ebd9b1-900c-44f4-b381-0dd48014718d" serde = @json)
EAttachRule{
    Default = 0,
    KeepWorldTransform = 0x01
};

sreflect_struct(
    guid = "4cb20865-0d27-43ee-90b9-7b43ac4c067c";
    rttr = @enable;
    serde = @bin | @json)
SKR_SCENE_API Actor
{
    friend class ActorManager;

public:
    SKR_GENERATE_BODY()
    SKR_RC_IMPL();

    Actor() SKR_NOEXCEPT {}
    virtual ~Actor() SKR_NOEXCEPT;
    static RCWeak<Actor> GetRoot();
    void BindWorld(skr::ecs::World * world) { this->world = world; }
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

    sattr(serde = @disable)
    skr::UPtr<Spawner> spawner;

protected:
    virtual void Initialize(); // The Actual Initialize Method

protected:
    friend JsonSerde<Actor>;
    skr::String display_name; // for editor, profiler, and runtime dump
    skr::GUID guid;
    skr::GUID rttr_type_guid;


    EAttachRule attach_rule = EAttachRule::Default;
    bool bIsInitialized = false;

    sattr(serde = @disable)
    skr::InlineVector<skr::ecs::Entity, 1> scene_entities;
    sattr(serde = @disable)
    skr::Vector<skr::RC<Actor>> children;
    sattr(serde = @disable)
    skr::RC<Actor> _parent = nullptr;
    sattr(serde = @disable)
    skr::ecs::World* world = nullptr; // Pointer to the ECS world for actor management
    

    skr::SerializeConstVector<skr_guid_t> children_serialized;
    skr_guid_t parent_serialized = skr_guid_t{};
    skr::SerializeConstVector<sugoi_entity_t> scene_entities_serialized;

public:
    void serialize() SKR_NOEXCEPT;
    void deserialize() SKR_NOEXCEPT;
    inline const skr::String GetDisplayName() const { return display_name; }
    inline void SetDisplayName(const skr::String& name) { display_name = name; }
    template <typename ComponentType>
    ComponentType* GetComponent() const
    {
        auto entity = GetEntity();
        if (entity != skr::ecs::Entity{ SUGOI_NULL_ENTITY })
        {
            return world->random_readwrite<ComponentType>().get(entity);
        }
        SKR_LOG_ERROR(u8"Actor {%s} has no valid entity to get Component", display_name.c_str());
        return nullptr;
    }

    skr::GUID GetGUID() const SKR_NOEXCEPT { return guid; }
};
class SKR_SCENE_API ActorManager
{
public:
    static ActorManager& GetInstance()
    {
        static ActorManager instance;
        return instance;
    }
    void Initialize(skr::ecs::World* world);
    void Finalize();

    template <typename T>
    skr::RCWeak<Actor> CreateActor()
    {
        auto actor = CreateActorInstance<T>();
        actor.get()->Initialize();
        actor.get()->BindWorld(world);
        actors.add(actor->guid, actor);
        return actor;
    }
    template <typename T>
    skr::RC<Actor> CreateActorInstance()
    {
        return skr::RC<T>::New();
    }

    skr::RC<Actor> CreateActor(skr::GUID actor_rttr_guid);
    skr::RCWeak<Actor> GetActor(skr::GUID guid);

    bool DestroyActor(skr::GUID guid);
    void CreateActorEntity(skr::RCWeak<Actor> actor);
    void DestroyActorEntity(skr::RCWeak<Actor> actor);
    void UpdateHierarchy(skr::RCWeak<Actor> parent, skr::RCWeak<Actor> child, EAttachRule rule = EAttachRule::Default);

    void ClearAllActors();
    skr::RCWeak<Actor> GetRoot();

protected:
    // Factory method to create specific actor types

private:
    ActorManager() = default;
    ~ActorManager() = default;
    ActorManager(const ActorManager&) = delete;
    ActorManager& operator=(const ActorManager&) = delete;
    ActorManager(ActorManager&&) = delete;
    ActorManager& operator=(ActorManager&&) = delete;

    skr::ecs::World* world = nullptr; // Pointer to the ECS world for actor management
    // Currently, we only use Map<GUID, Actor*> and cpp new/delete for Actor management.
    // In the future, we can implement a more sophisticated memory management system.
    skr::Map<skr::GUID, skr::RC<Actor>> actors; // Map to manage actors by their GUIDs
};

// Actor that Carry ECS World Instance, and ActorRootComponent
sreflect_struct(
    guid = "01990a69-0bbf-7483-bbbf-36ab1580e83a";
    rttr = @enable)
SKR_SCENE_API RootActor : public Actor
{
public:
    RootActor() {}
    ~RootActor() SKR_NOEXCEPT override;
    skr::ecs::World* GetWorld() const { return world.get(); }
    void bind_scheduler(skr::task::scheduler_t& scheduler) { world->bind_scheduler(scheduler); }

protected:
    skr::UPtr<skr::ecs::World> world = nullptr;
    void Initialize() override;
    void InitWorld();
};

sreflect_struct(
    guid = "01987a21-a2b4-7488-924d-17639e937f87";
    rttr = @enable;)
SKR_SCENE_API MeshActor : public Actor
{
public:
    MeshActor() SKR_NOEXCEPT {}
    ~MeshActor() SKR_NOEXCEPT override;

protected:
    void Initialize() override;
};

sreflect_struct(
    guid = "01987a21-e796-76b6-89c4-fb550edf5610";
    rttr = @enable;)
SKR_SCENE_API SkelMeshActor : public MeshActor
{

public:
    SkelMeshActor() SKR_NOEXCEPT {}
    ~SkelMeshActor() SKR_NOEXCEPT override;
    void Initialize() override;
};

} // namespace skr