#pragma once
#include "SkrProfile/profile.h"
#include "SkrBase/misc/debug.h" 
#include "SkrBase/config.h"
#include <string.h>  // memset
#ifdef __cplusplus
    #include <new>         // 'operator new' function for non-allocating placement new expression
    #include <string.h>    // memset
    #include <cstddef>     // std::size_t
    #include <cstdint>     // PTRDIFF_MAX
    #include <type_traits> // std::true_type
    namespace skr { using std::forward; using std::move; }
    #if defined(SKR_PROFILE_ENABLE) && defined(TRACY_TRACE_ALLOCATION)
    #include "SkrBase/misc/demangle.hpp"
    #endif
#endif

SKR_EXTERN_C SKR_CORE_API void* _sakura_malloc(size_t size, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void* _sakura_calloc(size_t count, size_t size, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void* _sakura_malloc_aligned(size_t size, size_t alignment, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void* _sakura_calloc_aligned(size_t count, size_t size, size_t alignment, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void* _sakura_new_n(size_t count, size_t size, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void* _sakura_new_aligned(size_t size, size_t alignment, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void _sakura_free(void* p, const char* pool_name) SKR_NOEXCEPT;
SKR_EXTERN_C SKR_CORE_API void _sakura_free_aligned(void* p, size_t alignment, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void* _sakura_realloc(void* p, size_t newsize, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void* _sakura_realloc_aligned(void* p, size_t newsize, size_t alignment, const char* pool_name);

SKR_EXTERN_C SKR_CORE_API void* traced_os_malloc(size_t size, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void* traced_os_calloc(size_t count, size_t size, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void* traced_os_malloc_aligned(size_t size, size_t alignment, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void* traced_os_calloc_aligned(size_t count, size_t size, size_t alignment, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void traced_os_free(void* p, const char* pool_name) SKR_NOEXCEPT;
SKR_EXTERN_C SKR_CORE_API void traced_os_free_aligned(void* p, size_t alignment, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void* traced_os_realloc(void* p, size_t newsize, const char* pool_name);
SKR_EXTERN_C SKR_CORE_API void* traced_os_realloc_aligned(void* p, size_t newsize, size_t alignment, const char* pool_name);

SKR_EXTERN_C SKR_CORE_API void* containers_malloc_aligned(size_t size, size_t alignment);
SKR_EXTERN_C SKR_CORE_API void containers_free_aligned(void* p, size_t alignment);

#define SKR_ALLOC_TRACY_MARKER_COLOR 0xff0000
#define SKR_DEALLOC_TRACY_MARKER_COLOR 0x0000ff
#if defined(SKR_PROFILE_ENABLE) && defined(TRACY_TRACE_ALLOCATION)

SKR_FORCEINLINE void* SkrMallocWithCZone(size_t size, const char* line, const char* pool_name)
{
    SkrCZoneC(z, SKR_ALLOC_TRACY_MARKER_COLOR, 1);
    SkrCZoneText(z, line, strlen(line));
    SkrCZoneName(z, line, strlen(line));
    void* ptr = _sakura_malloc(size, pool_name);
    SkrCZoneEnd(z);
    return ptr;
}

SKR_FORCEINLINE void* SkrCallocWithCZone(size_t count, size_t size, const char* line, const char* pool_name)
{
    SkrCZoneC(z, SKR_ALLOC_TRACY_MARKER_COLOR, 1);
    SkrCZoneText(z, line, strlen(line));
    SkrCZoneName(z, line, strlen(line));
    void* ptr = _sakura_calloc(count, size, pool_name);
    SkrCZoneEnd(z);
    return ptr;
}

SKR_FORCEINLINE void* SkrMallocAlignedWithCZone(size_t size, size_t alignment, const char* line, const char* pool_name)
{
    SkrCZoneC(z, SKR_ALLOC_TRACY_MARKER_COLOR, 1);
    SkrCZoneText(z, line, strlen(line));
    SkrCZoneName(z, line, strlen(line));
    void* ptr = _sakura_malloc_aligned(size, alignment, pool_name);
    SkrCZoneEnd(z);
    return ptr;
}

SKR_FORCEINLINE void* SkrCallocAlignedWithCZone(size_t count, size_t size, size_t alignment, const char* line, const char* pool_name)
{
    SkrCZoneC(z, SKR_ALLOC_TRACY_MARKER_COLOR, 1);
    SkrCZoneText(z, line, strlen(line));
    SkrCZoneName(z, line, strlen(line));
    void* ptr = _sakura_calloc_aligned(count, size, alignment, pool_name);
    SkrCZoneEnd(z);
    return ptr;
}

SKR_FORCEINLINE void* SkrNewNWithCZone(size_t count, size_t size, const char* line, const char* pool_name)
{
    SkrCZoneC(z, SKR_ALLOC_TRACY_MARKER_COLOR, 1);
    SkrCZoneText(z, line, strlen(line));
    SkrCZoneName(z, line, strlen(line));
    void* ptr = _sakura_new_n(count, size, pool_name);
    SkrCZoneEnd(z);
    return ptr;
}

SKR_FORCEINLINE void* SkrNewAlignedWithCZone(size_t size, size_t alignment, const char* line, const char* pool_name)
{
    SkrCZoneC(z, SKR_ALLOC_TRACY_MARKER_COLOR, 1);
    SkrCZoneText(z, line, strlen(line));
    SkrCZoneName(z, line, strlen(line));
    void* ptr = _sakura_new_aligned(size, alignment, pool_name);
    SkrCZoneEnd(z);
    return ptr;
}

SKR_FORCEINLINE void SkrFreeWithCZone(void* p, const char* line, const char* pool_name)
{
    SkrCZoneC(z, SKR_DEALLOC_TRACY_MARKER_COLOR, 1);
    SkrCZoneText(z, line, strlen(line));
    SkrCZoneName(z, line, strlen(line));
    _sakura_free(p, pool_name);
    SkrCZoneEnd(z);
}

SKR_FORCEINLINE void SkrFreeAlignedWithCZone(void* p, size_t alignment, const char* line, const char* pool_name)
{
    SkrCZoneC(z, SKR_DEALLOC_TRACY_MARKER_COLOR, 1);
    SkrCZoneText(z, line, strlen(line));
    SkrCZoneName(z, line, strlen(line));
    _sakura_free_aligned(p, alignment, pool_name);
    SkrCZoneEnd(z);
}

SKR_FORCEINLINE void* SkrReallocWithCZone(void* p, size_t newsize, const char* line, const char* pool_name)
{
    SkrCZoneC(z, SKR_ALLOC_TRACY_MARKER_COLOR, 1);
    SkrCZoneText(z, line, strlen(line));
    SkrCZoneName(z, line, strlen(line));
    void* ptr = _sakura_realloc(p, newsize, pool_name);
    SkrCZoneEnd(z);
    return ptr;
}

SKR_FORCEINLINE void* SkrReallocAlignedWithCZone(void* p, size_t newsize, size_t alignment, const char* line, const char* pool_name)
{
    SkrCZoneC(z, SKR_ALLOC_TRACY_MARKER_COLOR, 1);
    SkrCZoneText(z, line, strlen(line));
    SkrCZoneName(z, line, strlen(line));
    void* ptr = _sakura_realloc_aligned(p, newsize, alignment, pool_name);
    SkrCZoneEnd(z);
    return ptr;
}

#define SKR_ALLOC_STRINGFY_IMPL(X) #X
#define SKR_ALLOC_STRINGFY(X) SKR_ALLOC_STRINGFY_IMPL(X)
#define SKR_ALLOC_CAT_IMPL(X,Y) X  Y
#define SKR_ALLOC_CAT(X,Y) SKR_ALLOC_CAT_IMPL(X,Y)

#define sakura_malloc(size) SkrMallocWithCZone((size), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), NULL )
#define sakura_calloc(count, size) SkrCallocWithCZone((count), (size), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), NULL )
#define sakura_malloc_aligned(size, alignment) SkrMallocAlignedWithCZone((size), (alignment), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), NULL )
#define sakura_calloc_aligned(count, size, alignment) SkrCallocAlignedWithCZone((count), (size), (alignment), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), NULL )
#define sakura_new_n(count, size) SkrNewNWithCZone((count), (size), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), NULL )
#define sakura_new_aligned(size, alignment) SkrNewAlignedWithCZone((size), (alignment), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), NULL )
#define sakura_realloc(p, newsize) SkrReallocWithCZone((p), (newsize), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), NULL )
#define sakura_realloc_aligned(p, newsize, align) SkrReallocAlignedWithCZone((p), (newsize), (align), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), NULL )
#define sakura_free(p) SkrFreeWithCZone((p), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), NULL )
#define sakura_free_aligned(p, alignment) SkrFreeAlignedWithCZone((p), (alignment), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), NULL )

#define sakura_mallocN(size, ...) SkrMallocWithCZone((size), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), __VA_ARGS__ )
#define sakura_callocN(count, size, ...) SkrCallocWithCZone((count), (size), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), __VA_ARGS__ )
#define sakura_malloc_alignedN(size, alignment, ...) SkrMallocAlignedWithCZone((size), (alignment), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), __VA_ARGS__ )
#define sakura_calloc_alignedN(count, size, alignment, ...) SkrCallocAlignedWithCZone((count), (size), (alignment), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), __VA_ARGS__ )
#define sakura_new_nN(count, size, ...) SkrNewNWithCZone((count), (size), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), __VA_ARGS__ )
#define sakura_new_alignedN(size, alignment, ...) SkrNewAlignedWithCZone((size), (alignment), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), __VA_ARGS__ )
#define sakura_realloc_alignedN(p, newsize, align, ...) SkrReallocAlignedWithCZone((p), (newsize), (align), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), __VA_ARGS__ )
#define sakura_reallocN(p, newsize, ...) SkrReallocWithCZone((p), (newsize), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), __VA_ARGS__ )
#define sakura_freeN(p, ...) SkrFreeWithCZone((p), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), __VA_ARGS__ )
#define sakura_free_alignedN(p, alignment, ...) SkrFreeAlignedWithCZone((p), (alignment), SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), __VA_ARGS__ )

#else

#define sakura_malloc(size) _sakura_malloc((size), NULL)
#define sakura_calloc(count, size) _sakura_calloc((count), (size), NULL)
#define sakura_malloc_aligned(size, alignment) _sakura_malloc_aligned((size), (alignment), NULL)
#define sakura_calloc_aligned(count, size, alignment) _sakura_calloc_aligned((count), (size), (alignment), NULL)
#define sakura_new_n(count, size) _sakura_new_n((count), (size), NULL)
#define sakura_new_aligned(size, alignment) _sakura_new_aligned((size), (alignment), NULL)
#define sakura_realloc(p, newsize) _sakura_realloc((p), (newsize), NULL)
#define sakura_realloc_aligned(p, newsize, align) _sakura_realloc_aligned((p), (newsize), (align), NULL)
#define sakura_free(p) _sakura_free((p), NULL)
#define sakura_free_aligned(p, alignment) _sakura_free_aligned((p), (alignment), NULL)

#define sakura_mallocN(size, ...) _sakura_malloc((size), __VA_ARGS__)
#define sakura_callocN(count, size, ...) _sakura_calloc((count), (size), __VA_ARGS__)
#define sakura_malloc_alignedN(size, alignment, ...) _sakura_malloc_aligned((size), (alignment), __VA_ARGS__)
#define sakura_calloc_alignedN(count, size, alignment, ...) _sakura_calloc_aligned((count), (size), (alignment), __VA_ARGS__)
#define sakura_new_nN(count, size, ...) _sakura_new_n((count), (size), __VA_ARGS__)
#define sakura_new_alignedN(size, alignment, ...) _sakura_new_aligned((size), (alignment), __VA_ARGS__)
#define sakura_reallocN(p, newsize, ...) _sakura_realloc((p), (newsize), __VA_ARGS__)
#define sakura_realloc_alignedN(p, newsize, align, ...) _sakura_realloc_aligned((p), (newsize), (align), __VA_ARGS__)
#define sakura_freeN(p, ...) _sakura_free((p), __VA_ARGS__)
#define sakura_free_alignedN(p, alignment, ...) _sakura_free_aligned((p), (alignment), __VA_ARGS__)

#endif

#ifdef _CRTDBG_MAP_ALLOC
    #define DEBUG_NEW_SOURCE_LINE (_NORMAL_BLOCK, __FILE__, __LINE__)
#else
    #define DEBUG_NEW_SOURCE_LINE
#endif

#if defined(__cplusplus)

#if defined(SKR_PROFILE_ENABLE) && defined(TRACY_TRACE_ALLOCATION)
struct SkrNewCore
{
    const std::string_view sourcelocation;
    const char* poolname;
    SkrNewCore(std::string_view sourcelocation) noexcept : sourcelocation(sourcelocation), poolname(NULL) {}
    SkrNewCore(std::string_view sourcelocation, const char* poolname) noexcept : sourcelocation(sourcelocation), poolname(poolname) {}

    template<class T>
    [[nodiscard]] SKR_FORCEINLINE T* Alloc()
    {
        const std::string_view name = skr::demangle<T>();
        SkrCMessage(name.data(), name.size());
        void* p = SkrNewAlignedWithCZone(sizeof(T), alignof(T), sourcelocation.data(), poolname);
        SKR_ASSERT(p != nullptr);
        return static_cast<T*>(p);
    };

    template<class T>
    [[nodiscard]] SKR_FORCEINLINE T* AllocSized(size_t size)
    {
        const std::string_view name = skr::demangle<T>();
        SkrCMessage(name.data(), name.size());
        SKR_ASSERT(size >= sizeof(T));
        void* p = SkrNewAlignedWithCZone(size, alignof(T), sourcelocation.data(), poolname);
        SKR_ASSERT(p != nullptr);
        return static_cast<T*>(p);
    };

    template<class T>
    SKR_FORCEINLINE void Free(T* p)
    {
        if (p != nullptr)
        {
            const std::string_view name = skr::demangle<T>();
            SkrCMessage(name.data(), name.size());
            SkrFreeAlignedWithCZone((void*)p, alignof(T), sourcelocation.data(), poolname);
        }
    }
};
#define SKR_MAKE_NEW_CORE SkrNewCore{SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__))}
#define SKR_MAKE_NEW_CORE_N(__N) SkrNewCore{SKR_ALLOC_CAT(SKR_ALLOC_STRINGFY(__FILE__),SKR_ALLOC_STRINGFY(__LINE__)), __N}
#else
struct SkrNewCore
{
    template<class T>
    [[nodiscard]] SKR_FORCEINLINE T* Alloc()
    {
        void* p = sakura_new_aligned(sizeof(T), alignof(T));
        SKR_ASSERT(p != nullptr);
        return static_cast<T*>(p);
    };

    template<class T>
    [[nodiscard]] SKR_FORCEINLINE T* AllocSized(size_t size)
    {
        SKR_ASSERT(size >= sizeof(T));
        void* p = sakura_new_aligned(size, alignof(T));
        SKR_ASSERT(p != nullptr);
        return static_cast<T*>(p);
    };

    template<class T>
    SKR_FORCEINLINE void Free(T* p)
    {
        if (p != nullptr)
        {
            sakura_free_aligned((void*)p, alignof(T));
        }
    }
};
#define SKR_MAKE_NEW_CORE SkrNewCore{}
#define SKR_MAKE_NEW_CORE_N(__N) SkrNewCore{}
#endif

template<typename T>
struct SkrNewImpl
{
    SkrNewCore core;
    SKR_FORCEINLINE SkrNewImpl(SkrNewCore core): core(core) {}

    template<typename...Args>
    [[nodiscard]] SKR_FORCEINLINE T* New(Args&&...args)
    {
        T* p = core.Alloc<T>();
        return new (p) DEBUG_NEW_SOURCE_LINE T{ skr::forward<Args>(args)... };
    }
    template<typename...Args>
    [[nodiscard]] SKR_FORCEINLINE T* NewZeroed(Args&&...args)
    {
        T* p = core.Alloc<T>();
        memset(p, 0, sizeof(T));
        return new (p) DEBUG_NEW_SOURCE_LINE T{ skr::forward<Args>(args)... };
    }
    template<class F>
    [[nodiscard]] SKR_FORCEINLINE F* NewLambda(F&& lambda)
    {
        F* p = core.Alloc<F>();
        return new (p) DEBUG_NEW_SOURCE_LINE auto(skr::forward<F>(lambda));
    }
    void Delete(T* p)
    {
        SKR_ASSERT(p != nullptr);
        p->~T();
        core.Free(p);
    }
};

struct SkrNewWrapper
{
    SkrNewCore core;
    SKR_FORCEINLINE SkrNewWrapper(SkrNewCore core): core(core) {}

    template<typename T, typename...Args>
    [[nodiscard]] SKR_FORCEINLINE T* New(Args&&...args)
    {
        return SkrNewImpl<T>(core).New(skr::forward<Args>(args)...);
    }
    template<typename T, typename...Args>
    [[nodiscard]] SKR_FORCEINLINE T* NewZeroed(Args&&...args)
    {
        return SkrNewImpl<T>(core).NewZeroed(skr::forward<Args>(args)...);
    }
    template<class F>
    [[nodiscard]] SKR_FORCEINLINE F* NewLambda(F&& lambda)
    {
        return SkrNewImpl<F>(core).NewLambda(skr::forward<F>(lambda));
    }
    template<typename T>
    void Delete(T* p)
    {
        return SkrNewImpl<T>(core).Delete(p);
    }
};

#define SkrNew SkrNewWrapper(SKR_MAKE_NEW_CORE).New
#define SkrNewZeroed SkrNewWrapper(SKR_MAKE_NEW_CORE).NewZeroed
#define SkrNewLambda SkrNewWrapper(SKR_MAKE_NEW_CORE).NewLambda
#define SkrDelete SkrNewWrapper(SKR_MAKE_NEW_CORE).Delete

#define SkrNewN(__N) SkrNewWrapper(SKR_MAKE_NEW_CORE_N(__N)).New
#define SkrNewZeroedN(__N) SkrNewWrapper(SKR_MAKE_NEW_CORE_N(__N)).NewZeroed
#define SkrNewLambdaN(__N) SkrNewWrapper(SKR_MAKE_NEW_CORE_N(__N)).NewLambda
#define SkrDeleteN(__N) SkrNewWrapper(SKR_MAKE_NEW_CORE_N(__N)).Delete

template<class T> 
struct skr_stl_allocator 
{
    typedef T                 value_type;
    typedef std::size_t       size_type;
    typedef std::ptrdiff_t    difference_type;
    typedef value_type&       reference;
    typedef value_type const& const_reference;
    typedef value_type*       pointer;
    typedef value_type const* const_pointer;
    template <class U> struct rebind { typedef skr_stl_allocator<U> other; };

    skr_stl_allocator()                                              SKR_NOEXCEPT = default;
    skr_stl_allocator(const skr_stl_allocator&)                      SKR_NOEXCEPT = default;
    template<class U> skr_stl_allocator(const skr_stl_allocator<U>&) SKR_NOEXCEPT { }
    skr_stl_allocator  select_on_container_copy_construction() const { return *this; }
    void              deallocate(T* p, size_type) { sakura_free(p); }

#if (__cplusplus >= 201703L)  // C++17
    [[nodiscard]] T* allocate(size_type count) { return static_cast<T*>(sakura_new_n(count, sizeof(T))); }
    [[nodiscard]] T* allocate(size_type count, const void*) { return allocate(count); }
#else
    [[nodiscard]] pointer allocate(size_type count, const void* = 0) { return static_cast<pointer>(sakura_new_n(count, sizeof(value_type))); }
#endif

#if ((__cplusplus >= 201103L) || (_MSC_VER > 1900))  // C++11
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap            = std::true_type;
    using is_always_equal                        = std::true_type;
    template <class U, class ...Args> void construct(U* p, Args&& ...args) { ::new(p) U(skr::forward<Args>(args)...); }
    template <class U> void destroy(U* p) SKR_NOEXCEPT { p->~U(); }
#else
    void construct(pointer p, value_type const& val) { ::new(p) value_type(val); }
    void destroy(pointer p) { p->~value_type(); }
#endif

    size_type     max_size() const SKR_NOEXCEPT { return (PTRDIFF_MAX / sizeof(value_type)); }
    pointer       address(reference x) const        { return &x; }
    const_pointer address(const_reference x) const  { return &x; }
};

template<class T1,class T2> bool operator==(const skr_stl_allocator<T1>& , const skr_stl_allocator<T2>& ) SKR_NOEXCEPT { return true; }
template<class T1,class T2> bool operator!=(const skr_stl_allocator<T1>& , const skr_stl_allocator<T2>& ) SKR_NOEXCEPT { return false; }
#endif // __cplusplus