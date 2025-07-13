#pragma once
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderGraph/frontend/base_types.hpp"
#include "cross_queue_sync_analysis.hpp"
#include "memory_aliasing_phase.hpp"
#include "pass_info_analysis.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/map.hpp"

namespace skr {
namespace render_graph {

// 屏障类型
enum class EBarrierType : uint8_t
{
    ResourceTransition,     // 资源状态转换屏障
    CrossQueueSync,        // 跨队列同步屏障
    MemoryAliasing,        // 内存别名屏障
    ExecutionDependency    // 执行依赖屏障
};

// GPU屏障描述
struct GPUBarrier
{
    EBarrierType type;
    PassNode* source_pass = nullptr;        // 源Pass
    PassNode* target_pass = nullptr;        // 目标Pass
    ResourceNode* resource = nullptr;       // 相关资源
    
    // 状态转换信息
    ECGPUResourceState before_state = CGPU_RESOURCE_STATE_UNDEFINED;
    ECGPUResourceState after_state = CGPU_RESOURCE_STATE_UNDEFINED;
    
    // 队列信息
    uint32_t source_queue = UINT32_MAX;
    uint32_t target_queue = UINT32_MAX;
    
    // 内存别名信息
    uint64_t memory_offset = 0;
    uint64_t memory_size = 0;
    ResourceNode* previous_resource = nullptr;  // 内存别名：前一个使用该内存的资源
    uint32_t memory_bucket_index = UINT32_MAX;  // 内存桶索引
    
    
    bool is_cross_queue() const { return source_queue != target_queue && source_queue != UINT32_MAX && target_queue != UINT32_MAX; }
    bool involves_memory_aliasing() const { return type == EBarrierType::MemoryAliasing; }
};

// 屏障批次
struct BarrierBatch
{
    skr::Vector<GPUBarrier> barriers;       // 批次中的屏障列表
    EBarrierType batch_type;                // 批次类型（批次中所有屏障应该是同一类型）
    uint32_t source_queue = UINT32_MAX;     // 源队列
    uint32_t target_queue = UINT32_MAX;     // 目标队列
    uint32_t batch_index = 0;               // 在该Pass中的批次索引
};

// 屏障生成结果
struct BarrierGenerationResult
{
    // 按Pass组织的屏障批次 - 主要的使用接口
    skr::FlatHashMap<PassNode*, skr::Vector<BarrierBatch>> pass_barrier_batches;
    
    // 统计信息
    uint32_t total_resource_barriers = 0;
    uint32_t total_sync_barriers = 0;
    uint32_t total_aliasing_barriers = 0;
    uint32_t total_execution_barriers = 0;
    uint32_t optimized_away_barriers = 0;   // 被优化掉的屏障数
    
    // 性能估算
    float estimated_barrier_cost = 0.0f;    // 估算的屏障性能开销
};

// 屏障生成配置
struct BarrierGenerationConfig
{
    bool enable_barrier_optimization = true;    // 启用屏障优化
    bool enable_barrier_batching = true;        // 启用屏障批处理
    bool eliminate_redundant_barriers = true;   // 消除冗余屏障
    
    // 调试选项
    bool enable_debug_output = false;           // 启用调试输出
    bool validate_barrier_correctness = true;   // 验证屏障正确性
    
    // 性能调优
    uint32_t max_barriers_per_batch = 16;      // 每批最大屏障数
    float barrier_cost_threshold = 2.5f;       // 屏障成本阈值（单位：微秒）
    bool enable_split_barriers = true;         // 启用分离屏障优化
};

// 屏障生成Phase（整合SSIS和内存别名结果）
class SKR_RENDER_GRAPH_API BarrierGenerationPhase : public IRenderGraphPhase
{
public:
    BarrierGenerationPhase(
        const CrossQueueSyncAnalysis& sync_analysis,
        const MemoryAliasingPhase& aliasing_phase,
        const PassInfoAnalysis& pass_info_analysis,
        const BarrierGenerationConfig& config = {});
    ~BarrierGenerationPhase() override = default;

    // IRenderGraphPhase 接口
    void on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT override;
    void on_initialize(RenderGraph* graph) SKR_NOEXCEPT override;
    void on_finalize(RenderGraph* graph) SKR_NOEXCEPT override;

    // 查询接口
    const BarrierGenerationResult& get_result() const { return barrier_result_; }
    
    // 主要查询接口
    const skr::Vector<BarrierBatch>& get_pass_barrier_batches(PassNode* pass) const;
    
    // 屏障统计
    uint32_t get_total_barriers() const;
    uint32_t get_total_batches() const;
    float get_estimated_barrier_cost() const { return barrier_result_.estimated_barrier_cost; }
    uint32_t get_optimized_barriers_count() const { return barrier_result_.optimized_away_barriers; }
    
