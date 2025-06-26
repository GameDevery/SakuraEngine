#pragma once
#include <SkrRTTR/generic/generic_base.hpp>
#include <SkrRTTR/generic/generic_sparase_hash_base.hpp>
#include <SkrContainers/map.hpp>

// generic kv pair
namespace skr
{
struct SKR_CORE_API GenericKVPair final : IGenericBase {
    SKR_RC_IMPL(override final)

    // ctor & dtor
    GenericKVPair(RC<IGenericBase> inner_key, RC<IGenericBase> inner_value);
    ~GenericKVPair() override;

    //===> IGenericBase API
    // get type info
    EGenericKind kind() const override;
    GUID         id() const override;

    // get utils
    MemoryTraitsData memory_traits_data() const override;

    // basic info
    uint64_t size() const override;
    uint64_t alignment() const override;

    // operations, used for generic container algorithms
    bool   support(EGenericFeature feature) const override;
    void   default_ctor(void* dst, uint64_t count = 1) const override;
    void   dtor(void* dst, uint64_t count = 1) const override;
    void   copy(void* dst, const void* src, uint64_t count = 1) const override;
    void   move(void* dst, void* src, uint64_t count = 1) const override;
    void   assign(void* dst, const void* src, uint64_t count = 1) const override;
    void   move_assign(void* dst, void* src, uint64_t count = 1) const override;
    bool   equal(const void* lhs, const void* rhs, uint64_t count = 1) const override;
    size_t hash(const void* src) const override;
    void   swap(void* dst, void* src, uint64_t count = 1) const override;
    //===> IGenericBase API

    // getter
    bool                            is_valid() const;
    RC<IGenericBase>                inner_key() const;
    RC<GenericSparseHashSetStorage> inner_value() const;

    // get key & value
    void*       key(void* dst) const;
    void*       value(void* dst) const;
    const void* key(const void* dst) const;
    const void* value(const void* dst) const;

private:
    RC<IGenericBase> _inner_key            = nullptr;
    MemoryTraitsData _inner_key_mem_traits = {};
    uint64_t         _inner_key_size       = 0;
    uint64_t         _inner_key_alignment  = 0;

    RC<IGenericBase> _inner_value            = nullptr;
    MemoryTraitsData _inner_value_mem_traits = {};
    uint64_t         _inner_value_size       = 0;
    uint64_t         _inner_value_alignment  = 0;
};
} // namespace skr