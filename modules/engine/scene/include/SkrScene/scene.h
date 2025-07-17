#pragma once
#include "SkrBase/config.h"
#include "SkrRT/ecs/sugoi_types.h"
#include "SkrScene/scene_components.h"

namespace skr
{
struct SKR_SCENE_API TransformSystem {
public:
    static TransformSystem* Create(sugoi_storage_t* world) SKR_NOEXCEPT;
    static void Destroy(TransformSystem* system) SKR_NOEXCEPT;

    void update() SKR_NOEXCEPT;

private:
    TransformSystem() SKR_NOEXCEPT = default;
    ~TransformSystem() SKR_NOEXCEPT = default;
    struct Impl;
    Impl* impl;
};
} // namespace skr

SKR_EXTERN_C SKR_SCENE_API skr::TransformSystem* skr_transform_system_create(sugoi_storage_t* world);
SKR_EXTERN_C SKR_SCENE_API void skr_transform_system_destroy(skr::TransformSystem* system);
SKR_EXTERN_C SKR_SCENE_API void skr_transform_system_update(skr::TransformSystem* system);

SKR_EXTERN_C SKR_SCENE_API void skr_propagate_transform(sugoi_storage_t* world, sugoi_entity_t* entities, uint32_t count);
SKR_EXTERN_C SKR_SCENE_API void skr_save_scene(sugoi_storage_t* world, struct skr::archive::JsonWriter* writer);
SKR_EXTERN_C SKR_SCENE_API void skr_load_scene(sugoi_storage_t* world, struct skr::archive::JsonReader* reader);
