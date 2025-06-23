#include <SkrRTTR/generic/generic_sparase_hash_base.hpp>

namespace skr
{
// ctor & dtor
GenericSparseHashSetStorage::GenericSparseHashSetStorage(RC<IGenericBase> inner)
    : _inner(std::move(inner))
{
    if (_inner)
    {
        _inner_mem_traits = _inner->memory_traits_data();
        _inner_size       = _inner->size();
        _inner_alignment  = _inner->alignment();
    }
}
GenericSparseHashSetStorage::~GenericSparseHashSetStorage() = default;

//===> IGenericBase API
// get type info
EGenericKind GenericSparseHashSetStorage::kind() const
{
    return EGenericKind::Generic;
}
GUID GenericSparseHashSetStorage::id() const
{
    return kSparseHashSetStorageGenericId;
}

// get utils
MemoryTraitsData GenericSparseHashSetStorage::memory_traits_data() const
{
    SKR_ASSERT(is_valid());
    return _inner_mem_traits;
}

// basic info
uint64_t GenericSparseHashSetStorage::size() const
{
    SKR_ASSERT(is_valid());
    auto padded_inner_size = std::max(_inner_size, alignof(uint64_t));
    return padded_inner_size + sizeof(uint64_t) * 2; // hash + next
}
uint64_t GenericSparseHashSetStorage::alignment() const
{
    SKR_ASSERT(is_valid());
    return std::max(_inner_alignment, alignof(uint64_t));
}

// operations, used for generic container algorithms
bool GenericSparseHashSetStorage::support(EGenericFeature feature) const
{
    SKR_ASSERT(is_valid());

    switch (feature)
    {
    case EGenericFeature::DefaultCtor:
    case EGenericFeature::Dtor:
    case EGenericFeature::Copy:
    case EGenericFeature::Move:
    case EGenericFeature::Assign:
    case EGenericFeature::MoveAssign:
    case EGenericFeature::Swap:
        return _inner->support(feature);
    case EGenericFeature::Equal:
    case EGenericFeature::Hash:
        return false;
    default:
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
void GenericSparseHashSetStorage::default_ctor(void* dst, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(count > 0);
    auto storage_size = size();

    if (_inner_mem_traits.use_ctor)
    {
        for (uint64_t i = 0; i < count; ++i)
        {
            auto p_dst = ::skr::memory::offset_bytes(dst, storage_size);

            // call ctor
            _inner->default_ctor(p_dst, 1);

            // set hash and next
            item_hash(p_dst) = 0; // hash
            next(p_dst)      = 0; // next
        }
    }
    else
    {
        ::skr::memory::zero_memory(dst, storage_size * count);
    }
}
void GenericSparseHashSetStorage::dtor(void* dst, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(count > 0);
    auto storage_size = size();

    if (_inner_mem_traits.use_dtor)
    {
        for (uint64_t i = 0; i < count; ++i)
        {
            auto p_dst = ::skr::memory::offset_bytes(dst, storage_size);

            // call dtor
            _inner->dtor(p_dst, 1);

            // reset hash and next
            item_hash(p_dst) = 0; // hash
            next(p_dst)      = 0; // next
        }
    }
    else
    {
        ::skr::memory::zero_memory(dst, storage_size * count);
    }
}
void GenericSparseHashSetStorage::copy(void* dst, const void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);
    auto storage_size = size();

    if (_inner_mem_traits.use_copy)
    {
        for (uint64_t i = 0; i < count; ++i)
        {
            auto p_dst = ::skr::memory::offset_bytes(dst, storage_size);
            auto p_src = ::skr::memory::offset_bytes(src, storage_size);

            // copy item data
            _inner->copy(item_data(p_dst), item_data(p_src), 1);

            // copy hash and next
            item_hash(p_dst) = item_hash(p_src);
            next(p_dst)      = next(p_src);
        }
    }
    else
    {
        ::std::memcpy(dst, src, storage_size * count);
    }
}
void GenericSparseHashSetStorage::move(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);
    auto storage_size = size();

    if (_inner_mem_traits.use_move)
    {
        for (uint64_t i = 0; i < count; ++i)
        {
            auto p_dst = ::skr::memory::offset_bytes(dst, storage_size);
            auto p_src = ::skr::memory::offset_bytes(src, storage_size);

            // move item data
            _inner->move(item_data(p_dst), item_data(p_src), 1);

            // move hash and next
            item_hash(p_dst) = item_hash(p_src);
            next(p_dst)      = next(p_src);

            // reset src
            item_hash(p_src) = 0;
            next(p_src)      = 0;
        }
    }
    else
    {
        ::std::memmove(dst, src, storage_size * count);
        ::skr::memory::zero_memory(src, storage_size * count);
    }
}
void GenericSparseHashSetStorage::assign(void* dst, const void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);
    auto storage_size = size();

