#include <SkrRTTR/generic/generic_sparse_hash_map.hpp>

namespace skr
{
// ctor & dtor
GenericKVPair::GenericKVPair(RC<IGenericBase> inner_key, RC<IGenericBase> inner_value)
    : _inner_key(std::move(inner_key))
    , _inner_value(std::move(inner_value))
{
    if (_inner_key)
    {
        _inner_key_mem_traits = _inner_key->memory_traits_data();
        _inner_key_size       = _inner_key->size();
        _inner_key_alignment  = _inner_key->alignment();
    }

    if (_inner_value)
    {
        _inner_value_mem_traits = _inner_value->memory_traits_data();
        _inner_value_size       = _inner_value->size();
        _inner_value_alignment  = _inner_value->alignment();
    }
}
GenericKVPair::~GenericKVPair() = default;

// get type info
EGenericKind GenericKVPair::kind() const
{
    return EGenericKind::Generic;
}
GUID GenericKVPair::id() const
{
    return kKVPairGenericId;
}

// get utils
MemoryTraitsData GenericKVPair::memory_traits_data() const
{
    SKR_ASSERT(is_valid());

    MemoryTraitsData data;
    data.use_ctor             = _inner_key_mem_traits.use_ctor || _inner_value_mem_traits.use_ctor;
    data.use_dtor             = _inner_key_mem_traits.use_dtor || _inner_value_mem_traits.use_dtor;
    data.use_move             = _inner_key_mem_traits.use_move || _inner_value_mem_traits.use_move;
    data.use_assign           = _inner_key_mem_traits.use_assign || _inner_value_mem_traits.use_assign;
    data.use_move_assign      = _inner_key_mem_traits.use_move_assign || _inner_value_mem_traits.use_move_assign;
    data.need_dtor_after_move = _inner_key_mem_traits.need_dtor_after_move || _inner_value_mem_traits.need_dtor_after_move;
    data.use_realloc          = _inner_key_mem_traits.use_realloc && _inner_value_mem_traits.use_realloc;
    data.use_compare          = _inner_key_mem_traits.use_compare || _inner_value_mem_traits.use_compare;
    return data;
}

// basic info
uint64_t GenericKVPair::size() const
{
    auto alignment = std::max(_inner_key_alignment, _inner_value_alignment);
    return (int_div_ceil(_inner_key_size, alignment) + int_div_ceil(_inner_value_size, alignment)) * alignment;
}
uint64_t GenericKVPair::alignment() const
{
    return std::max(_inner_key_alignment, _inner_value_alignment);
}

// operations, used for generic container algorithms
bool GenericKVPair::support(EGenericFeature feature) const
{
    switch (feature)
    {
    case EGenericFeature::DefaultCtor:
    case EGenericFeature::Dtor:
    case EGenericFeature::Copy:
    case EGenericFeature::Move:
    case EGenericFeature::Assign:
    case EGenericFeature::MoveAssign:
    case EGenericFeature::Equal:
    case EGenericFeature::Swap:
        return _inner_key->support(feature) && _inner_value->support(feature);
    case EGenericFeature::Hash:
        return _inner_key->support(feature);
    default:
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
void GenericKVPair::default_ctor(void* dst, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(count > 0);
    auto size = this->size();

    for (uint64_t i = 0; i < count; ++i)
    {
        auto p_dst = ::skr::memory::offset_item(dst, size, i);
        _inner_key->default_ctor(key(p_dst));
        _inner_value->default_ctor(value(p_dst));
    }
}
void GenericKVPair::dtor(void* dst, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(count > 0);
    auto size = this->size();

    for (uint64_t i = 0; i < count; ++i)
    {
        auto p_dst = ::skr::memory::offset_item(dst, size, i);
        _inner_key->dtor(key(p_dst));
        _inner_value->dtor(value(p_dst));
    }
}
void GenericKVPair::copy(void* dst, const void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);
    auto size = this->size();

    if (_inner_key_mem_traits.use_copy && _inner_value_mem_traits.use_copy)
    {
        // each items
        for (uint64_t i = 0; i < count; ++i)
        {
            auto p_dst = ::skr::memory::offset_item(dst, size, i);
            auto p_src = ::skr::memory::offset_item(src, size, i);
            _inner_key->copy(key(p_dst), key(p_src));
            _inner_value->copy(value(p_dst), value(p_src));
        }
    }
    else
    {
        ::std::memcpy(dst, src, size * count);
    }
}
void GenericKVPair::move(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);
    auto size = this->size();

    if (_inner_key_mem_traits.use_move && _inner_value_mem_traits.use_move)
    {
        // each items
        for (uint64_t i = 0; i < count; ++i)
        {
            auto p_dst = ::skr::memory::offset_item(dst, size, i);
            auto p_src = ::skr::memory::offset_item(src, size, i);
            _inner_key->move(key(p_dst), key(p_src));
            _inner_value->move(value(p_dst), value(p_src));
        }
    }
    else
    {
        ::std::memcpy(dst, src, size * count);
    }
}
void GenericKVPair::assign(void* dst, const void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);
    auto size = this->size();

