#include <SkrRTTR/generic/generic_vector.hpp>

namespace skr
{
// ctor & dtor
GenericVector::GenericVector(RC<IGenericBase> inner)
    : _inner(inner)
{
    if (_inner)
    {
        _inner_mem_traits = _inner->memory_traits_data();
    }
}
GenericVector::~GenericVector() = default;

// get type info
EGenericKind GenericVector::kind() const
{
    return EGenericKind::Generic;
}
GUID GenericVector::id() const
{
    return kVectorGenericId;
}

// get utils
MemoryTraitsData GenericVector::memory_traits_data() const
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
uint64_t GenericVector::size() const
{
    return sizeof(VectorMemoryBase);
}
uint64_t GenericVector::alignment() const
{
    return alignof(VectorMemoryBase);
}

// operations, used for generic container algorithms
bool GenericVector::support(EGenericFeature feature) const
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
void GenericVector::default_ctor(void* dst, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(count > 0);

    std::memset(dst, 0, size() * count);
}
void GenericVector::dtor(void* dst, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(count > 0);

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* p_dst = ::skr::memory::offset_item(dst, sizeof(VectorMemoryBase), i);
        release(p_dst);
    }
}
void GenericVector::copy(void* dst, const void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(
            ::skr::memory::offset_item(dst, sizeof(VectorMemoryBase), i)
        );
        const auto* src_mem = reinterpret_cast<const VectorMemoryBase*>(
            ::skr::memory::offset_item(src, sizeof(VectorMemoryBase), i)
        );

        if (src_mem->size())
        {
            _realloc(dst, src_mem->size());
            _inner->copy(
                dst_mem->_generic_only_data(),
                src_mem->_generic_only_data(),
                src_mem->size()
            );
            dst_mem->set_size(src_mem->size());
        }
    }
}
void GenericVector::move(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(
            ::skr::memory::offset_item(dst, sizeof(VectorMemoryBase), i)
        );
        auto* src_mem = reinterpret_cast<VectorMemoryBase*>(
            ::skr::memory::offset_item(src, sizeof(VectorMemoryBase), i)
        );

        dst_mem->set_size(src_mem->size());
        dst_mem->_generic_only_set_capacity(src_mem->capacity());
        dst_mem->_generic_only_set_data(src_mem->_generic_only_data());

        src_mem->_generic_only_set_data(nullptr);
        src_mem->_generic_only_set_capacity(0);
        src_mem->set_size(0);
    }
}
void GenericVector::assign(void* dst, const void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(
            ::skr::memory::offset_item(dst, sizeof(VectorMemoryBase), i)
        );
        const auto* src_mem = reinterpret_cast<const VectorMemoryBase*>(
            ::skr::memory::offset_item(src, sizeof(VectorMemoryBase), i)
        );

        // clean up dst
        clear(dst);

        // copy data
        if (src_mem->size())
        {
            // reserve memory
            if (dst_mem->capacity() < src_mem->size())
            {
                _realloc(dst, src_mem->size());
            }

            // copy data
            _inner->copy(
                dst_mem->_generic_only_data(),
                src_mem->_generic_only_data(),
                src_mem->size()
            );
            dst_mem->set_size(src_mem->size());
        }
    }
}
void GenericVector::move_assign(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(
            ::skr::memory::offset_item(dst, sizeof(VectorMemoryBase), i)
        );
        auto* src_mem = reinterpret_cast<VectorMemoryBase*>(
            ::skr::memory::offset_item(src, sizeof(VectorMemoryBase), i)
        );

        // clean up dst
        clear(dst);
        free(dst);

        // move data
        dst_mem->set_size(src_mem->size());
        dst_mem->_generic_only_set_capacity(src_mem->capacity());
        dst_mem->_generic_only_set_data(src_mem->_generic_only_data());

        // clean up src
        src_mem->_generic_only_set_data(nullptr);
        src_mem->_generic_only_set_capacity(0);
        src_mem->set_size(0);
    }
}
bool GenericVector::equal(const void* lhs, const void* rhs, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(lhs);
    SKR_ASSERT(rhs);
    SKR_ASSERT(count > 0);

    for (uint64_t i = 0; i < count; ++i)
    {
        auto lhs_mem = reinterpret_cast<const VectorMemoryBase*>(
            ::skr::memory::offset_item(lhs, sizeof(VectorMemoryBase), i)
        );
        auto rhs_mem = reinterpret_cast<const VectorMemoryBase*>(
            ::skr::memory::offset_item(rhs, sizeof(VectorMemoryBase), i)
        );

        // check size
        if (lhs_mem->size() != rhs_mem->size())
        {
            return false;
        }

        // check data
        if (!_inner->equal(
                lhs_mem->_generic_only_data(),
                rhs_mem->_generic_only_data(),
                lhs_mem->size()
            ))
        {
            return false;
        }
    }

    return true;
}
size_t GenericVector::hash(const void* src) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(src);
    SKR_ASSERT(false && "GenericVector does not support hash operation, please check feature first");
    return 0;
}
void GenericVector::swap(void* dst, void* src, uint64_t count) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(src);
    SKR_ASSERT(count > 0);

    for (uint64_t i = 0; i < count; ++i)
    {
        auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(
            ::skr::memory::offset_item(dst, sizeof(VectorMemoryBase), i)
        );
        auto* src_mem = reinterpret_cast<VectorMemoryBase*>(
            ::skr::memory::offset_item(src, sizeof(VectorMemoryBase), i)
        );

        // swap size
        auto tmp_size = dst_mem->size();
        dst_mem->set_size(src_mem->size());
        src_mem->set_size(tmp_size);

        // swap capacity
        auto tmp_capacity = dst_mem->capacity();
        dst_mem->_generic_only_set_capacity(src_mem->capacity());
        src_mem->_generic_only_set_capacity(tmp_capacity);

        // swap data
        auto tmp_data = dst_mem->_generic_only_data();
        dst_mem->_generic_only_set_data(src_mem->_generic_only_data());
        src_mem->_generic_only_set_data(tmp_data);
    }
}

