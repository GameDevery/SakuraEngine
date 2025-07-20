#include "SkrRT/ecs/scheduler.hpp"
#include "SkrCore/async/wait_timeout.hpp"
#include "./../sugoi/impl/query.hpp"

namespace skr::ecs
{

void StaticDependencyAnalyzer::process(skr::RC<TaskSignature> new_task)
{
    SkrZoneScopedN("StaticDependencyAnalyzer::Process");
    _collector.clear();

    for (auto read : new_task->reads)
    {
        auto access = accesses.try_add_default(read.type);
        if (access.already_exist() && access.value().last_writer) // RAW
        {
            _collector.add(access.value().last_writer);
        }
        access.value().readers.add(new_task);
    }
    for (auto write : new_task->writes)
    {
        auto access = accesses.try_add_default(write.type);
        if (access.already_exist() && access.value().readers.size()) // WAR
        {
            _collector.append(access.value().readers);
        }
        if (access.already_exist() && access.value().last_writer) // WAW
        {
            _collector.add(access.value().last_writer);
        }
        access.value().last_writer = new_task;
        access.value().readers.clear();
    }

    // now we get the dependency level, update the access info
    new_task->_static_dependencies = _collector;
}

void WorkUnitGenerator::process(skr::RC<TaskSignature> new_task)
{
    SkrZoneScopedN("WorkUnitGenerator::Process");

    // collect work units
    struct CollectContext
    {
        const sugoi_query_t* query;
        WorkUnitGenerator* _this;
        skr::RC<TaskSignature> new_task;
        WorkGroup* work_group = nullptr;
    } ctx;
    ctx.query = new_task->query;
    ctx._this = this;
    ctx.new_task = new_task;

    static const auto collect_units = +[](void* usr_data, sugoi_chunk_view_t* view) {
        if (view->count == 0)
            return;

        CollectContext* ctx = (CollectContext*)usr_data;
        auto& unit = ctx->work_group->units.try_add_default(view->chunk).value();
        unit.chunk_view = *view;
        unit.finish.add(1);
        for (auto dependency : ctx->work_group->dependencies)
        {
            if (auto depend_group = dependency->_work_groups.find(ctx->work_group->group))
            {
                if (auto depend_unit = depend_group.value().units.find(view->chunk))
                {
                    unit.dependencies.add(depend_unit.value().finish);
                }
            }
        }
    };

    static const auto filter_group = +[](void* u, sugoi_group_t* group) {
        auto& ctx = *(CollectContext*)u;
        ctx.work_group = &ctx.new_task->_work_groups.try_add_default(group).value();
        ctx.work_group->group = group;
        for (auto static_dependency : ctx.new_task->_static_dependencies)
        {
            if (bool confict = sugoiQ_match_group(static_dependency->query, group))
            {
                ctx.work_group->dependencies.add(static_dependency);
            }
        }
        sugoiQ_in_group(ctx.query, group, collect_units, &ctx);
    };
    sugoiQ_get_groups(ctx.query, filter_group, &ctx);
}

TaskScheduler::TaskScheduler(const ServiceThreadDesc& desc, skr::task::scheduler_t& scheduler) SKR_NOEXCEPT
    : AsyncService(desc),
      _scheduler(scheduler)
{
}

void TaskScheduler::add_task(sugoi_query_t* query, skr::stl_function<void(sugoi_chunk_view_t)>&& func, uint32_t batch_size)
{
    auto new_task = skr::RC<TaskSignature>::New();
    new_task->query = query;
    auto params = query->pimpl->parameters;
    for (uint32_t i = 0; i < params.length; i++)
    {
        auto type = params.types[i];
        auto access = params.accesses[i];
        EAccessMode mode = EAccessMode::Seq;
        if (access.randomAccess)
            mode = EAccessMode::Random;
        if (access.atomic)
            mode = EAccessMode::Atomic;

        if (access.readonly)
            new_task->reads.push_back({ type, mode });
        else
            new_task->writes.push_back({ type, mode });
    }
    new_task->task = skr::RC<Task>::New();
    new_task->task->func = std::move(func);
    new_task->task->batch_size = batch_size;
    add_task(new_task);
}

void TaskScheduler::add_task(skr::RC<TaskSignature> task)
{
    _tasks.enqueue(task);
    skr_atomic_fetch_add(&_enqueued_tasks, 1);
    awake();
}

// clang-format off
void TaskScheduler::dispatch(skr::RC<TaskSignature> signature)
{
    const auto& wgps = signature->work_groups();

    for (const auto& [group, work_group] : wgps)
    {
        if (work_group.units.is_empty())
            continue;

        SkrZoneScopedN("TaskScheduler::DispatchGroup");
        // create a task for each work unit
        for (const auto& [chunk, unit] : work_group.units)
        {
            SkrZoneScopedN("TaskScheduler::DispatchUnit");

            auto batch_size = signature->task->batch_size ? signature->task->batch_size : unit.chunk_view.count;
            batch_size = std::min(batch_size, unit.chunk_view.count);
            const auto batch_count = (unit.chunk_view.count + batch_size - 1) / batch_size;
            running.add(1);
            skr::task::schedule([
                this, &unit, signature, batch_count, batch_size,
                // capture task lifetime into payload body
                task = signature->task
            ](){
                {
                    for (auto dep : unit.dependencies)
                        dep.lock().wait(false);
                }
                SKR_DEFER({ running.decrement(); unit.finish.decrement(); });
                if (batch_count == 1)
                {
                    task->func(unit.chunk_view);
                }
                else
                {
                    SkrZoneScopedN("WorkUnit::Batch");
                    skr::task::counter_t batch_counter;
                    batch_counter.add(batch_count);
                    for (uint32_t i = 0; i < batch_count; i += 1)
                    {
                        const auto remain = unit.chunk_view.count - i * batch_size;
                        auto view = unit.chunk_view;
                        view.count = std::min(remain, batch_size);
                        view.start = unit.chunk_view.start + i * batch_size;
                        skr::task::schedule([task, view, batch_counter]() mutable {
                            task->func(view);
                            batch_counter.decrement();
                        }, nullptr);
                    }
                    batch_counter.wait(true);
                }
                unit.chunk_view = {};
            }, nullptr);
        }
    }

    // release the ownership so tasks can free the task from payload
    signature->task.reset(nullptr);
}
// clang-format on

void TaskScheduler::on_run() SKR_NOEXCEPT
{
    _scheduler.bind();
}

void TaskScheduler::on_exit() SKR_NOEXCEPT
{
    _scheduler.unbind();
}

void TaskScheduler::run() SKR_NOEXCEPT
{
    AsyncService::run();
}

AsyncResult TaskScheduler::serve() SKR_NOEXCEPT
{
    skr::RC<TaskSignature> task;
    if (_tasks.try_dequeue(task))
    {
        _analyzer.process(task);
        _generator.process(task);

        dispatch(task);
        skr_atomic_fetch_add(&_enqueued_tasks, -1);

        _dispatched_tasks.enqueue(task);
    }
    else
    {
        sleep();
    }
    return ASYNC_RESULT_OK;
}

void TaskScheduler::flush_all()
{
    while (skr_atomic_load(&_enqueued_tasks) != 0)
        ;
}

void TaskScheduler::sync_all()
{
    flush_all();
    running.wait(true);
}

void TaskScheduler::stop_and_exit()
{
    flush_all();
    sync_all();
    
    if (get_status() == skr::ServiceThread::Status::kStatusRunning)
    {
        SKR_LOG_BACKTRACE(u8"runner: request to stop.");
        setServiceStatus(SKR_ASYNC_SERVICE_STATUS_QUITING);
        stop();
    }
    else
    {
        SKR_LOG_BACKTRACE(u8"runner: stop already requested.");
    }

    wait_timeout<u8"WaitTaskScheduler">([&] { return get_status() == skr::ServiceThread::kStatusStopped; });
    exit();
}

} // namespace skr::ecs