#pragma once
#include "SkrBase/config.h"
#include "SkrScene/scene_components.h"

namespace skr::ecs { struct World; }

namespace skr
{
struct SKR_SCENE_API TransformSystem {
public:
    static TransformSystem* Create(skr::ecs::World* world) SKR_NOEXCEPT;
    static void Destroy(TransformSystem* system) SKR_NOEXCEPT;
    void update() SKR_NOEXCEPT;


private:
    // 正向全量脏树传播：每帧调用，处理所有标记为脏的变换树，进行批量计算
    void CalculateFromRoot() SKR_NOEXCEPT;

    // 反向触发：反向寻找最高的脏节点，一路计算下来，并且刷新所有的子树
    void CalculateTransform(sugoi_entity_t entity) SKR_NOEXCEPT;

private:
    TransformSystem() SKR_NOEXCEPT = default;
    ~TransformSystem() SKR_NOEXCEPT = default;
    struct Impl;
    Impl* impl;
};
} // namespace skr

SKR_EXTERN_C SKR_SCENE_API skr::TransformSystem* skr_transform_system_create(skr::ecs::World* world);
SKR_EXTERN_C SKR_SCENE_API void skr_transform_system_destroy(skr::TransformSystem* system);
SKR_EXTERN_C SKR_SCENE_API void skr_transform_system_update(skr::TransformSystem* system);

SKR_EXTERN_C SKR_SCENE_API void skr_save_scene(skr::ecs::World* world, struct skr::archive::JsonWriter* writer);
SKR_EXTERN_C SKR_SCENE_API void skr_load_scene(skr::ecs::World* world, struct skr::archive::JsonReader* reader);
