#pragma once
#include <SkrRTTR/generic/generic_base.hpp>
#include <SkrContainers/optional.hpp>

namespace skr
{
struct SKR_CORE_API GenericOptional final : IGenericBase {
    SKR_RC_IMPL(override final)

    // ctor & dtor
    GenericOptional(RC<IGenericBase> inner);
    ~GenericOptional();

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
    Expected<EGenericError>         default_ctor(void* dst) const override final;
    Expected<EGenericError>         dtor(void* dst) const override final;
    Expected<EGenericError>         copy(void* dst, const void* src) const override final;
    Expected<EGenericError>         move(void* dst, void* src) const override final;
    Expected<EGenericError>         assign(void* dst, const void* src) const override final;
    Expected<EGenericError>         move_assign(void* dst, void* src) const override final;
    Expected<EGenericError, bool>   equal(const void* lhs, const void* rhs) const override final;
    Expected<EGenericError, size_t> hash(const void* src) const override final;
    //===> IGenericBase API

    // getter
    bool is_valid() const;

    // has value
    bool  has_value(const void* memory) const;
    bool& has_value(void* memory) const;

    // value pointer
    void*       value_ptr(void* memory) const;
    const void* value_ptr(const void* memory) const;

    // reset
    void reset(void* memory) const;

    // assign
    void assign_value(void* dst, const void* v) const;
    void assign_value_move(void* dst, void* v) const;
    void assign_default(void* dst);

private:
    RC<IGenericBase> _inner            = nullptr;
    MemoryTraitsData _inner_mem_traits = {};
};
} // namespace skr