    if (_inner_mem_traits.use_assign)
    {
        for (uint64_t i = 0; i < count; ++i)
        {
            auto p_dst = ::skr::memory::offset_bytes(dst, storage_size);
            auto p_src = ::skr::memory::offset_bytes(src, storage_size);

            // assign item data
            _inner->assign(item_data(p_dst), item_data(p_src), 1);

            // assign hash and next
            item_hash(p_dst) = item_hash(p_src);
            next(p_dst)      = next(p_src);
        }
    }
    else
    {
        ::std::memcpy(dst, src, storage_size * count);
    }
}
void GenericSparseHashSetStorage::move_assign(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);
    auto storage_size = size();

    if (_inner_mem_traits.use_move_assign)
    {
        for (uint64_t i = 0; i < count; ++i)
        {
            auto p_dst = ::skr::memory::offset_bytes(dst, storage_size);
            auto p_src = ::skr::memory::offset_bytes(src, storage_size);

            // move assign item data
            _inner->move_assign(item_data(p_dst), item_data(p_src), 1);

            // move assign hash and next
            item_hash(p_dst) = item_hash(p_src);
            next(p_dst)      = next(p_src);

            // reset src
            item_hash(p_src) = 0;
            next(p_src)      = 0;
        }
    }
    else
    {
        ::std::memmove(dst, src, storage_size * count);
        ::skr::memory::zero_memory(src, storage_size * count);
    }
}
bool GenericSparseHashSetStorage::equal(const void* lhs, const void* rhs, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(lhs);
    SKR_ASSERT(rhs);
    SKR_ASSERT(count > 0);

    SKR_ASSERT(false && "equal not support for GenericOptional, please check feature before call this function");
    return false;
}
size_t GenericSparseHashSetStorage::hash(const void* src) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(false && "hash not support for GenericOptional, please check feature before call this function");
    return 0;
}
void GenericSparseHashSetStorage::swap(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);
    auto storage_size = size();

    for (uint64_t i = 0; i < count; ++i)
    {
        auto p_dst = ::skr::memory::offset_bytes(dst, storage_size);
        auto p_src = ::skr::memory::offset_bytes(src, storage_size);

        // swap item data
        _inner->swap(item_data(p_dst), item_data(p_src), 1);

        // swap hash
        auto temp_hash   = item_hash(p_dst);
        item_hash(p_dst) = item_hash(p_src);
        item_hash(p_src) = temp_hash;

        // swap next
        auto temp_next = next(p_dst);
        next(p_dst)    = next(p_src);
        next(p_src)    = temp_next;
    }
}
//===> IGenericBase API

// getter
bool             GenericSparseHashSetStorage::is_valid() const { return _inner != nullptr; }
RC<IGenericBase> GenericSparseHashSetStorage::inner() const { return _inner; }

// item getter
void* GenericSparseHashSetStorage::item_data(void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    return dst; // item data is at the beginning of the storage
}
const void* GenericSparseHashSetStorage::item_data(const void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    return dst; // item data is at the beginning of the storage
}
size_t GenericSparseHashSetStorage::item_hash(const void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto padded_inner_size = std::max(_inner_size, alignof(uint64_t));
    auto hash_ptr          = ::skr::memory::offset_bytes(dst, padded_inner_size);
    return *static_cast<const size_t*>(hash_ptr);
}
size_t& GenericSparseHashSetStorage::item_hash(void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto padded_inner_size = std::max(_inner_size, alignof(uint64_t));
    auto hash_ptr          = ::skr::memory::offset_bytes(dst, padded_inner_size);
    return *static_cast<size_t*>(hash_ptr);
}
uint64_t GenericSparseHashSetStorage::next(const void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto padded_inner_size = std::max(_inner_size, alignof(uint64_t));
    auto next_ptr          = ::skr::memory::offset_bytes(dst, padded_inner_size + sizeof(size_t));
    return *static_cast<const uint64_t*>(next_ptr);
}
uint64_t& GenericSparseHashSetStorage::next(void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto padded_inner_size = std::max(_inner_size, alignof(uint64_t));
    auto next_ptr          = ::skr::memory::offset_bytes(dst, padded_inner_size + sizeof(size_t));
    return *static_cast<uint64_t*>(next_ptr);
}
} // namespace skr