#pragma once
#include "SkrContainersDef/concurrent_queue.hpp"
#include "SkrContainersDef/stl_function.hpp"
#include "SkrContainersDef/map.hpp"
#include "SkrCore/memory/rc.hpp"
#include "SkrCore/async/async_service.h"
#include "SkrRT/ecs/component.hpp"

namespace skr::ecs
{

struct Task;
struct TaskSignature;
struct StaticDependencyAnalyzer;

struct WorkUnit
{
    mutable sugoi_chunk_view_t chunk_view;
    mutable skr::task::counter_t finish;
    skr::Vector<skr::task::weak_counter_t> dependencies;
};
static_assert(sizeof(WorkUnit) <= sizeof(uint64_t) * 8, "better cacheline for WorkUnit");

struct WorkGroup
{
    const sugoi_group_t* group;
    skr::Map<const sugoi_chunk_t*, WorkUnit> units;
    skr::Vector<skr::RC<TaskSignature>> dependencies;
};

struct Task
{
    SKR_RC_IMPL();
    using Type = skr::stl_function<void(sugoi_chunk_view_t, uint32_t count, uint32_t offset)>;

    Task::Type func;
    uint32_t batch_size = 0;
};

// Seq Read/Write: 会使用正常的 ChunkView 粒度去同步优化
// Random Read/Write: 会禁用 ChunkView 粒度去同步优化（或许后续可以支持 Query 查重的有限去同步？）
// Environment: 会针对访问的 Env 类型进行 DependencyLevel 计算并禁用去同步优化
// Message Send/Receive: 待定，因为未知全局消息消费的调度形式 
enum class EAccessMode : uint8_t {
    Seq,
    Random,
    Atomic,
    Environment,
    Message
};

struct ComponentAccess
{
    TypeIndex type;
    EAccessMode mode;
};

struct TaskSignature
{
public:
    SKR_RC_IMPL();
    virtual ~TaskSignature() = default;

    skr::RC<Task> task;
    skr::Vector<ComponentAccess> reads;
    skr::Vector<ComponentAccess> writes;
    uint32_t thread_affinity = ~0;
    const sugoi_query_t* query = nullptr;

    const auto& work_groups() const { return _work_groups; }

protected:
    friend struct StaticDependencyAnalyzer;
    skr::Vector<skr::RC<TaskSignature>> _static_dependencies;

    friend struct WorkUnitGenerator;
    skr::Map<const sugoi_group_t*, WorkGroup> _work_groups;
};

struct SKR_RUNTIME_API StaticDependencyAnalyzer
{
    void process(skr::RC<TaskSignature> task);

    struct AccessInfo
    {
        skr::RC<TaskSignature> last_writer;
        skr::Vector<skr::RC<TaskSignature>> readers;
    };
    skr::Map<TypeIndex, AccessInfo> accesses;

    // TODO: WRITE A STACK ALLOCATOR AND REPLACE THIS WITH IT
    skr::Vector<skr::RC<TaskSignature>> _collector;
};

struct SKR_RUNTIME_API WorkUnitGenerator
{
    void process(skr::RC<TaskSignature> task);
};

struct SKR_RUNTIME_API TaskScheduler : protected AsyncService
{
public:
    TaskScheduler(const ServiceThreadDesc& desc, skr::task::scheduler_t& scheduler) SKR_NOEXCEPT;
    
    void run() SKR_NOEXCEPT;
    void flush_all();
    void sync_all();
    void stop_and_exit();

    void add_task(sugoi_query_t* query, Task::Type&& func, uint32_t batch_size);

protected:
    AsyncResult serve() SKR_NOEXCEPT override;
    void on_run() SKR_NOEXCEPT override;
    void on_exit() SKR_NOEXCEPT override;
    void dispatch(skr::RC<TaskSignature> task);
    
    friend struct World;
    void add_task(skr::RC<TaskSignature> task);
    skr::task::counter_t running;

    friend struct StaticDependencyAnalyzer;
    friend struct WorkUnitGenerator;
    SAtomicU32 _enqueued_tasks = 0;
    skr::ConcurrentQueue<skr::RC<TaskSignature>> _tasks;
    skr::ConcurrentQueue<skr::RC<TaskSignature>> _dispatched_tasks;

    StaticDependencyAnalyzer _analyzer;
    WorkUnitGenerator _generator;
    skr::task::scheduler_t& _scheduler;
};

} // namespace skr::ecs