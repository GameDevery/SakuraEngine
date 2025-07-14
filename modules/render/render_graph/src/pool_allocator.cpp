#include "SkrRenderGraph/pool_allocator.hpp"
#include "SkrContainers/vector.hpp"
#include "SkrContainers/hashmap.hpp"
#include "SkrOS/thread.h"

namespace skr::render_graph
{

// Global singleton instance
static RenderGraphPoolAllocator::Impl* g_pool_impl = nullptr;
static SMutex g_instance_mutex = {};
static const char* kRenderGraphMemoryPoolName = "RenderGraph";

void RenderGraphPoolAllocator::Initialize() {
    static bool initialized = false;
    if (initialized) return;
    
    skr_init_mutex(&g_instance_mutex);
    skr_mutex_acquire(&g_instance_mutex);
    {
        // ...
    }
    skr_mutex_release(&g_instance_mutex);
    
    initialized = true;
}

void RenderGraphPoolAllocator::Finalize() {
    skr_mutex_acquire(&g_instance_mutex);
    {
        // ...
    }
    skr_mutex_release(&g_instance_mutex);
    
    skr_destroy_mutex(&g_instance_mutex);
}

static RenderGraphPoolAllocator::Impl& get_impl() {
    SKR_ASSERT(g_pool_impl && "RenderGraphPoolAllocator not initialized! Call RenderGraphPoolAllocator::Initialize() first.");
    return *g_pool_impl;
}

RenderGraphPoolAllocator::RenderGraphPoolAllocator(CtorParam param) noexcept 
{

}

RenderGraphPoolAllocator::~RenderGraphPoolAllocator() noexcept 
{

}

void* RenderGraphPoolAllocator::alloc_raw(size_t count, size_t item_size, size_t item_align) 
{
#if defined(TRACY_TRACE_ALLOCATION)
    SkrCZoneNCS(z, "containers::allocate", SKR_ALLOC_TRACY_MARKER_COLOR, 16, 1);
    void* p = sakura_malloc_alignedN(count * item_size, item_align, kRenderGraphMemoryPoolName);
    SkrCZoneEnd(z);
    return p;
#else
    return sakura_malloc_aligned(count * item_size, item_align);
#endif
}

void RenderGraphPoolAllocator::free_raw(void* p, size_t item_align) 
{
#if defined(TRACY_TRACE_ALLOCATION)
    SkrCZoneNCS(z, "containers::free", SKR_DEALLOC_TRACY_MARKER_COLOR, 16, 1);
    sakura_free_alignedN(p, item_align, kRenderGraphMemoryPoolName);
    SkrCZoneEnd(z);
#else
    sakura_free_aligned(p, item_align);
#endif
}

void* RenderGraphPoolAllocator::realloc_raw(void* p, size_t count, size_t item_size, size_t item_align) {
#if defined(TRACY_TRACE_ALLOCATION)
    SkrCZoneNCS(z, "containers::realloc", SKR_DEALLOC_TRACY_MARKER_COLOR, 16, 1);
    void* new_mem = sakura_realloc_alignedN(p, count * item_size, item_align, kRenderGraphMemoryPoolName);
    SkrCZoneEnd(z);
    return new_mem;
#else
    return sakura_realloc_aligned(p, count * item_size, item_align);
#endif
}

} // namespace skr::render_graph