    // 调试接口
    void dump_barrier_analysis() const SKR_NOEXCEPT;
    void validate_barrier_correctness() const SKR_NOEXCEPT;

private:
    // 核心屏障生成算法
    void generate_barriers(RenderGraph* graph) SKR_NOEXCEPT;
    void generate_cross_queue_sync_barriers(RenderGraph* graph) SKR_NOEXCEPT;
    void generate_memory_aliasing_barriers(RenderGraph* graph) SKR_NOEXCEPT;
    void generate_resource_transition_barriers(RenderGraph* graph) SKR_NOEXCEPT;
    void generate_execution_dependency_barriers(RenderGraph* graph) SKR_NOEXCEPT;
    
    // 屏障优化
    void optimize_barriers() SKR_NOEXCEPT;
    void eliminate_redundant_barriers() SKR_NOEXCEPT;
    void batch_barriers() SKR_NOEXCEPT;
    
    
    // 辅助方法
    bool are_barriers_compatible(const GPUBarrier& barrier1, const GPUBarrier& barrier2) const SKR_NOEXCEPT;
    bool is_barrier_redundant(const GPUBarrier& barrier) const SKR_NOEXCEPT;
    float estimate_barrier_cost(const GPUBarrier& barrier) const SKR_NOEXCEPT;
    void calculate_barrier_statistics() SKR_NOEXCEPT;
    
    // 屏障创建辅助
    GPUBarrier create_cross_queue_barrier(const CrossQueueSyncPoint& sync_point) const SKR_NOEXCEPT;
    GPUBarrier create_aliasing_barrier(ResourceNode* resource, PassNode* pass) const SKR_NOEXCEPT;
    GPUBarrier create_resource_transition_barrier(ResourceNode* resource, PassNode* from_pass, PassNode* to_pass) const SKR_NOEXCEPT;
    
    // 资源状态转换分析（基于博客算法）
    void analyze_resource_usage_patterns(RenderGraph* graph, 
                                       skr::FlatHashMap<ResourceNode*, skr::FlatHashMap<PassNode*, ECGPUResourceState>>& resource_states,
                                       skr::FlatHashMap<ResourceNode*, skr::FlatHashSet<uint32_t>>& cross_queue_reads) SKR_NOEXCEPT;
    
    void identify_transition_rerouting_requirements(RenderGraph* graph,
                                                  const skr::FlatHashMap<ResourceNode*, skr::FlatHashSet<uint32_t>>& cross_queue_reads,
                                                  skr::FlatHashSet<ResourceNode*>& resources_requiring_rerouting) SKR_NOEXCEPT;
    
    void generate_transitions_for_resource(RenderGraph* graph, ResourceNode* resource,
                                         const skr::FlatHashMap<PassNode*, ECGPUResourceState>& pass_states,
                                         const skr::FlatHashSet<ResourceNode*>& resources_requiring_rerouting) SKR_NOEXCEPT;
    
    // 队列能力检测
    uint32_t find_most_competent_queue(const skr::FlatHashSet<uint32_t>& queue_set) const SKR_NOEXCEPT;
    bool is_state_transition_supported_on_queue(uint32_t queue_index, ECGPUResourceState before_state, ECGPUResourceState after_state) const SKR_NOEXCEPT;
    bool can_use_split_barriers(uint32_t transmitting_queue, uint32_t receiving_queue, ECGPUResourceState before_state, ECGPUResourceState after_state) const SKR_NOEXCEPT;
    
    // 状态转换计算
    ECGPUResourceState calculate_combined_read_state(const skr::Vector<ECGPUResourceState>& read_states) const SKR_NOEXCEPT;
    ECGPUResourceState get_resource_state_for_usage(ResourceNode* resource, PassNode* pass, bool is_write) const SKR_NOEXCEPT;
    
    // 辅助方法
    bool check_resource_has_graphics_states(ResourceNode* resource) const SKR_NOEXCEPT;
    bool are_passes_adjacent_or_synchronized(PassNode* source_pass, PassNode* target_pass) const SKR_NOEXCEPT;
    
    // 屏障生成方法
    void generate_rerouted_transition(ResourceNode* resource, ECGPUResourceState before_state, ECGPUResourceState after_state, uint32_t competent_queue, PassNode* source_pass, PassNode* target_pass) SKR_NOEXCEPT;
    void generate_normal_transition(ResourceNode* resource, ECGPUResourceState before_state, ECGPUResourceState after_state, PassNode* source_pass, PassNode* target_pass) SKR_NOEXCEPT;
    void generate_split_barrier(ResourceNode* resource, ECGPUResourceState before_state, ECGPUResourceState after_state, PassNode* source_pass, PassNode* target_pass) SKR_NOEXCEPT;

private:
    // 配置
    BarrierGenerationConfig config_;

    // 输入Phase引用
    const CrossQueueSyncAnalysis& sync_analysis_;
    const MemoryAliasingPhase& aliasing_phase_;
    const PassInfoAnalysis& pass_info_analysis_;

    // 分析结果
    BarrierGenerationResult barrier_result_;
    
    // 工作数据
    skr::Vector<GPUBarrier> temp_barriers_;
    skr::FlatHashSet<ResourceNode*> processed_resources_;
};

} // namespace render_graph
} // namespace skr