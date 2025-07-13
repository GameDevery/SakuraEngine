#include "SkrBase/config/platform.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/phases_v2/queue_schedule.hpp"
#include "SkrRenderGraph/phases_v2/schedule_reorder.hpp"
#include "SkrRenderGraph/phases_v2/cross_queue_sync_analysis.hpp"
#include "SkrRenderGraph/phases_v2/resource_lifetime_analysis.hpp"
#include "SkrRenderGraph/phases_v2/memory_aliasing_phase.hpp"
#include "SkrRenderGraph/phases_v2/barrier_generation_phase.hpp"
#include "SkrCore/time.h"
#include "SkrContainersDef/set.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iomanip>

// 模拟复杂的图形 + 异步计算工作负载
class TimelineStressTest
{
public:
    TimelineStressTest() = default;
    ~TimelineStressTest() = default;

    void run_stress_test()
    {
        skr_log_set_level(SKR_LOG_LEVEL_INFO);

        SKR_LOG_INFO(u8"🔥 Timeline Stress Test Starting...");

        // Create instance
        CGPUInstanceDescriptor instance_desc = {};
#if SKR_PLAT_WIN64
        instance_desc.backend = CGPU_BACKEND_D3D12;
#elif SKR_PLAT_MACOSX
        instance_desc.backend = CGPU_BACKEND_METAL;
#endif
        instance_desc.enable_debug_layer = true;
        instance_desc.enable_gpu_based_validation = true;
        instance_desc.enable_set_name = true;
        auto instance = cgpu_create_instance(&instance_desc);

        // Filter adapters
        uint32_t adapters_count = 0;
        cgpu_enum_adapters(instance, CGPU_NULLPTR, &adapters_count);
        CGPUAdapterId adapters[64];
        cgpu_enum_adapters(instance, adapters, &adapters_count);
        auto adapter = adapters[0];

        // Create device
        CGPUQueueGroupDescriptor queue_group_descs[3];
        queue_group_descs[0].queue_type = CGPU_QUEUE_TYPE_GRAPHICS;
        queue_group_descs[0].queue_count = 1;
        queue_group_descs[1].queue_type = CGPU_QUEUE_TYPE_COMPUTE;
        queue_group_descs[1].queue_count = 1;
        queue_group_descs[2].queue_type = CGPU_QUEUE_TYPE_TRANSFER;
        queue_group_descs[2].queue_count = 1;

        CGPUDeviceDescriptor device_desc = {};
        device_desc.queue_groups = queue_group_descs;
        device_desc.queue_group_count = 3;
        auto device = cgpu_create_device(adapter, &device_desc);

        // 创建RenderGraph（前端模式，不需要真实设备）
        auto* graph = skr::render_graph::RenderGraph::create(
            [=](auto& builder) {
                auto cpy_queues = skr::Vector<CGPUQueueId>();
                cpy_queues.add(cgpu_get_queue(device, CGPU_QUEUE_TYPE_TRANSFER, 0));
                auto cmpt_queues = skr::Vector<CGPUQueueId>();
                cmpt_queues.add(cgpu_get_queue(device, CGPU_QUEUE_TYPE_COMPUTE, 0));
                auto gfx_queue = cgpu_get_queue(device, CGPU_QUEUE_TYPE_GRAPHICS, 0);

                builder.with_device(device)
                    .with_gfx_queue(gfx_queue)
                    .with_cpy_queues(cpy_queues);
            });

        // 添加TimelinePhase
        auto timeline_config = skr::render_graph::QueueScheduleConfig{};
        timeline_config.enable_async_compute = true;
        timeline_config.enable_copy_queue = true;

        // 构建复杂的渲染管线
        build_complex_pipeline(graph);

        {
            auto info_analysis = skr::render_graph::PassInfoAnalysis();
            auto dependency_analysis = skr::render_graph::PassDependencyAnalysis(info_analysis);
            auto timeline_phase = skr::render_graph::QueueSchedule(dependency_analysis, timeline_config);
            auto reorder_phase = skr::render_graph::ExecutionReorderPhase(info_analysis, dependency_analysis, timeline_phase, {});
            auto lifetime_analysis = skr::render_graph::ResourceLifetimeAnalysis(dependency_analysis, timeline_phase, {});
            auto ssis_phase = skr::render_graph::CrossQueueSyncAnalysis(dependency_analysis, timeline_phase, {});
            auto aliasing_phase = skr::render_graph::MemoryAliasingPhase(lifetime_analysis, ssis_phase, {});
            skr::render_graph::BarrierGenerationConfig barrier_config = {};
            auto barrier_phase = skr::render_graph::BarrierGenerationPhase(ssis_phase, aliasing_phase, info_analysis, barrier_config);

            // info -> dependency -> timeline -> reorder -> segmentation ->
            // virtual-allocation -> commit(commit alloc + bindgroups + barrier) -> execute
            // 手动调用完整的Phase链进行测试
            info_analysis.on_initialize(graph);
            dependency_analysis.on_initialize(graph);
            timeline_phase.on_initialize(graph);
            reorder_phase.on_initialize(graph);
            lifetime_analysis.on_initialize(graph);
            ssis_phase.on_initialize(graph);
            aliasing_phase.on_initialize(graph);
            barrier_phase.on_initialize(graph);

            SHiresTimer timer;
            skr_init_hires_timer(&timer);
            info_analysis.on_execute(graph, nullptr);
            auto infoAnalysisTime = skr_hires_timer_get_usec(&timer, true);

            dependency_analysis.on_execute(graph, nullptr);
            auto dependencyAnalysisTime = skr_hires_timer_get_usec(&timer, true);

            timeline_phase.on_execute(graph, nullptr);
            auto queueAnalysisTime = skr_hires_timer_get_usec(&timer, true);

            reorder_phase.on_execute(graph, nullptr);
            auto reorderAnalysisTime = skr_hires_timer_get_usec(&timer, true);

            lifetime_analysis.on_execute(graph, nullptr);
            auto lifetimeAnalysisTime = skr_hires_timer_get_usec(&timer, true);

            ssis_phase.on_execute(graph, nullptr);
            auto ssisAnalysisTime = skr_hires_timer_get_usec(&timer, true);

            aliasing_phase.on_execute(graph, nullptr);
            auto aliasingAnalysisTime = skr_hires_timer_get_usec(&timer, true);

            barrier_phase.on_execute(graph, nullptr);
            auto barrierAnalysisTime = skr_hires_timer_get_usec(&timer, true);

            SKR_LOG_INFO(u8"Complete Phase Chain Analysis Times: "
                         u8"Info: %llfms, Dependency: %llfms, Queue: %llfms, Reorder: %llfms, "
                         u8"Lifetime: %llfms, SSIS: %llfms, Aliasing: %llfms, Barrier: %llfms"
                         u8" (Total: %llfms)",
                (double)infoAnalysisTime / 1000,
                (double)dependencyAnalysisTime / 1000,
                (double)queueAnalysisTime / 1000,
                (double)reorderAnalysisTime / 1000,
                (double)lifetimeAnalysisTime / 1000,
                (double)ssisAnalysisTime / 1000,
                (double)aliasingAnalysisTime / 1000,
                (double)barrierAnalysisTime / 1000,
                (double)(infoAnalysisTime + dependencyAnalysisTime + queueAnalysisTime +
                    reorderAnalysisTime + lifetimeAnalysisTime + ssisAnalysisTime +
                    aliasingAnalysisTime + barrierAnalysisTime) /
                    1000);

            // 打印依赖分析结果
            dependency_analysis.dump_dependencies();

            // 打印队列分配
            auto non_reorder = timeline_phase.get_schedule_result();
            timeline_phase.dump_timeline_result(u8"🔥 Timeline Stress Test Results", non_reorder);

            // 打印 reordered 结果
            auto reorder_result = non_reorder;
            reorder_result.queue_schedules = reorder_phase.get_optimized_timeline();
            timeline_phase.dump_timeline_result(u8"🔥 Timeline Stress Test Reordered Results", reorder_result);

            // 验证调度结果
            validate_schedule_result(timeline_phase.get_schedule_result());

            // 输出分析结果
            dependency_analysis.dump_logical_topology();
            ssis_phase.dump_ssis_analysis();

            // 输出内存优化结果
            aliasing_phase.dump_aliasing_result();
            aliasing_phase.dump_memory_buckets();
            SKR_LOG_INFO(u8"🔧 Memory Optimization Results:");
            SKR_LOG_INFO(u8"  Memory reduction: %f%", aliasing_phase.get_compression_ratio() * 100.0f);
            SKR_LOG_INFO(u8"  Memory savings: %lld MB", aliasing_phase.get_memory_savings() / (1024 * 1024));

            // 输出屏障分析结果
            barrier_phase.dump_barrier_analysis();
            SKR_LOG_INFO(u8"🛡️ Barrier Analysis Results:");
            SKR_LOG_INFO(u8"  Total barriers: %u", barrier_phase.get_total_barriers());
            SKR_LOG_INFO(u8"  Estimated cost: %f", barrier_phase.get_estimated_barrier_cost());
            SKR_LOG_INFO(u8"  Optimized barriers: %u", barrier_phase.get_optimized_barriers_count());

            // 生成 Graphviz 可视化
            generate_graphviz_visualization(graph, timeline_phase, ssis_phase, barrier_phase, aliasing_phase, lifetime_analysis);

            // 生成额外的可视化文件显示每个Pass的屏障详情
            generate_barrier_details_visualization(graph, timeline_phase, barrier_phase);

            // 清理
            barrier_phase.on_finalize(graph);
            aliasing_phase.on_finalize(graph);
            ssis_phase.on_finalize(graph);
            lifetime_analysis.on_finalize(graph);
            reorder_phase.on_finalize(graph);
            timeline_phase.on_finalize(graph);
            dependency_analysis.on_finalize(graph);
            info_analysis.on_finalize(graph);
        }
        skr::render_graph::RenderGraph::destroy(graph);

        SKR_LOG_INFO(u8"✅ Timeline Stress Test Completed Successfully!");
    }

private:
    void generate_graphviz_visualization(
        skr::render_graph::RenderGraph* graph,
        const skr::render_graph::QueueSchedule& timeline_phase,
        const skr::render_graph::CrossQueueSyncAnalysis& ssis_phase,
        const skr::render_graph::BarrierGenerationPhase& barrier_phase,
        const skr::render_graph::MemoryAliasingPhase& aliasing_phase,
        const skr::render_graph::ResourceLifetimeAnalysis& lifetime_analysis)
    {
        using namespace skr::render_graph;

        std::stringstream dot;

        // Graphviz 头部
        dot << "digraph RenderGraphExecution {\n";
        dot << "  rankdir=TB;\n";
        dot << "  compound=true;\n";
        dot << "  fontname=\"Arial\";\n";
        dot << "  node [shape=box, style=\"rounded,filled\", fontname=\"Arial\"];\n";
        dot << "  edge [fontname=\"Arial\", fontsize=10];\n\n";

        const auto& schedule_result = timeline_phase.get_schedule_result();
        const auto& aliasing_result = aliasing_phase.get_result();
        const auto& lifetime_result = lifetime_analysis.get_result();

        // 定义通用的 find_pass_position lambda
        auto find_pass_position = [&](PassNode* pass) -> std::pair<uint32_t, uint32_t> {
            for (uint32_t q = 0; q < schedule_result.queue_schedules.size(); ++q)
            {
                const auto& queue_schedule = schedule_result.queue_schedules[q];
                for (uint32_t i = 0; i < queue_schedule.size(); ++i)
                {
                    if (queue_schedule[i] == pass)
                        return { q, i };
                }
            }
            return { UINT32_MAX, UINT32_MAX };
        };

        // 队列颜色定义
        const char* queue_colors[] = { "#FFE6E6", "#E6F3FF", "#E6FFE6" };
        const char* queue_names[] = { "Graphics Queue", "Compute Queue", "Copy Queue" };

        // 1. 生成队列和Pass节点
        for (uint32_t q = 0; q < schedule_result.queue_schedules.size(); ++q)
        {
            const auto& queue_schedule = schedule_result.queue_schedules[q];
            if (queue_schedule.is_empty()) continue;

            // 队列子图
            dot << "  subgraph cluster_queue_" << q << " {\n";
            dot << "    label=\"" << queue_names[std::min(q, 2u)] << "\";\n";
            dot << "    style=filled;\n";
            dot << "    fillcolor=\"" << queue_colors[std::min(q, 2u)] << "\";\n";
            dot << "    node [fillcolor=white];\n\n";

            // Pass节点
            for (uint32_t i = 0; i < queue_schedule.size(); ++i)
            {
                auto* pass = queue_schedule[i];

                // 根据Pass类型选择不同的节点样式
                const char* shape = "box";
                const char* color = "white";
                if (pass->pass_type == EPassType::Compute)
                {
                    shape = "ellipse";
                    color = "#E8F5E9"; // Light green for compute
                }
                else if (pass->pass_type == EPassType::Copy)
                {
                    shape = "octagon";
                    color = "#FFF3E0"; // Light orange for copy
                }
                else if (pass->pass_type == EPassType::Present)
                {
                    shape = "diamond";
                    color = "#F3E5F5"; // Light purple for present
                }

                dot << "    pass_q" << q << "_" << i
                    << " [label=\"" << (const char*)pass->get_name()
                    << "\\nOrder: " << i
                    << "\", shape=" << shape
                    << ", fillcolor=\"" << color << "\"];\n";
            }

            // 队列内执行顺序边
            for (uint32_t i = 1; i < queue_schedule.size(); ++i)
            {
                dot << "    pass_q" << q << "_" << (i - 1)
                    << " -> pass_q" << q << "_" << i
                    << " [style=solid, color=gray];\n";
            }

            dot << "  }\n\n";
        }

        // 2. 生成内存别名转换边
        dot << "  // Memory aliasing transitions\n";
        for (const auto& transition : aliasing_result.alias_transitions)
        {
            if (!transition.to_resource || !transition.transition_pass)
                continue;

            // 找到旧资源的最后使用Pass
            PassNode* from_pass = nullptr;
            if (transition.from_resource)
            {
                auto from_lifetime = lifetime_result.resource_lifetimes.find(transition.from_resource);
                if (from_lifetime != lifetime_result.resource_lifetimes.end() &&
                    !from_lifetime->second.using_passes.is_empty())
                {
                    // 找到最后一个使用
                    from_pass = from_lifetime->second.using_passes.back();
                }
            }

            auto [to_q, to_i] = find_pass_position(transition.transition_pass);

            if (from_pass && to_q != UINT32_MAX)
            {
                auto [from_q, from_i] = find_pass_position(from_pass);
                if (from_q != UINT32_MAX)
                {
                    std::string label = "Alias: ";
                    label += (const char*)transition.from_resource->get_name();
                    label += " → ";
                    label += (const char*)transition.to_resource->get_name();
                    label += "\\nBucket " + std::to_string(transition.bucket_index);

                    dot << "  pass_q" << from_q << "_" << from_i
                        << " -> pass_q" << to_q << "_" << to_i
                        << " [style=dashed, color=purple, penwidth=2, label=\"" << label
                        << "\", fontcolor=purple];\n";
                }
            }
        }

        // 3. 生成跨队列同步边
        dot << "\n  // Cross-queue synchronization\n";
        const auto& ssis_result = ssis_phase.get_ssis_result();
        for (const auto& sync : ssis_result.optimized_sync_points)
        {
            if (!sync.producer_pass || !sync.consumer_pass)
                continue;

            auto [prod_q, prod_i] = find_pass_position(sync.producer_pass);
            auto [cons_q, cons_i] = find_pass_position(sync.consumer_pass);

            if (prod_q != UINT32_MAX && cons_q != UINT32_MAX)
            {
                std::string label = "Sync";
                if (sync.resource)
                {
                    label += ": ";
                    label += (const char*)sync.resource->get_name();
                }

                dot << "  pass_q" << prod_q << "_" << prod_i
                    << " -> pass_q" << cons_q << "_" << cons_i
                    << " [style=dashed, color=red, penwidth=2, label=\"" << label
                    << "\", fontcolor=red];\n";
            }
        }

        // 4. 生成资源状态转换边 (ResourceTransition barriers)
        dot << "\n  // Resource state transitions\n";
        const auto& barrier_result = barrier_phase.get_result();

        // Track processed transitions to avoid duplicates
        skr::FlatHashSet<std::string> processed_transitions;

        // Debug: Count barriers
        int resource_transition_count = 0;
        int skipped_null_pass = 0;
        int skipped_duplicate = 0;
        int skipped_invalid_position = 0;
        int generated_edges = 0;

        // Iterate through pass barrier batches instead of removed all_barriers
        for (const auto& [pass, batches] : barrier_result.pass_barrier_batches)
        {
            for (const auto& batch : batches)
            {
                for (const auto& barrier : batch.barriers)
                {
                    if (barrier.type != EBarrierType::ResourceTransition)
                        continue;

                    resource_transition_count++;

                    if (!barrier.source_pass || !barrier.target_pass || !barrier.resource)
                    {
                        skipped_null_pass++;
                        SKR_LOG_DEBUG(u8"Skipping ResourceTransition: source_pass=%p, target_pass=%p, resource=%p",
                            barrier.source_pass,
                            barrier.target_pass,
                            barrier.resource);
                        continue;
                    }

                    // Create unique key for this transition
                    std::string transition_key = std::to_string(reinterpret_cast<uintptr_t>(barrier.source_pass)) + "_" +
                        std::to_string(reinterpret_cast<uintptr_t>(barrier.target_pass)) + "_" +
                        std::to_string(reinterpret_cast<uintptr_t>(barrier.resource));

                    if (processed_transitions.contains(transition_key))
                    {
                        skipped_duplicate++;
                        continue;
                    }
                    processed_transitions.insert(transition_key);

                    auto [src_q, src_i] = find_pass_position(barrier.source_pass);
                    auto [tgt_q, tgt_i] = find_pass_position(barrier.target_pass);

                    if (src_q == UINT32_MAX || tgt_q == UINT32_MAX)
                    {
                        skipped_invalid_position++;
                        SKR_LOG_DEBUG(u8"Invalid position for passes: %s -> %s (q:%u,%u i:%u,%u)",
                            barrier.source_pass ? barrier.source_pass->get_name() : u8"null",
                            barrier.target_pass ? barrier.target_pass->get_name() : u8"null",
                            src_q,
                            tgt_q,
                            src_i,
                            tgt_i);
                        continue;
                    }

                    if (src_q != UINT32_MAX && tgt_q != UINT32_MAX)
                    {
                        generated_edges++;
                        // Format state names
                        auto format_state = [](ECGPUResourceState state) -> const char* {
                            switch (state)
                            {
                            case CGPU_RESOURCE_STATE_UNDEFINED:
                                return "Undefined";
                            case CGPU_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER:
                                return "VB/CB";
                            case CGPU_RESOURCE_STATE_INDEX_BUFFER:
                                return "IB";
                            case CGPU_RESOURCE_STATE_RENDER_TARGET:
                                return "RT";
                            case CGPU_RESOURCE_STATE_UNORDERED_ACCESS:
                                return "UAV";
                            case CGPU_RESOURCE_STATE_DEPTH_WRITE:
                                return "DepthW";
                            case CGPU_RESOURCE_STATE_DEPTH_READ:
                                return "DepthR";
                            case CGPU_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE:
                                return "SRV(VS)";
                            case CGPU_RESOURCE_STATE_PIXEL_SHADER_RESOURCE:
                                return "SRV(PS)";
                            case CGPU_RESOURCE_STATE_SHADER_RESOURCE:
                                return "SRV";
                            case CGPU_RESOURCE_STATE_STREAM_OUT:
                                return "StreamOut";
                            case CGPU_RESOURCE_STATE_INDIRECT_ARGUMENT:
                                return "Indirect";
                            case CGPU_RESOURCE_STATE_COPY_DEST:
                                return "CopyDst";
                            case CGPU_RESOURCE_STATE_COPY_SOURCE:
                                return "CopySrc";
                            case CGPU_RESOURCE_STATE_GENERIC_READ:
                                return "GenericR";
                            case CGPU_RESOURCE_STATE_PRESENT:
                                return "Present";
                            case CGPU_RESOURCE_STATE_COMMON:
                                return "Common";
                            case CGPU_RESOURCE_STATE_SHADING_RATE_SOURCE:
                                return "SRS";
                            default:
                                return "Unknown";
                            }
                        };

                        std::string label = (const char*)barrier.resource->get_name();
                        label += "\\n";
                        label += format_state(barrier.before_state);
                        label += " → ";
                        label += format_state(barrier.after_state);

                        // Choose color and style based on barrier type and Begin/End
                        const char* color = (src_q == tgt_q) ? "blue" : "orange";
                        const char* style = (src_q == tgt_q) ? "dotted" : "dashed";
                        
                        // Special handling for Begin/End barriers - create portal nodes
                        if (barrier.is_begin || barrier.is_end)
                        {
                            std::string portal_id = "portal_" + transition_key + (barrier.is_begin ? "_begin" : "_end");
                            std::string portal_label = barrier.is_begin ? "BEGIN" : "END";
                            const char* portal_color = barrier.is_begin ? "#FFE4B5" : "#E0E4CC"; // Moccasin for begin, light gray for end
                            const char* portal_shape = "diamond";
                            
                            // Create portal node
                            dot << "  " << portal_id 
                                << " [label=\"" << portal_label << "\\n" << (const char*)barrier.resource->get_name()
                                << "\", shape=" << portal_shape 
                                << ", style=\"filled,dashed\", fillcolor=\"" << portal_color 
                                << "\", fontsize=8, width=0.5, height=0.5];\n";
                            
                            if (barrier.is_begin)
                            {
                                // Begin: Source Pass -> Portal
                                dot << "  pass_q" << src_q << "_" << src_i
                                    << " -> " << portal_id
                                    << " [style=dashed, color=green, penwidth=2, label=\"BEGIN\", fontcolor=green, fontsize=8];\n";
                            }
                            else
                            {
                                // End: Portal -> Target Pass
                                dot << "  " << portal_id
                                    << " -> pass_q" << tgt_q << "_" << tgt_i
                                    << " [style=dashed, color=red, penwidth=2, label=\"END\", fontcolor=red, fontsize=8];\n";
                            }
                        }
                        else
                        {
                            // Normal resource transition
                            dot << "  pass_q" << src_q << "_" << src_i
                                << " -> pass_q" << tgt_q << "_" << tgt_i
                                << " [style=" << style << ", color=" << color
                                << ", penwidth=1.5, label=\"" << label
                                << "\", fontcolor=" << color << ", fontsize=9];\n";
                        }
                    }
                }
            }
        }

        // Debug output
        SKR_LOG_INFO(u8"ResourceTransition debug: total=%d, null_pass=%d, duplicate=%d, invalid_pos=%d, generated=%d",
            resource_transition_count,
            skipped_null_pass,
            skipped_duplicate,
            skipped_invalid_position,
            generated_edges);

        // 5. 生成内存桶信息
        dot << "\n  // Memory buckets\n";
        dot << "  subgraph cluster_memory {\n";
        dot << "    label=\"Memory Aliasing Buckets\";\n";
        dot << "    style=filled;\n";
        dot << "    fillcolor=\"#FFFACD\";\n";
        dot << "    node [shape=record, fillcolor=white];\n\n";

        for (uint32_t i = 0; i < aliasing_result.memory_buckets.size(); ++i)
        {
            const auto& bucket = aliasing_result.memory_buckets[i];

            std::stringstream label;
            label << "{Bucket " << i;
            label << "|Size: " << (bucket.total_size / 1024) << " KB";
            label << "|Compression: " << int(bucket.compression_ratio * 100) << "%";
            label << "|Resources:";

            for (auto* resource : bucket.aliased_resources)
            {
                label << "\\n• " << (const char*)resource->get_name();

                auto lifetime_it = lifetime_result.resource_lifetimes.find(resource);
                if (lifetime_it != lifetime_result.resource_lifetimes.end())
                {
                    label << " [L" << lifetime_it->second.start_dependency_level
                          << "-" << lifetime_it->second.end_dependency_level << "]";
                }
            }

            if (bucket.aliased_resources.size() > 1)
            {
                label << "\\n\\n→ Memory reused";
            }
            label << "}";

            dot << "    bucket_" << i << " [label=\"" << label.str() << "\"];\n";
        }

        dot << "  }\n\n";

        // 6. 生成统计信息
        dot << "  // Statistics\n";
        dot << "  subgraph cluster_stats {\n";
        dot << "    label=\"Barrier Statistics\";\n";
        dot << "    style=filled;\n";
        dot << "    fillcolor=\"#E8EAF6\";\n";
        dot << "    node [shape=record, fillcolor=white];\n\n";

        const auto& barrier_stats = barrier_phase.get_result();

        std::stringstream stats_label;
        stats_label << "{Statistics";
        stats_label << "|Total Barriers: " << barrier_phase.get_total_barriers();
        stats_label << "|Resource Transitions: " << barrier_stats.total_resource_barriers;
        stats_label << "|Cross-Queue Syncs: " << barrier_stats.total_sync_barriers;
        stats_label << "|Memory Aliasing: " << barrier_stats.total_aliasing_barriers;
        stats_label << "|Execution Dependencies: " << barrier_stats.total_execution_barriers;
        stats_label << "|Optimized Away: " << barrier_stats.optimized_away_barriers;
        stats_label << "|Estimated Cost: " << std::fixed << std::setprecision(2) << barrier_stats.estimated_barrier_cost;
        stats_label << "}";

        dot << "    stats_node [label=\"" << stats_label.str() << "\"];\n";
        dot << "  }\n\n";

        // 7. 生成图例
        dot << "  // Legend\n";
        dot << "  subgraph cluster_legend {\n";
        dot << "    label=\"Legend\";\n";
        dot << "    style=filled;\n";
        dot << "    fillcolor=lightgray;\n";
        dot << "    node [shape=plaintext];\n";
        dot << "    legend [label=<\n";
        dot << "      <TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\">\n";
        dot << "        <TR><TD COLSPAN=\"2\"><B>Edge Types</B></TD></TR>\n";
        dot << "        <TR><TD>Solid Gray</TD><TD>Execution Order</TD></TR>\n";
        dot << "        <TR><TD>Dashed Red</TD><TD>Cross-Queue Sync</TD></TR>\n";
        dot << "        <TR><TD>Dashed Purple</TD><TD>Memory Alias</TD></TR>\n";
        dot << "        <TR><TD>Dotted Blue</TD><TD>Resource Transition (Same Queue)</TD></TR>\n";
        dot << "        <TR><TD>Dashed Orange</TD><TD>Resource Transition (Cross Queue)</TD></TR>\n";
        dot << "        <TR><TD>Green Diamond (BEGIN)</TD><TD>Split Barrier Begin Portal</TD></TR>\n";
        dot << "        <TR><TD>Red Diamond (END)</TD><TD>Split Barrier End Portal</TD></TR>\n";
        dot << "        <TR><TD COLSPAN=\"2\"><B>Pass Types</B></TD></TR>\n";
        dot << "        <TR><TD>Box</TD><TD>Render Pass</TD></TR>\n";
        dot << "        <TR><TD>Ellipse</TD><TD>Compute Pass</TD></TR>\n";
        dot << "        <TR><TD>Octagon</TD><TD>Copy Pass</TD></TR>\n";
        dot << "        <TR><TD>Diamond</TD><TD>Present Pass</TD></TR>\n";
        dot << "      </TABLE>\n";
        dot << "    >];\n";
        dot << "  }\n";

        dot << "}\n";

        // 保存到文件
        std::ofstream file("render_graph_execution.dot");
        file << dot.str();
        file.close();

        SKR_LOG_INFO(u8"📊 Graphviz visualization saved to render_graph_execution.dot");
        SKR_LOG_INFO(u8"   Run: dot -Tpng render_graph_execution.dot -o render_graph_execution.png");
    }

