#include "SkrBase/config/platform.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderGraph/phases_v2/schedule_timeline.hpp"
#include "SkrRenderGraph/phases_v2/schedule_reorder.hpp"
#include "SkrCore/time.h"

// 模拟复杂的图形 + 异步计算工作负载
class TimelineStressTest
{
public:
    TimelineStressTest() = default;
    ~TimelineStressTest() = default;

    void run_stress_test()
    {
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
                    .with_cmpt_queues(cmpt_queues)
                    .with_cpy_queues(cpy_queues);
            });

        // 添加TimelinePhase
        auto timeline_config = skr::render_graph::ScheduleTimelineConfig{};
        timeline_config.enable_async_compute = true;
        timeline_config.enable_copy_queue = true;

        {
            auto info_analysis = skr::render_graph::PassInfoAnalysis();
            auto dependency_analysis = skr::render_graph::PassDependencyAnalysis(info_analysis);
            auto timeline_phase = skr::render_graph::ScheduleTimeline(dependency_analysis, timeline_config);
            auto reorder_phase = skr::render_graph::ExecutionReorderPhase(
                info_analysis, dependency_analysis, timeline_phase, {});

            // 构建复杂的渲染管线
            build_complex_pipeline(graph);

            // info -> dependency -> timeline -> reorder -> segmentation ->
            // virtual-allocation -> commit(commit alloc + bindgroups + barrier) -> execute
            // 手动调用TimelinePhase进行测试
            info_analysis.on_initialize(graph);
            dependency_analysis.on_initialize(graph);
            timeline_phase.on_initialize(graph);
            reorder_phase.on_initialize(graph);

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

            SKR_LOG_INFO(u8"Timeline Stress Test Analysis Times: "
                u8"Info Analysis: %llfms, Dependency Analysis: %llfms, "
                u8"Queue Analysis: %llfms, Reorder Analysis: %llfms",
                (double)infoAnalysisTime / 1000, (double)dependencyAnalysisTime / 1000, 
                (double)queueAnalysisTime / 1000, (double)reorderAnalysisTime / 1000
            );

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

            // 清理
            reorder_phase.on_finalize(graph);
            timeline_phase.on_finalize(graph);
            dependency_analysis.on_finalize(graph);
            info_analysis.on_finalize(graph);
        }
        skr::render_graph::RenderGraph::destroy(graph);

        SKR_LOG_INFO(u8"✅ Timeline Stress Test Completed Successfully!");
    }

private:
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
            [particle_buffer, culling_buffer, ddgi_irradiance_atlas](RenderGraph& graph, RenderGraph::ComputePassBuilder& builder) {
                builder.set_name(u8"ParticleSimulationPass")
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
                    .read(u8"DDGIDistance", ddgi_distance_atlas) // 使用距离场进行碰撞检测
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
                .extent(1920, 1080)
                .format(CGPU_FORMAT_R8G8B8A8_UNORM)
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