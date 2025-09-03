// The SceneSample_Serde: The Principle for Scene Transform Update task
#include "SkrBase/math/gen/misc/transform.hpp"
#include <SkrCore/module/module_manager.hpp>
#include <SkrCore/log.h>
#include <SkrOS/filesystem.hpp>
#include <SkrContainers/span.hpp>

#include <SkrScene/actor.h>
#include <SkrSceneCore/scene_components.h>
#include <SkrSceneCore/transform_system.h>
#include <SkrRT/ecs/world.hpp>
#include <SkrRT/misc/cmd_parser.hpp>

struct SceneSampleSerdeModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

    bool bIsSerialize = false;
    bool bIsDeserialize = false;

    skr::TransformSystem* transform_system = nullptr;
    skr::task::scheduler_t scheduler;
    skr::ecs::World world;
    skr::ActorManager& actor_manager = skr::ActorManager::GetInstance();
};

IMPLEMENT_DYNAMIC_MODULE(SceneSampleSerdeModule, SceneSample_Serde);
void SceneSampleSerdeModule::on_load(int argc, char8_t** argv)
{
    SKR_LOG_INFO(u8"Scene Sample Simple Module Loaded");
    skr::cmd::parser parser(argc, (char**)argv);
    parser.add(u8"ser", u8"serialize", u8"-s", false);
    parser.add(u8"deser", u8"deserialize", u8"-d", false);
    if (!parser.parse())
    {
        SKR_LOG_ERROR(u8"Failed to parse command line arguments.");
        return;
    }
    auto ser_opt = parser.get_optional<bool>(u8"ser");
    auto des_opt = parser.get_optional<bool>(u8"deser");
    if (ser_opt) { bIsSerialize = *ser_opt; }
    if (des_opt) { bIsDeserialize = *des_opt; }

    SKR_LOG_INFO(u8"Serialize: %d", bIsSerialize);
    SKR_LOG_INFO(u8"Deserialize: %d", bIsDeserialize);
}

void SceneSampleSerdeModule::on_unload()
{

    SKR_LOG_INFO(u8"Scene SceneSampleSerde Unloaded");
}

int SceneSampleSerdeModule::main_module_exec(int argc, char8_t** argv)
{
    SkrZoneScopedN("SceneSampleSerdeModule::main_module_exec");
    SKR_LOG_INFO(u8"Running SceneSampleSerde Module");
    scheduler.initialize(skr::task::scheudler_config_t());
    scheduler.bind();
    skr::Path path = u8"D://ws/data/mid/skr/test.bin";

    if (bIsSerialize)
    {
        // initialize world
        SKR_LOG_INFO(u8"Serialize");
    }
    skr::Vector<uint8_t> rbuffer;
    if (bIsDeserialize)
    {
        SKR_LOG_INFO(u8"Deserialize");

        skr::fs::File::read_all_bytes(path, rbuffer);
        skr::archive::BinSpanReader reader{ rbuffer, 0 };
        SBinaryReader read_archive(reader);
        skr::bin_read(&read_archive, world);
    }
    
    world.bind_scheduler(scheduler); // bind task scheduler
    world.initialize();              // create storage

    transform_system = skr_transform_system_create(&world);
    actor_manager.Initialize(&world);

    skr::Path root_path = u8"D://ws/data/mid/skr/root.json";
    auto pos_accessor = world.random_read<const skr::scene::PositionComponent>();

    if (bIsSerialize)
    {
        auto root = skr::Actor::GetRoot();
        root.lock()->CreateEntity();
        root.lock()->GetComponent<skr::scene::PositionComponent>()->set({ 3.0f, 5.0f, 7.0f }); // trigger update for the first time
        auto pos = pos_accessor[(skr::ecs::Entity)root.lock()->GetEntity()].get();
        SKR_LOG_INFO(u8"Position: ({%f}, {%f}, {%f})", pos.x, pos.y, pos.z);
        SKR_LOG_INFO(u8"Entity: {%u}", root.lock()->GetEntity());

        root.lock()->serialize();
        skr::archive::JsonWriter writer(2);
        skr::json_write(&writer, (*root.lock()));
        auto jsString = writer.Write();
        skr::fs::File::write_all_text(root_path, jsString);
    }
    if (bIsDeserialize)
    {
        auto root = skr::Actor::GetRoot();
        skr::String jsString;
        skr::fs::File::read_all_text(root_path, jsString);
        skr::archive::JsonReader reader(jsString);
        skr::json_read(&reader, (*root.lock()));
        root.lock()->deserialize();
        auto pos = pos_accessor[(skr::ecs::Entity)root.lock()->GetEntity()].get();
        SKR_LOG_INFO(u8"Position: ({%f}, {%f}, {%f})", pos.x, pos.y, pos.z);
        SKR_LOG_INFO(u8"Entity: {%u}", root.lock()->GetEntity());
    }

    skr::Vector<uint8_t> wbuffer;
    if (bIsSerialize)
    {
        skr::archive::BinVectorWriter writer{ &wbuffer };
        SBinaryWriter archive(writer);
        skr::bin_write(&archive, world);
        skr::fs::File::write_all_bytes(path, wbuffer);
    }

    skr_transform_system_destroy(transform_system);
    actor_manager.Finalize();
    world.finalize();
    scheduler.unbind();
    return 0;
}