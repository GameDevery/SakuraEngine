#pragma once
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderGraph/frontend/base_types.hpp"
#include "pass_dependency_analysis.hpp"
#include "queue_schedule.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/map.hpp"

namespace skr {
namespace render_graph {

// 资源生命周期信息
struct ResourceLifetime
{
    ResourceNode* resource;
    uint32_t start_dependency_level;    // 首次使用的依赖级别
    uint32_t end_dependency_level;      // 最后使用的依赖级别
    uint32_t start_queue_local_index;   // 队列内首次使用索引
    uint32_t end_queue_local_index;     // 队列内最后使用索引
    uint32_t primary_queue;             // 主要使用的队列
    
    // 内存需求信息
    uint64_t memory_size;               // 资源内存大小
    uint32_t memory_alignment;          // 内存对齐要求
    
    // 使用统计
    skr::Vector<PassNode*> using_passes; // 使用该资源的所有Pass
    bool is_cross_queue = false;        // 是否跨队列使用
    
    // 判断两个资源生命周期是否冲突（用于别名化）
    bool conflicts_with(const ResourceLifetime& other) const;
    
    // 获取生命周期长度（用于别名化排序）
    uint32_t get_lifetime_span() const { return end_dependency_level - start_dependency_level + 1; }
};

// 资源生命周期分析结果
struct ResourceLifetimeResult
{
    // 按资源分组的生命周期信息
    skr::FlatHashMap<ResourceNode*, ResourceLifetime> resource_lifetimes;
    
    // 按大小降序排列的资源（用于内存别名化）
    skr::Vector<ResourceNode*> resources_by_size_desc;
    
    // 跨队列资源列表
    skr::Vector<ResourceNode*> cross_queue_resources;
    
    // 统计信息
    uint64_t total_memory_requirement;
    uint32_t max_concurrent_resources;
    uint32_t total_resource_count;
};

// 资源生命周期分析配置
struct ResourceLifetimeConfig
{
    bool use_dependency_levels = true;     // 使用依赖级别而非简单执行顺序
    bool track_cross_queue_usage = true;   // 跟踪跨队列资源使用
    bool calculate_memory_stats = true;    // 计算内存统计信息
    bool enable_debug_output = false;      // 启用调试输出
};

// 资源生命周期分析Phase
class SKR_RENDER_GRAPH_API ResourceLifetimeAnalysis : public IRenderGraphPhase
{
public:
    ResourceLifetimeAnalysis(
        const PassDependencyAnalysis& dependency_analysis,
        const QueueSchedule& queue_schedule,
        const ResourceLifetimeConfig& config = {});
    ~ResourceLifetimeAnalysis() override = default;

    // IRenderGraphPhase 接口
    void on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT override;
    void on_initialize(RenderGraph* graph) SKR_NOEXCEPT override;
    void on_finalize(RenderGraph* graph) SKR_NOEXCEPT override;

    // 查询接口
    const ResourceLifetimeResult& get_result() const { return lifetime_result_; }
    const ResourceLifetime* get_resource_lifetime(ResourceNode* resource) const;
    bool resources_can_alias(ResourceNode* res1, ResourceNode* res2) const;

    // 调试接口
    void dump_lifetimes() const SKR_NOEXCEPT;
    void dump_aliasing_candidates() const SKR_NOEXCEPT;

private:
    // 核心分析方法
    void analyze_resource_lifetimes(RenderGraph* graph) SKR_NOEXCEPT;
    void calculate_dependency_level_lifetimes() SKR_NOEXCEPT;
    void calculate_queue_local_lifetimes() SKR_NOEXCEPT;
    void identify_cross_queue_resources() SKR_NOEXCEPT;
    void calculate_memory_requirements() SKR_NOEXCEPT;
    void sort_resources_by_size() SKR_NOEXCEPT;
    void collect_lifetime_statistics() SKR_NOEXCEPT;

    // 辅助方法
    uint64_t estimate_resource_memory_size(ResourceNode* resource) const SKR_NOEXCEPT;
    uint32_t get_resource_memory_alignment(ResourceNode* resource) const SKR_NOEXCEPT;

private:
    // 配置
    ResourceLifetimeConfig config_;

    // 输入Phase引用
    const PassDependencyAnalysis& dependency_analysis_;
    const QueueSchedule& queue_schedule_;

    // 分析结果
    ResourceLifetimeResult lifetime_result_;
};

} // namespace render_graph
} // namespace skr