// getter
bool GenericVector::is_valid() const
{
    return _inner != nullptr;
}
RC<IGenericBase> GenericVector::inner() const
{
    SKR_ASSERT(is_valid());
    return _inner;
}

// vector getter
uint64_t GenericVector::size(const void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    return reinterpret_cast<const VectorMemoryBase*>(dst)->size();
}
uint64_t GenericVector::capacity(const void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    return reinterpret_cast<const VectorMemoryBase*>(dst)->capacity();
}
uint64_t GenericVector::slack(const void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem = reinterpret_cast<const VectorMemoryBase*>(dst);
    return dst_mem->capacity() - dst_mem->size();
}
bool GenericVector::is_empty(const void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    return reinterpret_cast<const VectorMemoryBase*>(dst)->size() == 0;
}
const void* GenericVector::data(const void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    return reinterpret_cast<const VectorMemoryBase*>(dst)->_generic_only_data();
}
void* GenericVector::data(void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    return reinterpret_cast<VectorMemoryBase*>(dst)->_generic_only_data();
}

// validate
bool GenericVector::is_valid_index(const void* dst, uint64_t idx) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);

    auto* dst_mem = reinterpret_cast<const VectorMemoryBase*>(dst);
    return idx >= 0 && idx < dst_mem->size();
}

