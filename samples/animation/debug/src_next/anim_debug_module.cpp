#include "SkrCore/async/async_service.h"
#include "SkrCore/module/module.hpp"
#include "SkrCore/log.h"
#include "SkrRT/ecs/component.hpp"
#include "SkrRT/ecs/scheduler.hpp"
#include "SkrRT/ecs/world.hpp"

#include "AnimView/components.h"

class SAnimDebugModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;
};

static SAnimDebugModule* g_anim_debug_module = nullptr;
///////
/// Module entry point
///////
IMPLEMENT_DYNAMIC_MODULE(SAnimDebugModule, AnimDebugNext);

void SAnimDebugModule::on_load(int argc, char8_t** argv)
{
}

void SAnimDebugModule::on_unload()
{
    g_anim_debug_module = nullptr;
    SKR_LOG_INFO(u8"anim debug runtime unloaded!");
}

int SAnimDebugModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("AnimDebugExecution");
    SKR_LOG_INFO(u8"anim debug runtime executed as main module!");



    skr::task::scheduler_t task_scheduler;
    skr::ecs::World world{task_scheduler};
    world.initialize();
    sugoi_storage_t* storage = world.get_storage();

    struct Spawner
    {
        void build(skr::ecs::ArchetypeBuilder& Builder)
        {
            Builder.add_component(&Spawner::dummy);
        }

        void run(skr::ecs::TaskContext& ctx) 
        {
            auto entities = ctx.entities();
            result = entities[0];

            auto compDummy = ctx.components<animd::DummyComponent>();

            compDummy[0].value = 42; // Initialize dummy component
        }

        skr::ecs::ComponentView<animd::DummyComponent> dummy;
        uint32_t result = SUGOI_NULL_ENTITY;
    } spawner;
    world.create_entities(spawner, 1);

    // query
    struct ReadJob 
    {
        void build(skr::ecs::AccessBuilder& Builder){
            Builder.read(&ReadJob::dummy);
        }
        void run(skr::ecs::TaskContext& Context) {
            SkrZoneScopedN("ReadJob");
            for (auto i = 0; i < Context.size(); i++)
            {
                SKR_LOG_INFO(u8"Dummy Component Value: %d", dummy[i].value);
            }
        }
        skr::ecs::ComponentView<const animd::DummyComponent> dummy;
    } rjob;
    auto q1 = world.dispatch_task(rjob, 128, nullptr);
    world.get_scheduler()->sync_all();




    world.finalize();

    return 0;
}