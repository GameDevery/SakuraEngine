#include "SkrRenderer/allocators/soa_segment.hpp"
#include <utility>

namespace skr::renderer
{

// ==================== Builder Implementation ====================

SOASegmentBuffer::Builder::Builder(CGPUDeviceId device)
    : device_(device)
{
}

SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::with_config(const Config& cfg)
{
    config_ = cfg;
    return *this;
}

SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::with_size(size_t initial_size, size_t max_size)
{
    config_.initial_size = initial_size;
    config_.max_size = max_size > 0 ? max_size : initial_size * 6; // 默认最大 6 倍
    return *this;
}

SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::with_page_size(uint32_t page_size)
{
    config_.page_size = page_size;
    return *this;
}

SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::add_component(SegmentID type_id, uint32_t element_size, uint32_t element_align)
{
    components_.push_back({ type_id, element_size, element_align });
    return *this;
}

// ==================== SOASegmentBuffer Implementation ====================

SOASegmentBuffer::~SOASegmentBuffer()
{
    shutdown();
}

void SOASegmentBuffer::shutdown()
{
    release_buffer();

    device = nullptr;
    capacity_bytes = 0;
    instance_capacity = 0;
    component_segments.clear();
}

void SOASegmentBuffer::initialize(const Builder& builder)
{
    device = builder.device_;
    config = builder.config_;
    component_infos = builder.components_;

    // 计算布局
    capacity_bytes = config.initial_size;
    calculate_layout();

    // 创建 GPU 缓冲区
    create_buffer();
}

void SOASegmentBuffer::calculate_layout()
{
    component_segments.clear();

    if (config.page_size == UINT32_MAX)
    {
        // 连续布局（传统 SOA）
        // 计算每个实例的总大小（所有组件大小之和）
        uint32_t total_instance_size = 0;
        for (const auto& comp : component_infos)
        {
            // 考虑对齐
            uint32_t aligned_size = (comp.element_size + comp.element_align - 1) & ~(comp.element_align - 1);
            total_instance_size += aligned_size;
        }

        // 计算最大实例数
        if (total_instance_size > 0)
        {
            instance_capacity = static_cast<uint32_t>(capacity_bytes / total_instance_size);
        }

        SKR_LOG_INFO(u8"SOASegmentBuffer: continuous layout, total_instance_size=%d bytes, max_instances=%d",
            total_instance_size,
            instance_capacity);

        // 为每个组件分配连续的段
        uint32_t current_offset = 0;
        for (const auto& info : component_infos)
        {
            ComponentSegment segment;
            segment.element_count = instance_capacity;
            segment.buffer_offset = current_offset;

            component_segments.push_back(segment);

            // 计算下一个组件的偏移（考虑对齐）
            uint32_t segment_size = info.element_size * instance_capacity;
            uint32_t aligned_size = (segment_size + info.element_align - 1) & ~(info.element_align - 1);
            current_offset += aligned_size;

            SKR_LOG_INFO(u8"  Segment for component %u: offset=%d, size=%d bytes, count=%d",
                info.type_id,
                segment.buffer_offset,
                segment_size,
                segment.element_count);
        }
    }
    else
    {
        // 分页布局
        // 计算每页的大小
        page_stride_bytes = 0;
        for (const auto& comp : component_infos)
        {
            // 对齐
            page_stride_bytes = (page_stride_bytes + comp.element_align - 1) & ~(comp.element_align - 1);
            // 添加组件段大小
            page_stride_bytes += comp.element_size * config.page_size;
        }
        // 页边界对齐到 256 字节
        page_stride_bytes = (page_stride_bytes + 255) & ~255;

        // 计算能容纳的页数和实例数
        uint32_t page_count = static_cast<uint32_t>(capacity_bytes / page_stride_bytes);
        instance_capacity = page_count * config.page_size;

        SKR_LOG_INFO(u8"SOASegmentBuffer: paged layout, page_size=%d, page_stride=%d bytes, pages=%d, max_instances=%d",
            config.page_size,
            page_stride_bytes,
            page_count,
            instance_capacity);

        // 设置组件段信息（用于记录每个组件在页内的偏移）
        uint32_t offset_in_page = 0;
        for (const auto& info : component_infos)
        {
            ComponentSegment segment;
            segment.element_count = instance_capacity;
            segment.buffer_offset = offset_in_page;  // 在分页模式下，这是页内偏移

            component_segments.push_back(segment);

            // 对齐
            offset_in_page = (offset_in_page + info.element_align - 1) & ~(info.element_align - 1);
            // 下一个组件的偏移
            offset_in_page += info.element_size * config.page_size;

            SKR_LOG_INFO(u8"  Component %u: offset_in_page=%d, element_size=%d",
                info.type_id,
                segment.buffer_offset,
                info.element_size);
        }
    }
}

const SOASegmentBuffer::ComponentSegment* SOASegmentBuffer::get_segment(SegmentID type_id) const
{
    for (uint32_t i = 0; i < component_infos.size(); i++)
    {
        const auto& info = component_infos[i];
        if (info.type_id == type_id)
            return &component_segments[i];
    }
    return nullptr;
}

uint32_t SOASegmentBuffer::get_component_offset(SegmentID type_id, InstanceID instance_id) const
{
    for (uint32_t i = 0; i < component_infos.size(); i++)
    {
        const auto& info = component_infos[i];
        if (info.type_id == type_id)
        {
            const auto& segment = component_segments[i];
            
            if (config.page_size == UINT32_MAX)
            {
                // 连续布局
                return segment.buffer_offset + (instance_id * info.element_size);
            }
            else
            {
                // 分页布局
                uint32_t page = instance_id / config.page_size;
                uint32_t offset_in_page = instance_id % config.page_size;
                uint32_t page_start = page * page_stride_bytes;
                return page_start + segment.buffer_offset + (offset_in_page * info.element_size);
            }
        }
    }
    SKR_LOG_WARN(u8"SOASegmentBuffer: Component type %u not found", type_id);
    return 0;
}

void SOASegmentBuffer::create_buffer()
{
    if (!device)
    {
        SKR_LOG_ERROR(u8"SOASegmentBuffer: Device not set");
        return;
    }

    if (buffer)
    {
        SKR_LOG_WARN(u8"SOASegmentBuffer: Buffer already created");
        return;
    }

    if (capacity_bytes == 0)
    {
        SKR_LOG_WARN(u8"SOASegmentBuffer: No data to allocate");
        return;
    }

    CGPUBufferDescriptor buffer_desc = {};
    buffer_desc.name = u8"SOASegmentBuffer";
    buffer_desc.size = capacity_bytes;
    buffer_desc.memory_usage = CGPU_MEM_USAGE_GPU_ONLY;
    buffer_desc.descriptors = CGPU_RESOURCE_TYPE_BUFFER_RAW | CGPU_RESOURCE_TYPE_RW_BUFFER_RAW;
    buffer_desc.flags = CGPU_BCF_DEDICATED_BIT;

    buffer = cgpu_create_buffer(device, &buffer_desc);
    if (!buffer)
    {
        SKR_LOG_ERROR(u8"SOASegmentBuffer: Failed to create GPU buffer");
    }
    else
    {
        SKR_LOG_INFO(u8"SOASegmentBuffer: Created GPU buffer (%lld MB)", capacity_bytes / (1024 * 1024));
    }
}

void SOASegmentBuffer::release_buffer()
{
    if (buffer)
    {
        cgpu_free_buffer(buffer);
        buffer = nullptr;
        SKR_LOG_DEBUG(u8"SOASegmentBuffer: Released GPU buffer");
    }
}

bool SOASegmentBuffer::needs_resize(uint32_t required_instances) const
{
    return required_instances > instance_capacity;
}

CGPUBufferId SOASegmentBuffer::resize(uint32_t new_instance_capacity)
{
    if (new_instance_capacity <= instance_capacity)
    {
        SKR_LOG_WARN(u8"SOASegmentBuffer: New capacity %u not greater than current %u",
                     new_instance_capacity, instance_capacity);
        return nullptr;
    }
    
    // 保存旧 buffer
    CGPUBufferId old_buffer = buffer;
    
    // 新 buffer 需要重新开始状态跟踪
    first_use = true;
    
    // 更新容量并重新计算布局
    uint32_t old_instance_capacity = instance_capacity;
    instance_capacity = new_instance_capacity;
    
    // 重新计算所有 segments 的布局
    capacity_bytes = calculate_size_for_capacity(new_instance_capacity);
    calculate_layout();
    
    // 创建新 buffer
    buffer = create_buffer_with_capacity(new_instance_capacity);
    
    SKR_LOG_INFO(u8"SOASegmentBuffer: Resized from %u to %u instances, new buffer size: %llu MB",
                 old_instance_capacity, new_instance_capacity, capacity_bytes / (1024 * 1024));
    
    return old_buffer;
}

void SOASegmentBuffer::copy_segments(skr::render_graph::RenderGraph* graph,
                                     skr::render_graph::BufferHandle src_buffer,
                                     skr::render_graph::BufferHandle dst_buffer,
                                     uint32_t instance_count) const
{
    if (instance_count == 0)
        return;
        
    // 添加拷贝 pass
    graph->add_copy_pass(
        [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::CopyPassBuilder& builder) {
            builder.set_name(u8"SOASegmentBuffer_Copy");
            
            if (config.page_size == UINT32_MAX)
            {
                // 连续布局：按组件段拷贝
                // 注意：这里假设源和目标都是相同的连续布局
                for (uint32_t i = 0; i < component_infos.size(); i++)
                {
                    const auto& info = component_infos[i];
                    const auto& segment = component_segments[i];

                    // 计算要拷贝的大小
                    uint32_t copy_size = info.element_size * instance_count;
                    
                    // 源和目标使用相同的偏移（都是连续布局）
                    uint32_t offset = segment.buffer_offset;
                    
                    // 拷贝这个组件段
                    builder.buffer_to_buffer(
                        src_buffer.range(offset, offset + copy_size),
                        dst_buffer.range(offset, offset + copy_size)
                    );
                }
            }
            else
            {
                // 分页布局：按页拷贝更高效
                uint32_t pages_to_copy = (instance_count + config.page_size - 1) / config.page_size;
                uint32_t last_page_instances = instance_count % config.page_size;
                if (last_page_instances == 0 && instance_count > 0)
                    last_page_instances = config.page_size;
                
                // 拷贝完整页
                for (uint32_t page = 0; page < pages_to_copy; page++)
                {
                    uint32_t page_offset = page * page_stride_bytes;
                    
                    if (page < pages_to_copy - 1)
                    {
                        // 完整页：直接拷贝整页
                        builder.buffer_to_buffer(
                            src_buffer.range(page_offset, page_offset + page_stride_bytes),
                            dst_buffer.range(page_offset, page_offset + page_stride_bytes)
                        );
                    }
                    else
                    {
                        // 最后一页：可能需要部分拷贝
                        // 按组件拷贝以避免越界
                        uint32_t offset_in_page = 0;
                        for (const auto& info : component_infos)
                        {
                            // 对齐
                            offset_in_page = (offset_in_page + info.element_align - 1) & ~(info.element_align - 1);
                            
                            // 计算这个组件在最后一页的大小
                            uint32_t component_size = info.element_size * last_page_instances;
                            
                            // 拷贝
                            builder.buffer_to_buffer(
                                src_buffer.range(page_offset + offset_in_page, 
                                               page_offset + offset_in_page + component_size),
                                dst_buffer.range(page_offset + offset_in_page, 
                                               page_offset + offset_in_page + component_size)
                            );
                            
                            // 下一个组件的偏移
                            offset_in_page += info.element_size * config.page_size;
                        }
                    }
                }
            }
        }, {}
    );
}

uint64_t SOASegmentBuffer::calculate_size_for_capacity(uint64_t capacity) const
{
    uint64_t buffer_size = 0;
    for (const auto& segment : component_infos)
    {
        uint32_t segment_size = segment.element_size * capacity;
        uint32_t aligned_size = (segment_size + segment.element_align - 1) & ~(segment.element_align - 1);
        buffer_size += aligned_size;
    }
    return buffer_size;
}

CGPUBufferId SOASegmentBuffer::create_buffer_with_capacity(uint32_t capacity)
{
    if (!device)
    {
        SKR_LOG_ERROR(u8"SOASegmentBuffer: Device not set");
        return nullptr;
    }
    
    // 计算所需缓冲区大小
    uint64_t buffer_size = calculate_size_for_capacity(capacity);
    
    if (buffer_size == 0)
    {
        SKR_LOG_WARN(u8"SOASegmentBuffer: No data to allocate");
        return nullptr;
    }
    
    CGPUBufferDescriptor buffer_desc = {};
    buffer_desc.name = u8"SOASegmentBuffer";
    buffer_desc.size = buffer_size;
    buffer_desc.memory_usage = CGPU_MEM_USAGE_GPU_ONLY;
    buffer_desc.descriptors = CGPU_RESOURCE_TYPE_BUFFER_RAW | CGPU_RESOURCE_TYPE_RW_BUFFER_RAW;
    buffer_desc.flags = CGPU_BCF_DEDICATED_BIT;
    
    CGPUBufferId new_buffer = cgpu_create_buffer(device, &buffer_desc);
    if (!new_buffer)
    {
        SKR_LOG_ERROR(u8"SOASegmentBuffer: Failed to create GPU buffer");
    }
    
    return new_buffer;
}

skr::render_graph::BufferHandle SOASegmentBuffer::import_buffer(
    skr::render_graph::RenderGraph* graph,
    const char8_t* name)
{
    if (!buffer || !graph)
        return {};
        
    // 决定导入状态
    ECGPUResourceState import_state = first_use ? 
        CGPU_RESOURCE_STATE_UNDEFINED : 
        CGPU_RESOURCE_STATE_SHADER_RESOURCE;
    
    auto handle = graph->create_buffer(
        [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::BufferBuilder& builder) {
            builder.set_name(name)
                .import(buffer, import_state)
                .allow_structured_readwrite();
        }
    );
    
    first_use = false;
    return handle;
}

} // namespace skr::renderer
