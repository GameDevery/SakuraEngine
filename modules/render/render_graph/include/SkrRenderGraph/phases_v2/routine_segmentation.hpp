#pragma once
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderGraph/frontend/base_types.hpp"
#include "schedule_timeline.hpp"
#include "schedule_reorder.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/hashmap.hpp"

namespace skr {
namespace render_graph {

// 前向声明
class ScheduleTimeline;
class ExecutionReorderPhase;

// 执行例程：队列Timeline中被SyncPoint切分的片段
struct ExecutionRoutine 
{
    uint32_t routine_id;                 // 例程ID
    uint32_t queue_index;                // 所属队列
    skr::Vector<PassNode*> passes;       // 例程内的Pass序列
    
    // 例程边界
    PassNode* first_pass = nullptr;      // 首个Pass
    PassNode* last_pass = nullptr;       // 最后一个Pass
    SyncRequirement* terminal_sync = nullptr;  // 结束时的同步点(可选)
    
    // 例程的资源需求(可选，用于优化)
    skr::Vector<ResourceNode*> input_resources;   // 例程开始时需要的资源
    skr::Vector<ResourceNode*> output_resources;  // 例程结束时产生的资源
    skr::Vector<ResourceNode*> temp_resources;    // 例程内部临时资源
};

// 切分后的队列调度信息
struct SegmentedQueueSchedule 
{
    ERenderGraphQueueType queue_type;
    uint32_t queue_index;
    skr::Vector<ExecutionRoutine> routines;  // 按顺序排列的例程
    
    // 统计信息
    uint32_t total_passes = 0;
    uint32_t total_routines = 0;
};

// 例程切分配置
struct RoutineSegmentationConfig 
{
    bool enable_routine_segmentation = true;    // 启用例程切分
    bool optimize_routine_resources = true;     // 优化例程资源分析
    uint32_t min_routine_size = 1;              // 最小例程大小
    uint32_t max_routine_size = 32;             // 最大例程大小
    bool enable_debug_output = false;           // 启用调试输出
};

// 例程切分结果
struct RoutineSegmentationResult 
{
    skr::Vector<SegmentedQueueSchedule> segmented_queues;  // 切分后的队列
    skr::FlatHashMap<PassNode*, uint32_t> pass_routine_assignments;  // Pass到例程的映射
    
    // 统计信息
    uint32_t total_routines = 0;
    uint32_t max_routine_size = 0;        // 最大例程的Pass数量
    float avg_routine_size = 0.0f;        // 平均例程大小
};

// 例程切分Phase - 在 ExecutionReorderPhase 之后执行
class SKR_RENDER_GRAPH_API RoutineSegmentationPhase : public IRenderGraphPhase 
{
public:
    RoutineSegmentationPhase(const ScheduleTimeline& timeline_phase, 
                           const ExecutionReorderPhase& reorder_phase,
                           const RoutineSegmentationConfig& config = {});
    ~RoutineSegmentationPhase() override;

    // IRenderGraphPhase 接口
    void on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT override;
    void on_initialize(RenderGraph* graph) SKR_NOEXCEPT override;
    void on_finalize(RenderGraph* graph) SKR_NOEXCEPT override;

    // 获取切分结果
    const RoutineSegmentationResult& get_result() const { return result; }
    
    // 查询Pass所属的例程
    uint32_t get_pass_routine_id(PassNode* pass) const;
    const ExecutionRoutine* get_routine_by_id(uint32_t routine_id) const;
    
    // 调试接口
    void dump_routine_segmentation() const;

private:
    // 核心切分逻辑
    void segment_queues_into_routines() SKR_NOEXCEPT;
    void segment_single_queue(const QueueScheduleInfo& queue_schedule, 
                             SegmentedQueueSchedule& segmented_queue) SKR_NOEXCEPT;
    
    // 例程分析和优化
    void analyze_routine_resources(ExecutionRoutine& routine) SKR_NOEXCEPT;
    void optimize_routine_boundaries() SKR_NOEXCEPT;
    
    // 验证和统计
    void validate_routine_segmentation() const SKR_NOEXCEPT;
    void calculate_segmentation_statistics() SKR_NOEXCEPT;
    void clear_frame_data() SKR_NOEXCEPT;

private:
    RoutineSegmentationConfig config;
    
    // 依赖的Phase
    const ScheduleTimeline& timeline_phase;
    const ExecutionReorderPhase& reorder_phase;
    
    // 输出结果
    RoutineSegmentationResult result;
};

} // namespace render_graph
} // namespace skr