#pragma once
#include "SkrBase/config.h"
#include "SkrContainersDef/vector.hpp"
#include "SkrGraphics/api.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderer/shared/soa_layout.hpp"
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
        SegmentID type_id;      // 组件类型 ID
        uint32_t element_size;  // 每个元素大小
        uint32_t element_align; // 对齐要求
    };

    // 组件段描述（包含布局后的信息）
    struct ComponentSegment
    {
        uint32_t element_count; // 元素数量（等于实例容量）
        uint32_t buffer_offset; // 在缓冲区中的偏移
    };

    // 配置
    struct Config
    {
        uint32_t initial_instances = 16384;      // 初始实例容量
        uint32_t page_size = UINT32_MAX;         // 页大小（实例数），UINT32_MAX表示连续布局
    };

    // Builder 模式初始化器
    class Builder
    {
    public:
        Builder(CGPUDeviceId device);

        // 设置实例容量和页大小
        Builder& with_instances(uint32_t initial_instances);
        Builder& with_page_size(uint32_t page_size);

        // 添加组件
        Builder& add_component(SegmentID type_id, uint32_t element_size, uint32_t element_align = 16);

        // 从 PagedLayout 构造
        template <typename Layout>
        Builder& from_layout(uint32_t initial_instances = 1024);

    private:
        friend struct SOASegmentBuffer;
        CGPUDeviceId device_;
        Config config_;
        skr::Vector<ComponentInfo> components_;
    };

    void initialize(const Builder& builder);
    void shutdown();

    // Resize 功能
    bool needs_resize(uint32_t required_instances) const;
    CGPUBufferId resize(uint32_t new_instance_capacity);
    void copy_segments(skr::render_graph::RenderGraph* graph,
        skr::render_graph::BufferHandle src_buffer,
        skr::render_graph::BufferHandle dst_buffer,
        uint32_t instance_count) const;

    const ComponentSegment* get_segment(SegmentID type_id) const;
    inline CGPUBufferId get_buffer() const { return buffer; }

    uint32_t get_component_offset(SegmentID type_id, InstanceID instance_id) const;
    inline uint32_t get_page_size() const { return config.page_size; }
    inline uint64_t get_capacity_bytes() const { return capacity_bytes; }
    inline uint32_t get_instance_capacity() const { return instance_capacity; }

    inline const skr::Vector<ComponentSegment>& get_segments() const { return component_segments; }
    inline const skr::Vector<ComponentInfo>& get_infos() const { return component_infos; }

    // RenderGraph 导入
    skr::render_graph::BufferHandle import_buffer(skr::render_graph::RenderGraph* graph, const char8_t* name);

    SOASegmentBuffer() = default;
    ~SOASegmentBuffer();

    // 禁止拷贝
    SOASegmentBuffer(const SOASegmentBuffer&) = delete;
    SOASegmentBuffer& operator=(const SOASegmentBuffer&) = delete;

private:
    friend class Builder;
    SOASegmentBuffer(const Builder& builder);

    // 设备引用
    CGPUDeviceId device = nullptr;
    CGPUBufferId buffer = nullptr;

    Config config;
    skr::Vector<ComponentInfo> component_infos;
    skr::Vector<ComponentSegment> component_segments;

    uint64_t capacity_bytes = 0;
    uint32_t instance_capacity = 0;

    // 分页布局相关
    uint32_t page_stride_bytes = 0; // 每页的字节大小

    // Buffer 状态跟踪
    mutable bool first_use = true;

    // 辅助函数
    void calculate_layout();
    void create_buffer();
    void release_buffer();
    CGPUBufferId create_buffer_with_capacity(uint32_t instance_capacity);
};

// 模板方法实现
template <typename Layout>
inline SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::from_layout(uint32_t initial_instances)
{
    // 设置页大小和初始实例数
    config_.page_size = Layout::page_size;
    
    // 清空现有组件
    components_.clear();
    
    // 使用 Layout 的通用遍历方法
    Layout::for_each_component([this](uint32_t id, uint32_t size, uint32_t align, uint32_t index) {
        this->add_component(id, size, align);
    });
    
    config_.initial_instances = initial_instances;

    return *this;
}

} // namespace skr::renderer
