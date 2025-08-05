#pragma once
#include "SkrContainersDef/map.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrCore/log.h"
#include "SkrGraphics/api.h"
#include "SkrRenderGraph/frontend/render_graph.hpp"

namespace skr::renderer
{

// 通用页面分配器类型定义
using PageID = uint32_t;
using PageUserID = uint64_t;  // 用户自定义的页面使用者ID

static constexpr PageID INVALID_PAGE_ID = 0xFFFFFFFF;
static constexpr size_t DEFAULT_PAGE_SIZE = 16 * 1024; // 16KB - TLB友好

// 页面描述
struct PageDescriptor
{
    PageID page_id;           // 页面ID
    PageUserID user_id;       // 使用者ID（由用户定义含义）
    uint32_t used_bytes;      // 已使用字节数
    uint32_t element_count;   // 存储的元素数量
    uint64_t buffer_offset;   // 在缓冲区中的偏移
};

// 页面分配请求
struct PageRequest
{
    PageUserID user_id;       // 请求者ID
    uint32_t element_size;    // 元素大小
    uint32_t element_count;   // 需要存储的元素数量
    uint32_t alignment = 16;  // 对齐要求
};

// 通用页面池分配器 - 纯粹的页面管理，不含业务逻辑
class SKR_RENDERER_API PagePoolAllocator
{
public:
    // 配置结构
    struct Config
    {
        size_t initial_buffer_size = 64 * 1024 * 1024;    // 初始缓冲区大小 64MB
        size_t max_buffer_size = 512 * 1024 * 1024;       // 最大缓冲区大小 512MB
        size_t page_size = DEFAULT_PAGE_SIZE;             // 页面大小（默认16KB）
        bool allow_resize = true;                         // 是否允许自动扩容
    };
    
    // Builder 模式
    class Builder
    {
    public:
        Builder(CGPUDeviceId device);
        
        // 设置配置
        Builder& with_config(const Config& config);
        
        // 单独设置配置项
        Builder& with_size(size_t initial_size, size_t max_size = 0);
        Builder& with_page_size(size_t page_size);
        Builder& allow_resize(bool allow);
        
    private:
        friend class PagePoolAllocator;
        CGPUDeviceId device_;
        Config config_;
    };

    PagePoolAllocator() = default;
    ~PagePoolAllocator();

    // 初始化和销毁
    void initialize(const Builder& builder);
    void shutdown();

    // 页面分配和释放
    PageID allocate_page(const PageRequest& request);
    void free_page(PageID page_id);
    
    // 批量分配
    skr::Vector<PageID> allocate_pages(const PageRequest& request, uint32_t count);
    void free_pages(const skr::Vector<PageID>& pages);
    
    // 页面信息查询
    const PageDescriptor* get_page_descriptor(PageID page_id) const;
    skr::Vector<PageID> get_pages_by_user(PageUserID user_id) const;
    
    // 容量管理
    bool needs_expansion(size_t required_pages) const;
    CGPUBufferId expand_buffer(size_t additional_pages);
    
    // 页面数据拷贝（用于扩容时的数据迁移）
    void copy_pages(skr::render_graph::RenderGraph* graph,
                   skr::render_graph::BufferHandle src_buffer,
                   skr::render_graph::BufferHandle dst_buffer) const;
    
    // RenderGraph 集成
    skr::render_graph::BufferHandle import_buffer(skr::render_graph::RenderGraph* graph, const char8_t* name);

    // 获取页面在缓冲区中的地址
    uint64_t get_page_offset(PageID page_id) const;
    
    // 统计信息
    inline CGPUBufferId get_buffer() const { return buffer; }
    inline uint64_t get_capacity_bytes() const { return capacity_bytes; }
    inline size_t get_page_size() const { return config.page_size; }
    uint32_t get_total_page_count() const { return total_page_count; }
    uint32_t get_free_page_count() const { return static_cast<uint32_t>(_free_pages.size()); }
    uint32_t get_allocated_page_count() const { return total_page_count - get_free_page_count(); }

    // 调试信息
    void log_usage_stats() const;

    // 禁止拷贝
    PagePoolAllocator(const PagePoolAllocator&) = delete;
    PagePoolAllocator& operator=(const PagePoolAllocator&) = delete;

private:
    friend class Builder;
    
    // 设备和缓冲区管理
    CGPUDeviceId device = nullptr;
    CGPUBufferId buffer = nullptr;
    Config config;
    
    // 页面管理
    skr::Vector<PageID> _free_pages;                     // 空闲页面池
    skr::Map<PageID, PageDescriptor> page_descriptors;  // 页面描述符
    skr::Map<PageUserID, skr::Vector<PageID>> user_pages; // 用户ID到页面的映射
    
    uint32_t total_page_count = 0;
    uint64_t capacity_bytes = 0;
    
    // Buffer 状态跟踪
    mutable bool first_use = true;
    
    // 辅助函数
    void create_buffer();
    void release_buffer();
    CGPUBufferId create_buffer_with_size(size_t buffer_size);
    PageID allocate_page_internal();
    void free_page_internal(PageID page_id);
};

} // namespace skr::renderer