    void generate_barrier_details_visualization(
        skr::render_graph::RenderGraph* graph,
        const skr::render_graph::QueueSchedule& timeline_phase,
        const skr::render_graph::BarrierGenerationPhase& barrier_phase)
    {
        using namespace skr::render_graph;

        std::stringstream dot;

        // Graphviz header for barrier details
        dot << "digraph BarrierDetails {\n";
        dot << "  rankdir=LR;\n";
        dot << "  compound=true;\n";
        dot << "  fontname=\"Arial\";\n";
        dot << "  node [shape=record, style=\"rounded,filled\", fontname=\"Arial\", fillcolor=white];\n";
        dot << "  edge [fontname=\"Arial\", fontsize=10];\n\n";

        const auto& schedule_result = timeline_phase.get_schedule_result();
        const auto& barrier_result = barrier_phase.get_result();

        // For each pass, show its barriers
        uint32_t node_id = 0;
        for (uint32_t q = 0; q < schedule_result.queue_schedules.size(); ++q)
        {
            const auto& queue_schedule = schedule_result.queue_schedules[q];
            for (uint32_t i = 0; i < queue_schedule.size(); ++i)
            {
                auto* pass = queue_schedule[i];

                // Check if this pass has barriers
                const auto& barrier_batches = barrier_phase.get_pass_barrier_batches(pass);
                if (barrier_batches.is_empty())
                    continue;

                // Create a subgraph for this pass
                dot << "  subgraph cluster_pass_" << node_id++ << " {\n";
                dot << "    label=\"" << (const char*)pass->get_name() << " (Queue " << q << ")\";\n";
                dot << "    style=filled;\n";
                dot << "    fillcolor=\"#F5F5F5\";\n\n";

                // Add barrier nodes
                uint32_t barrier_idx = 0;
                for (const auto& batch : barrier_batches)
                {
                    for (const auto& barrier : batch.barriers)
                    {
                        std::stringstream barrier_label;
                        barrier_label << "{Barrier Batch " << batch.batch_index << "|";

                        // Barrier type
                        const char* type_name = "Unknown";
                        switch (barrier.type)
                        {
                        case EBarrierType::ResourceTransition:
                            type_name = "Resource Transition";
                            break;
                        case EBarrierType::CrossQueueSync:
                            type_name = "Cross-Queue Sync";
                            break;
                        case EBarrierType::MemoryAliasing:
                            type_name = "Memory Aliasing";
                            break;
                        case EBarrierType::ExecutionDependency:
                            type_name = "Execution Dependency";
                            break;
                        }
                        barrier_label << "Type: " << type_name;

                        if (barrier.is_begin)
                            barrier_label << "|🚪 BEGIN Portal";
                        else if (barrier.is_end)
                            barrier_label << "|🚪 END Portal";

                        // Resource info
                        if (barrier.resource)
                        {
                            barrier_label << "|Resource: " << (const char*)barrier.resource->get_name();
                        }

                        // State transition info
                        if (barrier.type == EBarrierType::ResourceTransition)
                        {
                            auto format_state_short = [](ECGPUResourceState state) -> const char* {
                                switch (state)
                                {
                                case CGPU_RESOURCE_STATE_RENDER_TARGET:
                                    return "RT";
                                case CGPU_RESOURCE_STATE_UNORDERED_ACCESS:
                                    return "UAV";
                                case CGPU_RESOURCE_STATE_DEPTH_WRITE:
                                    return "DepthW";
                                case CGPU_RESOURCE_STATE_SHADER_RESOURCE:
                                    return "SRV";
                                case CGPU_RESOURCE_STATE_COPY_DEST:
                                    return "CopyDst";
                                case CGPU_RESOURCE_STATE_COPY_SOURCE:
                                    return "CopySrc";
                                default:
                                    return "Other";
                                }
                            };

                            barrier_label << "|State: " << format_state_short(barrier.before_state)
                                          << " → " << format_state_short(barrier.after_state);
                        }

                        // Queue info for cross-queue barriers
                        if (barrier.is_cross_queue())
                        {
                            barrier_label << "|Queue: " << barrier.source_queue << " → " << barrier.target_queue;
                        }

                        // Memory info for aliasing barriers
                        if (barrier.type == EBarrierType::MemoryAliasing && barrier.previous_resource)
                        {
                            barrier_label << "|From: " << (const char*)barrier.previous_resource->get_name();
                            barrier_label << "|Bucket: " << barrier.memory_bucket_index;
                        }

                        barrier_label << "}";

                        // Choose color based on barrier type
                        const char* color = "#FFFFFF";
                        switch (barrier.type)
                        {
                        case EBarrierType::ResourceTransition:
                            color = "#E3F2FD";
                            break;
                        case EBarrierType::CrossQueueSync:
                            color = "#FFEBEE";
                            break;
                        case EBarrierType::MemoryAliasing:
                            color = "#F3E5F5";
                            break;
                        case EBarrierType::ExecutionDependency:
                            color = "#E8F5E9";
                            break;
                        }

                        dot << "    barrier_" << node_id << "_" << barrier_idx++
                            << " [label=\"" << barrier_label.str()
                            << "\", fillcolor=\"" << color << "\"];\n";
                    }
                }

                dot << "  }\n\n";
            }
        }

        // Add summary info
        dot << "  // Summary\n";
        dot << "  summary [shape=plaintext, label=<\n";
        dot << "    <TABLE BORDER=\"1\" CELLBORDER=\"0\" CELLSPACING=\"0\">\n";
        dot << "      <TR><TD BGCOLOR=\"#E3F2FD\">Resource Transition</TD></TR>\n";
        dot << "      <TR><TD BGCOLOR=\"#FFEBEE\">Cross-Queue Sync</TD></TR>\n";
        dot << "      <TR><TD BGCOLOR=\"#F3E5F5\">Memory Aliasing</TD></TR>\n";
        dot << "      <TR><TD BGCOLOR=\"#E8F5E9\">Execution Dependency</TD></TR>\n";
        dot << "    </TABLE>\n";
        dot << "  >];\n";

        dot << "}\n";

        // Save to file
        std::ofstream file("barrier_details.dot");
        file << dot.str();
        file.close();

        SKR_LOG_INFO(u8"📊 Barrier details visualization saved to barrier_details.dot");
        SKR_LOG_INFO(u8"   Run: dot -Tpng barrier_details.dot -o barrier_details.png");
    }

