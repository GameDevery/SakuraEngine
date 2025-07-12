#pragma once
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderGraph/frontend/base_types.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/hashmap.hpp"

// Forward declare types from dependency analysis
namespace skr
{
namespace render_graph
{

// Resource access type
enum class EResourceAccessType : uint32_t
{
    Read,
    Write,
    ReadWrite
};

// Resource access info - detailed for dependency analysis
struct ResourceAccessInfo {
    ResourceNode* resource = nullptr;
    EResourceAccessType access_type;
    ECGPUResourceState resource_state = CGPU_RESOURCE_STATE_UNDEFINED;
};

// Resource info - direct extraction with detailed access info
struct PassResourceInfo {
    skr::Vector<ResourceNode*> read_resources;
    skr::Vector<ResourceNode*> write_resources;
    skr::Vector<ResourceNode*> readwrite_resources;
    skr::Vector<ResourceAccessInfo> all_resource_accesses; // For dependency analysis
    uint32_t total_resource_count = 0;
    bool has_graphics_resources = false; // Pre-computed for dependency analysis
};

// Performance info - from hints only
struct PassPerformanceInfo {
    bool is_compute_intensive = false;    // ComputeIntensive flag
    bool is_bandwidth_intensive = false;  // BandwidthIntensive flag
    bool is_vertex_bound = false;         // VertexBoundIntensive flag
    bool is_pixel_bound = false;          // PixelBoundIntensive flag
    bool has_small_working_set = false;   // SmallWorkingSet flag
    bool has_large_working_set = false;   // LargeWorkingSet flag
    bool has_random_access = false;       // RandomAccess flag
    bool has_streaming_access = false;    // StreamingAccess flag
    bool prefers_async_compute = false;   // PreferAsyncCompute flag
    bool separate_command_buffer = false; // SeperateFromCommandBuffer flag
};

// Pass info container
struct PassInfo {
    PassNode* pass = nullptr;
    EPassType pass_type = EPassType::Render;
    PassResourceInfo resource_info;
    PassPerformanceInfo performance_info;
};

// Analysis phase - runs before DependencyAnalysis
class SKR_RENDER_GRAPH_API PassInfoAnalysis : public IRenderGraphPhase
{
public:
    PassInfoAnalysis() = default;
    ~PassInfoAnalysis() override = default;

    // IRenderGraphPhase interface
    void on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT override;
    void on_initialize(RenderGraph* graph) SKR_NOEXCEPT override;
    void on_finalize(RenderGraph* graph) SKR_NOEXCEPT override;

    // Query interface
    const PassInfo* get_pass_info(PassNode* pass) const;
    const PassResourceInfo* get_resource_info(PassNode* pass) const;
    const PassPerformanceInfo* get_performance_info(PassNode* pass) const;

    // For dependency analysis - avoid recomputation
    EResourceAccessType get_resource_access_type(PassNode* pass, ResourceNode* resource) const;
    ECGPUResourceState get_resource_state(PassNode* pass, ResourceNode* resource) const;

private:
    void extract_pass_info(PassNode* pass);
    void extract_resource_info(PassNode* pass, PassResourceInfo& info);
    void extract_performance_info(PassNode* pass, PassPerformanceInfo& info);

private:
    skr::FlatHashMap<PassNode*, PassInfo> pass_infos;
};

} // namespace render_graph
} // namespace skr