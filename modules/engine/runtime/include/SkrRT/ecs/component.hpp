#pragma once
#include "SkrRT/sugoi/sugoi.h"
#include "SkrRT/sugoi/array.hpp"

namespace skr::ecs {

struct World;

template <typename T, typename = void>
struct ComponentStorage
{
    using Type = T;
};

template <typename T>
struct ComponentStorage<T, std::enable_if_t<(sugoi_array_count<std::decay_t<T>> > 0)>>
{
    using Type = sugoi::ArrayComponent<std::decay<T>, sugoi_array_count<std::decay_t<T>>>;
};

struct Entity
{
public:
    Entity(sugoi_entity_t InEntityId = sugoi::kEntityNull);
    bool operator==(Entity Other) const;
    bool operator==(sugoi_entity_t Other) const;
    operator sugoi_entity_t() const;
    operator bool() const;
    bool operator<(Entity Other) const;
    skr::String to_string() const;
    static size_t _skr_hash(const Entity& Handle);

private:
    sugoi_entity_t EntityId = sugoi::kEntityNull;
};
static_assert(sizeof(Entity) == sizeof(sugoi_entity_t), "Entity size must match sugoi_entity_t size");

template <typename T>
concept EntityConcept = std::is_same_v<T, Entity>;

struct ComponentViewBase
{
protected:
    ComponentViewBase(void* ptr = nullptr, uint32_t local_type = 0)
        : _ptr(ptr), _local_type(local_type) {}
    friend struct World;
    friend struct CreationContext;
    void* _ptr;
    uint32_t _local_type;
};

template <typename T>
struct ComponentView : public ComponentViewBase
{
    using Storage = typename ComponentStorage<T>::Type;
    Storage& operator[](int32_t Index)
    {
        return ((Storage*)_ptr)[Index];
    }

    explicit operator bool() const
    {
        return _ptr != nullptr;
    }
protected:
    friend struct World;
    friend struct CreationContext;
    ComponentView(T* ptr = nullptr, uint32_t local_type = 0)
        : ComponentViewBase(ptr, local_type) {}
};

struct SubQuery
{
public:
    bool is_valid() const;
    bool match(sugoi_entity_t Entity) const;
    uint32_t Count() const;
    SKR_RUNTIME_API Entity get_singleton() const;

private:
    sugoi_query_t* query;
};

struct ComponentAccessorBase
{
    sugoi_storage_t* World;
    SubQuery BoundQuery;
    sugoi_type_index_t Type;
    sugoi_chunk_view_t CachedView;
    void* CachedPtr;
};

template <class T>
struct ComponentAccessor : public ComponentAccessorBase
{
    using Storage = typename ComponentStorage<T>::Type;
    Storage& operator[](size_t Index)
    {
        return get_checked(Index);
    }

    Storage& get_checked(size_t Index)
    {
        Storage* Result = get(Index);
        check(Result);
        return *Result;
    }

    Storage* get(size_t Index)
    {
        sugoi_chunk_view_t view;
        sugoiS_access(World, Index, &view);
        if (view.chunk == nullptr)
            return nullptr;
        if(CachedPtr != nullptr && CachedView.chunk == view.chunk)
        {
            auto Offset = (int64_t)view.start - (int64_t)CachedView.start;
            return ((Storage*)CachedPtr) + Offset;
        }
        Storage* Result;
        if constexpr(std::is_const_v<T>)
        {
            Result = (Storage*)sugoiV_get_owned_ro(&view, Type);
        }
        else
        {
            Result = (Storage*)sugoiV_get_owned_rw(&view, Type);
        }
        CachedView = view;
        CachedPtr = (void*)Result;
        return Result;
    }
};

} // namespace skr::ecs

// inline implementations
namespace skr::ecs {

inline Entity::Entity(sugoi_entity_t InEntityId)
    : EntityId(InEntityId)
{

}

inline bool Entity::operator==(Entity Other) const
{
    return EntityId == Other.EntityId;
}

inline bool Entity::operator==(sugoi_entity_t Other) const
{
    return EntityId == Other;
}

inline Entity::operator sugoi_entity_t() const
{
    return EntityId;
}

inline Entity::operator bool() const
{
    return EntityId != sugoi::kEntityNull;
}

inline bool Entity::operator<(Entity Other) const
{
    return EntityId < Other.EntityId;
}

inline size_t Entity::_skr_hash(const Entity& Handle)
{
    return Handle.EntityId;
}

inline skr::String Entity::to_string() const
{
    return skr::format(u8"[{}/{}]", sugoi::e_id(EntityId), sugoi::e_version(EntityId));
}

inline bool SubQuery::is_valid() const
{
    return query != nullptr;
}

inline bool SubQuery::match(sugoi_entity_t Entity) const
{
    return sugoiQ_match_entity(query, Entity) != 0;
}

inline uint32_t SubQuery::Count() const
{
    return sugoiQ_get_count(query);
}

} // namespace skr::ecs
