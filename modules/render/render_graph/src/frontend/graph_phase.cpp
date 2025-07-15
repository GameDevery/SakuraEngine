#include "SkrRenderGraph/frontend/render_graph.hpp"

namespace skr {
namespace render_graph {

IRenderGraphPhase::~IRenderGraphPhase() SKR_NOEXCEPT
{

}

void IRenderGraphPhase::on_execute(RenderGraph* graph, RenderGraphFrameExecutor* executor, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{

}

skr::Vector<ResourceNode*>& IRenderGraphPhase::get_resources(RenderGraph* graph) SKR_NOEXCEPT
{
    return graph->resources;
}

skr::Vector<PassNode*>& IRenderGraphPhase::get_passes(RenderGraph* graph) SKR_NOEXCEPT
{
    return graph->passes;
}

} // namespace render_graph
} // namespace skr