    void build_complex_pipeline(skr::render_graph::RenderGraph* graph)
    {
        using namespace skr::render_graph;

        // 场景描述：复杂的延迟渲染 + 后处理 + 异步计算

        // ===== 阶段1: 资源准备 =====

        // 创建G-Buffer纹理
        auto gbuffer_albedo = graph->create_texture([](RenderGraph& graph, RenderGraph::TextureBuilder& builder) {
            builder.set_name(u8"GBuffer_Albedo")
                .extent(1920, 1080)
                .format(CGPU_FORMAT_R8G8B8A8_UNORM)
                .allow_render_target();
        });

        auto gbuffer_normal = graph->create_texture([](RenderGraph& graph, RenderGraph::TextureBuilder& builder) {
            builder.set_name(u8"GBuffer_Normal")
                .extent(1920, 1080)
                .format(CGPU_FORMAT_R10G10B10A2_UNORM)
                .allow_render_target();
        });

        auto gbuffer_depth = graph->create_texture([](RenderGraph& graph, RenderGraph::TextureBuilder& builder) {
            builder.set_name(u8"GBuffer_Depth")
                .extent(1920, 1080)
                .format(CGPU_FORMAT_D24_UNORM_S8_UINT)
                .allow_depth_stencil();
        });

        // 创建光照缓冲区
        auto lighting_buffer = graph->create_buffer([](RenderGraph& graph, RenderGraph::BufferBuilder& builder) {
            builder.set_name(u8"LightingBuffer")
                .size(1920 * 1080 * 4 * sizeof(float))
                .allow_shader_readwrite()
                .as_uniform_buffer();
        });

        // Culling Compute (为G-Buffer Pass提供剔除信息，应在G-Buffer之前执行)
        auto culling_buffer = graph->create_buffer([](RenderGraph& graph, RenderGraph::BufferBuilder& builder) {
            builder.set_name(u8"CullingBuffer")
                .size(1024 * 1024 * sizeof(uint32_t))
                .allow_shader_readwrite();
        });

        // ===== 阶段2: 几何通道（强制Graphics队列）=====

        // Shadow Map Pass
        auto shadow_pass = graph->add_render_pass(
            [gbuffer_depth](RenderGraph& graph, RenderGraph::RenderPassBuilder& builder) {
                builder.set_name(u8"ShadowMapPass")
                    .set_depth_stencil(gbuffer_depth);
            },
            [](RenderGraph& graph, RenderPassContext& context) {
                // 模拟阴影渲染
            });

        auto culling_pass = graph->add_compute_pass(
            [culling_buffer](RenderGraph& graph, RenderGraph::ComputePassBuilder& builder) {
                builder.set_name(u8"FrustumCullingPass")
                    .with_flags(EPassFlags::PreferAsyncCompute)
                    .readwrite(u8"CullingResults", culling_buffer.range(0, ~0));
            },
            [](RenderGraph& graph, ComputePassContext& context) {
                // 模拟视锥剔除计算，为几何渲染提供剔除信息
            });

        // G-Buffer Pass (主几何通道，使用剔除数据)
        auto gbuffer_pass = graph->add_render_pass(
            [gbuffer_albedo, gbuffer_normal, gbuffer_depth, culling_buffer](RenderGraph& graph, RenderGraph::RenderPassBuilder& builder) {
                builder.set_name(u8"GBufferPass")
                    .write(0, gbuffer_albedo, CGPU_LOAD_ACTION_CLEAR)
                    .write(1, gbuffer_normal, CGPU_LOAD_ACTION_CLEAR)
                    .set_depth_stencil(gbuffer_depth)
                    .read(u8"CullingData", culling_buffer.range(0, ~0));
            },
            [](RenderGraph& graph, RenderPassContext& context) {
                // 模拟几何渲染，使用剔除结果优化绘制
            });

        // ===== 阶段4: DDGI全局光照系统 =====

        // DDGI探针资源
        auto ddgi_probe_positions = graph->create_buffer([](RenderGraph& graph, RenderGraph::BufferBuilder& builder) {
            builder.set_name(u8"DDGIProbePositions")
                .size(2048 * 3 * sizeof(float)) // 16x8x16 probe grid
                .allow_shader_readwrite();
        });

        auto ddgi_irradiance_atlas = graph->create_texture([](RenderGraph& graph, RenderGraph::TextureBuilder& builder) {
            builder.set_name(u8"DDGIIrradianceAtlas")
                .extent(6 * 32, 6 * 16) // 6x6 per probe, 32x16 probe layout
                .format(CGPU_FORMAT_B10G11R11_UFLOAT);
        });

        auto ddgi_distance_atlas = graph->create_texture([](RenderGraph& graph, RenderGraph::TextureBuilder& builder) {
            builder.set_name(u8"DDGIDistanceAtlas")
                .extent(16 * 32, 16 * 16) // 16x16 per probe, 32x16 probe layout
                .format(CGPU_FORMAT_R16G16_SFLOAT);
        });

        // DDGI Ray Tracing Pass
        auto ddgi_ray_trace_pass = graph->add_compute_pass(
            [ddgi_probe_positions, gbuffer_depth, ddgi_irradiance_atlas, ddgi_distance_atlas](RenderGraph& graph, RenderGraph::ComputePassBuilder& builder) {
                builder.set_name(u8"DDGIRayTracingPass")
                    .with_flags(EPassFlags::PreferAsyncCompute)
                    .read(u8"ProbePositions", ddgi_probe_positions.range(0, ~0))
                    .read(u8"SceneDepth", gbuffer_depth) // 使用深度信息构建BVH
                    .readwrite(u8"RawIrradiance", ddgi_irradiance_atlas)
                    .readwrite(u8"RawDistance", ddgi_distance_atlas);
            },
            [](RenderGraph& graph, ComputePassContext& context) {
                // 模拟DDGI光线追踪：为每个探针发射64条光线采样环境
            });

        // DDGI Probe Filtering Pass
        auto ddgi_filter_pass = graph->add_compute_pass(
            [ddgi_irradiance_atlas, ddgi_distance_atlas](RenderGraph& graph, RenderGraph::ComputePassBuilder& builder) {
                builder.set_name(u8"DDGIProbeFilteringPass")
                    .with_flags(EPassFlags::PreferAsyncCompute)
                    .readwrite(u8"IrradianceAtlas", ddgi_irradiance_atlas)
                    .readwrite(u8"DistanceAtlas", ddgi_distance_atlas);
            },
            [](RenderGraph& graph, ComputePassContext& context) {
                // 模拟探针数据时间滤波和空间滤波
            });

        // Particle Simulation (使用剔除结果，为后续渲染提供粒子数据)
        auto particle_buffer = graph->create_buffer([](RenderGraph& graph, RenderGraph::BufferBuilder& builder) {
            builder.set_name(u8"ParticleBuffer")
                .size(100000 * 16 * sizeof(float))
                .allow_shader_readwrite();
        });

        auto particle_pass = graph->add_compute_pass(
            [particle_buffer, culling_buffer](RenderGraph& graph, RenderGraph::ComputePassBuilder& builder) {
                builder.set_name(u8"ParticleSimulationPass")
                    .with_flags(EPassFlags::PreferAsyncCompute)
                    .read(u8"CullingInput", culling_buffer.range(0, ~0)) // 使用剔除信息优化粒子计算
                    .readwrite(u8"ParticleData", particle_buffer.range(0, ~0));
            },
            [](RenderGraph& graph, ComputePassContext& context) {
                // 模拟粒子物理模拟，使用视锥剔除优化计算，考虑全局光照影响
            });

        // Physics Simulation (为粒子系统提供物理约束，使用DDGI距离场)
        auto physics_buffer = graph->create_buffer([](RenderGraph& graph, RenderGraph::BufferBuilder& builder) {
            builder.set_name(u8"PhysicsBuffer")
                .size(50000 * 12 * sizeof(float))
                .allow_shader_readwrite();
        });

        auto physics_pass = graph->add_compute_pass(
            [physics_buffer, particle_buffer, ddgi_distance_atlas](RenderGraph& graph, RenderGraph::ComputePassBuilder& builder) {
                builder.set_name(u8"PhysicsSimulationPass")
                    .with_flags(EPassFlags::PreferAsyncCompute)
                    .read(u8"ParticleInput", particle_buffer.range(0, ~0)) // 读取粒子数据进行物理约束
                    .read(u8"DDGIDistance", ddgi_distance_atlas)           // 使用距离场进行碰撞检测
                    .readwrite(u8"PhysicsData", physics_buffer.range(0, ~0));
            },
            [](RenderGraph& graph, ComputePassContext& context) {
                // 模拟物理约束计算，影响粒子行为，使用DDGI距离场优化碰撞检测
            });

        // ===== 阶段5: 光照计算（需要等待G-Buffer和DDGI）=====

        auto lighting_pass = graph->add_compute_pass(
            [gbuffer_albedo, gbuffer_normal, lighting_buffer, ddgi_irradiance_atlas, ddgi_distance_atlas, ddgi_probe_positions](RenderGraph& graph, RenderGraph::ComputePassBuilder& builder) {
                builder.set_name(u8"DeferredLightingPass")
                    .read(u8"GBufferAlbedo", gbuffer_albedo)
                    .read(u8"GBufferNormal", gbuffer_normal)
                    .read(u8"DDGIIrradiance", ddgi_irradiance_atlas)
                    .read(u8"DDGIDistance", ddgi_distance_atlas)
                    .read(u8"ProbePositions", ddgi_probe_positions.range(0, ~0))
                    .readwrite(u8"LightingOutput", lighting_buffer.range(0, ~0));
            },
            [](RenderGraph& graph, ComputePassContext& context) {
                // 模拟延迟光照计算 + DDGI全局光照插值
            });

        // ===== 阶段6: 拷贝和资源传输 =====
        /*
        // 模拟大量纹理拷贝操作
        auto temp_texture1 = graph->create_texture([](RenderGraph& graph, RenderGraph::TextureBuilder& builder) {
            builder.set_name(u8"TempTexture1")
                .extent(512, 512)
                .format(CGPU_FORMAT_R8G8B8A8_UNORM);
        });

        auto temp_texture2 = graph->create_texture([](RenderGraph& graph, RenderGraph::TextureBuilder& builder) {
            builder.set_name(u8"TempTexture2")
                .extent(512, 512)
                .format(CGPU_FORMAT_R8G8B8A8_UNORM);
        });

        // 独立的拷贝操作（可以在Copy队列执行）
        auto copy_pass1 = graph->add_copy_pass(
            [temp_texture1, temp_texture2](RenderGraph& graph, RenderGraph::CopyPassBuilder& builder) {
                builder.set_name(u8"TextureCopyPass1")
                    .can_be_lone() // 标记为可以独立执行
                    .texture_to_texture(temp_texture1, temp_texture2);
            },
            [](RenderGraph& graph, CopyPassContext& context) {
                // 模拟纹理拷贝
            });

        auto copy_pass2 = graph->add_copy_pass(
            [temp_texture2, temp_texture1](RenderGraph& graph, RenderGraph::CopyPassBuilder& builder) {
                builder.set_name(u8"TextureCopyPass2")
                    .can_be_lone()
                    .texture_to_texture(temp_texture2, temp_texture1);
            },
            [](RenderGraph& graph, CopyPassContext& context) {
                // 模拟另一个纹理拷贝
            });
        */

        // ===== 阶段7: 后处理链（混合Graphics和Compute）=====

        // Bloom Compute
        auto bloom_pass = graph->add_compute_pass(
            [lighting_buffer](RenderGraph& graph, RenderGraph::ComputePassBuilder& builder) {
                builder.set_name(u8"BloomComputePass")
                    .with_flags(EPassFlags::PreferAsyncCompute)
                    .read(u8"BloomCompute", lighting_buffer.range(0, ~0));
            },
            [](RenderGraph& graph, ComputePassContext& context) {
                // 模拟Bloom效果计算（无Graphics资源依赖，可以异步）
            });

        // Tone Mapping (Graphics渲染)
        auto final_texture = graph->create_texture([](RenderGraph& graph, RenderGraph::TextureBuilder& builder) {
            builder.set_name(u8"FinalTexture")
                .import(nullptr, ECGPUResourceState::CGPU_RESOURCE_STATE_UNDEFINED)
                .allow_render_target();
        });

        auto tonemap_pass = graph->add_render_pass(
            [final_texture, lighting_buffer](RenderGraph& graph, RenderGraph::RenderPassBuilder& builder) {
                builder.set_name(u8"ToneMappingPass")
                    .write(0, final_texture, CGPU_LOAD_ACTION_CLEAR)
                    .read(u8"lighting_input", lighting_buffer.range(0, ~0));
            },
            [](RenderGraph& graph, RenderPassContext& context) {
                // 模拟色调映射渲染
            });

        // UI Overlay (Graphics渲染)
        auto ui_pass = graph->add_render_pass(
            [final_texture](RenderGraph& graph, RenderGraph::RenderPassBuilder& builder) {
                builder.set_name(u8"UIOverlayPass")
                    .write(0, final_texture, CGPU_LOAD_ACTION_LOAD);
            },
            [](RenderGraph& graph, RenderPassContext& context) {
                // 模拟UI渲染
            });

        // ===== 阶段8: 最终输出 =====

        auto present_pass = graph->add_present_pass(
            [final_texture](RenderGraph& graph, RenderGraph::PresentPassBuilder& builder) {
                builder.set_name(u8"PresentPass")
                    .texture(final_texture);
            });

        SKR_LOG_INFO(u8"📝 Built complex pipeline with DDGI global illumination:");
        SKR_LOG_INFO(u8"   - %d render passes (Graphics queue)", 5);
        SKR_LOG_INFO(u8"   - %d compute passes (Mixed queues) including DDGI", 7);
        SKR_LOG_INFO(u8"   - %d copy passes (Copy queue)", 2);
        SKR_LOG_INFO(u8"   - %d present pass", 1);
    }

    void validate_schedule_result(const skr::render_graph::TimelineScheduleResult& result)
    {
        SKR_LOG_INFO(u8"🔍 Validating schedule result...");

        // 验证基本完整性
        if (result.queue_schedules.is_empty())
        {
            SKR_LOG_ERROR(u8"❌ No queue schedules generated!");
            return;
        }

        if (result.pass_queue_assignments.empty())
        {
            SKR_LOG_ERROR(u8"❌ No pass assignments generated!");
            return;
        }

        SKR_LOG_INFO(u8"✅ Schedule validation completed successfully!");
    }
};

int main()
{
    TimelineStressTest test;
    test.run_stress_test();
    return 0;
}