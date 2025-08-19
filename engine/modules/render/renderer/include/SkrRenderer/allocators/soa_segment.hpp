#pragma once
#include "SkrBase/config.h"
#include "SkrContainersDef/vector.hpp"
#include "SkrGraphics/api.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"
#include "SkrRenderer/shared/soa_layout.hpp"

namespace skr
{

// SOA 段式缓冲区分配器
// 用于管理 Structure of Arrays 布局的 GPU 缓冲区
// 每个组件类型占用一个连续的段，所有实例的同一组件存储在一起
struct SKR_RENDERER_API SOASegmentBuffer
{
public:
    using SegmentID = uint32_t;
    using InstanceID = uint32_t;

    // Segment - Buffer中的基本分配单位
    struct Segment
    {
        uint32_t stride;         // 每个实例的大小（对于bundle是所有组件的总大小）
        uint32_t align;          // 对齐要求
        uint32_t buffer_offset;  // 在缓冲区中的偏移
        uint32_t element_count;  // 元素数量（等于实例容量）
    };

    // ComponentMapping - 组件到Segment的映射关系
    struct ComponentMapping
    {
        SegmentID component_id;      // 组件类型 ID
        uint32_t segment_index;       // 所属的Segment索引
        uint32_t element_offset;      // 在Segment内每个实例的偏移
        uint32_t element_size;        // 组件大小
        uint32_t element_align;       // 组件对齐
    };

    // 配置
    struct Config
    {
        uint32_t initial_instances = 16384;      // 初始实例容量
        uint32_t page_size = UINT32_MAX;         // 页大小（实例数），UINT32_MAX表示连续布局
    };

    // Builder 模式初始化器
    class SKR_RENDERER_API Builder
    {
    public:
        Builder() : device_(nullptr) {}
        Builder(CGPUDeviceId device);

        // 设置实例容量和页大小
        Builder& with_instances(uint32_t initial_instances);
        Builder& with_page_size(uint32_t page_size);

        // 添加单个组件（创建独立Segment）
        Builder& add_component(SegmentID type_id, uint32_t element_size, uint32_t element_align = 16);
        
        // 开始一个Bundle（多个组件共享Segment）
        Builder& begin_bundle();
        Builder& add_bundle_component(SegmentID type_id, uint32_t element_size, uint32_t element_align = 16);
        Builder& end_bundle();

        // 从 PagedLayout 构造
        template <typename Layout>
        Builder& from_layout(uint32_t initial_instances = 1024);

    private:
        friend struct SOASegmentBuffer;
        CGPUDeviceId device_;
        Config config_;
        
        // Bundle构建状态
        struct BundleBuilder {
            skr::Vector<ComponentMapping> components;
            uint32_t stride = 0;
            uint32_t align = 0;
            bool active = false;
            
            void reset() {
                components.clear();
                stride = 0;
                align = 0;
                active = false;
            }
        };
        
        skr::Vector<Segment> segments_;
        skr::Vector<ComponentMapping> mappings_;
        BundleBuilder bundle_builder_;  // 栈上分配，避免堆分配
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

    // 核心查询接口
    uint32_t get_component_offset(SegmentID component_id, InstanceID instance_id) const;
    
    // Buffer和容量信息
    inline CGPUBufferId get_buffer() const { return buffer; }
    inline uint64_t get_capacity_bytes() const { return capacity_bytes; }
    inline uint32_t get_instance_capacity() const { return instance_capacity; }
    
    // 分页信息
    inline uint32_t get_page_size() const { return config.page_size; }
    inline uint32_t get_page_stride_bytes() const { return page_stride_bytes; }

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
    skr::Vector<Segment> segments;              // 所有的Segment
    skr::Vector<ComponentMapping> component_mappings;  // 组件到Segment的映射

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

template <typename Layout>
inline SOASegmentBuffer::Builder& SOASegmentBuffer::Builder::from_layout(uint32_t initial_instances)
{
    config_.page_size = Layout::page_size;
    config_.initial_instances = initial_instances;
    segments_.clear();
    mappings_.clear();
    
    // 解析Layout，识别Bundle和独立组件
    []<typename... Es>(typename Layout::template TypePack<Es...>*, auto* builder) {
        uint32_t comp_id = 0;
        auto process = [&comp_id, builder]<typename E>() {
            if constexpr (is_bundle_v<E>) {
                // 处理Bundle
                builder->begin_bundle();
                []<typename... Bs>(ComponentBundle<Bs...>, auto* b, uint32_t& id) {
                    ((b->add_bundle_component(id++, sizeof(Bs), alignof(Bs))), ...);
                }(E{}, builder, comp_id);
                builder->end_bundle();
            } else {
                // 处理独立组件
                builder->add_component(comp_id++, sizeof(E), alignof(E));
            }
        };
        (process.template operator()<Es>(), ...);
    }(static_cast<typename Layout::Elements*>(nullptr), this);
    
    return *this;
}

} // namespace skr
