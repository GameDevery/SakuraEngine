#include "SkrCore/module/module_manager.hpp"
#include "SkrRT/ecs/world.hpp"
#include "SkrTask/fib_task.hpp"
#include "SkrSystem/system_app.h"
#include "SkrScene/scene_components.h"
#include <random>
#include <cmath>

class RGRaytracingSampleModule : public skr::IDynamicModule
{
    virtual void on_load(int argc, char8_t** argv) override;
    virtual int main_module_exec(int argc, char8_t** argv) override;
    virtual void on_unload() override;

public:
    static RGRaytracingSampleModule* Get();

    void spawn_entities();
    RGRaytracingSampleModule()
        : world(scheduler)
    {

    }
    skr::SystemApp app;
    skr::ecs::World world;
    skr::task::scheduler_t scheduler;
};

IMPLEMENT_DYNAMIC_MODULE(RGRaytracingSampleModule, RenderGraphRaytracingSample);

RGRaytracingSampleModule* RGRaytracingSampleModule::Get()
{
    auto mm = skr_get_module_manager();
    static auto rm = static_cast<RGRaytracingSampleModule*>(mm->get_module(u8"RenderGraphRaytracingSample"));
    return rm;
}

void RGRaytracingSampleModule::on_load(int argc, char8_t** argv)
{
    scheduler.initialize({});
    scheduler.bind();
    world.initialize();

    spawn_entities();
}

int RGRaytracingSampleModule::main_module_exec(int argc, char8_t** argv) 
{
    app.initialize(nullptr);
    auto wm = app.get_window_manager();
    auto eq = app.get_event_queue();
    auto main_window = wm->create_window({
        .title = u8"Render Graph Raytracing Sample",
        .size = { 1280, 720 },
        .is_resizable = false
    });
    main_window->show();
    SKR_DEFER({ wm->destroy_window(main_window); });

    static bool want_quit = false;
    struct QuitListener : public skr::ISystemEventHandler
    {
        void handle_event(const SkrSystemEvent& event) SKR_NOEXCEPT
        {
            if (event.type == SKR_SYSTEM_EVENT_QUIT)
                want_quit = true;
            if (event.type == SKR_SYSTEM_EVENT_WINDOW_CLOSE_REQUESTED)
                want_quit = event.window.window_native_handle == main_window->get_native_handle();
        }
        skr::SystemWindow* main_window = nullptr;
    } quitListener;
    quitListener.main_window = main_window;
    eq->add_handler(&quitListener);
    SKR_DEFER({ eq->remove_handler(&quitListener); });

    while (!want_quit)
    {
        eq->pump_messages();

    }

    return 0;
}