// memory op
void GenericVector::clear(void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    if (dst_mem->size())
    {
        if (_inner_mem_traits.use_dtor)
        {
            _inner->dtor(
                dst_mem->_generic_only_data(),
                dst_mem->size()
            );
        }
        dst_mem->set_size(0);
    }
}
void GenericVector::release(void* dst, uint64_t reserve_capacity) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    clear(dst);

    if (reserve_capacity)
    {
        if (reserve_capacity != dst_mem->capacity())
        {
            _realloc(dst, reserve_capacity);
        }
    }
    else
    {
        _free(dst);
    }
}
void GenericVector::reserve(void* dst, uint64_t expect_capacity) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    if (expect_capacity > dst_mem->capacity())
    {
        _realloc(dst, expect_capacity);
    }
}
void GenericVector::shrink(void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    auto new_capacity = container::default_get_shrink<uint64_t>(
        dst_mem->size(),
        dst_mem->capacity()
    );
    SKR_ASSERT(new_capacity >= dst_mem->size());
    if (new_capacity < dst_mem->capacity())
    {
        if (new_capacity)
        {
            _realloc(dst, new_capacity);
        }
        else
        {
            _free(dst);
        }
    }
}
void GenericVector::resize(void* dst, uint64_t expect_size, void* new_value) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem   = reinterpret_cast<VectorMemoryBase*>(dst);
    auto  item_size = _inner->size();

    // realloc memory if need
    if (expect_size > dst_mem->capacity())
    {
        _realloc(dst, expect_size);
    }

    // construct item or destruct item if need
    if (expect_size > dst_mem->size())
    {
        for (uint64_t i = dst_mem->size(); i < expect_size; ++i)
        {
            void* p_copy_data = ::skr::memory::offset_item(
                dst_mem->_generic_only_data(),
                item_size,
                i
            );
            _inner->copy(
                p_copy_data,
                new_value
            );
        }
    }
    else if (expect_size < dst_mem->size())
    {
        void* p_destruct_data = ::skr::memory::offset_item(
            dst_mem->_generic_only_data(),
            item_size,
            expect_size
        );
        _inner->dtor(
            p_destruct_data,
            dst_mem->size() - expect_size
        );
    }

    // set size
    dst_mem->set_size(expect_size);
}
void GenericVector::resize_unsafe(void* dst, uint64_t expect_size) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem   = reinterpret_cast<VectorMemoryBase*>(dst);
    auto  item_size = _inner->size();

    // realloc memory if need
    if (expect_size > dst_mem->capacity())
    {
        _realloc(dst, expect_size);
    }

    // construct item or destruct item if need
    if (expect_size < dst_mem->size())
    {
        void* p_destruct_data = ::skr::memory::offset_item(
            dst_mem->_generic_only_data(),
            item_size,
            expect_size
        );
        _inner->dtor(
            p_destruct_data,
            dst_mem->size() - expect_size
        );
    }

    // set size
    dst_mem->set_size(expect_size);
}
void GenericVector::resize_default(void* dst, uint64_t expect_size) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem   = reinterpret_cast<VectorMemoryBase*>(dst);
    auto  item_size = _inner->size();

    // realloc memory if need
    if (expect_size > dst_mem->capacity())
    {
        _realloc(dst, expect_size);
    }

    // construct item or destruct item if need
    if (expect_size > dst_mem->size())
    {
        void* p_construct_data = ::skr::memory::offset_item(
            dst_mem->_generic_only_data(),
            item_size,
            dst_mem->size()
        );
        _inner->default_ctor(
            p_construct_data,
            expect_size - dst_mem->size()
        );
    }
    else if (expect_size < dst_mem->size())
    {
        void* p_destruct_data = ::skr::memory::offset_item(
            dst_mem->_generic_only_data(),
            item_size,
            expect_size
        );
        _inner->dtor(
            p_destruct_data,
            dst_mem->size() - expect_size
        );
    }

    // set size
    dst_mem->set_size(expect_size);
}
void GenericVector::resize_zeroed(void* dst, uint64_t expect_size) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem   = reinterpret_cast<VectorMemoryBase*>(dst);
    auto  item_size = _inner->size();

    // realloc memory if need
    if (expect_size > dst_mem->capacity())
    {
        _realloc(dst, expect_size);
    }

    // construct item or destruct item if need
    if (expect_size > dst_mem->size())
    {
        void* p_zero_data = ::skr::memory::offset_item(
            dst_mem->_generic_only_data(),
            item_size,
            dst_mem->size()
        );
        std::memset(p_zero_data, 0, (expect_size - dst_mem->size()) * item_size);
    }
    else if (expect_size < dst_mem->size())
    {
        void* p_destruct_data = ::skr::memory::offset_item(
            dst_mem->_generic_only_data(),
            item_size,
            expect_size
        );
        _inner->dtor(
            p_destruct_data,
            dst_mem->size() - expect_size
        );
    }

    // set size
    dst_mem->set_size(expect_size);
}

