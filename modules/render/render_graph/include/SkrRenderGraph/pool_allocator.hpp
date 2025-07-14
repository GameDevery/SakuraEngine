#pragma once
#include "SkrRenderGraph/api.h"
#include "SkrCore/memory/memory.h"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/map.hpp"
#include "SkrContainersDef/set.hpp"

namespace skr::render_graph
{

struct SKR_RENDER_GRAPH_API RenderGraphPoolAllocator {
    struct CtorParam {
        const char* name = "RenderGraphPool";
    };
    
    static constexpr bool support_realloc = true; // Pool allocators don't support realloc
    
    RenderGraphPoolAllocator(CtorParam param) noexcept;
    ~RenderGraphPoolAllocator() noexcept;
    
    template <typename T>
    inline static T* alloc(size_t count)
    {
        return reinterpret_cast<T*>(alloc_raw(count, sizeof(T), alignof(T)));
    }
    
    template <typename T>
    inline static void free(T* p)
    {
        if (p) {
            free_raw(reinterpret_cast<void*>(p), alignof(T));
        }
    }
    
    template <typename T>
    inline static T* realloc(T* p, size_t size)
    {
        return reinterpret_cast<T*>(realloc_raw(
            reinterpret_cast<void*>(p),
            size,
            sizeof(T),
            alignof(T)
        ));
    }
    
    // Raw allocation API
    static void* alloc_raw(size_t count, size_t item_size, size_t item_align);
    static void free_raw(void* p, size_t item_align);
    static void* realloc_raw(void* p, size_t count, size_t item_size, size_t item_align);
    
    // Global initialization/finalization
    static void Initialize();
    static void Finalize();
    
    struct Impl;
};

// Global type alias for easier use
using RenderGraphPool = RenderGraphPoolAllocator;

template <typename T>
using PooledVector = skr::Vector<T, RenderGraphPoolAllocator>;

template <typename K, typename V>
using PooledMap = skr::Map<K, V, skr::container::HashTraits<K>, RenderGraphPoolAllocator>;

template <typename K>
using PooledSet = skr::Set<K, skr::container::HashTraits<K>, RenderGraphPoolAllocator>;

} // namespace skr::render_graph