void RGRaytracingSampleModule::spawn_entities()
{
    using namespace skr::ecs;
    
    // Scene dimensions: 1000x1000x1000 units
    constexpr float SCENE_SIZE = 1000.0f;
    constexpr int TOTAL_ENTITIES = 50000;
    
    // Three attachment levels hierarchy
    constexpr int LEVEL_1_COUNT = 100;    // Root level: 100 entities (cities)
    constexpr int LEVEL_2_COUNT = 900;    // Mid level: 900 entities (buildings) - 9 per city
    constexpr int LEVEL_3_COUNT = 49000;  // Leaf level: 49000 entities (objects) - ~54 per building

    struct LevelSpawner
    {
        void build(skr::ecs::ArchetypeBuilder& Builder)
        {
            Builder.add_component(&LevelSpawner::children)
                .add_component(&LevelSpawner::transforms)
                .add_component(&LevelSpawner::translations)
                .add_component(&LevelSpawner::rotations)
                .add_component(&LevelSpawner::scales);
        }

        ComponentView<skr::scene::ChildrenComponent> children;
        ComponentView<skr::scene::TransformComponent> transforms;
        ComponentView<skr::scene::TranslationComponent> translations;
        ComponentView<skr::scene::RotationComponent> rotations;
        ComponentView<skr::scene::ScaleComponent> scales;
    };

    // Level 1 spawner (Cities) - Root entities without parents
    struct Level1Spawner : public LevelSpawner
    {
        void run(skr::ecs::TaskContext& Context)
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> pos_dist(-SCENE_SIZE * 0.5f, SCENE_SIZE * 0.5f);
            std::uniform_real_distribution<float> scale_dist(0.5f, 3.0f);
            std::uniform_real_distribution<float> rotation_dist(0.0f, 2.0f * skr::kPi);

            for (int i = 0; i < Context.size(); ++i)
            {
                // Position cities in a grid with some randomness
                int grid_x = i % 10;
                int grid_z = i / 10;
                float base_x = (grid_x - 4.5f) * (SCENE_SIZE * 0.9f / 10.0f);
                float base_z = (grid_z - 4.5f) * (SCENE_SIZE * 0.9f / 10.0f);
                
                translations[i].value = skr_float3_t{
                    base_x + pos_dist(gen) * 0.1f,
                    0.0f,
                    base_z + pos_dist(gen) * 0.1f
                };

                rotations[i].euler = skr::RotatorF{ 0.0f, rotation_dist(gen), 0.0f };

                float city_scale = scale_dist(gen) * 2.0f;
                scales[i].value = skr_float3_t{ city_scale, city_scale, city_scale };

                transforms[i].value = skr::TransformF::Identity();
            }
        }
    } level1_spawner;

    // Level 2 spawner (Buildings) - With parent relationships
    struct Level2Spawner : public LevelSpawner
    {
        void build(skr::ecs::ArchetypeBuilder& Builder)
        {
            LevelSpawner::build(Builder);
            Builder.add_component(&Level2Spawner::parents);
        }

        void run(skr::ecs::TaskContext& Context)
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> pos_dist(-SCENE_SIZE * 0.5f, SCENE_SIZE * 0.5f);
            std::uniform_real_distribution<float> scale_dist(0.5f, 3.0f);
            std::uniform_real_distribution<float> rotation_dist(0.0f, 2.0f * skr::kPi);

            for (int i = 0; i < Context.size(); ++i)
            {
                // Position buildings around city centers
                float angle = (i % 9) * (2.0f * skr::kPi / 9.0f) + rotation_dist(gen) * 0.1f;
                float radius = 30.0f + pos_dist(gen) * 0.1f;
                
                translations[i].value = skr_float3_t{
                    std::cos(angle) * radius,
                    pos_dist(gen) * 10.0f,
                    std::sin(angle) * radius
                };

                rotations[i].euler = skr::RotatorF{ 0.0f, rotation_dist(gen), 0.0f };

                float building_scale = scale_dist(gen);
                scales[i].value = skr_float3_t{ building_scale, building_scale, building_scale };

                transforms[i].value = skr::TransformF::Identity();
            }
        }
        ComponentView<skr::scene::ParentComponent> parents;
    } level2_spawner;

    // Level 3 spawner (Objects) - Leaf entities
    struct Level3Spawner : public LevelSpawner
    {
        void build(skr::ecs::ArchetypeBuilder& Builder)
        {
            LevelSpawner::build(Builder);
            Builder.add_component(&Level3Spawner::parents);
        }

        void run(skr::ecs::TaskContext& Context)
        {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<float> pos_dist(-SCENE_SIZE * 0.5f, SCENE_SIZE * 0.5f);
            std::uniform_real_distribution<float> scale_dist(0.5f, 3.0f);
            std::uniform_real_distribution<float> rotation_dist(0.0f, 2.0f * skr::kPi);

            for (int i = 0; i < Context.size(); ++i)
            {
                // Distribute objects around buildings
                float local_radius = 15.0f;
                float local_angle = rotation_dist(gen);
                float local_distance = pos_dist(gen) * 0.01f * local_radius;
                
                translations[i].value = skr_float3_t{
                    std::cos(local_angle) * local_distance,
                    pos_dist(gen) * 5.0f,
                    std::sin(local_angle) * local_distance
                };

                rotations[i].euler = skr::RotatorF{
                    rotation_dist(gen) * 0.2f,  // Small pitch variation
                    rotation_dist(gen),         // Full yaw rotation
                    rotation_dist(gen) * 0.1f   // Small roll variation
                };

                float object_scale = scale_dist(gen) * 0.3f;
                scales[i].value = skr_float3_t{ object_scale, object_scale, object_scale };

                transforms[i].value = skr::TransformF::Identity();
            }
        }
        ComponentView<skr::scene::ParentComponent> parents;
    } level3_spawner;

    SKR_LOG_INFO(u8"Spawned hierarchical scene with {} entities:", TOTAL_ENTITIES);
    SKR_LOG_INFO(u8"  Level 1 (Cities): {} entities", LEVEL_1_COUNT);
    SKR_LOG_INFO(u8"  Level 2 (Buildings): {} entities", LEVEL_2_COUNT);
    SKR_LOG_INFO(u8"  Level 3 (Objects): {} entities", LEVEL_3_COUNT);
    SKR_LOG_INFO(u8"Scene distributed in {}x{}x{} space", 
                 static_cast<int>(SCENE_SIZE), 
                 static_cast<int>(SCENE_SIZE), 
                 static_cast<int>(SCENE_SIZE));
    SKR_LOG_INFO(u8"Established parent-child relationships successfully");
}

void RGRaytracingSampleModule::on_unload()
{
    world.finalize();
    app.shutdown();
    scheduler.unbind();
}