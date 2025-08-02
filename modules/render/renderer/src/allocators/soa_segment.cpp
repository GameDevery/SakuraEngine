#include "SkrRenderer/allocators/soa_segment.hpp"

namespace skr::renderer
{

void SOASegmentBuffer::initialize(const Config& cfg)
{
    config = cfg;
    capacity_bytes = cfg.initial_size;
    used_bytes = 0;
    instance_count = 0;
    instance_capacity = 0;
    component_segments.clear();
    pending_components.clear();
    layout_finalized = false;
}

void SOASegmentBuffer::register_component(SegmentID type_id, uint32_t element_size, uint32_t element_align)
{
    if (layout_finalized)
    {
        SKR_LOG_ERROR(u8"SOASegmentBuffer: Cannot register component after layout is finalized");
        return;
    }
    
    ComponentInfo info;
    info.type_id = type_id;
    info.element_size = element_size;
    info.element_align = element_align;
    pending_components.push_back(info);
}

void SOASegmentBuffer::finalize_layout()
{
    if (layout_finalized)
    {
        SKR_LOG_WARN(u8"SOASegmentBuffer: Layout already finalized");
        return;
    }
    
    // 计算每个实例的总大小（所有组件大小之和）
    uint32_t total_instance_size = 0;
    for (const auto& comp : pending_components)
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
        total_instance_size, instance_capacity);
    
    // 为每个组件分配连续的段
    uint32_t current_offset = 0;
    for (const auto& comp : pending_components)
    {
        ComponentSegment segment;
        segment.type_id = comp.type_id;
        segment.element_size = comp.element_size;
        segment.element_count = instance_capacity;
        segment.buffer_offset = current_offset;
        
        component_segments.push_back(segment);
        
        // 计算下一个组件的偏移（考虑对齐）
        uint32_t segment_size = segment.element_size * instance_capacity;
        uint32_t aligned_size = (segment_size + comp.element_align - 1) & ~(comp.element_align - 1);
        current_offset += aligned_size;
        
        SKR_LOG_INFO(u8"  Segment for component %u: offset=%d, size=%d bytes, count=%d",
            segment.type_id, segment.buffer_offset, segment_size, segment.element_count);
    }
    
    used_bytes = current_offset;
    layout_finalized = true;
    
    SKR_LOG_INFO(u8"SOASegmentBuffer layout finalized: %lld MB used, %d segments",
        used_bytes / (1024 * 1024), component_segments.size());
}

SOASegmentBuffer::InstanceID SOASegmentBuffer::allocate_instance()
{
    if (!layout_finalized)
    {
        SKR_LOG_ERROR(u8"SOASegmentBuffer: Cannot allocate instance before layout is finalized");
        return static_cast<InstanceID>(-1);
    }
    
    uint32_t current_count = instance_count.load();
    if (current_count >= instance_capacity)
    {
        SKR_LOG_WARN(u8"SOASegmentBuffer: Instance capacity reached (%u/%u)", current_count, instance_capacity);
        return static_cast<InstanceID>(-1);
    }
    
    uint32_t instance_id = instance_count.fetch_add(1);
    return static_cast<InstanceID>(instance_id);
}

const SOASegmentBuffer::ComponentSegment* SOASegmentBuffer::get_segment(SegmentID type_id) const
{
    for (const auto& segment : component_segments)
    {
        if (segment.type_id == type_id)
            return &segment;
    }
    return nullptr;
}

uint32_t SOASegmentBuffer::get_component_offset(SegmentID type_id, InstanceID instance_id) const
{
    const ComponentSegment* segment = get_segment(type_id);
    if (!segment)
    {
        SKR_LOG_WARN(u8"SOASegmentBuffer: Component type %u not found", type_id);
        return 0;
    }
    
    if (instance_id >= instance_capacity)
    {
        SKR_LOG_ERROR(u8"SOASegmentBuffer: Instance ID %u out of range", instance_id);
        return segment->buffer_offset;
    }
    
    return segment->buffer_offset + (instance_id * segment->element_size);
}

} // namespace skr::renderer
