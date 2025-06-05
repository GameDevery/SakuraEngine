#include <SkrRTTR/generic/generic_base.hpp>

namespace skr
{
IGenericBase::~IGenericBase() {}
} // namespace skr

namespace skr
{
// ctor & dtor
GenericRecord::GenericRecord(not_null<RTTRType*> type)
    : _type(type)
{
}

//===> IGenericBase API
// get type info
EGenericKind GenericRecord::kind() const
{
    return EGenericKind::Type;
}
GUID GenericRecord::id() const
{
    return _type->type_id();
}

// get utils
MemoryTraitsData GenericRecord::memory_traits_data() const
{
    return _type->memory_traits_data();
}

// basic info
uint64_t GenericRecord::size() const
{
    return _type->size();
}
uint64_t GenericRecord::alignment() const
{
    return _type->alignment();
}

// operations, used for generic container algorithms
Expected<EGenericError> GenericRecord::default_ctor(void* dst) const
{
    SKR_ASSERT(dst);
    if (!_type->memory_traits_data().use_ctor) { return EGenericError::ShouldNotCall; }
    auto default_ctor = _type->find_default_ctor();
    if (!default_ctor) { return EGenericError::NoSuchFeature; }
    default_ctor.invoke(dst);
    return {};
}
Expected<EGenericError> GenericRecord::dtor(void* dst) const
{
    SKR_ASSERT(dst);
    if (!_type->memory_traits_data().use_dtor) { return EGenericError::ShouldNotCall; }
    auto dtor = _type->dtor_invoker();
    if (!dtor) { return EGenericError::NoSuchFeature; }
    dtor(dst);
    return {};
}
Expected<EGenericError> GenericRecord::copy(void* dst, const void* src) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    if (!_type->memory_traits_data().use_copy) { return EGenericError::ShouldNotCall; }
    auto copy = _type->find_copy_ctor();
    if (!copy) { return EGenericError::NoSuchFeature; }
    copy.invoke(dst, src);
    return {};
}
Expected<EGenericError> GenericRecord::move(void* dst, void* src) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    if (!_type->memory_traits_data().use_move) { return EGenericError::ShouldNotCall; }
    auto move = _type->find_move_ctor();
    if (!move) { return EGenericError::NoSuchFeature; }
    move.invoke(dst, src);
    return {};
}
Expected<EGenericError> GenericRecord::assign(void* dst, const void* src) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    if (!_type->memory_traits_data().use_assign) { return EGenericError::ShouldNotCall; }
    auto assign = _type->find_assign();
    if (!assign) { return EGenericError::NoSuchFeature; }
    assign.invoke(dst, src);
    return {};
}
Expected<EGenericError> GenericRecord::move_assign(void* dst, void* src) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    if (!_type->memory_traits_data().use_move_assign) { return EGenericError::ShouldNotCall; }
    auto assign_move = _type->find_move_assign();
    if (!assign_move) { return EGenericError::NoSuchFeature; }
    assign_move.invoke(dst, src);
    return {};
}
Expected<EGenericError, bool> GenericRecord::equal(const void* lhs, const void* rhs) const
{
    SKR_ASSERT(lhs);
    SKR_ASSERT(rhs);
    if (!_type->memory_traits_data().use_compare) { return EGenericError::ShouldNotCall; }
    auto equal = _type->find_equal();
    if (!equal) { return EGenericError::NoSuchFeature; }
    return equal.invoke(lhs, rhs);
}
Expected<EGenericError, size_t> GenericRecord::hash(const void* src) const
{
    SKR_ASSERT(src);
    auto hash = _type->find_hash();
    if (!hash) { return EGenericError::NoSuchFeature; }
    return hash.invoke(src);
}
//===> IGenericBase API
} // namespace skr