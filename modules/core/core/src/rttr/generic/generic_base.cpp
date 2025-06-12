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
    case EGenericFeature::Swap:
        return _type->find_swap();
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
bool GenericType::equal(const void* lhs, const void* rhs, uint64_t count) const
{
    SKR_ASSERT(lhs);
    SKR_ASSERT(rhs);
    SKR_ASSERT(count > 0);

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

            auto item_size = _type->size();
            for (uint64_t i = 0; i < count; ++i)
            {
                if (!equal_op.invoke(
                        ::skr::memory::offset_item(lhs, item_size, i),
                        ::skr::memory::offset_item(rhs, item_size, i)
                    ))
                {
                    return false;
                }
            }
            return true;
        }
        else
        {
            // use memcmp
            return std::memcmp(lhs, rhs, _type->size() * count) == 0;
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
void GenericType::swap(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(src);

    if (is_decayed_pointer())
    {
        for (uint64_t i = 0; i < count; ++i)
        {
            auto  dst_ptr = reinterpret_cast<void**>(dst) + i;
            auto  src_ptr = reinterpret_cast<void**>(src) + i;
            void* tmp     = *dst_ptr;
            *dst_ptr      = *src_ptr;
            *src_ptr      = tmp;
        }
    }
    else
    {
        auto swap_op = _type->find_swap();
        SKR_ASSERT(swap_op && "please check feature before call this function");
        for (uint64_t i = 0; i < count; ++i)
        {
            swap_op.invoke(
                ::skr::memory::offset_item(dst, _type->size(), i),
                ::skr::memory::offset_item(src, _type->size(), i)
            );
        }
    }
}
//===> IGenericBase API

// generic registry
static auto& _generic_processor_map()
{
    static Map<GUID, GenericProcessor> _generic_processor_map;
    return _generic_processor_map;
};
void register_generic_processor(GUID generic_id, GenericProcessor processor)
{
    SKR_ASSERT(!generic_id.is_zero());
    SKR_ASSERT(processor);

    if (auto ref = _generic_processor_map().find(generic_id))
    {
        SKR_LOG_FMT_ERROR(u8"generic processor for '{}' already registered", generic_id);
    }
    else
    {
        _generic_processor_map().add(generic_id, processor, ref);
    }
}
bool dry_build_generic(TypeSignatureView signature)
{
    // check all type id & generic type id
    while (!signature.is_empty())
    {
        // check decayed pointer level
        if (signature.decayed_pointer_level() > 1)
        {
            return false;
        }

        // jump modifiers
        signature.jump_modifier();

        // check type_id / generic_id
        auto signal = signature.peek_signal();
        if (signal == ETypeSignatureSignal::TypeId)
        {
            GUID type_id;
            signature.read_type_id(type_id);
            RTTRType* type = get_type_from_guid(type_id);
            if (!type)
            {
                return false;
            }
        }
        else if (signal == ETypeSignatureSignal::GenericTypeId)
        {
            GUID     generic_id;
            uint32_t data_count;
            signature.read_generic_type_id(generic_id, data_count);
            if (!_generic_processor_map().contains(generic_id))
            {
                return false;
            }
        }

        // jump data
        signature.jump_next_data();
    }
    return true;
}
RC<IGenericBase> build_generic(TypeSignatureView signature)
{
    // process signature
    auto jumped_modifiers = signature.jump_modifier();
    if (!jumped_modifiers.is_empty())
    {
        SKR_LOG_FMT_WARN(u8"generic signature has modifiers, modifiers will be ignored, call `sig.jump_modifier();` to suppress this warning");
    }

    auto signal = jumped_modifiers.peek_signal();
    if (signal == ETypeSignatureSignal::TypeId)
    {
        GUID type_id;
        jumped_modifiers.read_type_id(type_id);
        RTTRType* type = get_type_from_guid(type_id);
        if (type)
        {
            return RC<GenericType>::New(type);
        }
        else
        {
            return nullptr;
        }
    }
    else if (signal == ETypeSignatureSignal::GenericTypeId)
    {
        GUID     generic_id;
        uint32_t data_count;
        jumped_modifiers.read_generic_type_id(generic_id, data_count);
        if (auto found = _generic_processor_map().find(generic_id))
        {
            return found.value()(jumped_modifiers);
        }
        else
        {
            return nullptr;
        }
    }
    else
    {
        SKR_LOG_FMT_ERROR(u8"invalid generic signature, expected TypeId or GenericTypeId");
        return nullptr;
    }
}
} // namespace skr