    if (_inner_key_mem_traits.use_assign && _inner_value_mem_traits.use_assign)
    {
        // each items
        for (uint64_t i = 0; i < count; ++i)
        {
            auto p_dst = ::skr::memory::offset_item(dst, size, i);
            auto p_src = ::skr::memory::offset_item(src, size, i);
            _inner_key->assign(key(p_dst), key(p_src));
            _inner_value->assign(value(p_dst), value(p_src));
        }
    }
    else
    {
        ::std::memcpy(dst, src, size * count);
    }
}
void GenericKVPair::move_assign(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);
    auto size = this->size();

    if (_inner_key_mem_traits.use_move_assign && _inner_value_mem_traits.use_move_assign)
    {
        // each items
        for (uint64_t i = 0; i < count; ++i)
        {
            auto p_dst = ::skr::memory::offset_item(dst, size, i);
            auto p_src = ::skr::memory::offset_item(src, size, i);
            _inner_key->move_assign(key(p_dst), key(p_src));
            _inner_value->move_assign(value(p_dst), value(p_src));
        }
    }
    else
    {
        ::std::memcpy(dst, src, size * count);
        ::skr::memory::zero_memory(src, size * count); // reset src
    }
}
bool GenericKVPair::equal(const void* lhs, const void* rhs, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(lhs);
    SKR_ASSERT(rhs);
    SKR_ASSERT(count > 0);

    if (_inner_key_mem_traits.use_compare && _inner_value_mem_traits.use_compare)
    {
        // each items
        for (uint64_t i = 0; i < count; ++i)
        {
            auto p_lhs = ::skr::memory::offset_item(lhs, size(), i);
            auto p_rhs = ::skr::memory::offset_item(rhs, size(), i);
            if (!_inner_key->equal(key(p_lhs), key(p_rhs)) ||
                !_inner_value->equal(value(p_lhs), value(p_rhs)))
            {
                return false;
            }
        }
        return true;
    }
    else
    {
        SKR_ASSERT(false && "equal not support for GenericKVPair, please check feature before call this function");
        return false;
    }
}
size_t GenericKVPair::hash(const void* src) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(src);
    SKR_ASSERT(_inner_key->support(EGenericFeature::Hash));

    // hash key
    return _inner_key->hash(key(src));
}
void GenericKVPair::swap(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);
    auto size = this->size();

    for (uint64_t i = 0; i < count; ++i)
    {
        auto p_dst = ::skr::memory::offset_item(dst, size, i);
        auto p_src = ::skr::memory::offset_item(src, size, i);
        _inner_key->swap(key(p_dst), key(p_src));
        _inner_value->swap(value(p_dst), value(p_src));
    }
}

// getter
bool GenericKVPair::is_valid() const
{
    return _inner_key && _inner_value;
}
RC<IGenericBase> GenericKVPair::inner_key() const
{
    SKR_ASSERT(is_valid());
    return _inner_key;
}
RC<GenericSparseHashSetStorage> GenericKVPair::inner_value() const
{
    SKR_ASSERT(is_valid());
    return _inner_value;
}

// get key & value
void* GenericKVPair::key(void* dst) const
{
    return dst;
}
void* GenericKVPair::value(void* dst) const
{
    auto alignment       = std::max(_inner_key_alignment, _inner_value_alignment);
    auto padded_key_size = int_div_ceil(_inner_key_size, alignment) * alignment;
    return ::skr::memory::offset_bytes(dst, padded_key_size);
}
const void* GenericKVPair::key(const void* dst) const
{
    return key(const_cast<void*>(dst));
}
const void* GenericKVPair::value(const void* dst) const
{
    return value(const_cast<void*>(dst));
}
} // namespace skr