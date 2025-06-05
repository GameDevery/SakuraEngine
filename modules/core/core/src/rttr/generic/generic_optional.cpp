#include <SkrRTTR/generic/generic_optional.hpp>

namespace skr
{
// ctor & dtor
GenericOptional::GenericOptional(RC<IGenericBase> inner)
    : _inner(std::move(inner))
{
    if (_inner)
    {
        _inner_mem_traits = _inner->memory_traits_data();
    }
}
GenericOptional::~GenericOptional() = default;

//===> IGenericBase API
// get type info
EGenericKind GenericOptional::kind() const
{
    return EGenericKind::Generic;
}
GUID GenericOptional::id() const
{
    return kOptionalGenericId;
}

// get utils
MemoryTraitsData GenericOptional::memory_traits_data() const
{
    SKR_ASSERT(is_valid());
    MemoryTraitsData data = _inner_mem_traits;
    data.use_ctor         = true; // optional need default ctor
    data.use_compare      = true; // optional will compare has_value
    return data;
}

// basic info
uint64_t GenericOptional::size() const
{
    SKR_ASSERT(is_valid());
    auto inner_size      = _inner->size();
    auto inner_alignment = _inner->alignment();
    return inner_size + std::max(sizeof(bool), inner_alignment);
}
uint64_t GenericOptional::alignment() const
{
    SKR_ASSERT(is_valid());
    return std::max(sizeof(bool), _inner->alignment());
}

// operations, used for generic container algorithms
Expected<EGenericError> GenericOptional::default_ctor(void* dst) const
{
    SKR_ASSERT(is_valid());
    has_value(dst) = false;
    return {};
}
Expected<EGenericError> GenericOptional::dtor(void* dst) const
{
    SKR_ASSERT(is_valid());
    if (!_inner_mem_traits.use_dtor) return EGenericError::ShouldNotCall;
    reset(dst);
    return {};
}
Expected<EGenericError> GenericOptional::copy(void* dst, const void* src) const
{
    SKR_ASSERT(is_valid());
    if (!_inner_mem_traits.use_copy) return EGenericError::ShouldNotCall;
    if (has_value(src))
    {
        has_value(dst) = true;
        _inner->copy(value_ptr(dst), value_ptr(src));
    }
    else
    {
        has_value(dst) = false;
    }
    return {};
}
Expected<EGenericError> GenericOptional::move(void* dst, void* src) const
{
    SKR_ASSERT(is_valid());
    if (!_inner_mem_traits.use_move) return EGenericError::ShouldNotCall;
    if (has_value(src))
    {
        has_value(dst) = true;
        _inner->move(value_ptr(dst), value_ptr(src));
    }
    else
    {
        has_value(dst) = false;
    }
    return {};
}
Expected<EGenericError> GenericOptional::assign(void* dst, const void* src) const
{
    SKR_ASSERT(is_valid());
    if (!_inner_mem_traits.use_assign) return EGenericError::ShouldNotCall;
    if (has_value(src))
    {
        assign_value(dst, value_ptr(src));
    }
    else
    {
        reset(dst);
    }
    return {};
}
Expected<EGenericError> GenericOptional::move_assign(void* dst, void* src) const
{
    SKR_ASSERT(is_valid());
    if (!_inner_mem_traits.use_move_assign) return EGenericError::ShouldNotCall;
    if (has_value(src))
    {
        assign_value_move(dst, value_ptr(src));
        if (_inner_mem_traits.need_dtor_after_move)
        {
            reset(src);
        }
        else
        {
            has_value(src) = false; // reset has_value only
        }
    }
    else
    {
        reset(dst);
    }
    return {};
}
Expected<EGenericError, bool> GenericOptional::equal(const void* lhs, const void* rhs) const
{
    SKR_ASSERT(is_valid());
    if (!_inner_mem_traits.use_compare) return EGenericError::ShouldNotCall;
    if (has_value(lhs) && has_value(rhs))
    {
        return _inner->equal(value_ptr(lhs), value_ptr(rhs));
    }
    else
    {
        return has_value(lhs) == has_value(rhs);
    }
}
Expected<EGenericError, size_t> GenericOptional::hash(const void* src) const
{
    SKR_ASSERT(is_valid());
    return EGenericError::NoSuchFeature;
}
//===> IGenericBase API

// getter
bool GenericOptional::is_valid() const { return _inner != nullptr; }

// has value
bool GenericOptional::has_value(const void* memory) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(memory);
    return *reinterpret_cast<const bool*>(memory);
}
bool& GenericOptional::has_value(void* memory) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(memory);
    return *reinterpret_cast<bool*>(memory);
}

// value pointer
void* GenericOptional::value_ptr(void* memory) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(memory);
    return reinterpret_cast<uint8_t*>(memory) + sizeof(bool);
}
const void* GenericOptional::value_ptr(const void* memory) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(memory);
    return reinterpret_cast<const uint8_t*>(memory) + sizeof(bool);
}

// reset
void GenericOptional::reset(void* memory) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(memory);
    if (has_value(memory))
    {
        if (_inner_mem_traits.use_dtor)
        {
            auto value_ptr = this->value_ptr(memory);
            _inner->dtor(value_ptr);
        }
    }
}

// assign
void GenericOptional::assign_value(void* dst, const void* v) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(v);

    if (has_value(dst))
    {
        if (_inner_mem_traits.use_assign)
        {
            _inner->assign(value_ptr(dst), v);
        }
        else
        {
            ::std::memcpy(value_ptr(dst), v, _inner->size());
        }
    }
    else
    {
        has_value(dst) = true;
        if (_inner_mem_traits.use_ctor)
        {
            _inner->copy(value_ptr(dst), v);
        }
        else
        {
            ::std::memcpy(value_ptr(dst), v, _inner->size());
        }
    }
}
void GenericOptional::assign_value_move(void* dst, void* v) const
{
    SKR_ASSERT(dst);
    SKR_ASSERT(v);

    if (has_value(dst))
    {
        if (_inner_mem_traits.use_move_assign)
        {
            _inner->move_assign(value_ptr(dst), v);
        }
        else
        {
            ::std::memmove(value_ptr(dst), v, _inner->size());
        }
    }
    else
    {
        has_value(dst) = true;
        if (_inner_mem_traits.use_move)
        {
            _inner->move(value_ptr(dst), v);
        }
        else
        {
            ::std::memmove(value_ptr(dst), v, _inner->size());
        }
    }
}
void GenericOptional::assign_default(void* dst)
{
    SKR_ASSERT(dst);
    reset(dst);
    has_value(dst) = false;
    if (_inner_mem_traits.use_ctor)
    {
        _inner->default_ctor(value_ptr(dst));
    }
    else
    {
        ::std::memset(value_ptr(dst), 0, _inner->size());
    }
}
} // namespace skr