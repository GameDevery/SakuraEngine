#pragma once
#include "./sp_util.hpp"
#include <SkrBase/misc/hash.hpp>

namespace skr
{
// 保守 unique pointer 实现，允许协变，但是不处理虚析构情况，禁止 custom deleter，只用于 RAII 的内存管理
template <typename T>
struct UPtr {
    // ctor & dtor
    UPtr();
    UPtr(std::nullptr_t);
    UPtr(T* ptr);
    template <UPtrConvertible<T> U>
    UPtr(U* ptr);
    ~UPtr();

    // copy & move
    UPtr(const UPtr& rhs) = delete;
    UPtr(UPtr&& rhs);
    template <UPtrConvertible<T> U>
    UPtr(UPtr<U>&& rhs) = delete;

    // assign & move assign
    UPtr& operator=(const UPtr& rhs) = delete;
    UPtr& operator=(UPtr&& rhs);
    template <UPtrConvertible<T> U>
    UPtr& operator=(UPtr<U>&& rhs) = delete;

    // factory
    template <typename... Args>
    static UPtr New(Args&&... args);
    template <typename... Args>
    static UPtr NewZeroed(Args&&... args);

    // getter
    T* get() const;

    // empty
    bool is_empty() const;
    operator bool() const;

    // ops
    void reset();
    void reset(T* ptr);
    template <UPtrConvertible<T> U>
    void reset(U* ptr);
    T*   release();
    void swap(UPtr& rhs);

    // pointer behaviour
    T* operator->() const;
    T& operator*() const;

    // skr hash
    static size_t _skr_hash(const UPtr& obj);

private:
    T* _ptr = nullptr;
};

} // namespace skr