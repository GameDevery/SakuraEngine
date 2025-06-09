#include <SkrRTTR/generic/generic_base.hpp>
#include <SkrRTTR/rttr_traits.hpp>

namespace skr
{
IGenericBase::~IGenericBase() {}
} // namespace skr

namespace skr
{
// ctor & dtor
GenericType::GenericType(not_null<RTTRType*> type, EGenericTypeFlag flag)
    : _type(type)
    , _flag(flag)
{
}

// getter
EGenericTypeFlag GenericType::flag() const
{
    return _flag;
}
bool GenericType::is_const() const
{
    return flag_all(_flag, EGenericTypeFlag::Const);
}
bool GenericType::is_pointer() const
{
    return flag_all(_flag, EGenericTypeFlag::Pointer);
}
bool GenericType::is_ref() const
{
    return flag_all(_flag, EGenericTypeFlag::Ref);
}
bool GenericType::is_rvalue_ref() const
{
    return flag_all(_flag, EGenericTypeFlag::RValueRef);
}
bool GenericType::is_decayed_pointer() const
{
    return flag_any(
        _flag,
        EGenericTypeFlag::Pointer |
            EGenericTypeFlag::Ref |
            EGenericTypeFlag::RValueRef
    );
}

//===> IGenericBase API
// get type info
EGenericKind GenericType::kind() const
{
    return EGenericKind::Type;
}
GUID GenericType::id() const
{
    return _type->type_id();
}

// get utils
MemoryTraitsData GenericType::memory_traits_data() const
{
    return is_decayed_pointer() ? MemoryTraitsData::Make<void*>() : _type->memory_traits_data();
}

// basic info
uint64_t GenericType::size() const
{
    return is_decayed_pointer() ? sizeof(void*) : _type->size();
}
uint64_t GenericType::alignment() const
{
    return is_decayed_pointer() ? alignof(void*) : _type->alignment();
}

// operations, used for generic container algorithms
Expected<EGenericError> GenericType::default_ctor(void* dst) const
{
    if (is_decayed_pointer())
    {
        return EGenericError::ShouldNotCall;
    }
    else
    {
        SKR_ASSERT(dst);
        if (!_type->memory_traits_data().use_ctor) { return EGenericError::ShouldNotCall; }
        auto default_ctor = _type->find_default_ctor();
        if (!default_ctor) { return EGenericError::NoSuchFeature; }
        default_ctor.invoke(dst);
        return {};
    }
}
Expected<EGenericError> GenericType::dtor(void* dst) const
{
    if (is_decayed_pointer())
    {
        return EGenericError::ShouldNotCall;
    }
    else
    {
        SKR_ASSERT(dst);
        if (!_type->memory_traits_data().use_dtor) { return EGenericError::ShouldNotCall; }
        auto dtor = _type->dtor_invoker();
        if (!dtor) { return EGenericError::NoSuchFeature; }
        dtor(dst);
        return {};
    }
}
Expected<EGenericError> GenericType::copy(void* dst, const void* src) const
{
    if (is_decayed_pointer())
    {
        return EGenericError::ShouldNotCall;
    }
    else
    {
        SKR_ASSERT(dst);
        SKR_ASSERT(src);
        if (!_type->memory_traits_data().use_copy) { return EGenericError::ShouldNotCall; }
        auto copy = _type->find_copy_ctor();
        if (!copy) { return EGenericError::NoSuchFeature; }
        copy.invoke(dst, src);
        return {};
    }
}
Expected<EGenericError> GenericType::move(void* dst, void* src) const
{
    if (is_decayed_pointer())
    {
        return EGenericError::ShouldNotCall;
    }
    else
    {
        SKR_ASSERT(dst);
        SKR_ASSERT(src);
        if (!_type->memory_traits_data().use_move) { return EGenericError::ShouldNotCall; }
        auto move = _type->find_move_ctor();
        if (!move) { return EGenericError::NoSuchFeature; }
        move.invoke(dst, src);
        return {};
    }
}
Expected<EGenericError> GenericType::assign(void* dst, const void* src) const
{
    if (is_decayed_pointer())
    {
        return EGenericError::ShouldNotCall;
    }
    else
    {
        SKR_ASSERT(dst);
        SKR_ASSERT(src);
        if (!_type->memory_traits_data().use_assign) { return EGenericError::ShouldNotCall; }
        auto assign = _type->find_assign();
        if (!assign) { return EGenericError::NoSuchFeature; }
        assign.invoke(dst, src);
        return {};
    }
}
Expected<EGenericError> GenericType::move_assign(void* dst, void* src) const
{
    if (is_decayed_pointer())
    {
        return EGenericError::ShouldNotCall;
    }
    else
    {
        SKR_ASSERT(dst);
        SKR_ASSERT(src);
        if (!_type->memory_traits_data().use_move_assign) { return EGenericError::ShouldNotCall; }
        auto assign_move = _type->find_move_assign();
        if (!assign_move) { return EGenericError::NoSuchFeature; }
        assign_move.invoke(dst, src);
        return {};
    }
}
Expected<EGenericError, bool> GenericType::equal(const void* lhs, const void* rhs) const
{
    if (is_decayed_pointer())
    {
        return EGenericError::ShouldNotCall;
    }
    else
    {
        SKR_ASSERT(lhs);
        SKR_ASSERT(rhs);
        if (!_type->memory_traits_data().use_compare) { return EGenericError::ShouldNotCall; }
        auto equal = _type->find_equal();
        if (!equal) { return EGenericError::NoSuchFeature; }
        return equal.invoke(lhs, rhs);
    }
}
Expected<EGenericError, size_t> GenericType::hash(const void* src) const
{
    if (is_decayed_pointer())
    {
        if (
            _type->type_id() == type_id_of<char>() ||
            _type->type_id() == type_id_of<wchar_t>() ||
            _type->type_id() == type_id_of<char8_t>() ||
            _type->type_id() == type_id_of<char16_t>() ||
            _type->type_id() == type_id_of<char32_t>()
        )
        {
            debug_break();
            SKR_LOG_FMT_ERROR(u8"call hash on pointer of cstring '{}' is undefined, please use String/StringView instead", _type->name());
        }
        return Hash<void*>()(*reinterpret_cast<void* const*>(src));
    }
    else
    {
        SKR_ASSERT(src);
        auto hash = _type->find_hash();
        if (!hash) { return EGenericError::NoSuchFeature; }
        return hash.invoke(src);
    }
}
//===> IGenericBase API
} // namespace skr