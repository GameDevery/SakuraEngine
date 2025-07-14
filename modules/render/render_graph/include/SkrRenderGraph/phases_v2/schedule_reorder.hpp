#pragma once
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderGraph/frontend/base_types.hpp"
#include "pass_info_analysis.hpp"
#include "pass_dependency_analysis.hpp"
#include "queue_schedule.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/hashmap.hpp"

namespace skr {
namespace render_graph {

// Configuration for reorder phase
struct ExecutionReorderConfig {
    // Enable specific optimizers
    bool enable_cache_opt = true;      // Cache locality optimization - attract passes with shared resources
    bool enable_lifetime_opt = true;   // Resource lifetime compression - attract passes to compress resource lifetimes
    
    // Conservative thresholds
    uint32_t max_attraction_distance = 10;  // Max distance a pass can be attracted from
    float min_affinity_score = 0.1f;        // Minimum resource sharing ratio for attraction (lowered for testing)
};

// Result of execution reordering
struct ExecutionReorderResult {
    // Optimized timeline - modified copy of original QueueScheduleInfo
    skr::Vector<skr::Vector<PassNode*>> optimized_timeline;
};

// Simplified - use RenderGraph DAG directly instead of rebuilding resource chains

// The main ExecutionReorder phase
class SKR_RENDER_GRAPH_API ExecutionReorderPhase : public IRenderGraphPhase {
public:
    ExecutionReorderPhase(
        const PassInfoAnalysis& pass_info,
        const PassDependencyAnalysis& dependency_analysis,
        const QueueSchedule& timeline,
        const ExecutionReorderConfig& config = {});
    ~ExecutionReorderPhase() override = default;
    
    // IRenderGraphPhase interface
    void on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT override;
    void on_initialize(RenderGraph* graph) SKR_NOEXCEPT override;
    void on_finalize(RenderGraph* graph) SKR_NOEXCEPT override;
    
    // Results access
    const ExecutionReorderResult& get_result() const { return result; }
    const skr::Vector<skr::Vector<PassNode*>>& get_optimized_timeline() const { return result.optimized_timeline; }
    
private:
    // Core optimization flow
    void run_graph_based_optimization() SKR_NOEXCEPT;
    
    // Graph-based optimization
    void optimize_queue_with_graph(uint32_t queue_idx) SKR_NOEXCEPT;
    bool can_attract_pass_safely(uint32_t queue_idx, size_t current_pos, size_t target_pos) SKR_NOEXCEPT;
    void attract_pass(uint32_t queue_idx, size_t from_pos, size_t to_pos) SKR_NOEXCEPT;
    
    // Attraction criteria using graph - unified to avoid duplicate resource scanning
    bool should_attract_passes(PassNode* current_pass, PassNode* target_pass, bool check_cache, bool check_lifetime) const SKR_NOEXCEPT;
    
    // Graph-based safety checks
    bool has_path_between_passes(PassNode* from_pass, PassNode* to_pass) const SKR_NOEXCEPT;
    skr::Vector<ResourceNode*> get_shared_resources(PassNode* pass1, PassNode* pass2) const SKR_NOEXCEPT;
    
    // Helper functions
    float calculate_resource_affinity(PassNode* pass1, PassNode* pass2) const SKR_NOEXCEPT;
    float calculate_resource_affinity_from_shared(PassNode* pass1, PassNode* pass2, const skr::Vector<ResourceNode*>& shared_resources) const SKR_NOEXCEPT;
    
private:
    // Configuration
    ExecutionReorderConfig config;
    
    // Input phase references
    const PassInfoAnalysis& pass_info_analysis;
    const PassDependencyAnalysis& dependency_analysis;
    const QueueSchedule& timeline_schedule;
    
    // Working data - timeline copy
    skr::Vector<skr::Vector<PassNode*>> working_timeline;
    
    // internal state for path checks
    mutable skr::Set<PassNode*> path_check_visited_ = skr::Set<PassNode*>(64);
    mutable skr::Vector<PassNode*> path_check_queue_ = skr::Vector<PassNode*>(64);
    
    // internal state for shared resource checks
    mutable skr::Set<ResourceNode*> shared_resource_set_ = skr::Set<ResourceNode*>(64);
    mutable skr::Vector<ResourceNode*> shared_resources_ = skr::Vector<ResourceNode*>(64);

    // Working data - direct access to RenderGraph for DAG queries
    RenderGraph* render_graph = nullptr;
    
    // Results
    ExecutionReorderResult result;
};

} // namespace render_graph
} // namespace skr