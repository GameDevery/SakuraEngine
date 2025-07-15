#include "SkrRenderGraph/stack_allocator.hpp"
#include "SkrContainers/vector.hpp"
#include "SkrContainers/hashmap.hpp"
#include "SkrOS/thread.h"
#include "SkrCore/log.hpp"
#include <memory>

namespace skr::render_graph
{

static const char* kStackAllocatorName = "RenderGraphStackAllocator";

// Implementation details
struct RenderGraphStackAllocator::Impl
{
    static constexpr size_t kDefaultChunkSize = 64 * 1024; // 64KB 默认块大小
    static constexpr size_t kAlignment = 16; // 16字节对齐

    struct MemoryChunk
    {
        void* memory = nullptr;
        size_t size = 0;
        size_t used = 0;
        
        MemoryChunk(size_t chunk_size)
            : size(chunk_size)
            , used(0)
        {
            memory = sakura_calloc_alignedN(1, chunk_size, kAlignment, kStackAllocatorName);
            SKR_ASSERT(memory && "Failed to allocate memory chunk for RenderGraphStackAllocator");
        }
        
        ~MemoryChunk()
        {
            if (memory)
            {
                sakura_free_alignedN(memory, kAlignment, kStackAllocatorName);
                memory = nullptr;
            }
        }
        
        // 不可复制但可移动
        MemoryChunk(const MemoryChunk&) = delete;
        MemoryChunk& operator=(const MemoryChunk&) = delete;
        
        MemoryChunk(MemoryChunk&& other) noexcept
            : memory(other.memory)
            , size(other.size)
            , used(other.used)
        {
            other.memory = nullptr;
            other.size = 0;
            other.used = 0;
        }
        
        MemoryChunk& operator=(MemoryChunk&& other) noexcept
        {
            if (this != &other)
            {
                if (memory)
                {
                    sakura_free_alignedN(memory, kAlignment, kStackAllocatorName);
                }
                
                memory = other.memory;
                size = other.size;
                used = other.used;
                
                other.memory = nullptr;
                other.size = 0;
                other.used = 0;
            }
            return *this;
        }
        
        void* allocate(size_t requested_size, size_t align)
        {
            // 计算对齐后的起始位置
            uintptr_t current_pos = reinterpret_cast<uintptr_t>(memory) + used;
            uintptr_t aligned_pos = (current_pos + align - 1) & ~(align - 1);
            size_t alignment_padding = aligned_pos - current_pos;
            
            size_t total_needed = alignment_padding + requested_size;
            
            if (used + total_needed > size)
            {
                return nullptr; // 空间不足
            }
            
            used += total_needed;
            return reinterpret_cast<void*>(aligned_pos);
        }
        
        void reset()
        {
            used = 0;
        }
        
        size_t available() const
        {
            return size - used;
        }
    };
    
    skr::Vector<MemoryChunk> chunks_;
    size_t current_chunk_size_ = kDefaultChunkSize;
    const char* name_;
    
    // 统计信息
    size_t total_allocated_bytes_ = 0;
    size_t peak_used_bytes_ = 0;
    size_t allocation_count_ = 0;
    
    Impl(const char* name) : name_(name)
    {
        SKR_LOG_TRACE(u8"RenderGraphStackAllocator::Impl created: %s", name_);
    }
    
    ~Impl()
    {
        SKR_LOG_TRACE(u8"RenderGraphStackAllocator::Impl destroyed: %s", name_);
        SKR_LOG_TRACE(u8"  Total allocated: %zu bytes", total_allocated_bytes_);
        SKR_LOG_TRACE(u8"  Peak used: %zu bytes", peak_used_bytes_);
        SKR_LOG_TRACE(u8"  Total allocations: %zu", allocation_count_);
        
        chunks_.clear(); // 这会调用每个chunk的析构函数
    }
    
