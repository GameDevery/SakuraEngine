#include <SkrRTTR/generic/generic_base.hpp>
#include <SkrRTTR/rttr_traits.hpp>

namespace skr
{
IGenericBase::~IGenericBase() {}
} // namespace skr

// generic type
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
bool GenericType::support(EGenericFeature feature) const
{
    switch (feature)
    {
    case EGenericFeature::DefaultCtor:
        return !_type->memory_traits_data().use_ctor || _type->find_default_ctor();
    case EGenericFeature::Dtor:
        return !_type->memory_traits_data().use_dtor || _type->dtor_invoker();
    case EGenericFeature::Copy:
        return !_type->memory_traits_data().use_copy || _type->find_copy_ctor();
    case EGenericFeature::Move:
        return !_type->memory_traits_data().use_move || _type->find_move_ctor();
    case EGenericFeature::Assign:
        return !_type->memory_traits_data().use_assign || _type->find_assign();
    case EGenericFeature::MoveAssign:
        return !_type->memory_traits_data().use_move_assign || _type->find_move_assign();
    case EGenericFeature::Equal:
        return !_type->memory_traits_data().use_compare || _type->find_equal();
    case EGenericFeature::Hash:
        return _type->find_hash();
    default:
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
void GenericType::default_ctor(void* dst, uint64_t count) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(count > 0);

    if (is_decayed_pointer())
    {
        // do noting
    }
    else
    {
        if (_type->memory_traits_data().use_ctor)
        {
            auto default_ctor = _type->find_default_ctor();
            SKR_ASSERT(default_ctor && "please check feature before call this function");
            for (uint64_t i = 0; i < count; ++i)
            {
                default_ctor.invoke(dst);
            }
        }
        else
        {
            // set to zero
            ::skr::memory::zero_memory(dst, _type->size() * count);
        }
    }
}
void GenericType::dtor(void* dst, uint64_t count) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(count > 0);

    if (is_decayed_pointer())
    {
        // do nothing
    }
    else
    {
        if (_type->memory_traits_data().use_dtor)
        {
            auto dtor = _type->dtor_invoker();
            SKR_ASSERT(dtor && "please check feature before call this function");
            for (uint64_t i = 0; i < count; ++i)
            {
                dtor(dst);
            }
        }
        else
        {
            // do noting
        }
    }
}
void GenericType::copy(void* dst, const void* src, uint64_t count) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    if (is_decayed_pointer())
    {
        // do noting
    }
    else
    {
        if (_type->memory_traits_data().use_copy)
        {
            auto copy_ctor = _type->find_copy_ctor();
            SKR_ASSERT(copy_ctor && "please check feature before call this function");
            for (uint64_t i = 0; i < count; ++i)
            {
                copy_ctor.invoke(dst, src);
            }
        }
        else
        {
            // use memcpy
            std::memcpy(dst, src, _type->size() * count);
        }
    }
}
void GenericType::move(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    if (is_decayed_pointer())
    {
        // do noting
    }
    else
    {
        if (_type->memory_traits_data().use_move)
        {
            // do move
            auto move_ctor = _type->find_move_ctor();
            SKR_ASSERT(move_ctor && "please check feature before call this function");
            for (uint64_t i = 0; i < count; ++i)
            {
                move_ctor.invoke(dst, src);
            }

            // call dtor after move if needed
            if (_type->memory_traits_data().need_dtor_after_move)
            {
                auto dtor = _type->dtor_invoker();
                SKR_ASSERT(dtor && "please check feature before call this function");
                for (uint64_t i = 0; i < count; ++i)
                {
                    dtor(src);
                }
            }
        }
        else
        {
            // use memmove
            std::memmove(dst, src, _type->size() * count);
        }
    }
}
void GenericType::assign(void* dst, const void* src, uint64_t count) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    if (is_decayed_pointer())
    {
        // do noting
    }
    else
    {
        if (_type->memory_traits_data().use_assign)
        {
            auto copy_assign = _type->find_assign();
            SKR_ASSERT(copy_assign && "please check feature before call this function");
            for (uint64_t i = 0; i < count; ++i)
            {
                copy_assign.invoke(dst, src);
            }
        }
        else
        {
            // use memcpy
            std::memcpy(dst, src, _type->size() * count);
        }
    }
}
void GenericType::move_assign(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    if (is_decayed_pointer())
    {
        // do noting
    }
    else
    {
        if (_type->memory_traits_data().use_move_assign)
        {
            auto move_assign = _type->find_move_assign();
            SKR_ASSERT(move_assign && "please check feature before call this function");
            for (uint64_t i = 0; i < count; ++i)
            {
                move_assign.invoke(dst, src);
            }

            // call dtor after move if needed
            if (_type->memory_traits_data().need_dtor_after_move)
            {
                auto dtor = _type->dtor_invoker();
                SKR_ASSERT(dtor && "please check feature before call this function");
                for (uint64_t i = 0; i < count; ++i)
                {
                    dtor(src);
                }
            }
        }
        else
        {
            // use memmove
            std::memmove(dst, src, _type->size() * count);
        }
    }
}
bool GenericType::equal(const void* lhs, const void* rhs) const
{
    SKR_ASSERT(lhs);
    SKR_ASSERT(rhs);

    if (is_decayed_pointer())
    {
        return reinterpret_cast<void* const*>(lhs) == reinterpret_cast<void* const*>(rhs);
    }
    else
    {
        if (_type->memory_traits_data().use_compare)
        {
            auto equal_op = _type->find_equal();
            SKR_ASSERT(equal_op && "please check feature before call this function");

            return equal_op.invoke(lhs, rhs);
        }
        else
        {
            // use memcmp
            return std::memcmp(lhs, rhs, _type->size()) == 0;
        }
    }
}
size_t GenericType::hash(const void* src) const
{
    SKR_ASSERT(src);

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
        auto hash_op = _type->find_hash();
        SKR_ASSERT(hash_op && "please check feature before call this function");
        return hash_op.invoke(src);
    }
}
//===> IGenericBase API
} // namespace skr