#pragma once
// The Scene ECS Components Definitions
#include "SkrRT/ecs/component.hpp"
#include "SkrBase/math.h"
#ifndef __meta__
    #include "SkrScene/scene_components.generated.h" // IWYU pragma: export
#endif

#ifndef SKR_SCENE_MAX_NAME_LENGTH
    #define SKR_SCENE_MAX_NAME_LENGTH 32
#endif

namespace skr::scene
{

#define SCENE_PRECISION_FLOAT
#ifdef SCENE_PRECISION_DOUBLE
using PositionElement = double;
using Position = double3;
using Transform = skr_transform_d_t;
#elif defined(SCENE_PRECISION_FLOAT)
using PositionElement = float;
using Position = float3;
using Transform = skr_transform_f_t;
#endif
using Rotator = skr::RotatorF;
struct TransformJob;

sreflect_struct(
    guid = "01981176-1f33-777f-b448-10ddaf1d03e2";
    ecs.comp = @enable;)
IndexComponent
{
    uint32_t value;
};

// transforms

sreflect_struct(
    guid = "01981176-5d6c-777a-8b33-feb16c1de2b6";
    serde = @bin | @json;
    ecs.comp = @enable;)
RotationComponent
{
public:
    SKR_GENERATE_BODY()

    inline RotationComponent() = default;
    inline RotationComponent(float pitch, float yaw, float roll)
        : euler(pitch, yaw, roll)
        , dirty(true)
    {
    }
    inline RotationComponent(Rotator v)
        : euler(v)
        , dirty(true)
    {
    }

    inline void set(float pitch, float yaw, float roll)
    {
        euler = { pitch, yaw, roll };
        dirty = true;
    }
    inline void set(Rotator v)
    {
        euler = v;
        dirty = true;
    }
    inline auto get() const { return euler; }
    inline bool get_dirty() const { return dirty; }

private:
    friend struct TransformJob;
    Rotator euler;
    mutable bool dirty;
};
static_assert(sizeof(RotationComponent) <= sizeof(float) * 4, "RotationComponent size mismatch!");

sreflect_struct(
    guid = "01981176-7361-72ca-a0c5-5b539d1764e1";
    serde = @bin | @json;
    ecs.comp = @enable;)
SKR_ALIGNAS(16) PositionComponent
{
public:
    SKR_GENERATE_BODY()

    inline PositionComponent() = default;
    inline PositionComponent(PositionElement x, PositionElement y, PositionElement z)
        : value(x, y, z)
        , dirty(true)
    {
    }
    inline PositionComponent(Position v)
        : value(v)
        , dirty(true)
    {
    }

    inline void set(PositionElement x, PositionElement y, PositionElement z)
    {
        value = { x, y, z };
        dirty = true;
    }
    inline void set(Position v)
    {
        value = v;
        dirty = true;
    }
    inline Position get() const { return value; }
    inline bool get_dirty() const { return dirty; }

private:
    friend struct TransformJob;
    Position value;
    mutable bool dirty;
};
static_assert(sizeof(PositionComponent) <= sizeof(PositionElement) * 4, "PositionComponent size mismatch!");

sreflect_struct(
    guid = "01981176-8537-72d1-88a4-e459116c0717";
    serde = @bin | @json;
    ecs.comp = @enable;)
SKR_ALIGNAS(16) ScaleComponent
{
public:
    SKR_GENERATE_BODY()

    inline ScaleComponent() = default;
    inline ScaleComponent(float x, float y, float z)
        : value(x, y, z)
        , dirty(true)
    {
    }
    inline ScaleComponent(float scalar)
        : value(scalar, scalar, scalar)
        , dirty(true)
    {
    }
    inline ScaleComponent(skr::float3 scale)
        : value(scale)
        , dirty(true)
    {
    }

    inline void set(float scale)
    {
        value = { scale, scale, scale };
        dirty = true;
    }
    inline void set(float x, float y, float z)
    {
        value = { x, y, z };
        dirty = true;
    }
    inline void set(skr::float3 scale)
    {
        value = scale;
        dirty = true;
    }
    inline skr::float3 get() const { return value; }
    inline bool get_dirty() const { return dirty; }

private:
    friend struct TransformJob;
    skr::float3 value;
    mutable bool dirty;
};
static_assert(sizeof(ScaleComponent) <= sizeof(float) * 4, "ScaleComponent size mismatch!");

sreflect_struct(
    guid = "01981176-4865-779f-9e5d-ebaf834fc004";
    ecs.comp = @enable;)
SKR_ALIGNAS(16) TransformComponent
{
public:
    inline TransformComponent(Position position, QuatF rotation, float3 scale)
    {
        set(position, rotation, scale);
    }
    inline Transform get() const { return value; }
    inline void set(Transform transform) { value = transform; }
    inline void set(Position position, QuatF rotation, float3 scale)
    {
        value.position = position;
        value.rotation = { rotation.x, rotation.y, rotation.z, rotation.w };
        value.scale = { scale.x, scale.y, scale.z };
    }

private:
    Transform value;
};

sreflect_struct(
    guid = "01981176-b891-77ed-aafb-05019c8340d0";
    ecs.comp = @enable;)
CameraComponent
{
    uint32_t viewport_id;
    uint32_t viewport_width;
    uint32_t viewport_height;
};

sreflect_struct(
    guid = "01981177-1059-7687-8b72-50788033fdd4";
    ecs.comp = @enable;)
ParentComponent
{
    skr::ecs::Entity entity;
};

sreflect_struct(
    guid = "01981176-f870-738b-8cac-5e3d87ba37b5";
    ecs.comp.array = 4;)
ChildrenComponent
{
    skr::ecs::Entity entity;
};

#ifdef __cplusplus
using ChildrenArray = sugoi::ArrayComponent<ChildrenComponent, 4>;
#endif

sreflect_enum_class(
    guid = "c8df77f2-9830-4004-84e6-1d55e0f9a83f" serde = @bin | @json)
ETransformDirtyState : uint32_t{
    NotDirty = 0,
    Location = 0x1,
    Rotation = 0x2,
    Scale = 0x4,
    Transform = Location | Rotation | Scale,

    Relative = 0x100,
    Absolute = 0x200
};

sreflect_struct(
    guid = "0f3ca51a-3d6d-4614-97dc-a388969301a9";
    ecs.comp = @enable;)
TransformDirtyComponent
{
    ETransformDirtyState this_state = ETransformDirtyState::NotDirty;
};

// ------------------------------------------------------
// ETransformDirtyState flag operators
// ------------------------------------------------------

inline static ETransformDirtyState operator|(ETransformDirtyState l, ETransformDirtyState r)
{
    return static_cast<ETransformDirtyState>(static_cast<uint32_t>(l) | static_cast<uint32_t>(r));
}

inline static ETransformDirtyState operator&(ETransformDirtyState l, ETransformDirtyState r)
{
    return static_cast<ETransformDirtyState>(static_cast<uint32_t>(l) & static_cast<uint32_t>(r));
}

inline static ETransformDirtyState operator^(ETransformDirtyState l, ETransformDirtyState r)
{
    return static_cast<ETransformDirtyState>(static_cast<uint32_t>(l) ^ static_cast<uint32_t>(r));
}

inline static ETransformDirtyState operator~(ETransformDirtyState v)
{
    return static_cast<ETransformDirtyState>(~static_cast<uint32_t>(v));
}

inline static ETransformDirtyState& operator|=(ETransformDirtyState& l, ETransformDirtyState r)
{
    l = l | r;
    return l;
}

inline static ETransformDirtyState& operator&=(ETransformDirtyState& l, ETransformDirtyState r)
{
    l = l & r;
    return l;
}

inline static ETransformDirtyState& operator^=(ETransformDirtyState& l, ETransformDirtyState r)
{
    l = l ^ r;
    return l;
}

inline static bool operator!(ETransformDirtyState v)
{
    return static_cast<uint32_t>(v) == 0;
}

inline static bool HasFlag(ETransformDirtyState value, ETransformDirtyState flag)
{
    return (value & flag) == flag;
}

// ------------------------------------------------------
// Component operators
// ------------------------------------------------------

inline static bool operator==(skr::scene::ScaleComponent l, skr::scene::ScaleComponent r)
{
    return all(l.get() == r.get());
}
inline static bool operator!=(skr::scene::ScaleComponent l, skr::scene::ScaleComponent r)
{
    return any(l.get() != r.get());
}

inline static bool operator==(skr::scene::PositionComponent l, skr::scene::PositionComponent r)
{
    return all(l.get() == r.get());
}
inline static bool operator!=(skr::scene::PositionComponent l, skr::scene::PositionComponent r)
{
    return any(l.get() != r.get());
}

inline static bool operator==(skr::scene::RotationComponent l, skr::scene::RotationComponent r)
{
    return all(l.get() == r.get());
}
inline static bool operator!=(skr::scene::RotationComponent l, skr::scene::RotationComponent r)
{
    return any(l.get() != r.get());
}

} // namespace skr::scene

#ifndef SKR_SCENE_COMPONENTS
    #define SKR_SCENE_COMPONENTS                                                                      \
        skr::scene::ParentComponent, skr::scene::ChildrenComponent,                                   \
            skr::scene::TransformComponent,                                                           \
            skr::scene::PositionComponent, skr::scene::RotationComponent, skr::scene::ScaleComponent, \
            skr::scene::TransformDirtyComponent
#endif