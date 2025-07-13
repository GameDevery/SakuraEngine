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

    // Query interface
    bool has_dependency_on(PassNode* pass) const;
    skr::Vector<ResourceDependency> get_dependencies_on(PassNode* pass) const;
};

// Forward declaration
class PassInfoAnalysis;

// Pass dependency analysis phase - 独立的RenderGraph阶段
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

    // Debug output
    void dump_dependencies() const;

private:
    // Analysis result: Pass -> its dependency info
    skr::FlatHashMap<PassNode*, PassDependencies> pass_dependencies_;

    // Reference to pass info analysis (to avoid recomputation)
    const PassInfoAnalysis& pass_info_analysis;

    // Helper methods
    void analyze_pass_dependencies(PassNode* current_pass, const skr::Vector<PassNode*>& previous_passes);
    void build_pass_level_dependencies(); // Extract pass-level dependency info from resource dependencies
    bool has_resource_conflict(EResourceAccessType current, EResourceAccessType previous, EResourceDependencyType& out_dependency_type);
};

} // namespace render_graph
} // namespace skr