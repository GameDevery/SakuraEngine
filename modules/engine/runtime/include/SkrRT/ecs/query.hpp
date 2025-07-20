#pragma once
#include "SkrBase/types/expected.hpp"
#include "SkrCore/memory/rc.hpp"
#include "SkrRT/ecs/world.hpp"

namespace skr::ecs {

enum class QueryBuilderError
{
    UnknownError,
};

// struct EntityQuery;
using EntityQuery = sugoi_query_t;
using QueryBuilderResult = skr::Expected<QueryBuilderError, EntityQuery*>;

enum class Presence : uint8_t
{
    Own,
    Optional,
    Exclude,
    Share
};

struct QueryBuilder
{
public:
    QueryBuilder(World* world)
        : world(world)
    {

    }

    template <typename...Ts, typename...Idxs>
    QueryBuilder& ReadWriteAll(Idxs... idxs) { return All<false, Ts...>(idxs...); }
    template <typename...Ts, typename...Idxs>
    QueryBuilder& ReadAll(Idxs... idxs) { return All<true, Ts...>(idxs...); }
    template <typename...Ts, typename...Idxs>
    QueryBuilder& ReadWriteOptional(Idxs... idxs) { return Optional<false, Ts...>(idxs...); }
    template <typename...Ts, typename...Idxs>
    QueryBuilder& ReadOptional(Idxs... idxs) { return Optional<true, Ts...>(idxs...);  }

    template <skr::ecs::EntityConcept...Ents>
    QueryBuilder& WithMetaEntity(Ents... ents)
    {
        (all_meta.push_back(Entity(ents)), ...);
        return *this;
    }

    template <skr::ecs::EntityConcept...Ents>
    QueryBuilder& WithoutMetaEntity(Ents... ents)
    {
        (none_meta.push_back(Entity(ents)), ...);
        return *this;
    }

    QueryBuilderResult commit() SKR_NOEXCEPT
    {
        sugoi_parameters_t parameters;
        parameters.types = types.data();
        parameters.accesses = ops.data();
        parameters.length = types.size();    

        SKR_DECLARE_ZERO(sugoi_filter_t, filter);
        all.sort([](auto a, auto b) { return a < b; });
        filter.all.data = all.data();
        filter.all.length = all.size();
        filter.none.data = none.data();
        filter.none.length = none.size();

        auto q = sugoiQ_create(world->get_storage(), &filter, &parameters);
        if (q)
        {
            SKR_DECLARE_ZERO(sugoi_meta_filter_t, meta_filter);
            if (all_meta.size())
            {
                meta_filter.all_meta.data = all_meta.data();
                meta_filter.all_meta.length = all_meta.size();
            }
            if (none_meta.size())
            {
                meta_filter.none_meta.data = none_meta.data();
                meta_filter.none_meta.length = none_meta.size();
            }
            sugoiQ_set_meta(q, &meta_filter);
        }
        return (EntityQuery*)q;
    }

    template <typename...Ts, typename...Idxs>
    QueryBuilder& None(Idxs... idxs)
    {
        return AddComponentRequirement<true, Ts..., Idxs...>(Presence::Exclude, idxs...);
    }

    template <bool readonly, typename...Ts, typename...Idxs>
    QueryBuilder& All(Idxs... idxs)
    {
        return AddComponentRequirement<readonly, Ts..., Idxs...>(Presence::Own, idxs...);
    }
    template <bool readonly, typename...Ts, typename...Idxs>
    QueryBuilder& Share(Idxs... idxs)
    {
        return AddComponentRequirement<readonly, Ts..., Idxs...>(Presence::Share, idxs...);
    }
    template <bool readonly, typename...Ts, typename...Idxs>
    QueryBuilder& Optional(Idxs... idxs)
    {
        return AddComponentRequirement<readonly, Ts..., Idxs...>(Presence::Optional, idxs...);
    }

private:
    template <bool readonly, typename...Ts, typename...Idxs>
    QueryBuilder& AddComponentRequirement(Presence presence, Idxs... idxs)
    {
        if constexpr (sizeof...(Ts) > 0)
        {
            if (presence == Presence::Own)
                (all.push_back(idxs), ...);
            else if (presence == Presence::Share)
                (share.push_back(idxs), ...);
            else if (presence == Presence::Exclude)
                (none.push_back(idxs), ...);

            (types.push_back(idxs), ...);
            (ops.push_back({0 * (int)idxs, readonly, false, SOS_SEQ}), ...);
        }
        if (presence == Presence::Own)
            (all.push_back(sugoi_id_of<Ts>::get()), ...);
        else if (presence == Presence::Share)
            (share.push_back(sugoi_id_of<Ts>::get()), ...);
        else if (presence == Presence::Exclude)
            (none.push_back(sugoi_id_of<Ts>::get()), ...);

        (types.push_back(sugoi_id_of<Ts>::get()), ...);
        (ops.push_back({0 * (int)sugoi_id_of<Ts>::get(), readonly, false, SOS_SEQ}), ...);
        return *this;
    }

    World* world = nullptr;

    skr::InlineVector<sugoi_entity_t, 4> all_meta;
    skr::InlineVector<sugoi_entity_t, 4> none_meta;

    skr::InlineVector<sugoi_type_index_t, 8> all;
    skr::InlineVector<sugoi_type_index_t, 4> none;
    skr::InlineVector<sugoi_type_index_t, 1> share;
    skr::InlineVector<sugoi_type_index_t, 8> types;
    skr::InlineVector<sugoi_operation_t, 8> ops;
};


} // namespace skr::ecs