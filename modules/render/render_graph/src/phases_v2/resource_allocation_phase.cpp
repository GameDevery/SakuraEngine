#include "SkrRenderGraph/phases_v2/resource_allocation_phase.hpp"
#include "SkrRenderGraph/backend/graph_backend.hpp"
#include "SkrRenderGraph/frontend/pass_node.hpp"
#include "SkrRenderGraph/frontend/resource_node.hpp"
#include "SkrCore/log.hpp"
#include "SkrProfile/profile.h"

namespace skr
{
namespace render_graph
{

ResourceAllocationPhase::ResourceAllocationPhase(
    const MemoryAliasingPhase& aliasing_phase,
    const PassInfoAnalysis& pass_info_analysis,
    const ResourceAllocationConfig& config)
    : config_(config)
    , aliasing_phase_(aliasing_phase)
    , pass_info_analysis_(pass_info_analysis)
{
}

void ResourceAllocationPhase::on_initialize(RenderGraph* graph) SKR_NOEXCEPT
{
    // 获取渲染设备
    this->graph = (RenderGraphBackend*)graph;
    device_ = graph->get_backend_device();

    buffer_pool_.initialize(device_);
    texture_pool_.initialize(device_);

    // 预分配容器
    const size_t estimated_resource_count = 128;
    allocation_result_.bucket_id_to_textures.reserve(estimated_resource_count);
    allocation_result_.bucket_id_to_buffers.reserve(estimated_resource_count);
}

void ResourceAllocationPhase::on_execute(RenderGraph* graph, RenderGraphProfiler* profiler) SKR_NOEXCEPT
{
    SkrZoneScopedN("ResourceAllocationPhase");

    // 步骤1: 清理上一帧的资源状态
    release_resources_to_pool();

    // 步骤2: 为池化资源创建实际的GPU资源
    allocate_pooled_resources(graph);

    if (config_.enable_debug_output)
    {
        SKR_LOG_INFO(u8"ResourceAllocationPhase: Created %u textures and %u buffers, total memory: %.2f MB",
            allocation_result_.total_textures_created,
            allocation_result_.total_buffers_created,
            allocation_result_.total_allocated_memory / (1024.0f * 1024.0f));
    }
}

void ResourceAllocationPhase::on_finalize(RenderGraph* graph) SKR_NOEXCEPT
{
    release_resources_to_pool();

    buffer_pool_.finalize();
    texture_pool_.finalize();

    allocation_result_ = ResourceAllocationResult{};
}

void ResourceAllocationPhase::allocate_pooled_resources(RenderGraph* graph) SKR_NOEXCEPT
{
    const auto& aliasing_result = aliasing_phase_.get_result();

    // 为每个池化资源创建实际的GPU资源
    for (size_t i = 0; i < aliasing_result.memory_buckets.size(); ++i)
    {
        ResourceNode* resource = aliasing_result.memory_buckets[i].aliased_resources[0];
        if (!resource) continue;

        // 根据资源类型创建对应的GPU资源
        if (resource->type == EObjectType::Texture)
        {
            TextureNode* texture = static_cast<TextureNode*>(resource);
            create_texture_from_pool(i, texture);
        }
        else if (resource->type == EObjectType::Buffer)
        {
            BufferNode* buffer = static_cast<BufferNode*>(resource);
            create_buffer_from_pool(i, buffer);
        }
    }
}

void ResourceAllocationPhase::create_texture_from_pool(uint64_t bucket_id, TextureNode* texture) SKR_NOEXCEPT
{
    // 从纹理池获取GPU纹理
    // uint64_t latest_frame = (texture->get_tags() & kRenderGraphDynamicResourceTag) ?  graph->get_latest_finished_frame() : UINT64_MAX;
    auto [gpu_texture, init_state] = texture_pool_.allocate(
        texture->get_desc(), { graph->get_frame_index(), texture->get_tags() });
    if (!gpu_texture)
    {
        SKR_LOG_ERROR(u8"ResourceAllocationPhase: Failed to acquire texture from pool for %s", texture->get_name());
        return;
    }

    // 存储映射关系
    allocation_result_.bucket_id_to_textures.add(bucket_id, { gpu_texture, init_state });

    // 更新统计信息
    allocation_result_.total_textures_created++;
    allocation_result_.total_allocated_memory += texture->get_size();

    if (config_.enable_debug_output)
    {
        auto desc = texture->get_desc();
        SKR_LOG_TRACE(u8"Allocated texture %s at pool (size: %u x %u, format: %d)",
            texture->get_name(),
            desc.width,
            desc.height,
            desc.format);
    }
}

void ResourceAllocationPhase::create_buffer_from_pool(uint64_t bucket_id, BufferNode* buffer) SKR_NOEXCEPT
{
    // 从缓冲区池获取GPU缓冲区
    uint64_t latest_frame = (buffer->get_tags() & kRenderGraphDynamicResourceTag) ? graph->get_latest_finished_frame() : UINT64_MAX;
    auto [gpu_buffer, init_state] = buffer_pool_.allocate(
        buffer->get_desc(), { graph->get_frame_index(), buffer->get_tags() }, latest_frame);

    if (!gpu_buffer)
    {
        SKR_LOG_ERROR(u8"ResourceAllocationPhase: Failed to acquire buffer from pool for %s", buffer->get_name());
        return;
    }

    // 存储映射关系
    allocation_result_.bucket_id_to_buffers.add(bucket_id, { gpu_buffer, init_state });

    // 更新统计信息
    allocation_result_.total_buffers_created++;
    allocation_result_.total_allocated_memory += buffer->get_desc().size;

    if (config_.enable_debug_output)
    {
        auto desc = buffer->get_desc();
        SKR_LOG_TRACE(u8"Allocated buffer %s at pool (size: %llu bytes)",
            buffer->get_name(),
            desc.size
        );
    }
}

void ResourceAllocationPhase::release_resources_to_pool() SKR_NOEXCEPT
{
    // 步骤1: 将上一帧的所有资源归还给资源池
    // 因为这些资源已经退出了生命周期
    for (const auto& [bucket_id, gpu_texture] : allocation_result_.bucket_id_to_textures)
    {
        const auto& bucket = aliasing_phase_.get_result().memory_buckets[bucket_id];
        auto node = (TextureNode*)bucket.aliased_resources.at_last();
        auto resource_info = pass_info_analysis_.get_resource_info(node);
        auto last_state = resource_info->used_states.at_last().value;
        texture_pool_.deallocate(node->get_desc(), gpu_texture.v, last_state, { graph->get_frame_index(), node->get_tags() });
    }

    for (const auto& [bucket_id, gpu_buffer] : allocation_result_.bucket_id_to_buffers)
    {
        const auto& bucket = aliasing_phase_.get_result().memory_buckets[bucket_id];
        auto node = (BufferNode*)bucket.aliased_resources.at_last();
        auto resource_info = pass_info_analysis_.get_resource_info(node);
        auto last_state = resource_info->used_states.at_last().value;
        buffer_pool_.deallocate(node->get_desc(), gpu_buffer.v, last_state, { graph->get_frame_index(), node->get_tags() });
    }

    // 清理映射表
    allocation_result_.bucket_id_to_buffers.clear();
    allocation_result_.bucket_id_to_textures.clear();

    // 重置统计信息
    allocation_result_.total_textures_created = 0;
    allocation_result_.total_buffers_created = 0;
    allocation_result_.total_allocated_memory = 0;

    if (config_.enable_debug_output)
    {
        SKR_LOG_TRACE(u8"ResourceAllocationPhase: Released all resources from previous frame to pool");
    }
}

CGPUTextureId ResourceAllocationPhase::get_resource(TextureNode* texture) const
{
    const auto& aliasing_result = aliasing_phase_.get_result();
    if (auto redirect_it = aliasing_result.resource_to_bucket.find(texture))
    {
        const auto bucket_id = redirect_it.value();
        return allocation_result_.bucket_id_to_textures.find(bucket_id).value().v;
    }
    return nullptr;
}

CGPUBufferId ResourceAllocationPhase::get_resource(BufferNode* buffer) const
{
    const auto& aliasing_result = aliasing_phase_.get_result();

    // 通过resource_redirects查找重定向的资源
    if (auto redirect_it = aliasing_result.resource_to_bucket.find(buffer))
    {
        const auto bucket_id = redirect_it.value();
        return allocation_result_.bucket_id_to_buffers.find(bucket_id).value().v;
    }
    return nullptr;
}

} // namespace render_graph
} // namespace skr