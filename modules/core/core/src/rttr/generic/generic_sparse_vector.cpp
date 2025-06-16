#include <SkrRTTR/generic/generic_sparse_vector.hpp>

namespace skr
{
// ctor & dtor
GenericSparseVector::GenericSparseVector(RC<IGenericBase> inner)
    : _inner(inner)
{
    if (_inner)
    {
        _inner_mem_traits = _inner->memory_traits_data();
    }
}
GenericSparseVector::~GenericSparseVector() = default;

// get type info
EGenericKind GenericSparseVector::kind() const
{
    return EGenericKind::Generic;
}
GUID GenericSparseVector::id() const
{
    return kSparseVectorGenericId;
}

// get utils
MemoryTraitsData GenericSparseVector::memory_traits_data() const
{
    SKR_ASSERT(is_valid());
    MemoryTraitsData data     = _inner_mem_traits;
    data.use_ctor             = true;
    data.use_dtor             = true;
    data.use_move             = true;
    data.use_assign           = true;
    data.use_move_assign      = true;
    data.need_dtor_after_move = false;
    data.use_realloc          = false;
    data.use_compare          = true;
    return data;
}

// basic info
uint64_t GenericSparseVector::size() const
{
    return sizeof(SparseVectorMemoryBase);
}
uint64_t GenericSparseVector::alignment() const
{
    return alignof(SparseVectorMemoryBase);
}

// operations, used for generic container algorithms
bool GenericSparseVector::support(EGenericFeature feature) const
{
    switch (feature)
    {
    case EGenericFeature::DefaultCtor:
        return true;
    case EGenericFeature::Dtor:
    case EGenericFeature::Copy:
    case EGenericFeature::Move:
    case EGenericFeature::Assign:
    case EGenericFeature::MoveAssign:
    case EGenericFeature::Equal:
        return _inner && _inner->support(feature);
    case EGenericFeature::Hash:
        return false;
    case EGenericFeature::Swap:
        return true;
    default:
        SKR_UNREACHABLE_CODE()
        return false;
    }
}
void GenericSparseVector::default_ctor(void* dst, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(count > 0);

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* dst_mem = reinterpret_cast<SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(dst, sizeof(SparseVectorMemoryBase), i)
        );
        dst_mem->_reset();
    }
}
void GenericSparseVector::dtor(void* dst, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(count > 0);

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* p_dst = ::skr::memory::offset_item(dst, sizeof(SparseVectorMemoryBase), i);
        release(p_dst);
    }
}
void GenericSparseVector::copy(void* dst, const void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* dst_mem = reinterpret_cast<SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(dst, sizeof(SparseVectorMemoryBase), i)
        );
        const auto* src_mem = reinterpret_cast<const SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(src, sizeof(SparseVectorMemoryBase), i)
        );

        if (src_mem->sparse_size())
        {
            // reserve data
            _realloc(dst_mem, src_mem->sparse_size());

            // copy data
            _copy_sparse_vector_data(
                dst_mem->_data,
                src_mem->_data,
                src_mem->_data,
                src_mem->sparse_size()
            );
            _copy_sparse_vector_bit_data(
                dst_mem->_bit_data,
                src_mem->_bit_data,
                src_mem->sparse_size()
            );
            dst_mem->set_sparse_size(src_mem->sparse_size());
            dst_mem->set_freelist_head(src_mem->freelist_head());
            dst_mem->set_hole_size(src_mem->hole_size());
        }
    }
}
void GenericSparseVector::move(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* dst_mem = reinterpret_cast<SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(dst, sizeof(SparseVectorMemoryBase), i)
        );
        auto* src_mem = reinterpret_cast<SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(src, sizeof(SparseVectorMemoryBase), i)
        );

        *dst_mem = std::move(*src_mem);
        src_mem->_reset();
    }
}
void GenericSparseVector::assign(void* dst, const void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    if (dst == src) { return; }

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* dst_mem = reinterpret_cast<SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(dst, sizeof(SparseVectorMemoryBase), i)
        );
        const auto* src_mem = reinterpret_cast<const SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(src, sizeof(SparseVectorMemoryBase), i)
        );

        // clean up self
        clear(dst_mem);

        // copy data
        if ((src_mem->sparse_size() - src_mem->hole_size()) > 0)
        {
            // reserve data
            if (dst_mem->capacity() < src_mem->sparse_size())
            {
                _realloc(dst_mem, src_mem->sparse_size());
            }

            // copy data
            _copy_sparse_vector_data(
                dst_mem->_data,
                src_mem->_data,
                src_mem->_bit_data,
                src_mem->sparse_size()
            );
            _copy_sparse_vector_bit_data(
                dst_mem->_bit_data,
                src_mem->_bit_data,
                src_mem->sparse_size()
            );
            dst_mem->set_sparse_size(src_mem->sparse_size());
            dst_mem->set_freelist_head(src_mem->freelist_head());
            dst_mem->set_hole_size(src_mem->hole_size());
        }
    }
}
void GenericSparseVector::move_assign(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    if (dst == src) { return; }

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* dst_mem = reinterpret_cast<SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(dst, sizeof(SparseVectorMemoryBase), i)
        );
        auto* src_mem = reinterpret_cast<SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(src, sizeof(SparseVectorMemoryBase), i)
        );

        // clean up dst
        clear(dst_mem);

        // free dst
        _free(dst_mem);

        // move data
        *dst_mem = std::move(*src_mem);

        // reset src
        src_mem->_reset();
    }
}
bool GenericSparseVector::equal(const void* lhs, const void* rhs, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(lhs);
    SKR_ASSERT(rhs);
    SKR_ASSERT(count > 0);
    auto inner_size = _inner->size();

    for (uint64_t i = 0; i < count; ++i)
    {
        const auto* lhs_mem = reinterpret_cast<const SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(lhs, sizeof(SparseVectorMemoryBase), i)
        );
        const auto* rhs_mem = reinterpret_cast<const SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(rhs, sizeof(SparseVectorMemoryBase), i)
        );

        // compare size
        if (lhs_mem->sparse_size() != rhs_mem->sparse_size())
        {
            return false;
        }

        // compare data
        for (uint64_t cmp_idx = 0; cmp_idx < lhs_mem->sparse_size(); ++cmp_idx)
        {
            bool lhs_has_data = has_data(lhs_mem, cmp_idx);
            bool rhs_has_data = has_data(rhs_mem, cmp_idx);

            // compare data existence
            if (lhs_has_data != rhs_has_data)
            {
                return false;
            }

            // compare data
            if (lhs_has_data)
            {
                if (!_inner->equal(
                        lhs_mem->_storage_at(cmp_idx, inner_size),
                        rhs_mem->_storage_at(cmp_idx, inner_size),
                        1
                    ))
                {
                    return false;
                }
            }
        }
    }
    return true;
}
size_t GenericSparseVector::hash(const void* src) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(src);
    SKR_ASSERT(false && "GenericSparseVector does not support hash operation, please check feature first");
    return 0;
}
void GenericSparseVector::swap(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(count > 0);

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* dst_mem = reinterpret_cast<SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(dst, sizeof(SparseVectorMemoryBase), i)
        );
        auto* src_mem = reinterpret_cast<SparseVectorMemoryBase*>(
            ::skr::memory::offset_item(src, sizeof(SparseVectorMemoryBase), i)
        );

        // swap data
        SparseVectorMemoryBase tmp = std::move(*dst_mem);
        *dst_mem                   = std::move(*src_mem);
        *src_mem                   = std::move(tmp);
    }
}

} // namespace skr