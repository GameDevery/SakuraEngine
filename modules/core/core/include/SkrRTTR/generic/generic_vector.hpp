#pragma once
#include <SkrRTTR/generic/generic_base.hpp>
#include <SkrContainers/vector.hpp>

namespace skr
{
struct SKR_CORE_API GenericVector final : IGenericBase {
    SKR_RC_IMPL(override final)

    // ctor & dtor
    GenericVector(RC<IGenericBase> inner);
    ~GenericVector();

    //===> IGenericBase API
    // get type info
    EGenericKind kind() const override final;
    GUID         id() const override final;

    // get utils
    MemoryTraitsData memory_traits_data() const override final;

    // basic info
    uint64_t size() const override final;
    uint64_t alignment() const override final;

    // operations, used for generic container algorithms
    bool   support(EGenericFeature feature) const override final;
    void   default_ctor(void* dst, uint64_t count = 1) const override final;
    void   dtor(void* dst, uint64_t count = 1) const override final;
    void   copy(void* dst, const void* src, uint64_t count = 1) const override final;
    void   move(void* dst, void* src, uint64_t count = 1) const override final;
    void   assign(void* dst, const void* src, uint64_t count = 1) const override final;
    void   move_assign(void* dst, void* src, uint64_t count = 1) const override final;
    bool   equal(const void* lhs, const void* rhs) const override final;
    size_t hash(const void* src) const override final;
    //===> IGenericBase API

    // getter
    bool is_valid() const;

    // vector getter
    uint64_t    size(const void* dst) const;
    uint64_t    capacity(const void* dst) const;
    const void* data(const void* dst) const;
    void*       data(void* dst) const;

    // memory op
    void clear(void* dst) const;
    void release(void* dst, uint64_t reserve_capacity = 0) const;
    void reserve(void* dst, uint64_t expect_capacity) const;
    void shrink(void* dst) const;
    void resize(void* dst, uint64_t expect_size, void* new_value) const;
    void resize_unsafe(void* dst, uint64_t expect_size) const;
    void resize_default(void* dst, uint64_t expect_size) const;
    void resize_zeroed(void* dst, uint64_t expect_size) const;

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
    // helper
    void _realloc(void* dst, uint64_t new_capacity) const;
    void _free(void* dst) const;

private:
    RC<IGenericBase> _inner            = nullptr;
    MemoryTraitsData _inner_mem_traits = {};
};
} // namespace skr