#pragma once
#include "SkrBase/atomic/atomic_mutex.hpp"
#include "SkrCore/memory/rc.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrRenderer/render_device.h"
#include "SkrRenderGraph/frame_resource.hpp"
#include "SkrRenderer/shared/soa_layout.hpp"

namespace skr::gpu
{

using TableSegmentID = uint32_t;
using TableInstanceID = uint32_t;

// Segment - Buffer中的基本分配单位
struct TableSegment
{
    uint32_t stride;        // 每个实例的大小（对于bundle是所有组件的总大小）
    uint32_t align;         // 对齐要求
    uint32_t buffer_offset; // 在缓冲区中的偏移
    uint32_t element_count; // 元素数量（等于实例容量）
};

// ComponentMapping - 组件到Segment的映射关系
struct TableComponentMapping
{
    TableSegmentID component_id;  // 组件类型 ID
    uint32_t segment_index;  // 所属的Segment索引
    uint32_t element_offset; // 在Segment内每个实例的偏移
    uint32_t element_size;   // 组件大小
    uint32_t element_align;  // 组件对齐
};

// Builder 模式初始化器
struct SKR_RENDERER_API TableConfig
{
public:
    TableConfig(CGPUDeviceId device);

    TableConfig& with_instances(uint32_t initial_instances);
    TableConfig& with_page_size(uint32_t page_size);
    TableConfig& add_component(TableSegmentID type_id, uint32_t element_size, uint32_t element_align = 16);

    template <typename Layout>
    TableConfig& from_layout(uint32_t initial_instances = 1024);

private:
    friend struct TableInstance;
    CGPUDeviceId device_;
    uint32_t initial_instances = 16384;
    uint32_t page_size = UINT32_MAX; 
    skr::Vector<TableSegment> segments_;
    skr::Vector<TableComponentMapping> mappings_;
};

// 每个组件类型占用一个连续的段，所有实例的同一组件存储在一起
struct SKR_RENDERER_API TableInstance
{
public:
    TableInstance(const TableConfig& config);
    ~TableInstance();
    TableInstance(const TableInstance&) = delete;
    TableInstance& operator=(const TableInstance&) = delete;

    template <typename T>
    void Store(uint64_t offset, const T& data)
    {
        Store(offset, &data, sizeof(data));
    }
    void Store(uint64_t offset, const void* data, uint64_t size);
    uint64_t bindless_index() const;

    // Resize 功能
    bool needs_resize(uint32_t required_instances) const;
    CGPUBufferId resize(uint32_t new_instance_capacity);
    void copy_segments(skr::render_graph::RenderGraph* graph,
        skr::render_graph::BufferHandle src_buffer,
        skr::render_graph::BufferHandle dst_buffer,
        uint32_t instance_count) const;

    // 核心查询接口
    uint32_t get_component_offset(TableSegmentID component_id, TableInstanceID instance_id) const;

    // Buffer和容量信息
    inline CGPUBufferId get_buffer() const { return buffer; }
    inline uint64_t get_capacity_bytes() const { return capacity_bytes; }
    inline uint32_t get_instance_capacity() const { return instance_capacity; }

    // 分页信息
    inline uint32_t get_page_size() const { return config.page_size; }
    inline uint32_t get_page_stride_bytes() const { return page_stride_bytes; }

    // RenderGraph 导入
    skr::render_graph::BufferHandle import_buffer(skr::render_graph::RenderGraph* graph, const char8_t* name);

private:
    friend struct TableManager;

    auto& GetUpdateContext() { return updates[update_index]; }
    struct Update
    {
        uint64_t offset;
        uint64_t size;
    };
    struct UpdateContext
    {
        skr::Vector<Update> updates;
        skr::Vector<uint8_t> bytes;
    };
    UpdateContext updates[RG_MAX_FRAME_IN_FLIGHT];
    uint32_t update_index = 0;

    CGPUBufferId buffer = nullptr;
    render_graph::RenderGraphStateTracker state_tracker;

    TableConfig config;
    uint64_t capacity_bytes = 0;
    uint32_t instance_capacity = 0;
    uint32_t page_stride_bytes = 0; // 每页的字节大小

    void calculate_layout();
    void create_buffer();
    void release_buffer();
    CGPUBufferId create_buffer_with_capacity(uint32_t instance_capacity);

    SKR_RC_IMPL();
};

struct SKR_RENDERER_API TableManager
{
public:
    static skr::RC<TableManager> Create();
    skr::RC<TableInstance> CreateTable(const TableConfig& cfg);
    void UploadToGPU(skr::render_graph::RenderGraph& graph);

private:
    struct UploadContext
    {
        CGPUBufferId upload_buffer = nullptr;
        std::atomic_uint32_t upload_counter = 0;
        std::atomic<uint64_t> upload_size = 0;
        std::atomic<uint64_t> ops_size = 0;
        skr::task::event_t add_finish = skr::task::event_t(true);
        skr::task::event_t scan_finish = skr::task::event_t(true);
    };
    skr::render_graph::FrameResource<UploadContext> upload_ctxs;

    skr::shared_atomic_mutex mtx;
    skr::Vector<skr::RCWeak<TableInstance>> tables;
    SKR_RC_IMPL();
};

template <typename Layout>
inline TableConfig& TableConfig::from_layout(uint32_t initial_instances)
{
    page_size = Layout::page_size;
    initial_instances = initial_instances;
    segments_.clear();
    mappings_.clear();

    []<typename... Es>(typename Layout::template TypePack<Es...>*, auto* cfg) {
        uint32_t comp_id = 0;
        auto process = [&comp_id, cfg]<typename E>() {
            cfg->add_component(comp_id++, sizeof(E), alignof(E));
        };
        (process.template operator()<Es>(), ...);
    }(static_cast<typename Layout::Elements*>(nullptr), this);

    return *this;
}

} // namespace skr::gpu