// add
GenericVectorDataRef GenericVector::add(void* dst, const void* v, uint64_t n) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);

    auto result = add_unsafe(dst, n);
    for (uint64_t i = 0; i < n; ++i)
    {
        _inner->copy(
            ::skr::memory::offset_item(
                result.ptr,
                _inner->size(),
                i
            ),
            v
        );
    }
    return result;
}
GenericVectorDataRef GenericVector::add_move(void* dst, void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto result = add_unsafe(dst, 1);
    _inner->move(result.ptr, v, 1);
    return result;
}
GenericVectorDataRef GenericVector::add_unsafe(void* dst, uint64_t n) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    auto old_size = _grow(dst, n);
    return {
        ::skr::memory::offset_item(dst_mem->_generic_only_data(), _inner->size(), old_size),
        old_size
    };
}
GenericVectorDataRef GenericVector::add_default(void* dst, uint64_t n) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);

    auto result = add_unsafe(dst, n);
    _inner->default_ctor(
        ::skr::memory::offset_item(
            result.ptr,
            _inner->size(),
            n
        )
    );
    return result;
}
GenericVectorDataRef GenericVector::add_zeroed(void* dst, uint64_t n) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);

    auto result = add_unsafe(dst, n);
    ::skr::memory::zero_memory(result.ptr, n * _inner->size());
    return result;
}
GenericVectorDataRef GenericVector::add_unique(void* dst, const void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);

    if (auto ref = find(dst, v))
    {
        return ref;
    }
    else
    {
        return add(dst, v);
    }

    return {};
}
GenericVectorDataRef GenericVector::add_unique_move(void* dst, void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);

    if (auto ref = find(dst, v))
    {
        return ref;
    }
    else
    {
        return add_move(dst, v);
    }

    return {};
}

// add at
void GenericVector::add_at(void* dst, uint64_t idx, const void* v, uint64_t n) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    add_at_unsafe(dst, idx, n);
    for (uint64_t i = 0; i < n; ++i)
    {
        _inner->copy(
            ::skr::memory::offset_item(
                dst_mem->_generic_only_data(),
                _inner->size(),
                idx + i
            ),
            v
        );
    }
}
void GenericVector::add_at_move(void* dst, uint64_t idx, void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    add_at_unsafe(dst, idx, 1);
    for (uint64_t i = 0; i < dst_mem->size(); ++i)
    {
        _inner->move(
            ::skr::memory::offset_item(
                dst_mem->_generic_only_data(),
                _inner->size(),
                idx + i
            ),
            v
        );
    }
}
void GenericVector::add_at_unsafe(void* dst, uint64_t idx, uint64_t n) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT((is_empty(dst) && idx == 0) || is_valid_index(dst, idx));
    auto dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    auto move_n = dst_mem->size() - idx;
    add_unsafe(dst, n);
    _inner->move(
        ::skr::memory::offset_item(
            dst_mem->_generic_only_data(),
            _inner->size(),
            idx + n
        ),
        ::skr::memory::offset_item(
            dst_mem->_generic_only_data(),
            _inner->size(),
            idx
        ),
        move_n
    );
}
void GenericVector::add_at_default(void* dst, uint64_t idx, uint64_t n) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);

    add_at_unsafe(dst, idx, n);
    _inner->default_ctor(
        ::skr::memory::offset_item(
            reinterpret_cast<VectorMemoryBase*>(dst)->_generic_only_data(),
            _inner->size(),
            idx
        ),
        n
    );
}
void GenericVector::add_at_zeroed(void* dst, uint64_t idx, uint64_t n) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);

    add_at_unsafe(dst, idx, n);
    ::skr::memory::zero_memory(
        ::skr::memory::offset_item(
            reinterpret_cast<VectorMemoryBase*>(dst)->_generic_only_data(),
            _inner->size(),
            idx
        ),
        n
    );
}

// append
GenericVectorDataRef GenericVector::append(void* dst, const void* other) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(other);
    auto*       dst_mem   = reinterpret_cast<VectorMemoryBase*>(dst);
    const auto* other_mem = reinterpret_cast<const VectorMemoryBase*>(other);

    auto other_size = other_mem->size();
    if (other_size)
    {
        auto result = add_unsafe(dst, other_size);
        _inner->copy(
            result.ptr,
            other_mem->_generic_only_data(),
            other_size
        );
        return result;
    }
    return {
        ::skr::memory::offset_item(
            dst_mem->_generic_only_data(),
            _inner->size(),
            dst_mem->size()
        ),
        dst_mem->size()
    };
}

