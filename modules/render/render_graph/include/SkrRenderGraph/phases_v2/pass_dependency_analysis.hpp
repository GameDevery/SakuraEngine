#pragma once
#include "SkrContainers/hashmap.hpp"
#include "SkrRenderGraph/frontend/render_graph.hpp"

namespace skr {
namespace render_graph {

class PassNode;
class ResourceNode;

// Resource dependency type
enum class EResourceDependencyType : uint32_t
{
    RAR,  // Read After Read
    RAW,  // Read After Write (true dependency)
    WAR,  // Write After Read (anti dependency)
    WAW   // Write After Write (output dependency)
};

// Resource access type
enum class EResourceAccessType : uint32_t
{
    Read,
    Write,
    ReadWrite
};

// Single resource dependency info
struct ResourceDependency
{
    PassNode* dependent_pass;                    // The depended pass
    ResourceNode* resource;                      // The involved resource
    EResourceDependencyType dependency_type;    // Dependency type
    EResourceAccessType current_access;         // Current pass access type
    EResourceAccessType previous_access;        // Previous pass access type
    ECGPUResourceState current_state;           // Current pass expected resource state
    ECGPUResourceState previous_state;          // Previous pass resource state
};

// Pass dependencies result
struct PassDependencies
{
    skr::Vector<ResourceDependency> resource_dependencies;  // All resource dependencies of this pass
    bool has_graphics_resource_dependency = false;          // Whether this pass has graphics resource dependency
    
    // Query interface
    bool has_dependency_on(PassNode* pass) const;
    skr::Vector<ResourceDependency> get_dependencies_on(PassNode* pass) const;
};

// Pass dependency analysis phase - 独立的RenderGraph阶段
class SKR_RENDER_GRAPH_API PassDependencyAnalysis : public IRenderGraphPhase
{
public:
    PassDependencyAnalysis() = default;
    ~PassDependencyAnalysis() override = default;

    // IRenderGraphPhase 接口
    void on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT override;
    void on_initialize(RenderGraph* graph) SKR_NOEXCEPT override;
    void on_finalize(RenderGraph* graph) SKR_NOEXCEPT override;

    // Query interface - 供后续Phase使用
    const PassDependencies* get_pass_dependencies(PassNode* pass) const;
    bool has_dependencies(PassNode* pass) const;
    
    // Debug output
    void dump_dependencies() const;

private:
    // Analysis result: Pass -> its dependency info
    skr::FlatHashMap<PassNode*, PassDependencies> pass_dependencies_;
    
    // Helper methods
    EResourceAccessType get_resource_access_type(PassNode* pass, ResourceNode* resource);
    ECGPUResourceState get_resource_state(PassNode* pass, ResourceNode* resource);
    void analyze_pass_dependencies(PassNode* current_pass, const skr::Vector<PassNode*>& previous_passes);
    bool has_resource_conflict(EResourceAccessType current, EResourceAccessType previous, 
                              EResourceDependencyType& out_dependency_type);
};

} // namespace render_graph
} // namespace skr