#pragma once
#include "SkrBase/config.h"
#include "SkrContainersDef/vector.hpp"
#include "SkrCore/log.h"
#include <atomic>

namespace skr::renderer
{

// SOA 段式缓冲区分配器
// 用于管理 Structure of Arrays 布局的 GPU 缓冲区
// 每个组件类型占用一个连续的段，所有实例的同一组件存储在一起
struct SKR_RENDERER_API SOASegmentBuffer
{
    using SegmentID = uint32_t;
    using InstanceID = uint32_t;
    
    // 组件段描述
    struct ComponentSegment
    {
        SegmentID type_id;          // 组件类型 ID
        uint32_t element_size;      // 每个元素大小
        uint32_t element_count;     // 元素数量（等于实例容量）
        uint32_t buffer_offset;     // 在缓冲区中的偏移
    };
    
    // 配置
    struct Config
    {
        size_t initial_size = 256 * 1024 * 1024;      // 初始大小 256MB
        size_t max_size = 1536 * 1024 * 1024;         // 最大大小 1.5GB
        bool allow_resize = true;                     // 是否允许自动扩容
    };
    
    SOASegmentBuffer() = default;
    ~SOASegmentBuffer() = default;
    
    // 初始化
    void initialize(const Config& config);
    
    // 注册组件类型
    void register_component(SegmentID type_id, uint32_t element_size, uint32_t element_align);
    
    // 完成布局计算
    void finalize_layout();
    
    // 分配实例
    InstanceID allocate_instance();
    
    // 获取组件段信息
    const ComponentSegment* get_segment(SegmentID type_id) const;
    
    // 获取组件偏移
    uint32_t get_component_offset(SegmentID type_id, InstanceID instance_id) const;
    
    // 获取统计信息
    size_t get_capacity_bytes() const { return capacity_bytes; }
    size_t get_used_bytes() const { return used_bytes; }
    uint32_t get_instance_capacity() const { return instance_capacity; }
    uint32_t get_instance_count() const { return instance_count.load(); }
    const skr::Vector<ComponentSegment>& get_segments() const { return component_segments; }
    
private:
    // 配置
    Config config;
    
    // 缓冲区信息
    size_t capacity_bytes = 0;
    size_t used_bytes = 0;
    uint32_t instance_capacity = 0;
    std::atomic<uint32_t> instance_count{0};
    
    // 组件段
    skr::Vector<ComponentSegment> component_segments;
    
    // 组件信息（用于布局计算）
    struct ComponentInfo
    {
        SegmentID type_id;
        uint32_t element_size;
        uint32_t element_align;
    };
    skr::Vector<ComponentInfo> pending_components;
    
    bool layout_finalized = false;
};

} // namespace skr::renderer