// append at
void GenericVector::append_at(void* dst, uint64_t idx, const void* other) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(other);
    auto*       dst_mem   = reinterpret_cast<VectorMemoryBase*>(dst);
    const auto* other_mem = reinterpret_cast<const VectorMemoryBase*>(other);

    auto other_size = other_mem->size();
    if (other_size)
    {
        add_at_unsafe(dst, idx, other_size);
        _inner->copy(
            ::skr::memory::offset_item(
                dst_mem->_generic_only_data(),
                _inner->size(),
                idx
            ),
            other_mem->_generic_only_data(),
            other_size
        );
    }
}

// remove
void GenericVector::remove_at(void* dst, uint64_t idx, uint64_t n) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);
    SKR_ASSERT(is_valid_index(dst, idx) && (dst_mem->size() - idx >= n));

    if (n)
    {
        // calc move size
        auto move_n = dst_mem->size() - idx - n;

        // destruct remove items
        _inner->dtor(
            ::skr::memory::offset_item(
                dst_mem->_generic_only_data(),
                _inner->size(),
                idx
            ),
            n
        );

        // move data
        if (move_n)
        {
            _inner->move(
                ::skr::memory::offset_item(
                    dst_mem->_generic_only_data(),
                    _inner->size(),
                    idx
                ),
                ::skr::memory::offset_item(
                    dst_mem->_generic_only_data(),
                    _inner->size(),
                    idx + n
                ),
                move_n
            );
        }

        // update size
        dst_mem->set_size(dst_mem->size() - n);
    }
}
void GenericVector::remove_at_swap(void* dst, uint64_t idx, uint64_t n) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);
    SKR_ASSERT(is_valid_index(dst, idx) && (dst_mem->size() - idx >= n));

    if (n)
    {
        // calc move size
        auto move_n = std::min(dst_mem->size() - idx - n, n);

        // destruct remove items
        _inner->dtor(
            ::skr::memory::offset_item(
                dst_mem->_generic_only_data(),
                _inner->size(),
                idx
            ),
            n
        );

        // move data
        if (move_n)
        {
            _inner->move(
                ::skr::memory::offset_item(
                    dst_mem->_generic_only_data(),
                    _inner->size(),
                    idx
                ),
                ::skr::memory::offset_item(
                    dst_mem->_generic_only_data(),
                    _inner->size(),
                    dst_mem->size() - move_n
                ),
                move_n
            );
        }

        // update size
        dst_mem->set_size(dst_mem->size() - n);
    }
}
bool GenericVector::remove(void* dst, const void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(v);

    if (auto ref = find(dst, v))
    {
        remove_at(dst, ref.index);
        return true;
    }
    return false;
}
bool GenericVector::remove_swap(void* dst, const void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(v);

    if (auto ref = find(dst, v))
    {
        remove_at_swap(dst, ref.index);
        return true;
    }
    return false;
}
bool GenericVector::remove_last(void* dst, const void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(v);

    if (auto ref = find_last(dst, v))
    {
        remove_at(dst, ref.index);
        return true;
    }
    return false;
}
bool GenericVector::remove_last_swap(void* dst, const void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(v);

    if (auto ref = find_last(dst, v))
    {
        remove_at_swap(dst, ref.index);
        return true;
    }
    return false;
}
uint64_t GenericVector::remove_all(void* dst, const void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(v);
    auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    if (is_empty(dst))
    {
        return 0;
    }
    else
    {
        auto  inner_size = _inner->size();
        void* begin      = dst_mem->_generic_only_data();
        void* end        = ::skr::memory::offset_item(begin, inner_size, dst_mem->size());
        void* write      = begin;
        void* read       = begin;
        bool  do_remove  = _inner->equal(read, v, 1);

        // remove loop
        do
        {
            auto run_start = read;
            read           = ::skr::memory::offset_item(read, inner_size, 1);

            // collect run scope
            while (read < end && do_remove == _inner->equal(read, v, 1))
            {
                read = ::skr::memory::offset_item(read, inner_size, 1);
            }
            uint64_t run_len = ::skr::memory::distance_item(run_start, read, inner_size);
            SKR_ASSERT(run_len > 0);

            // do scope op
            if (do_remove)
            {
                _inner->dtor(
                    run_start,
                    run_len
                );
            }
            else
            {
                if (write != run_start)
                {
                    _inner->move(
                        write,
                        run_start,
                        run_len
                    );
                }
                write = ::skr::memory::offset_item(write, inner_size, run_len);
            }

            // update flag
            do_remove = !do_remove;
        } while (read < end);

        // get remove count
        uint64_t remove_count = ::skr::memory::distance_item(write, end, inner_size);

        // update size
        if (remove_count)
        {
            dst_mem->set_size(dst_mem->size() - remove_count);
        }
        return remove_count;
    }
}
uint64_t GenericVector::remove_all_swap(void* dst, const void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(v);
    auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    if (is_empty(dst))
    {
        return 0;
    }
    else
    {
        auto  inner_size = _inner->size();
        void* begin      = dst_mem->_generic_only_data();
        void* end        = ::skr::memory::offset_item(begin, inner_size, dst_mem->size());
        void* cache_end  = end;

        // for swap
        end = ::skr::memory::offset_item(end, inner_size, -1);

        // remove loop
        while (true)
        {
            // skip items that needn't remove on header
            while (true)
            {
                if (begin > end)
                {
                    goto LoopEnd;
                }
                if (_inner->equal(begin, v, 1))
                {
                    break;
                }
                begin = ::skr::memory::offset_item(begin, inner_size, 1);
            }

            // skip items that needn't remove on tail
            while (true)
            {
                if (begin > end)
                {
                    goto LoopEnd;
                }
                if (!_inner->equal(end, v, 1))
                {
                    break;
                }
                _inner->dtor(
                    end,
                    1
                );
                end = ::skr::memory::offset_item(end, inner_size, -1);
            }

            // swap items at bad pos
            _inner->dtor(
                begin,
                1
            );
            _inner->move(
                begin,
                end,
                1
            );

            // update iterator
            begin = ::skr::memory::offset_item(begin, inner_size, 1);
            end   = ::skr::memory::offset_item(end, inner_size, -1);
        }
    LoopEnd:

        // get remove count
        uint64_t remove_count = ::skr::memory::distance_item(begin, cache_end, inner_size);

        // update size
        if (remove_count)
        {
            dst_mem->set_size(dst_mem->size() - remove_count);
        }
        return remove_count;
    }
}

