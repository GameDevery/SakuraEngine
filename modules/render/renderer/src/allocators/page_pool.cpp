#include "SkrRenderer/allocators/page_pool.hpp"
#include <algorithm>

namespace skr::renderer
{

// ==================== Builder Implementation ====================

PagePoolAllocator::Builder::Builder(CGPUDeviceId device)
    : device_(device)
{
}

PagePoolAllocator::Builder& PagePoolAllocator::Builder::with_config(const Config& cfg)
{
    config_ = cfg;
    return *this;
}

PagePoolAllocator::Builder& PagePoolAllocator::Builder::with_size(size_t initial_size, size_t max_size)
{
    config_.initial_buffer_size = initial_size;
    config_.max_buffer_size = max_size > 0 ? max_size : initial_size * 8; // 默认最大 8 倍
    return *this;
}

PagePoolAllocator::Builder& PagePoolAllocator::Builder::with_page_size(size_t page_size)
{
    config_.page_size = page_size;
    return *this;
}

PagePoolAllocator::Builder& PagePoolAllocator::Builder::allow_resize(bool allow)
{
    config_.allow_resize = allow;
    return *this;
}

// ==================== PagePoolAllocator Implementation ====================

PagePoolAllocator::~PagePoolAllocator()
{
    shutdown();
}

void PagePoolAllocator::initialize(const Builder& builder)
{
    SKR_LOG_DEBUG(u8"PagePoolAllocator: Initializing with page size %u bytes", (uint32_t)builder.config_.page_size);
    
    device = builder.device_;
    config = builder.config_;
    
    // 计算初始页面数
    total_page_count = static_cast<uint32_t>(config.initial_buffer_size / config.page_size);
    capacity_bytes = config.initial_buffer_size;
    
    // 初始化空闲页面池
    _free_pages.clear();
    _free_pages.reserve(total_page_count);
    for (uint32_t i = 0; i < total_page_count; ++i)
    {
        _free_pages.push_back(i);
    }
    
    // 创建 GPU 缓冲区
    create_buffer();
    
    SKR_LOG_INFO(u8"PagePoolAllocator: Initialized with %u pages (%u KB each), total %lld MB",
                 total_page_count, (uint32_t)(config.page_size / 1024), capacity_bytes / (1024 * 1024));
}

void PagePoolAllocator::shutdown()
{
    SKR_LOG_DEBUG(u8"PagePoolAllocator: Shutting down...");
    
    release_buffer();
    
    _free_pages.clear();
    page_descriptors.clear();
    user_pages.clear();
    
    total_page_count = 0;
    capacity_bytes = 0;
    device = nullptr;
    
    SKR_LOG_DEBUG(u8"PagePoolAllocator: Shutdown completed");
}

PageID PagePoolAllocator::allocate_page(const PageRequest& request)
{
    if (_free_pages.is_empty())
    {
        SKR_LOG_WARN(u8"PagePoolAllocator: No free pages available");
        return INVALID_PAGE_ID;
    }
    
    // 分配页面
    PageID page_id = allocate_page_internal();
    if (page_id == INVALID_PAGE_ID)
    {
        return INVALID_PAGE_ID;
    }
    
    // 创建页面描述符
    PageDescriptor descriptor;
    descriptor.page_id = page_id;
    descriptor.user_id = request.user_id;
    descriptor.used_bytes = request.element_size * request.element_count;
    descriptor.element_count = request.element_count;
    descriptor.buffer_offset = page_id * config.page_size;
    
    // 记录页面信息
    page_descriptors.add(page_id, descriptor);
    
    // 记录用户-页面映射
    auto user_pages_iter = user_pages.try_add_default(request.user_id);
    user_pages_iter.value().push_back(page_id);
    
    SKR_LOG_TRACE(u8"PagePoolAllocator: Allocated page %u for user %llu (used: %u bytes)",
                  page_id, request.user_id, descriptor.used_bytes);
    
    return page_id;
}

void PagePoolAllocator::free_page(PageID page_id)
{
    auto desc_iter = page_descriptors.find(page_id);
    if (!desc_iter)
    {
        SKR_LOG_WARN(u8"PagePoolAllocator: Attempting to free invalid page %u", page_id);
        return;
    }
    
    PageDescriptor desc = desc_iter.value();
    
    // 从用户映射中移除
    if (auto user_iter = user_pages.find(desc.user_id))
    {
        auto& pages = user_iter.value();
        pages.remove(page_id);
        if (pages.is_empty())
        {
            user_pages.remove(desc.user_id);
        }
    }
    
    // 移除页面描述符
    page_descriptors.remove(page_id);
    
    // 归还到空闲池
    free_page_internal(page_id);
    
    SKR_LOG_TRACE(u8"PagePoolAllocator: Freed page %u (was used by user %llu)", page_id, desc.user_id);
}

skr::Vector<PageID> PagePoolAllocator::allocate_pages(const PageRequest& request, uint32_t count)
{
    skr::Vector<PageID> allocated_pages;
    allocated_pages.reserve(count);
    
    for (uint32_t i = 0; i < count; ++i)
    {
        PageID page_id = allocate_page(request);
        if (page_id == INVALID_PAGE_ID)
        {
            // 分配失败，释放已分配的页面
            for (auto id : allocated_pages)
            {
                free_page(id);
            }
            allocated_pages.clear();
            break;
        }
        allocated_pages.push_back(page_id);
    }
    
    return allocated_pages;
}

void PagePoolAllocator::free_pages(const skr::Vector<PageID>& pages)
{
    for (auto page_id : pages)
    {
        free_page(page_id);
    }
}

const PageDescriptor* PagePoolAllocator::get_page_descriptor(PageID page_id) const
{
    auto iter = page_descriptors.find(page_id);
    return iter ? &iter.value() : nullptr;
}

skr::Vector<PageID> PagePoolAllocator::get_pages_by_user(PageUserID user_id) const
{
    auto iter = user_pages.find(user_id);
    return iter ? iter.value() : skr::Vector<PageID>{};
}

bool PagePoolAllocator::needs_expansion(size_t required_pages) const
{
    return _free_pages.size() < required_pages;
}

CGPUBufferId PagePoolAllocator::expand_buffer(size_t additional_pages)
{
    if (!config.allow_resize)
    {
        SKR_LOG_WARN(u8"PagePoolAllocator: Resize is disabled");
        return nullptr;
    }
    
    // 计算新的缓冲区大小
    size_t additional_size = additional_pages * config.page_size;
    size_t new_buffer_size = capacity_bytes + additional_size;
    
    if (new_buffer_size > config.max_buffer_size)
    {
        SKR_LOG_WARN(u8"PagePoolAllocator: New buffer size %lld exceeds maximum %lld",
                     new_buffer_size, config.max_buffer_size);
        return nullptr;
    }
    
    // 保存旧 buffer
    CGPUBufferId old_buffer = buffer;
    uint32_t old_page_count = total_page_count;
    
    // 更新容量
    capacity_bytes = new_buffer_size;
    total_page_count = static_cast<uint32_t>(new_buffer_size / config.page_size);
    
    // 添加新的空闲页面
    for (uint32_t i = old_page_count; i < total_page_count; ++i)
    {
        _free_pages.push_back(i);
    }
    
    // 创建新 buffer
    buffer = create_buffer_with_size(capacity_bytes);
    first_use = true; // 新 buffer 需要重新开始状态跟踪
    
    SKR_LOG_INFO(u8"PagePoolAllocator: Expanded buffer by %u pages (%lld -> %lld MB)",
                 (uint32_t)additional_pages, 
                 (capacity_bytes - additional_size) / (1024 * 1024), 
                 capacity_bytes / (1024 * 1024));
    
    return old_buffer;
}

void PagePoolAllocator::copy_pages(skr::render_graph::RenderGraph* graph,
                                  skr::render_graph::BufferHandle src_buffer,
                                  skr::render_graph::BufferHandle dst_buffer) const
{
    if (page_descriptors.is_empty())
    {
        SKR_LOG_DEBUG(u8"PagePoolAllocator: No allocated pages to copy");
        return;
    }
    
    SKR_LOG_DEBUG(u8"PagePoolAllocator: Copying %llu allocated pages", page_descriptors.size());
    
    // 添加拷贝 pass
    graph->add_copy_pass(
        [=, this](skr::render_graph::RenderGraph& g, skr::render_graph::CopyPassBuilder& builder) {
            builder.set_name(u8"PagePoolAllocator_CopyPages");
            
            // 拷贝所有已分配的页面
            for (const auto& [page_id, descriptor] : page_descriptors)
            {
                uint64_t page_offset = descriptor.buffer_offset;
                uint64_t copy_size = config.page_size; // 拷贝整个页面
                
                builder.buffer_to_buffer(
                    src_buffer.range(page_offset, page_offset + copy_size),
                    dst_buffer.range(page_offset, page_offset + copy_size)
                );
                
                SKR_LOG_TRACE(u8"  Copying page %u: offset=%llu, size=%llu bytes",
                             page_id, page_offset, copy_size);
            }
        }, {}
    );
    
    SKR_LOG_INFO(u8"PagePoolAllocator: Scheduled copy pass for %llu pages", page_descriptors.size());
}

skr::render_graph::BufferHandle PagePoolAllocator::import_buffer(
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

uint64_t PagePoolAllocator::get_page_offset(PageID page_id) const
{
    return page_id * config.page_size;
}

void PagePoolAllocator::log_usage_stats() const
{
    SKR_LOG_INFO(u8"PagePoolAllocator Usage Statistics:");
    SKR_LOG_INFO(u8"  Total Pages: %u", total_page_count);
    SKR_LOG_INFO(u8"  Free Pages: %u", get_free_page_count());
    SKR_LOG_INFO(u8"  Allocated Pages: %u", get_allocated_page_count());
    SKR_LOG_INFO(u8"  Page Size: %u KB", (uint32_t)(config.page_size / 1024));
    SKR_LOG_INFO(u8"  Buffer Size: %lld MB", capacity_bytes / (1024 * 1024));
    SKR_LOG_INFO(u8"  Unique Users: %llu", user_pages.size());
    
    // 统计每个用户的页面使用情况
    if (!user_pages.is_empty())
    {
        SKR_LOG_INFO(u8"  Pages by User:");
        for (const auto& [user_id, pages] : user_pages)
        {
            size_t total_bytes = 0;
            for (auto page_id : pages)
            {
                if (auto desc = get_page_descriptor(page_id))
                {
                    total_bytes += desc->used_bytes;
                }
            }
            SKR_LOG_INFO(u8"    User %llu: %llu pages, ~%llu KB used",
                        user_id, pages.size(), total_bytes / 1024);
        }
    }
}

void PagePoolAllocator::create_buffer()
{
    if (!device)
    {
        SKR_LOG_ERROR(u8"PagePoolAllocator: Device not set");
        return;
    }
    
    if (buffer)
    {
        SKR_LOG_WARN(u8"PagePoolAllocator: Buffer already created");
        return;
    }
    
    if (capacity_bytes == 0)
    {
        SKR_LOG_WARN(u8"PagePoolAllocator: No buffer size specified");
        return;
    }
    
    buffer = create_buffer_with_size(capacity_bytes);
    if (buffer)
    {
        SKR_LOG_INFO(u8"PagePoolAllocator: Created GPU buffer (%lld MB)", capacity_bytes / (1024 * 1024));
    }
}

void PagePoolAllocator::release_buffer()
{
    if (buffer)
    {
        cgpu_free_buffer(buffer);
        buffer = nullptr;
        SKR_LOG_DEBUG(u8"PagePoolAllocator: Released GPU buffer");
    }
}

CGPUBufferId PagePoolAllocator::create_buffer_with_size(size_t buffer_size)
{
    if (!device)
    {
        SKR_LOG_ERROR(u8"PagePoolAllocator: Device not set");
        return nullptr;
    }
    
    if (buffer_size == 0)
    {
        SKR_LOG_WARN(u8"PagePoolAllocator: No data to allocate");
        return nullptr;
    }
    
    CGPUBufferDescriptor buffer_desc = {};
    buffer_desc.name = u8"PagePoolAllocator-Buffer";
    buffer_desc.size = buffer_size;
    buffer_desc.memory_usage = CGPU_MEM_USAGE_GPU_ONLY;
    buffer_desc.descriptors = CGPU_RESOURCE_TYPE_BUFFER_RAW | CGPU_RESOURCE_TYPE_RW_BUFFER_RAW;
    buffer_desc.flags = CGPU_BCF_DEDICATED_BIT;
    
    CGPUBufferId new_buffer = cgpu_create_buffer(device, &buffer_desc);
    if (!new_buffer)
    {
        SKR_LOG_ERROR(u8"PagePoolAllocator: Failed to create GPU buffer");
    }
    
    return new_buffer;
}

PageID PagePoolAllocator::allocate_page_internal()
{
    if (_free_pages.is_empty())
    {
        return INVALID_PAGE_ID;
    }
    
    PageID page_id = _free_pages.back();
    _free_pages.pop_back();
    return page_id;
}

void PagePoolAllocator::free_page_internal(PageID page_id)
{
    if (page_id == INVALID_PAGE_ID)
        return;
    
    _free_pages.push_back(page_id);
}

} // namespace skr::renderer