// The SceneSample_Simple Model
#
#include <SkrCore/module/module_manager.hpp>
#include <SkrCore/log.h>
#include <SkrScene/actor.h>
#include <SkrScene/scene_components.h>
#include <SkrRT/ecs/world.hpp>

struct SceneSampleSimpleModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;
};

SceneSampleSimpleModule g_scene_sample_simple_module;
IMPLEMENT_DYNAMIC_MODULE(SceneSampleSimpleModule, SceneSample_Simple);
void SceneSampleSimpleModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"Scene Sample Simple Module Loaded");
}

void SceneSampleSimpleModule::on_unload()
{
    SKR_LOG_INFO(u8"Scene Sample Simple Module Unloaded");
}

int SceneSampleSimpleModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("SceneSampleSimpleModule::main_module_exec");
    SKR_LOG_INFO(u8"Running Scene Sample Simple Module");

    auto root = skr::Actor::GetRoot();
    auto actor1 = skr::Actor::CreateActor();
    auto actor2 = skr::Actor::CreateActor();
    actor2.lock()->AttachTo(actor1, skr::EAttachRule::KeepWorldTransform);

    // random access sample
    sugoi_storage_t* storage;
    skr::task::scheduler_t scheduler;
    scheduler.initialize(skr::task::scheudler_config_t());
    scheduler.bind();
    skr::ecs::World world(scheduler);
    world.initialize();

    actor1.lock()->transform_entities.resize_zeroed(1);
    // spawn one entity in ECS world
    struct Spawner
    {
        using F = std::function<void(skr::ecs::TaskContext&)>;
        Spawner(F func)
            : f(func)
        {
        }

        void build(skr::ecs::ArchetypeBuilder& Builder)
        {
            Builder.add_component<skr::scene::TransformComponent>();
        }
        void run(skr::ecs::TaskContext& Context)
        {
            SkrZoneScopedN("Spawner");
            f(Context);
        }
        F f;
    };
    Spawner spawner{ [&](skr::ecs::TaskContext& Context) {
        actor1.lock()->transform_entities[0] = Context.entities()[0];
    } };
    world.create_entities(spawner, 1);

    struct RWriteJob0
    {
        using F = std::function<void(skr::ecs::TaskContext&, skr::ecs::RandomComponentWriter<skr::scene::TransformComponent>&)>;
        RWriteJob0(F func)
            : f(func)
        {
        }

        void build(skr::ecs::AccessBuilder& Builder)
        {
            Builder.access(&RWriteJob0::trans_accessor);
        }

        void run(skr::ecs::TaskContext& Context)
        {
            SkrZoneScopedN("RWriteJob0");
            f(Context, RWriteJob0::trans_accessor);
        }

        skr::ecs::RandomComponentWriter<skr::scene::TransformComponent> trans_accessor;
        F f;
    };

    RWriteJob0 rwritejob0{ [&](skr::ecs::TaskContext& Context, skr::ecs::RandomComponentWriter<skr::scene::TransformComponent>& Accessor) {
        SkrZoneScopedN("RWriteJob0::run");
        Accessor.write_at((skr::ecs::Entity)actor1.lock()->transform_entities[0], skr::scene::TransformComponent{ { 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 1.0f }, { 2.0f, 2.0f, 2.0f } });
    } };

    auto q1 = world.dispatch_task(rwritejob0, 1, nullptr);

    struct RReadJob0
    {
        using F = std::function<void(skr::ecs::TaskContext&, skr::ecs::RandomComponentReader<const skr::scene::TransformComponent>&)>;
        RReadJob0(F func)
            : f(func)
        {
        }

        void build(skr::ecs::AccessBuilder& Builder)
        {
            Builder.access(&RReadJob0::trans_accessor);
        }
        void run(skr::ecs::TaskContext& Context)
        {
            SkrZoneScopedN("RReadJob0");
            f(Context, RReadJob0::trans_accessor);
        }

        skr::ecs::RandomComponentReader<const skr::scene::TransformComponent> trans_accessor;
        F f;
    };

    RReadJob0 rreadjob0{ [&](skr::ecs::TaskContext& Context, skr::ecs::RandomComponentReader<const skr::scene::TransformComponent>& Accessor) {
        SkrZoneScopedN("RReadJob0::run");
        auto transform = Accessor[(skr::ecs::Entity)actor1.lock()->transform_entities[0]].get();
        SKR_LOG_INFO(u8"Transform Position: ({%f}, {%f}, {%f})", transform.position.x, transform.position.y, transform.position.z);
        SKR_LOG_INFO(u8"Transform Rotation: ({%f}, {%f}, {%f}, {%f})", transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
        SKR_LOG_INFO(u8"Transform Scale: ({%f}, {%f}, {%f})", transform.scale.x, transform.scale.y, transform.scale.z);
    } };

    auto q2 = world.dispatch_task(rreadjob0, 1, nullptr);
    //skr::ecs::TaskScheduler::Get()->flush_all();
    skr::ecs::TaskScheduler::Get()->sync_all();

    // cleanup
    world.finalize();
    scheduler.unbind();
    skr::SActorManager::GetInstance()
        .ClearAllActors();
    return 0;
}