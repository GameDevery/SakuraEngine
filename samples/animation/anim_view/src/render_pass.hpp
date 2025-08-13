#pragma once
#include "SkrProfile/profile.h"
#include "SkrGraphics/cgpux.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderer/primitive_draw.h"

#include "AnimView/helpers.hpp"

namespace animd
{
struct AnimDebugRenderPass
{
    static void create_frame_resources(skr::render_graph::RenderGraph* render_graph, const skr::StringView backbuffer_name = u8"backbuffer")
    {
        auto backbuffer = render_graph->get_texture(backbuffer_name.data());
        const auto back_desc = render_graph->resolve_descriptor(backbuffer);
        // MSAA, DEPTH
    }

    static void execute(skr::render_graph::RenderGraph* render_graph, skr::span<skr_primitive_draw_t> drawcalls, const skr::StringView backbuffer_name = u8"backbuffer")
    {
        auto backbuffer = render_graph->get_texture(backbuffer_name.data());
        const auto back_desc = render_graph->resolve_descriptor(backbuffer);
        if (!drawcalls.size()) return;
        if (drawcalls.size() == 0)
            return; // no models need to draw

        // get root signature from first drawcall
        CGPURootSignatureId root_signature = nullptr;
        root_signature = drawcalls[0].pipeline->root_signature;
        // add render pass
        render_graph->add_render_pass(
            [=](skr::render_graph::RenderGraph& g, skr::render_graph::RenderPassBuilder& builder) {
            },
            [=](skr::render_graph::RenderGraph& g, skr::render_graph::RenderPassContext& pass_context) {});
    }
};

} // namespace animd