// access
void* GenericVector::at(void* dst, uint64_t idx) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(!is_empty(dst) && is_valid_index(dst, idx));
    auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);
    return ::skr::memory::offset_item(
        dst_mem->_generic_only_data(),
        _inner->size(),
        idx
    );
}
void* GenericVector::at_last(void* dst, uint64_t idx) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(!is_empty(dst) && is_valid_index(dst, idx));
    auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);
    return ::skr::memory::offset_item(
        dst_mem->_generic_only_data(),
        _inner->size(),
        dst_mem->size() - 1 - idx
    );
}
const void* GenericVector::at(const void* dst, uint64_t idx) const
{
    return at(const_cast<void*>(dst), idx);
}
const void* GenericVector::at_last(const void* dst, uint64_t idx) const
{
    return at_last(const_cast<void*>(dst), idx);
}

// find
GenericVectorDataRef GenericVector::find(void* dst, const void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(v);
    auto* dst_mem    = reinterpret_cast<VectorMemoryBase*>(dst);
    auto  inner_size = _inner->size();

    if (is_empty(dst))
    {
        return {};
    }
    else
    {
        void* begin = dst_mem->_generic_only_data();
        void* end   = ::skr::memory::offset_item(begin, inner_size, dst_mem->size());

        while (begin < end)
        {
            if (_inner->equal(begin, v, 1))
            {
                return {
                    begin,
                    (uint64_t)::skr::memory::distance_item(
                        dst_mem->_generic_only_data(),
                        begin,
                        inner_size
                    )
                };
            }
            begin = ::skr::memory::offset_item(begin, inner_size, 1);
        }
        return {};
    }
}
GenericVectorDataRef GenericVector::find_last(void* dst, const void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(v);
    auto* dst_mem    = reinterpret_cast<VectorMemoryBase*>(dst);
    auto  inner_size = _inner->size();

    if (is_empty(dst))
    {
        return {};
    }
    else
    {
        void* begin = dst_mem->_generic_only_data();
        void* end   = ::skr::memory::offset_item(begin, inner_size, dst_mem->size());

        end = ::skr::memory::offset_item(end, inner_size, -1);

        while (end >= begin)
        {
            if (_inner->equal(end, v, 1))
            {
                return {
                    end,
                    (uint64_t)::skr::memory::distance_item(
                        dst_mem->_generic_only_data(),
                        end,
                        inner_size
                    )
                };
            }
            end = ::skr::memory::offset_item(end, inner_size, -1);
        }
        return {};
    }
}
CGenericVectorDataRef GenericVector::find(const void* dst, const void* v) const
{
    return find(const_cast<void*>(dst), v);
}
CGenericVectorDataRef GenericVector::find_last(const void* dst, const void* v) const
{
    return find_last(const_cast<void*>(dst), v);
}

