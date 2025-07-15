#pragma once
#include "SkrRenderGraph/api.h"
#include "SkrCore/memory/memory.h"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/map.hpp"
#include "SkrContainersDef/set.hpp"

namespace skr::render_graph
{

struct SKR_RENDER_GRAPH_API RenderGraphStackAllocator {
    struct CtorParam {
        const char* name = "RenderGraphPool";
    };
    
    static constexpr bool support_realloc = false; // Pool allocators don't support realloc
    
    RenderGraphStackAllocator(CtorParam param) noexcept;
    ~RenderGraphStackAllocator() noexcept;
    
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
        return nullptr;
    }
    
    // Raw allocation API
    static void* alloc_raw(size_t count, size_t item_size, size_t item_align);
    static void free_raw(void* p, size_t item_align);
    
    static void Initialize();
    static void Finalize();
    static void Reset();
    
    struct Impl;
};

// Global type alias for easier use
using RenderGraphPool = RenderGraphStackAllocator;

template <typename T>
using StackVector = skr::Vector<T, RenderGraphStackAllocator>;

template <typename K, typename V>
using StackMap = skr::Map<K, V, skr::container::HashTraits<K>, RenderGraphStackAllocator>;

template <typename K>
using StackSet = skr::Set<K, skr::container::HashTraits<K>, RenderGraphStackAllocator>;

} // namespace skr::render_graph