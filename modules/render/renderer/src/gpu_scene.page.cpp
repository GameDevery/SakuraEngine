#include "SkrRenderer/gpu_scene.h"
#include "SkrCore/log.h"
#include "SkrRT/ecs/world.hpp"

namespace skr::renderer
{

void GPUScene::RequireUpload(skr::ecs::Entity entity, CPUTypeID component)
{
    auto cs = dirties.try_add_default(entity);
    cs.value().add_unique(component);
    if (!cs.already_exist())
        dirty_ents.add(entity);
}

void GPUScene::ExecuteUpload(skr::render_graph::RenderGraph* graph)
{
    SkrZoneScopedN("GPUScene::ExecuteUpload");

    AdjustDataBuffers();

    struct ScanGPUScene
    {
        void build(skr::ecs::AccessBuilder& builder)
        {
            builder.access(&ScanGPUScene::instances);
        }
        void run(skr::ecs::Entity entity, uint32_t offset)
        {

        }
        skr::ecs::RandomComponentReader<const GPUSceneInstance> instances;
    } scan;
    ecs_world->dispatch_task(scan, 512, dirty_ents);
}

} // namespace skr::renderer