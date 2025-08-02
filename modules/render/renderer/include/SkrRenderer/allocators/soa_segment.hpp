#pragma once
#include "SkrBase/config.h"
#include "SkrContainersDef/vector.hpp"
#include "SkrGraphics/api.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include <atomic>

namespace skr::renderer
{

// SOA 段式缓冲区分配器
// 用于管理 Structure of Arrays 布局的 GPU 缓冲区
// 每个组件类型占用一个连续的段，所有实例的同一组件存储在一起
struct SKR_RENDERER_API SOASegmentBuffer
{
public:
    using SegmentID = uint32_t;
    using InstanceID = uint32_t;
    
    // 组件信息
    struct ComponentInfo
    {
        SegmentID type_id;          // 组件类型 ID
        uint32_t element_size;      // 每个元素大小
        uint32_t element_align;     // 对齐要求
    };
    
    // 组件段描述（包含布局后的信息）
    struct ComponentSegment : ComponentInfo
    {
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
    
    // Builder 模式初始化器
    class Builder
    {
    public:
        Builder(CGPUDeviceId device);
        
        // 设置配置
        Builder& with_config(const Config& config);
        
        // 单独设置配置项
        Builder& with_size(size_t initial_size, size_t max_size = 0);
        Builder& allow_resize(bool allow);
        
        // 添加组件
        Builder& add_component(SegmentID type_id, uint32_t element_size, uint32_t element_align = 16);
        
        // 构建最终的 SOASegmentBuffer
        SOASegmentBuffer build();
        
    private:
        friend struct SOASegmentBuffer;
        CGPUDeviceId device_;
        Config config_;
        skr::Vector<ComponentInfo> components_;
    };
    
    // 关闭并释放资源
    void shutdown();
    
    // 分配实例（允许超限分配）
    InstanceID allocate_instance();
    
    // Resize 功能
    bool needs_resize(uint32_t required_instances) const;
    CGPUBufferId resize_buffer(uint32_t new_instance_capacity);
    void copy_segments(skr::render_graph::RenderGraph* graph,
                      skr::render_graph::BufferHandle src_buffer,
                      skr::render_graph::BufferHandle dst_buffer,
                      uint32_t instance_count) const;
    
    // 获取组件段信息
    const ComponentSegment* get_segment(SegmentID type_id) const;
    inline CGPUBufferId get_buffer() const { return buffer; }
    
    // 获取组件偏移
    uint32_t get_component_offset(SegmentID type_id, InstanceID instance_id) const;
    
    // 获取统计信息
    inline uint64_t get_capacity_bytes() const { return capacity_bytes; }
    inline uint64_t get_total_bytes() const { return total_bytes; }
    inline uint32_t get_instance_capacity() const { return instance_capacity; }
    inline uint32_t get_instance_count() const { return instance_count.load(); }
    inline const skr::Vector<ComponentSegment>& get_segments() const { return component_segments; }
    
    // RenderGraph 导入
    skr::render_graph::BufferHandle import_buffer(skr::render_graph::RenderGraph* graph, const char8_t* name);
    
    SOASegmentBuffer() = default;
    ~SOASegmentBuffer();
    
    // 移动构造和赋值
    SOASegmentBuffer(SOASegmentBuffer&& other) noexcept;
    SOASegmentBuffer& operator=(SOASegmentBuffer&& other) noexcept;
    
    // 禁止拷贝
    SOASegmentBuffer(const SOASegmentBuffer&) = delete;
    SOASegmentBuffer& operator=(const SOASegmentBuffer&) = delete;

private:
    friend class Builder;
    SOASegmentBuffer(Builder&& builder);
    
    // 设备引用
    CGPUDeviceId device = nullptr;
    CGPUBufferId buffer = nullptr;
    
    Config config;
    skr::Vector<ComponentSegment> component_segments;

    uint64_t capacity_bytes = 0;
    uint64_t total_bytes = 0;
    uint32_t instance_capacity = 0;
    std::atomic<uint32_t> instance_count = 0;
    
    // Buffer 状态跟踪
    mutable bool first_use = true;
    
    // 辅助函数
    void initialize_from_builder(Builder&& builder);
    void calculate_layout(const skr::Vector<ComponentInfo>& components);
    void create_buffer();
    void release_buffer();
    CGPUBufferId create_buffer_with_capacity(uint32_t instance_capacity);
    skr::Vector<ComponentSegment> calculate_segments_for_capacity(uint32_t instance_capacity) const;
};

} // namespace skr::renderer
