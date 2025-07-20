#include "cpp_style.hpp"
#include "SkrTask/parallel_for.hpp"
#include "SkrRT/ecs/scheduler.hpp"
#include "SkrCore/log.h"

struct ECSJobs {
    ECSJobs() SKR_NOEXCEPT
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1200));
        world.initialize();

        spawnIntEntities();

        scheduler.initialize(skr::task::scheudler_config_t());
        scheduler.bind();
    }

    ~ECSJobs() SKR_NOEXCEPT
    {
        world.finalize();
        scheduler.unbind();

        std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    void spawnIntEntities()
    {
        SkrZoneScopedN("spawnIntEntities");
        struct Spawner
        {
            void build(skr::ecs::CreationBuilder& Builder)
            {
                Builder.add_component(&Spawner::ints)
                       .add_component(&Spawner::floats);
            }

            void run(skr::ecs::CreationContext& Context)
            {
                memset(&ints[0], 0, sizeof(IntComponent) * Context.size());
                memset(&floats[0], 0, sizeof(FloatComponent) * Context.size());
            }

            skr::ecs::ComponentView<IntComponent> ints;
            skr::ecs::ComponentView<FloatComponent> floats;
        } spawner;
        world.create_entites(spawner, 2'000'000);
    }

protected:
    skr::ecs::World world;
    skr::task::scheduler_t scheduler;
};

TEST_CASE_METHOD(ECSJobs, "WRW")
{
    SkrZoneScopedN("ECSJobs::WRW");

    auto ROQuery = skr::ecs::QueryBuilder(&world)
            .ReadAll<IntComponent>()
            .commit().value();
    auto RWQuery = skr::ecs::QueryBuilder(&world)
            .ReadWriteAll<IntComponent>()
            .commit().value();
    SKR_DEFER({
        world.destroy_query(ROQuery);
        world.destroy_query(RWQuery);
    });

    skr::ecs::TaskScheduler TS;
    auto WJob0 = [=](sugoi_chunk_view_t view) {
        SkrZoneScopedN("WJob0");
        auto ints = sugoi::get_owned<IntComponent>(&view);
        for (auto i = 0; i < view.count; i++)
        {
            EXPECT_EQ(ints[i].v, 0);
            ints[i].v = ints[i].v + 1;
        }
    };
    auto RJob = [=](sugoi_chunk_view_t view) {
        SkrZoneScopedN("RJob");
        auto ints = sugoi::get_owned<const IntComponent>(&view);
        for (auto i = 0; i < view.count; i++)
        {
            EXPECT_EQ(ints[i].v, 1);
        }
    };
    auto WJob1 = [=](sugoi_chunk_view_t view) {
        SkrZoneScopedN("WJob1");
        auto ints = sugoi::get_owned<IntComponent>(&view);
        for (auto i = 0; i < view.count; i++)
        {
            EXPECT_EQ(ints[i].v, 1);
            ints[i].v = ints[i].v + 1;
        }
    };
    TS.add_task(RWQuery, std::move(WJob0), 1'280);
    TS.add_task(ROQuery, std::move(RJob), 1'280);
    TS.add_task(RWQuery, std::move(WJob1), 1'280);
    TS.run();
    TS.sync_all();
}

TEST_CASE_METHOD(ECSJobs, "WRW-Complex")
{
    SkrZoneScopedN("ECSJobs::WRW-Complex");

    auto ROQuery = skr::ecs::QueryBuilder(&world)
            .ReadAll<IntComponent>()
            .commit().value();
    auto RWIntQuery = skr::ecs::QueryBuilder(&world)
            .ReadWriteAll<IntComponent>()
            .commit().value();
    auto RWFloatQuery = skr::ecs::QueryBuilder(&world)
            .ReadWriteAll<FloatComponent>()
            .commit().value();
    SKR_DEFER({
        world.destroy_query(ROQuery);
        world.destroy_query(RWIntQuery);
        world.destroy_query(RWFloatQuery);
    });

    skr::ecs::TaskScheduler TS;
    auto WriteInts = [=](sugoi_chunk_view_t view) {
        SkrZoneScopedN("WriteInts");
        auto ints = sugoi::get_owned<IntComponent>(&view);
        for (auto i = 0; i < view.count; i++)
        {
            ints[i].v = ints[i].v + 1;
        }
    };
    auto WriteFloats = [=](sugoi_chunk_view_t view) {
        SkrZoneScopedN("WriteFloats");
        auto floats = sugoi::get_owned<FloatComponent>(&view);
        for (auto i = 0; i < view.count; i++)
        {
            floats[i].v = floats[i].v + 1.f;
            floats[i].v = floats[i].v + 1.f;
        }
    };
    auto ReadJob = [=](sugoi_chunk_view_t view) {
        SkrZoneScopedN("ReadOnlyJob");
        auto ints = sugoi::get_owned<const IntComponent>(&view);
        // use an int-only query to schedule a job but reads floats
        // FloatComponents will not be synchronized but you can still read
        // which is an unsafe operation
        auto floats = sugoi::get_owned<const FloatComponent>(&view);
        for (auto i = 0; i < view.count; i++)
        {
            EXPECT_EQ(ints[i].v, 1);
            const auto cond0 = floats[i].v == 0.f;
            const auto cond1 = floats[i].v == 1.f;
            const auto cond2 = floats[i].v == 2.f;
            const auto cond3 = floats[i].v == 3.f;
            const auto cond4 = floats[i].v == 4.f;
            const auto cond = cond0 || cond1 || cond2 || cond3 || cond4;
            EXPECT_TRUE(cond);
        }
    };
    TS.add_task(RWIntQuery, std::move(WriteInts), 12'80);
    TS.add_task(RWFloatQuery, std::move(WriteFloats), 12'800);
    TS.add_task(ROQuery, std::move(ReadJob), 1'280);
    TS.run();
    TS.sync_all();
}