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
            component_readers.resize_zeroed(pScene->component_types.size());
            builder.access(&ScanGPUScene::instances);
            for (auto [cpu_type, gpu_type] : pScene->type_registry)
            {
                builder.random_read(cpu_type, &component_readers[gpu_type]);
            }
        }
        void run(skr::ecs::Entity entity, uint32_t offset)
        {
            for (auto [cpu_type, gpu_type] : pScene->type_registry)
            {
                sugoi_chunk_view_t v = {};
                sugoiS_access(pScene->ecs_world->get_storage(), entity, &v);
                
                const auto& gpu_type_info = pScene->component_types[gpu_type];
                auto component_data = component_readers[gpu_type].get<uint8_t>(entity);

            }
        }
        skr::renderer::GPUScene* pScene = nullptr;
        skr::ecs::RandomComponentReader<const GPUSceneInstance> instances;
        skr::Vector<skr::ecs::RandomComponentReader<>> component_readers;
    } scan;
    ecs_world->dispatch_task(scan, 512, dirty_ents);


}

} // namespace skr::renderer