#pragma once
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderGraph/frontend/base_types.hpp"
#include "pass_dependency_analysis.hpp"
#include "queue_schedule.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/set.hpp"
#include "SkrContainersDef/map.hpp"

namespace skr {
namespace render_graph {

// 同步点类型
enum class ESyncPointType : uint8_t
{
    Signal,      // 信号量发送
    Wait,        // 信号量等待
    Barrier      // 内存屏障
};

// 跨队列同步点
struct CrossQueueSyncPoint
{
    ESyncPointType type;
    uint32_t producer_queue_index;    // 生产者队列索引
    uint32_t consumer_queue_index;    // 消费者队列索引
    PassNode* producer_pass;          // 生产者Pass
    PassNode* consumer_pass;          // 消费者Pass
    ResourceNode* resource;           // 相关资源
    ECGPUResourceState from_state;    // 源状态
    ECGPUResourceState to_state;      // 目标状态
    uint32_t sync_value;              // 同步值（用于时间线信号量）
};

// SSIS (Sufficient Synchronization Index Set) 分析结果
struct SSISAnalysisResult
{
    // 原始同步需求（所有资源冲突点）
    skr::Vector<CrossQueueSyncPoint> raw_sync_points;
    
    // 优化后的同步点集合（SSIS算法结果）
    skr::Vector<CrossQueueSyncPoint> optimized_sync_points;
    
    // 统计信息
    uint32_t total_raw_syncs = 0;
    uint32_t total_optimized_syncs = 0;
    uint32_t sync_reduction_count = 0;
    float optimization_ratio = 0.0f;
};

// SSIS分析配置
struct CrossQueueSyncConfig
{
    bool enable_ssis_optimization = true;    // 启用SSIS优化
    bool enable_barrier_merging = true;      // 启用屏障合并
    uint32_t max_sync_distance = 16;         // 最大同步距离（Pass数量）
    bool enable_debug_output = false;        // 启用调试输出
};

// 跨队列同步分析阶段 - 实现SSIS算法
class SKR_RENDER_GRAPH_API CrossQueueSyncAnalysis : public IRenderGraphPhase
{
public:
    CrossQueueSyncAnalysis(
        const PassDependencyAnalysis& dependency_analysis,
        const QueueSchedule& queue_schedule,
        const CrossQueueSyncConfig& config = {});
    ~CrossQueueSyncAnalysis() override = default;

    // IRenderGraphPhase 接口
    void on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT override;
    void on_initialize(RenderGraph* graph) SKR_NOEXCEPT override;
    void on_finalize(RenderGraph* graph) SKR_NOEXCEPT override;

    // 获取分析结果
    const SSISAnalysisResult& get_ssis_result() const { return ssis_result_; }
    const skr::Vector<CrossQueueSyncPoint>& get_optimized_sync_points() const { return ssis_result_.optimized_sync_points; }

    // 调试接口
    void dump_ssis_analysis() const SKR_NOEXCEPT;
    void dump_sync_points() const SKR_NOEXCEPT;

private:
    // SSIS算法核心实现
    void analyze_cross_queue_dependencies() SKR_NOEXCEPT;
    void build_raw_sync_points() SKR_NOEXCEPT;
    void apply_ssis_optimization() SKR_NOEXCEPT;
    void merge_redundant_barriers() SKR_NOEXCEPT;
    void calculate_optimization_statistics() SKR_NOEXCEPT;

    // SSIS算法辅助方法
    bool is_sync_point_redundant(const CrossQueueSyncPoint& sync_point) const SKR_NOEXCEPT;
    bool can_sync_points_be_merged(const CrossQueueSyncPoint& a, const CrossQueueSyncPoint& b) const SKR_NOEXCEPT;
    skr::Vector<CrossQueueSyncPoint> find_dominating_sync_points(const CrossQueueSyncPoint& target) const SKR_NOEXCEPT;
    
    // 队列和资源状态查询
    bool are_passes_on_different_queues(PassNode* pass1, PassNode* pass2) const SKR_NOEXCEPT;
    uint32_t get_pass_queue_index(PassNode* pass) const SKR_NOEXCEPT;
    bool has_resource_state_transition(PassNode* producer, PassNode* consumer, ResourceNode* resource) const SKR_NOEXCEPT;

    // 依赖路径分析
    bool has_sync_path_between_passes(PassNode* from_pass, PassNode* to_pass, const CrossQueueSyncPoint& excluding_sync) const SKR_NOEXCEPT;
    skr::Vector<PassNode*> find_sync_path(PassNode* from_pass, PassNode* to_pass) const SKR_NOEXCEPT;

private:
    // 配置
    CrossQueueSyncConfig config_;

    // 输入Phase引用
    const PassDependencyAnalysis& dependency_analysis_;
    const QueueSchedule& queue_schedule_;

    // 分析结果
    SSISAnalysisResult ssis_result_;

    // 工作数据
    skr::FlatHashMap<PassNode*, uint32_t> pass_to_queue_mapping_;  // Pass到队列的映射缓存
    skr::FlatHashSet<std::pair<PassNode*, PassNode*>> cross_queue_dependencies_; // 跨队列依赖缓存
};

} // namespace render_graph
} // namespace skr