#pragma once
// The Scene ECS Components Definitions
#include "SkrRT/ecs/sugoi_types.h"
#include "SkrBase/math.h"

#ifndef __meta__
    #include "SkrScene/scene_components.generated.h" // IWYU pragma: export
#endif

#ifndef SKR_SCENE_MAX_NAME_LENGTH
    #define SKR_SCENE_MAX_NAME_LENGTH 32
#endif

struct SRenderer;

namespace skr::scene
{
// scene hierarchy

sreflect_struct(
    guid = "01981176-07ce-72ec-8ac0-ffd55ac16d14";
    ecs.comp = @enable;
)
    NameComponent {
    char str[SKR_SCENE_MAX_NAME_LENGTH + 1];
};

sreflect_struct(
    guid = "01981176-1f33-777f-b448-10ddaf1d03e2";
    ecs.comp = @enable;
)
    IndexComponent {
    uint32_t value;
};

// transforms

sreflect_struct(
    guid = "01981176-4865-779f-9e5d-ebaf834fc004";
    ecs.comp = @enable;
)
    SKR_ALIGNAS(16)
        TransformComponent {
    skr_transform_f_t value;
};

sreflect_struct(
    guid = "01981176-5d6c-777a-8b33-feb16c1de2b6";
    serde = @bin|@json;
    ecs.comp = @enable;
)
    RotationComponent {
    skr_rotator_f_t euler;
};

sreflect_struct(
    guid = "01981176-7361-72ca-a0c5-5b539d1764e1";
    serde = @bin|@json;
    ecs.comp = @enable;
)
    TranslationComponent {
    skr_float3_t value;
};

sreflect_struct(
    guid = "01981176-8537-72d1-88a4-e459116c0717";
    serde = @bin|@json;
    ecs.comp = @enable;
)
    ScaleComponent {
    skr_float3_t value;
};

sreflect_struct(
    guid = "01981176-9872-72cb-acd4-2b344136f36f";
    ecs.comp = @enable;
)
    MovementComponent {
    skr_float3_t value;
};

sreflect_struct(
    guid = "01981176-b891-77ed-aafb-05019c8340d0";
    ecs.comp = @enable;
)
    CameraComponent {
    struct SRenderer* renderer;
    uint32_t          viewport_id;
    uint32_t          viewport_width;
    uint32_t          viewport_height;
};

sreflect_struct(
    guid = "01981176-e2b1-75b9-ae84-e8edf3eda1a4";
    ecs.comp = @enable;
)
    RootComponent {
    uint32_t _;
};

sreflect_struct(
    guid = "01981176-f870-738b-8cac-5e3d87ba37b5";
    ecs.comp.array = 4;
) ChildrenComponent {
    sugoi_entity_t entity;
};

#ifdef __cplusplus
using ChildrenArray = sugoi::ArrayComponent<ChildrenComponent, 4>;
#endif

sreflect_struct(
    guid = "01981177-1059-7687-8b72-50788033fdd4";
    ecs.comp = @enable;
)
    ParentComponent {
    sugoi_entity_t entity;
};

} // namespace skr::scene

#ifndef SKR_SCENE_COMPONENTS
    #define SKR_SCENE_COMPONENTS                      \
        skr::ParentComponent, skr::ChildrenComponent, \
            skr::TransformComponent,                  \
            skr::TranslationComponent, skr::RotationComponent, skr::ScaleComponent
#endif

// ------------------------------------------------------
// Component operators
// ------------------------------------------------------

inline static bool operator==(skr::scene::ScaleComponent l, skr::scene::ScaleComponent r)
{
    return all(l.value == r.value);
}
inline static bool operator!=(skr::scene::ScaleComponent l, skr::scene::ScaleComponent r)
{
    return any(l.value != r.value);
}

inline static bool operator==(skr::scene::TranslationComponent l, skr::scene::TranslationComponent r)
{
    return all(l.value == r.value);
}
inline static bool operator!=(skr::scene::TranslationComponent l, skr::scene::TranslationComponent r)
{
    return any(l.value != r.value);
}

inline static bool operator==(skr::scene::RotationComponent l, skr::scene::RotationComponent r)
{
    return all(l.euler == r.euler);
}
inline static bool operator!=(skr::scene::RotationComponent l, skr::scene::RotationComponent r)
{
    return any(l.euler != r.euler);
}