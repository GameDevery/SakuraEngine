#pragma once
#include "./rc_util.hpp"

namespace skr
{
template <ObjectWithRC T>
struct RC {
    // ctor & dtor
    RC();
    RC(std::nullptr_t);
    RC(T* ptr);
    ~RC();

    // copy & move
    template <ObjectWithRCConvertible<T> U>
    RC(const RC<U>& rhs);
    RC(const RC& rhs);
    RC(RC&& rhs);

    // assign & move assign
    template <ObjectWithRCConvertible<T> U>
    RC& operator=(const RC<U>& rhs);
    RC& operator=(const RC& rhs);
    RC& operator=(RC&& rhs);

    // factory
    template <typename... Args>
    static RC New(Args&&... args);
    template <typename... Args>
    static RC NewZeroed(Args&&... args);

    // compare
    friend bool operator==(const RC& lhs, const RC& rhs);
    friend bool operator!=(const RC& lhs, const RC& rhs);
    friend bool operator<(const RC& lhs, const RC& rhs);
    friend bool operator>(const RC& lhs, const RC& rhs);
    friend bool operator<=(const RC& lhs, const RC& rhs);
    friend bool operator>=(const RC& lhs, const RC& rhs);

    // getter
    T*       get();
    const T* get() const;

    // ops
    void reset();
    void reset(T* ptr);
    void swap(RC& rhs);

    // pointer behaviour
    T*       operator->();
    const T* operator->() const;
    T&       operator*();
    const T& operator*() const;

    // skr hash
    static size_t _skr_hash(const RC& obj);

private:
    T* _ptr = nullptr;
};
template <ObjectWithRC T>
struct RCUnique {
};
template <ObjectWithRC T>
struct RCWeak {
};
} // namespace skr

namespace skr
{

}