#pragma once
#include <SkrRTTR/type.hpp>
#include <SkrCore/log.hpp>
#include <SkrContainers/optional.hpp>
#include <SkrContainers/vector.hpp>
#include <SkrCore/memory/rc.hpp>
#include <SkrContainers/not_null.hpp>

namespace skr
{
// generic interface
enum class EGenericKind
{
    Generic,
    Type,
};
enum class EGenericError
{
    Unknown,
    InvalidType,   // generic type is invalid
    NoSuchFeature, // no such feature
    ShouldNotCall, // should not call this function
};
struct IGenericBase {
    SKR_RC_INTEFACE()
    virtual ~IGenericBase() = default;

    // get type info
    virtual EGenericKind kind() const = 0;
    virtual GUID         id() const   = 0; // type or generic id, depends on kind

    // get utils
    virtual MemoryTraitsData memory_traits_data() const = 0;

    // basic info
    virtual uint64_t size() const      = 0;
    virtual uint64_t alignment() const = 0;

    // operations, used for generic container algorithms
    virtual Expected<EGenericError>         default_ctor(void* dst) const                 = 0;
    virtual Expected<EGenericError>         dtor(void* dst) const                         = 0;
    virtual Expected<EGenericError>         copy(void* dst, const void* src) const        = 0;
    virtual Expected<EGenericError>         move(void* dst, void* src) const              = 0;
    virtual Expected<EGenericError>         assign(void* dst, const void* src) const      = 0;
    virtual Expected<EGenericError>         move_assign(void* dst, void* src) const       = 0;
    virtual Expected<EGenericError, bool>   equal(const void* lhs, const void* rhs) const = 0;
    virtual Expected<EGenericError, size_t> hash(const void* src)                         = 0;
};

struct GenericRecord final : IGenericBase {
    SKR_RC_IMPL(override final)

    // ctor & dtor
    inline GenericRecord(not_null<RTTRType*> type)
        : _type(type)
    {
    }

    //===> IGenericBase API
    // get type info
    EGenericKind kind() const override final
    {
        return EGenericKind::Type;
    }
    GUID id() const override final
    {
        return _type->type_id();
    }

    // get utils
    MemoryTraitsData memory_traits_data() const override final
    {
        return _type->memory_traits_data();
    }

    // basic info
    uint64_t size() const override final
    {
        return _type->size();
    }
    uint64_t alignment() const override final
    {
        return _type->alignment();
    }

    // operations, used for generic container algorithms
    Expected<EGenericError> default_ctor(void* dst) const override final
    {
        SKR_ASSERT(dst);
        if (!_type->memory_traits_data().use_ctor) { return EGenericError::ShouldNotCall; }
        auto default_ctor = _type->find_default_ctor();
        if (!default_ctor) { return EGenericError::NoSuchFeature; }
        default_ctor.invoke(dst);
        return {};
    }
    Expected<EGenericError> dtor(void* dst) const override final
    {
        SKR_ASSERT(dst);
        if (!_type->memory_traits_data().use_dtor) { return EGenericError::ShouldNotCall; }
        auto dtor = _type->dtor_invoker();
        if (!dtor) { return EGenericError::NoSuchFeature; }
        dtor(dst);
        return {};
    }
    Expected<EGenericError> copy(void* dst, const void* src) const override final
    {
        SKR_ASSERT(dst);
        SKR_ASSERT(src);
        if (!_type->memory_traits_data().use_copy) { return EGenericError::ShouldNotCall; }
        auto copy = _type->find_copy_ctor();
        if (!copy) { return EGenericError::NoSuchFeature; }
        copy.invoke(dst, src);
        return {};
    }
    Expected<EGenericError> move(void* dst, void* src) const override final
    {
        SKR_ASSERT(dst);
        SKR_ASSERT(src);
        if (!_type->memory_traits_data().use_move) { return EGenericError::ShouldNotCall; }
        auto move = _type->find_move_ctor();
        if (!move) { return EGenericError::NoSuchFeature; }
        move.invoke(dst, src);
        return {};
    }
    Expected<EGenericError> assign(void* dst, const void* src) const override final
    {
        SKR_ASSERT(dst);
        SKR_ASSERT(src);
        if (!_type->memory_traits_data().use_assign) { return EGenericError::ShouldNotCall; }
        auto assign = _type->find_assign();
        if (!assign) { return EGenericError::NoSuchFeature; }
        assign.invoke(dst, src);
        return {};
    }
    Expected<EGenericError> move_assign(void* dst, void* src) const override final
    {
        SKR_ASSERT(dst);
        SKR_ASSERT(src);
        if (!_type->memory_traits_data().use_move_assign) { return EGenericError::ShouldNotCall; }
        auto assign_move = _type->find_move_assign();
        if (!assign_move) { return EGenericError::NoSuchFeature; }
        assign_move.invoke(dst, src);
        return {};
    }
    Expected<EGenericError, bool> equal(const void* lhs, const void* rhs) const override final
    {
        SKR_ASSERT(lhs);
        SKR_ASSERT(rhs);
        if (!_type->memory_traits_data().use_compare) { return EGenericError::ShouldNotCall; }
        auto equal = _type->find_equal();
        if (!equal) { return EGenericError::NoSuchFeature; }
        return equal.invoke(lhs, rhs);
    }
    Expected<EGenericError, size_t> hash(const void* src) override final
    {
        SKR_ASSERT(src);
        auto hash = _type->find_hash();
        if (!hash) { return EGenericError::NoSuchFeature; }
        return hash.invoke(src);
    }
    //===> IGenericBase API
private:
    RTTRType* _type = nullptr;
};

// TODO. Generic Registry
//   register generic handlers
//   make generic from type signature
} // namespace skr