#include "SkrRenderer/allocators/soa_segment.hpp"
#include <utility>

namespace skr::renderer
{

// ==================== Builder Implementation ====================

SOASegmentBuffer::Builder::Builder(CGPUDeviceId device)
    : device_(device)
{
}

SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::with_instances(uint32_t initial_instances)
{
    config_.initial_instances = initial_instances;
    return *this;
}

SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::with_page_size(uint32_t page_size)
{
    config_.page_size = page_size;
    return *this;
}

SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::add_component(SegmentID type_id, uint32_t element_size, uint32_t element_align)
{
    SKR_ASSERT(!bundle_builder_.active && "Cannot add component while building a bundle");
    
    // 创建独立的Segment
    Segment segment;
    segment.stride = element_size;
    segment.align = element_align;
    segment.buffer_offset = 0;  // 将在calculate_layout中计算
    segment.element_count = 0;  // 将在initialize中设置
    
    uint32_t segment_index = segments_.size();
    segments_.add(segment);
    
    // 创建Component到Segment的映射
    ComponentMapping mapping;
    mapping.component_id = type_id;
    mapping.segment_index = segment_index;
    mapping.element_offset = 0;  // 独立Segment，偏移为0
    mapping.element_size = element_size;
    mapping.element_align = element_align;
    
    mappings_.add(mapping);
    
    return *this;
}

SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::begin_bundle()
{
    SKR_ASSERT(!bundle_builder_.active && "Bundle already started");
    bundle_builder_.reset();
    bundle_builder_.active = true;
    return *this;
}

SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::add_bundle_component(SegmentID type_id, uint32_t element_size, uint32_t element_align)
{
    SKR_ASSERT(bundle_builder_.active && "Must call begin_bundle first");
    
    // 计算在bundle内的偏移
    uint32_t offset = bundle_builder_.stride;
    offset = (offset + element_align - 1) & ~(element_align - 1);
    
    ComponentMapping mapping;
    mapping.component_id = type_id;
    mapping.segment_index = UINT32_MAX;  // 暂时设置为无效值，end_bundle时更新
    mapping.element_offset = offset;
    mapping.element_size = element_size;
    mapping.element_align = element_align;
    
    bundle_builder_.components.add(mapping);
    bundle_builder_.stride = offset + element_size;
    bundle_builder_.align = (element_align > bundle_builder_.align) ? element_align : bundle_builder_.align;
    
    return *this;
}

SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::end_bundle()
{
    SKR_ASSERT(bundle_builder_.active && "No bundle to end");
    SKR_ASSERT(!bundle_builder_.components.is_empty() && "Bundle must have at least one component");
    
    // 创建共享的Segment
    Segment segment;
    segment.stride = bundle_builder_.stride;
    segment.align = bundle_builder_.align;
    segment.buffer_offset = 0;  // 将在calculate_layout中计算
    segment.element_count = 0;  // 将在initialize中设置
    
    uint32_t segment_index = segments_.size();
    segments_.add(segment);
    
    // 更新所有bundle组件的segment_index并添加到映射
    for (auto& mapping : bundle_builder_.components) {
        mapping.segment_index = segment_index;
        mappings_.add(mapping);
    }
    
    bundle_builder_.reset();  // 清理并标记为非活动
    
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
    segments.clear();
    component_mappings.clear();
}

void SOASegmentBuffer::initialize(const Builder& builder)
{
    device = builder.device_;
    config = builder.config_;
    segments = builder.segments_;
    component_mappings = builder.mappings_;
    
    // 根据布局类型确定实际容量
    if (config.page_size == UINT32_MAX)
    {
        // 连续布局：直接使用请求的容量
        instance_capacity = config.initial_instances;
        SKR_LOG_INFO(u8"SOASegmentBuffer: Continuous layout, capacity = %u instances", instance_capacity);
    }
    else
    {
        // 分页布局：容量必须是页大小的整数倍
        uint32_t page_count = (config.initial_instances + config.page_size - 1) / config.page_size;
        instance_capacity = page_count * config.page_size;
        SKR_LOG_INFO(u8"SOASegmentBuffer: Paged layout, requested = %u, page_size = %u, page_count = %u, capacity = %u instances",
                     config.initial_instances, config.page_size, page_count, instance_capacity);
    }
    
    // 设置所有Segment的element_count
    for (auto& segment : segments) {
        segment.element_count = instance_capacity;
    }
    
    // 计算布局
    calculate_layout();

    // 创建 GPU 缓冲区
    create_buffer();
}

void SOASegmentBuffer::calculate_layout()
{
    uint32_t offset = 0;
    
    if (config.page_size == UINT32_MAX)
    {
        // 连续布局（传统 SOA）
        for (auto& segment : segments)
        {
            // 对齐
            offset = (offset + segment.align - 1) & ~(segment.align - 1);
            
            segment.buffer_offset = offset;
            
            offset += segment.stride * instance_capacity;
        }
        
        capacity_bytes = offset;
    }
    else
    {
        // 分页布局
        // 基于 instance_capacity 计算需要的页数
        uint32_t page_count = (instance_capacity + config.page_size - 1) / config.page_size;
        
        // 计算每页的字节大小
        page_stride_bytes = 0;
        for (const auto& segment : segments)
        {
            page_stride_bytes = (page_stride_bytes + segment.align - 1) & ~(segment.align - 1);
            page_stride_bytes += segment.stride * config.page_size;
        }
        page_stride_bytes = (page_stride_bytes + 255) & ~255;  // 对齐 256 字节
        
        // 设置每个Segment在页内的偏移
        offset = 0;
        for (auto& segment : segments)
        {
            offset = (offset + segment.align - 1) & ~(segment.align - 1);
            segment.buffer_offset = offset;  // 页内偏移
            
            offset += segment.stride * config.page_size;
        }
        
        // 总容量字节数
        capacity_bytes = page_stride_bytes * page_count;
    }
}

uint32_t SOASegmentBuffer::get_component_offset(SegmentID component_id, InstanceID instance_id) const
{
    // 查找组件映射
    for (const auto& mapping : component_mappings)
    {
        if (mapping.component_id == component_id)
        {
            const auto& segment = segments[mapping.segment_index];
            
            if (config.page_size == UINT32_MAX)
            {
                // 连续布局
                // 地址 = Segment基址 + 实例偏移 * Segment步长 + 组件在Segment内的偏移
                return segment.buffer_offset + (instance_id * segment.stride) + mapping.element_offset;
            }
            else
            {
                // 分页布局
                uint32_t page = instance_id / config.page_size;
                uint32_t offset_in_page = instance_id % config.page_size;
                uint32_t page_start = page * page_stride_bytes;
                
                // 地址 = 页起始 + Segment在页内基址 + 页内实例偏移 * Segment步长 + 组件在Segment内的偏移
                uint32_t result = page_start + segment.buffer_offset + (offset_in_page * segment.stride) + mapping.element_offset;
                
                // 调试日志
                if (instance_id < 5) {
                    SKR_LOG_WARN(u8"SOA: Component %u, Instance %u → page=%u, offset_in_page=%u, page_start=%u, segment.buffer_offset=%u, segment.stride=%u, element_offset=%u → result=%u",
                        component_id, instance_id, page, offset_in_page, page_start, segment.buffer_offset, segment.stride, mapping.element_offset, result);
                }
                
                return result;
            }
        }
    }
    
    SKR_LOG_WARN(u8"SOASegmentBuffer: Component %u not found", component_id);
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
    // 对于分页布局，确保容量是页大小的整数倍
    if (config.page_size != UINT32_MAX)
    {
        uint32_t page_count = (new_instance_capacity + config.page_size - 1) / config.page_size;
        new_instance_capacity = page_count * config.page_size;
    }
    
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
    
    // 更新所有Segment的element_count
    for (auto& segment : segments) {
        segment.element_count = instance_capacity;
    }
    
    // 重新计算布局
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
                // 连续布局：按Segment拷贝
                for (const auto& segment : segments)
                {
                    // 计算要拷贝的大小（Segment步长 * 实例数）
                    uint32_t copy_size = segment.stride * instance_count;
                    uint32_t offset = segment.buffer_offset;
                    
                    // 拷贝整个Segment
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
                        // 按Segment拷贝以避免越界
                        uint32_t offset_in_page = 0;
                        for (const auto& segment : segments)
                        {
                            // 对齐
                            offset_in_page = (offset_in_page + segment.align - 1) & ~(segment.align - 1);
                            
                            // 计算这个Segment在最后一页的大小
                            uint32_t segment_size = segment.stride * last_page_instances;
                            
                            // 拷贝
                            builder.buffer_to_buffer(
                                src_buffer.range(page_offset + offset_in_page, 
                                               page_offset + offset_in_page + segment_size),
                                dst_buffer.range(page_offset + offset_in_page, 
                                               page_offset + offset_in_page + segment_size)
                            );
                            
                            // 下一个Segment的偏移
                            offset_in_page += segment.stride * config.page_size;
                        }
                    }
                }
            }
        }, {}
    );
}


CGPUBufferId SOASegmentBuffer::create_buffer_with_capacity(uint32_t capacity)
{
    if (!device)
    {
        SKR_LOG_ERROR(u8"SOASegmentBuffer: Device not set");
        return nullptr;
    }
    
    // capacity_bytes 已经在 calculate_layout 中计算
    if (capacity_bytes == 0)
    {
        SKR_LOG_WARN(u8"SOASegmentBuffer: No data to allocate");
        return nullptr;
    }
    
    CGPUBufferDescriptor buffer_desc = {};
    buffer_desc.name = u8"SOASegmentBuffer";
    buffer_desc.size = capacity_bytes;
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
