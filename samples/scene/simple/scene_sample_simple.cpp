// The SceneSample_Simple: The Principle for Scene Transform Update task
#include "SkrBase/math/gen/misc/transform.hpp"
#include <SkrCore/module/module_manager.hpp>
#include <SkrCore/log.h>
#include <SkrScene/actor.h>
#include <SkrScene/scene_components.h>
#include <SkrScene/transform_system.h>
#include <SkrRT/ecs/world.hpp>

struct SceneSampleSimpleModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

    skr::TransformSystem* transform_system = nullptr;
};

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
    skr::task::scheduler_t scheduler;

    scheduler.initialize(skr::task::scheudler_config_t());
    scheduler.bind();
    skr::ecs::World world(scheduler);
    world.initialize();
    transform_system = skr_transform_system_create(&world);
    auto& actor_manager = skr::ActorManager::GetInstance();
    actor_manager.initialize(&world);

    auto root = skr::Actor::GetRoot();
    auto actor1 = skr::Actor::CreateActor();
    auto actor2 = skr::Actor::CreateActor();
    root.lock()->CreateEntity();
    actor1.lock()->CreateEntity();
    actor2.lock()->CreateEntity();
    actor1.lock()->AttachTo(root);
    actor2.lock()->AttachTo(actor1);

    actor_manager.pos_accessor.write_at(actor1.lock()->GetEntity(), skr::scene::PositionComponent{ { 0.0f, 1.0f, 0.0f } });
    actor_manager.pos_accessor.write_at(actor2.lock()->GetEntity(), skr::scene::PositionComponent{ { 1.0f, 1.0f, 1.0f } });
    transform_system->update();
    skr::ecs::TaskScheduler::Get()->flush_all();
    skr::ecs::TaskScheduler::Get()->sync_all();
    auto transform = actor_manager.trans_accessor[(skr::ecs::Entity)actor1.lock()->GetEntity()].get();
    SKR_LOG_INFO(u8"Transform Position: ({%f}, {%f}, {%f})", transform.position.x, transform.position.y, transform.position.z);
    SKR_LOG_INFO(u8"Transform Rotation: ({%f}, {%f}, {%f}, {%f})", transform.rotation.x, transform.rotation.y, transform.rotation.z, transform.rotation.w);
    SKR_LOG_INFO(u8"Transform Scale: ({%f}, {%f}, {%f})", transform.scale.x, transform.scale.y, transform.scale.z);

    // auto q2 = world.dispatch_task(rreadjob0, 1, nullptr);

    // struct UpdateJob
    // {
    //     using F = std::function<void(skr::ecs::TaskContext&, skr::ecs::RandomComponentReadWrite<skr::scene::TransformComponent>&)>;
    //     UpdateJob(F func)
    //         : f(func)
    //     {
    //     }

    //     void build(skr::ecs::AccessBuilder& Builder)
    //     {
    //         Builder.access(&UpdateJob::trans_accessor);
    //     }

    //     void run(skr::ecs::TaskContext& Context)
    //     {
    //         SkrZoneScopedN("UpdateJob");
    //         f(Context, UpdateJob::trans_accessor);
    //     }

    //     skr::ecs::RandomComponentReadWrite<skr::scene::TransformComponent> trans_accessor;
    //     F f;
    // };

    // UpdateJob updatejob{ [&](skr::ecs::TaskContext& Context, skr::ecs::RandomComponentReadWrite<skr::scene::TransformComponent>& Accessor) {
    //     SkrZoneScopedN("UpdateJob::run");
    //     auto transform = Accessor[(skr::ecs::Entity)actor1.lock()->scene_entities[0]];
    //     auto transform2 = Accessor[(skr::ecs::Entity)actor2.lock()->scene_entities[0]];
    //     auto new_transform = skr::TransformF::Identity();
    //     new_transform *= transform.get();
    //     new_transform *= transform2.get();
    //     transform2.set(new_transform);                                                     // apply hierarchy transform
    //     Accessor.write_at((skr::ecs::Entity)actor2.lock()->scene_entities[0], transform2); // update transform2 with the new value
    // } };

    // cleanup
    skr_transform_system_destroy(transform_system);
    actor_manager.finalize();
    world.finalize();
    scheduler.unbind();

    return 0;
}