// contains & count
bool GenericVector::contains(const void* dst, const void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(v);

    return (bool)find(dst, v);
}
uint64_t GenericVector::count(const void* dst, const void* v) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    SKR_ASSERT(v);
    auto* dst_mem    = reinterpret_cast<const VectorMemoryBase*>(dst);
    auto  inner_size = _inner->size();

    if (is_empty(dst))
    {
        return 0;
    }
    else
    {
        uint64_t    count = 0;
        const void* begin = dst_mem->_generic_only_data();
        const void* end   = ::skr::memory::offset_item(begin, inner_size, dst_mem->size());

        while (begin < end)
        {
            if (_inner->equal(begin, v, 1))
            {
                ++count;
            }
            begin = ::skr::memory::offset_item(begin, inner_size, 1);
        }
        return count;
    }
}

// helper
void GenericVector::_realloc(void* dst, uint64_t new_capacity) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    SKR_ASSERT(new_capacity != dst_mem->capacity());
    SKR_ASSERT(new_capacity > 0);
    SKR_ASSERT(dst_mem->size() <= new_capacity);
    SKR_ASSERT((dst_mem->capacity() > 0 && dst_mem->_generic_only_data() != nullptr) || (dst_mem->capacity() == 0 && dst_mem->_generic_only_data() == nullptr));

    if (_inner_mem_traits.use_realloc)
    {
        dst_mem->_generic_only_set_data(
            SkrAllocator::realloc_raw(
                dst_mem->_generic_only_data(),
                new_capacity,
                _inner->size(),
                _inner->alignment()
            )
        );
    }
    else
    {
        // alloc new memory
        void* new_memory = SkrAllocator::alloc_raw(
            new_capacity,
            _inner->size(),
            _inner->alignment()
        );

        // move items
        if (dst_mem->size())
        {
            _inner->move(
                new_memory,
                dst_mem->_generic_only_data(),
                dst_mem->size()
            );
        }

        // release old memory
        SkrAllocator::free_raw(
            dst_mem->_generic_only_data(),
            _inner->alignment()
        );

        // update data
        dst_mem->_generic_only_set_data(new_memory);
    }

    // update capacity
    dst_mem->_generic_only_set_capacity(new_capacity);
}
void GenericVector::_free(void* dst) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto* dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    if (dst_mem->_generic_only_data())
    {
        SkrAllocator::free_raw(
            dst_mem->_generic_only_data(),
            _inner->alignment()
        );
        dst_mem->_generic_only_set_data(nullptr);
        dst_mem->_generic_only_set_capacity(0);
    }
}
uint64_t GenericVector::_grow(void* dst, uint64_t grow_size) const
{
    SKR_ASSERT(is_valid());
    SKR_ASSERT(dst);
    auto dst_mem = reinterpret_cast<VectorMemoryBase*>(dst);

    uint64_t old_size = dst_mem->size();
    uint64_t new_size = old_size + grow_size;

    if (new_size > dst_mem->capacity())
    {
        uint64_t new_capacity = container::default_get_grow<uint64_t>(
            new_size,
            dst_mem->capacity()
        );
        SKR_ASSERT(new_capacity >= dst_mem->capacity());
        if (new_capacity >= dst_mem->capacity())
        {
            _realloc(dst, new_capacity);
        }
    }

    dst_mem->set_size(new_size);
    return old_size;
}
} // namespace skr
