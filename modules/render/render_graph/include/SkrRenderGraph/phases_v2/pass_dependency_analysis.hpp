#pragma once
#include "SkrContainers/hashmap.hpp"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "pass_info_analysis.hpp"

namespace skr
{
namespace render_graph
{

class PassNode;
class ResourceNode;

// Resource dependency type
enum class EResourceDependencyType : uint32_t
{
    RAR, // Read After Read
    RAW, // Read After Write (true dependency)
    WAR, // Write After Read (anti dependency)
    WAW  // Write After Write (output dependency)
};

// Single resource dependency info
struct ResourceDependency {
    PassNode* dependent_pass;                // The depended pass
    ResourceNode* resource;                  // The involved resource
    EResourceDependencyType dependency_type; // Dependency type
    EResourceAccessType current_access;      // Current pass access type
    EResourceAccessType previous_access;     // Previous pass access type
    ECGPUResourceState current_state;        // Current pass expected resource state
    ECGPUResourceState previous_state;       // Previous pass resource state
};

// Pass dependencies result (extended with scheduling info)
struct PassDependencies {
    skr::Vector<ResourceDependency> resource_dependencies; // All resource dependencies of this pass

    // For scheduling (extracted from timeline logic)
    skr::Vector<PassNode*> dependent_passes;    // Pass-level dependencies (extracted from resource dependencies)
    skr::Vector<PassNode*> dependent_by_passes; // Pass-level dependents
    // 注意：队列调度现在完全基于手动标记，移除了自动推断字段
    
    // Dependency level information (NEW)
    uint32_t dependency_level = 0;        // 依赖级别（最长路径深度）
    uint32_t topological_order = 0;       // 拓扑排序索引
    uint32_t critical_path_length = 0;    // 到图末尾的关键路径长度

    // Query interface
    bool has_dependency_on(PassNode* pass) const;
    skr::Vector<ResourceDependency> get_dependencies_on(PassNode* pass) const;
};

// Dependency level analysis result (integrated into PassDependencyAnalysis)
struct DependencyLevelResult
{
    struct DependencyLevel
    {
        uint32_t level = 0;
        skr::Vector<PassNode*> passes;  // 该级别中的所有Pass
        
        // 统计信息
        uint32_t total_resources_accessed = 0;  // 该级别访问的资源总数
        uint32_t cross_level_dependencies = 0;  // 跨级别依赖数
    };
    
    // 按级别分组的Pass
    skr::Vector<DependencyLevel> levels;
    
    // 拓扑排序后的Pass列表
    skr::Vector<PassNode*> topological_order;
    
    // 关键路径（最长路径）上的Pass
    skr::Vector<PassNode*> critical_path;
    
    // 统计信息
    uint32_t max_dependency_depth = 0;  // 最大依赖深度
    uint32_t total_parallel_opportunities = 0;  // 可并行执行的Pass对数
};

// Forward declaration
class PassInfoAnalysis;

// Pass dependency analysis phase - 独立的RenderGraph阶段（现已集成依赖级别分析）
class SKR_RENDER_GRAPH_API PassDependencyAnalysis : public IRenderGraphPhase
{
public:
    PassDependencyAnalysis(const PassInfoAnalysis& pass_info_analysis);
    ~PassDependencyAnalysis() override = default;

    // IRenderGraphPhase 接口
    void on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT override;
    void on_initialize(RenderGraph* graph) SKR_NOEXCEPT override;
    void on_finalize(RenderGraph* graph) SKR_NOEXCEPT override;

    // Query interface - 供后续Phase使用
    const PassDependencies* get_pass_dependencies(PassNode* pass) const;
    bool has_dependencies(PassNode* pass) const;

    // For ScheduleTimeline - get pass-level dependencies directly
    const skr::Vector<PassNode*>& get_dependent_passes(PassNode* pass) const;
    const skr::Vector<PassNode*>& get_dependent_by_passes(PassNode* pass) const;
    
    // Dependency level queries (NEW)
    uint32_t get_pass_dependency_level(PassNode* pass) const;
    uint32_t get_pass_topological_order(PassNode* pass) const;
    const DependencyLevelResult& get_dependency_levels() const { return dependency_levels_; }
    const skr::Vector<PassNode*>& get_topological_order() const { return dependency_levels_.topological_order; }
    const skr::Vector<PassNode*>& get_critical_path() const { return dependency_levels_.critical_path; }
    
    // 判断两个Pass是否可以并行执行（在同一依赖级别）
    bool can_execute_in_parallel(PassNode* pass1, PassNode* pass2) const;

    // Debug output
    void dump_dependencies() const;
    void dump_dependency_levels() const;
    void dump_critical_path() const;

private:
    // Analysis result: Pass -> its dependency info
    skr::FlatHashMap<PassNode*, PassDependencies> pass_dependencies_;
    
    // Dependency level analysis result (NEW)
    DependencyLevelResult dependency_levels_;

    // Reference to pass info analysis (to avoid recomputation)
    const PassInfoAnalysis& pass_info_analysis;

    // Helper methods
    void analyze_pass_dependencies(PassNode* current_pass, const skr::Vector<PassNode*>& previous_passes);
    void build_pass_level_dependencies(); // Extract pass-level dependency info from resource dependencies
    bool has_resource_conflict(EResourceAccessType current, EResourceAccessType previous, EResourceDependencyType& out_dependency_type);
    
    // Dependency level analysis methods (NEW)
    void perform_topological_sort();
    void calculate_dependency_levels();
    void identify_critical_path();
    void collect_level_statistics();
    
    // Topological sort helper
    void topological_sort_dfs(PassNode* node, 
                             skr::FlatHashSet<PassNode*>& visited,
                             skr::FlatHashSet<PassNode*>& on_stack,
                             skr::Vector<PassNode*>& sorted_passes,
                             bool& has_cycle);
};

} // namespace render_graph
} // namespace skr