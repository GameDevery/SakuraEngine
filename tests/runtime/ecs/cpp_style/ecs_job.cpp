#include "cpp_style.hpp"
#include "SkrTask/parallel_for.hpp"
#include "SkrRT/ecs/world.hpp"
#include "SkrCore/log.h"

struct ECSJobs {
    ECSJobs() SKR_NOEXCEPT
        : world(scheduler)
    {
        // std::this_thread::sleep_for(std::chrono::milliseconds(1200));
        scheduler.initialize(skr::task::scheudler_config_t());
        scheduler.bind();
     
        world.initialize();

        spawnIntEntities();
    }

    ~ECSJobs() SKR_NOEXCEPT
    {
        world.finalize();
        scheduler.unbind();
        // std::this_thread::sleep_for(std::chrono::milliseconds(2000));
    }

    void spawnIntEntities()
    {
        SkrZoneScopedN("spawnIntEntities");
        struct Spawner
        {
            void build(skr::ecs::ArchetypeBuilder& Builder)
            {
                Builder.add_component(&Spawner::ints)
                       .add_component(&Spawner::floats);
            }

            void run(skr::ecs::TaskContext& Context)
            {
                memset(&ints[0], 0, sizeof(IntComponent) * Context.size());
                memset(&floats[0], 0, sizeof(FloatComponent) * Context.size());
            }

            skr::ecs::ComponentView<IntComponent> ints;
            skr::ecs::ComponentView<FloatComponent> floats;
        } spawner;
        world.create_entities(spawner, 2'000'000);
    }

protected:
    skr::ecs::World world;
    skr::task::scheduler_t scheduler;
};

TEST_CASE_METHOD(ECSJobs, "WRW")
{
    SkrZoneScopedN("ECSJobs::WRW");

    skr::ServiceThreadDesc desc = {
        .name = u8"ECSJob-TaskScheduler",
        .priority = SKR_THREAD_ABOVE_NORMAL
    };

    struct WriteJob0
    {
        void build(skr::ecs::AccessBuilder& Builder)
        {
            Builder.write(&WriteJob0::ints);
        }
        void run(skr::ecs::TaskContext& Context)
        {
            SkrZoneScopedN("WriteJob0");
            for (auto i = 0; i < Context.size(); i++)
            {
                EXPECT_EQ(ints[i].v, 0);
                ints[i].v = ints[i].v + 1;
            }
        }
        skr::ecs::ComponentView<IntComponent> ints;
    } wjob0;
    auto q0 = world.dispatch_task(wjob0, 1'280, nullptr);

    struct ReadJob
    {
        void build(skr::ecs::AccessBuilder& Builder)
        {
            Builder.read(&ReadJob::ints);
        }
        void run(skr::ecs::TaskContext& Context)
        {
            SkrZoneScopedN("ReadJob");
            for (auto i = 0; i < Context.size(); i++)
            {
                EXPECT_EQ(ints[i].v, 1);
            }
        }
        skr::ecs::ComponentView<const IntComponent> ints;
    } rjob;
    auto q1 = world.dispatch_task(rjob, 1'280, nullptr);

    struct WriteJob1
    {
        void build(skr::ecs::AccessBuilder& Builder)
        {
            Builder.write(&WriteJob1::ints);
        }
        void run(skr::ecs::TaskContext& Context)
        {
            SkrZoneScopedN("WriteJob1");
            for (auto i = 0; i < Context.size(); i++)
            {
                EXPECT_EQ(ints[i].v, 1);
                ints[i].v = ints[i].v + 1;
            }
        }
        skr::ecs::ComponentView<IntComponent> ints;
    } wjob1;
    world.dispatch_task(wjob1, 1'280, q0);

    world.get_scheduler()->stop_and_exit();

    world.destroy_query(q0);
    world.destroy_query(q1);
}

TEST_CASE_METHOD(ECSJobs, "WRW-Complex")
{
    SkrZoneScopedN("ECSJobs::WRW-Complex");

    struct WriteInts
    {
        void build(skr::ecs::AccessBuilder& Builder)
        {
            Builder.write(&WriteInts::ints);
        }
        void run(skr::ecs::TaskContext& Context)
        {
            SkrZoneScopedN("WriteInts");
            for (auto i = 0; i < Context.size(); i++)
            {
                ints[i].v = ints[i].v + 1;
            }
        }
        skr::ecs::ComponentView<IntComponent> ints;
    } writeInts;
    auto q0 = world.dispatch_task(writeInts, 12'800, nullptr);

    struct WriteFloats
    {
        void build(skr::ecs::AccessBuilder& Builder)
        {
            Builder.write(&WriteFloats::floats);
        }
        void run(skr::ecs::TaskContext& Context)
        {
            SkrZoneScopedN("WriteFloats");
            for (auto i = 0; i < Context.size(); i++)
            {
                floats[i].v = floats[i].v + 1.f;
                floats[i].v = floats[i].v + 1.f;
            }
        }
        skr::ecs::ComponentView<FloatComponent> floats;
    } writeFloats;
    auto q1 = world.dispatch_task(writeFloats, 12'800, nullptr);

    struct ReadJob
    {
        void build(skr::ecs::AccessBuilder& Builder)
        {
            Builder.read(&ReadJob::ints).read(&ReadJob::floats);
        }
        void run(skr::ecs::TaskContext& Context)
        {
            SkrZoneScopedN("ReadOnlyJob");
            for (auto i = 0; i < Context.size(); i++)
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
        }
        skr::ecs::ComponentView<const IntComponent> ints;
        skr::ecs::ComponentView<const FloatComponent> floats;
    } readJob;
    auto q2 = world.dispatch_task(readJob, 1'280, nullptr);

    world.get_scheduler()->stop_and_exit();

    world.destroy_query(q0);
    world.destroy_query(q1);
    world.destroy_query(q2);
}