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

SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::allow_resize(bool allow)
{
    config_.allow_resize = allow;
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

    SKR_LOG_INFO(u8"SOASegmentBuffer: total_instance_size=%d bytes, max_instances=%d",
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
            return component_segments[i].buffer_offset + (instance_id * info.element_size);
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
            
            // 计算源 buffer 的布局（基于实际的 instance_count）
            uint32_t src_offset = 0;
            
            for (uint32_t i = 0; i < component_infos.size(); i++)
            {
                const auto& info = component_infos[i];
                const auto& segment = component_segments[i];

                // 计算源段的大小
                uint32_t src_segment_size = info.element_size * instance_count;
                uint32_t src_aligned_size = (src_segment_size + info.element_align - 1) & ~(info.element_align - 1);
                
                // 目标偏移直接使用当前 segment 的偏移（新布局）
                uint32_t dst_offset = segment.buffer_offset;
                
                // 使用 buffer_to_buffer 进行拷贝
                builder.buffer_to_buffer(
                    src_buffer.range(src_offset, src_offset + src_segment_size),
                    dst_buffer.range(dst_offset, dst_offset + src_segment_size)
                );
                
                // 更新源偏移
                src_offset += src_aligned_size;
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
                .allow_shader_readwrite();
        }
    );
    
    first_use = false;
    return handle;
}

} // namespace skr::renderer
