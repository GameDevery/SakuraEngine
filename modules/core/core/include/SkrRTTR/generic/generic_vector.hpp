#pragma once
#include <SkrRTTR/generic/generic_base.hpp>
#include <SkrContainers/vector.hpp>

namespace skr
{
struct GenericVector {
    // ctor & dtor
    GenericVector(RC<IGenericBase> inner);
    ~GenericVector();

    // getter
    inline bool is_valid() const { return _inner != nullptr; }

    // vector getter
    // uint64_t size() const;
    // uint64_t capacity() const;
    // void*    data() const;

    // memory op
    // void clear();
    // void release(uint64_t reserve_capacity = 0);
    // void reserve(uint64_t expect_capacity);
    // void shrink();
    // void resize(uint64_t expect_size, void* new_value);
    // void resize_unsafe(uint64_t expect_size);
    // void resize_default(uint64_t expect_size);
    // void resize_zeroed(uint64_t expect_size);

    // add
    // void add(void* v, uint64_t n = 1);
    // void add_unsafe(uint64_t n = 1);
    // void add_default(uint64_t n = 1);
    // void add_zeroed(uint64_t n = 1);
    // void add_unique(void* v);

    // add at
    // void add_at(uint64_t idx, void* v, uint64_t n = 1);
    // void add_at_unsafe(uint64_t idx, uint64_t n = 1);
    // void add_at_default(uint64_t idx, uint64_t n = 1);
    // void add_at_zeroed(uint64_t idx, uint64_t n = 1);

    // append
    // void append(GenericVector* other);

    // append at
    // void append_at(uint64_t idx, GenericVector* other);

    // remove
    // void remove_at(uint64_t idx, uint64_t n = 1);
    // void remove_at_swap(uint64_t idx, uint64_t n = 1);
    // void remove(void* v);
    // void remove_swap(void* v);
    // void remove_last(void* v);
    // void remove_last_swap(void* v);
    // void remove_all(void* v);
    // void remove_all_swap(void* v);

    // access
    // void* at(uint64_t idx) const;
    // void* at_last(uint64_t idx) const;

    // find
    // uint64_t find(void* v) const;
    // uint64_t find_last(void* v) const;

    // sort
    // void sort_less();
    // void sort_greater();
    // void sort_stable_less();
    // void sort_stable_greater();

private:
    RC<IGenericBase> _inner            = nullptr;
    MemoryTraitsData _inner_mem_traits = {};
};
} // namespace skr