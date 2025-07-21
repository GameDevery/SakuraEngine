#include "SkrRT/sugoi/type_index.hpp"
#include "./chunk.hpp"
#include "./pool.hpp"
#include "./archetype.hpp"

sugoi_chunk_t* sugoi_chunk_t::create(sugoi::pool_type_t poolType)
{
    using namespace sugoi;
    switch (poolType)
    {
        case PT_small:
            return new (get_default_pool_small().allocate()) sugoi_chunk_t(poolType);
        case PT_default:
            return new (get_default_pool().allocate()) sugoi_chunk_t(poolType);
        case PT_large:
            return new (get_default_pool_large().allocate()) sugoi_chunk_t(poolType);
    };
    return nullptr;
}

void sugoi_chunk_t::destroy(sugoi_chunk_t* chunk)
{
    using namespace sugoi;
    switch (chunk->pt)
    {
        case PT_small:
            return get_default_pool_small().free(chunk);
        case PT_default:
            return get_default_pool().free(chunk);
        case PT_large:
            return get_default_pool_large().free(chunk);
    };
}

void sugoi_chunk_t::init(sugoi::archetype_t* structure)
{
    auto pData = (sugoi::slice_data_t*)(data() + structure->sliceDataOffsets[pt]);
    for (EIndex i = 0; i < structure->type.length; ++i)
    {
        new (pData + i) sugoi::slice_data_t{};
    }
}

EIndex sugoi_chunk_t::get_capacity()
{
    return structure->chunkCapacity[pt];
}

const sugoi_entity_t* sugoi_chunk_t::get_entities() const
{
    return (const sugoi_entity_t*)data();
}

sugoi_timestamp_t sugoi_chunk_t::get_timestamp_at(uint32_t idx) const
{
    const auto pData = getSliceData();
    return pData[idx].timestamp;
}

sugoi_timestamp_t sugoi_chunk_t::get_timestamp(sugoi_type_index_t type) const
{
    const auto idx = structure->index(type);
    return get_timestamp_at(idx);
}

sugoi_timestamp_t sugoi_chunk_t::set_timestamp_at(uint32_t at, sugoi_timestamp_t ts)
{
    const auto pData = getSliceData();
    pData[at].timestamp = ts;
    return ts;
}

sugoi_timestamp_t sugoi_chunk_t::set_timestamp(sugoi_type_index_t type, sugoi_timestamp_t ts)
{
    const auto idx = structure->index(type);
    return set_timestamp_at(idx, ts);
}

sugoi_chunk_t::RWSlice sugoi_chunk_t::x_lock(const sugoi_type_index_t& type, const sugoi_chunk_view_t& view)
{
    auto& lck = getSliceLock(type);
    lck.lock();
    return get_unsafe(type, view);
}

sugoi_chunk_t::RSlice sugoi_chunk_t::s_lock(const sugoi_type_index_t& type, const sugoi_chunk_view_t& view) const
{
    auto& lck = getSliceLock(type);
    lck.lock_shared();
    return get_unsafe(type, view);
}

void sugoi_chunk_t::x_unlock(const sugoi_type_index_t& type, const sugoi_chunk_view_t& view)
{
    (void)view;
    auto& lck = getSliceLock(type);
    lck.unlock();
}

void sugoi_chunk_t::s_unlock(const sugoi_type_index_t& type, const sugoi_chunk_view_t& view) const
{
    (void)view;
    auto& lck = getSliceLock(type);
    lck.unlock_shared();
}

sugoi_chunk_t::RWSlice sugoi_chunk_t::get_unsafe(const sugoi_type_index_t& type, const sugoi_chunk_view_t& view)
{
    EIndex offset = 0;
    const auto id = structure->index(type);
    if (!sugoi::type_index_t(type).is_chunk())
        offset = structure->sizes[id] * view.start;
    return { 
        data() + offset + structure->offsets[pt][id],
        data() + offset + structure->offsets[pt][id] + structure->sizes[id] * view.count
    };
}

sugoi_chunk_t::RSlice sugoi_chunk_t::get_unsafe(const sugoi_type_index_t& type, const sugoi_chunk_view_t& view) const
{
    EIndex offset = 0;
    const auto id = structure->index(type);
    if (!sugoi::type_index_t(type).is_chunk())
        offset = structure->sizes[id] * view.start;
    return { 
        data() + offset + structure->offsets[pt][id],
        data() + offset + structure->offsets[pt][id] + structure->sizes[id] * view.count
    };
}

sugoi::slice_data_t const* sugoi_chunk_t::getSliceData() const noexcept
{
    return (sugoi::slice_data_t const*)(data() + structure->sliceDataOffsets[pt]);
}

sugoi::slice_data_t* sugoi_chunk_t::getSliceData() noexcept
{
    return (sugoi::slice_data_t*)(data() + structure->sliceDataOffsets[pt]);
}

sugoi::slice_lock_t& sugoi_chunk_t::getSliceLock(const sugoi_type_index_t& type) const noexcept
{
    const auto id = structure->index(type);
    const auto pData = getSliceData();
    return pData[id].lck;
}

extern "C" {
sugoi_group_t* sugoiC_get_group(const sugoi_chunk_t* chunk)
{
    return chunk->group;
}

sugoi_storage_t* sugoiC_get_storage(const sugoi_chunk_t* chunk)
{
    return chunk->group->archetype->storage;
}

uint32_t sugoiC_get_count(const sugoi_chunk_t* chunk)
{
    return chunk->count;
}

void sugoiC_x_lock(sugoi_chunk_t* chunk, sugoi_type_index_t type)
{
    chunk->x_lock(type, sugoi_chunk_view_t{chunk, 0, chunk->count});
}

void sugoiC_x_unlock(sugoi_chunk_t* chunk, sugoi_type_index_t type)
{
    chunk->x_unlock(type, sugoi_chunk_view_t{chunk, 0, chunk->count});
}

void sugoiC_s_lock(const sugoi_chunk_t* chunk, sugoi_type_index_t type)
{
    chunk->s_lock(type, sugoi_chunk_view_t{(sugoi_chunk_t*)chunk, 0, chunk->count});
}

void sugoiC_s_unlock(const sugoi_chunk_t* chunk, sugoi_type_index_t type)
{
    chunk->s_unlock(type, sugoi_chunk_view_t{(sugoi_chunk_t*)chunk, 0, chunk->count});
}

}