    void* allocate(size_t requested_size, size_t align)
    {
        if (requested_size == 0) return nullptr;
        
        // 确保对齐至少是默认对齐
        align = std::max(align, kAlignment);
        
        // 尝试从现有chunks分配
        for (auto& chunk : chunks_)
        {
            if (void* ptr = chunk.allocate(requested_size, align))
            {
                ++allocation_count_;
                update_peak_usage();
                return ptr;
            }
        }
        
        // 需要新的chunk
        size_t chunk_size = std::max(current_chunk_size_, requested_size + align);
        chunks_.add(chunk_size);
        
        total_allocated_bytes_ += chunk_size;
        
        void* ptr = chunks_.back().allocate(requested_size, align);
        SKR_ASSERT(ptr && "Failed to allocate from newly created chunk");
        
        ++allocation_count_;
        update_peak_usage();
        
        SKR_LOG_TRACE(u8"RenderGraphStackAllocator allocated new chunk: %zu bytes (total: %zu bytes)", 
                     chunk_size, total_allocated_bytes_);
        
        return ptr;
    }
    
    void reset()
    {
        // 重置所有chunks的使用情况，但不释放内存
        for (auto& chunk : chunks_)
        {
            chunk.reset();
        }
        
        SKR_LOG_TRACE(u8"RenderGraphStackAllocator reset: %s (kept %zu chunks, %zu bytes)", 
                     name_, chunks_.size(), total_allocated_bytes_);
    }
    
    void finalize()
    {
        SKR_LOG_TRACE(u8"RenderGraphStackAllocator finalize: %s (releasing %zu chunks, %zu bytes)", 
                     name_, chunks_.size(), total_allocated_bytes_);
        
        chunks_.clear();
        total_allocated_bytes_ = 0;
        allocation_count_ = 0;
        peak_used_bytes_ = 0;
    }
    
private:
    void update_peak_usage()
    {
        size_t current_used = 0;
        for (const auto& chunk : chunks_)
        {
            current_used += chunk.used;
        }
        peak_used_bytes_ = std::max(peak_used_bytes_, current_used);
    }
};

// Global singleton instance
static std::unique_ptr<RenderGraphStackAllocator::Impl> g_pool_impl = nullptr;
static SMutex g_instance_mutex = {};
static bool g_initialized = false;

void RenderGraphStackAllocator::Initialize() {
    if (g_initialized) return;
    
    skr_init_mutex(&g_instance_mutex);
    skr_mutex_acquire(&g_instance_mutex);
    {
        if (!g_initialized)
        {
            g_pool_impl = std::make_unique<Impl>("RenderGraphStackAllocator");
            g_initialized = true;
            SKR_LOG_INFO(u8"RenderGraphStackAllocator initialized");
        }
    }
    skr_mutex_release(&g_instance_mutex);
}

void RenderGraphStackAllocator::Finalize() {
    if (!g_initialized) return;
    
    skr_mutex_acquire(&g_instance_mutex);
    {
        if (g_initialized)
        {
            g_pool_impl->finalize();
            g_pool_impl.reset();
            g_initialized = false;
            SKR_LOG_INFO(u8"RenderGraphStackAllocator finalized");
        }
    }
    skr_mutex_release(&g_instance_mutex);
    
    skr_destroy_mutex(&g_instance_mutex);
}

static RenderGraphStackAllocator::Impl& get_impl() {
    SKR_ASSERT(g_pool_impl && "RenderGraphStackAllocator not initialized! Call RenderGraphStackAllocator::Initialize() first.");
    return *g_pool_impl;
}

RenderGraphStackAllocator::RenderGraphStackAllocator(CtorParam param) noexcept 
{
    // 这个类现在只是一个接口，实际的实现在全局单例中
}

RenderGraphStackAllocator::~RenderGraphStackAllocator() noexcept 
{
    // 析构函数不做任何事，因为内存管理由全局单例负责
}

void* RenderGraphStackAllocator::alloc_raw(size_t count, size_t item_size, size_t item_align) 
{
    if (count == 0 || item_size == 0) return nullptr;
    
    size_t total_size = count * item_size;
    return get_impl().allocate(total_size, item_align);
}

void RenderGraphStackAllocator::free_raw(void* p, size_t item_align) 
{
    // Stack allocator忽略free操作
    // 内存将在reset()时被重用，在finalize()时被释放
}

// 添加reset和finalize的静态接口
void RenderGraphStackAllocator::Reset()
{
    if (g_initialized && g_pool_impl)
    {
        skr_mutex_acquire(&g_instance_mutex);
        g_pool_impl->reset();
        skr_mutex_release(&g_instance_mutex);
    }
}

} // namespace skr::render_graph