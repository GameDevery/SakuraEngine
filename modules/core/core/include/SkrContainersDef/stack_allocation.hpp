#pragma once
#include "SkrContainers/vector.hpp"

namespace skr
{
template <typename T>
struct StackAllocator
{
    struct MemoryChunk
    {
        void* memory = nullptr;
        size_t size = 0;
        size_t used = 0;
        
        MemoryChunk(size_t chunk_size)
            : size(chunk_size)
            , used(0)
        {
            memory = sakura_calloc_alignedN(1, chunk_size, T::kAlignment, T::GetAllocatorName());
            SKR_ASSERT(memory && "Failed to allocate memory chunk for RenderGraphStackAllocator");
        }
        
        ~MemoryChunk()
        {
            if (memory)
            {
                sakura_free_alignedN(memory, T::kAlignment, T::GetAllocatorName());
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
                    sakura_free_alignedN(memory, T::kAlignment, T::GetAllocatorName());
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
    size_t current_chunk_size_ = T::kDefaultChunkSize;
    
    // 统计信息
    size_t total_allocated_bytes_ = 0;
    size_t peak_used_bytes_ = 0;
    size_t allocation_count_ = 0;
    
    StackAllocator()
    {
        
    }
    
    ~StackAllocator()
    {
        chunks_.clear(); // 这会调用每个chunk的析构函数
    }
    
    void* allocate(size_t requested_size, size_t align)
    {
        if (requested_size == 0) return nullptr;
        
        // 确保对齐至少是默认对齐
        align = std::max(align, T::kAlignment);
        
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
        
        return ptr;
    }
    
    void reset()
    {
        // 重置所有chunks的使用情况，但不释放内存
        for (auto& chunk : chunks_)
        {
            chunk.reset();
        }
    }
    
    void finalize()
    {
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
} // namespace skr