#pragma once
#include "SkrRT/sugoi/sugoi.h"
#include "SkrRT/sugoi/array.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/bitset.hpp"
#include "SkrContainersDef/map.hpp"
#include "SkrBase/atomic/atomic_mutex.hpp"

namespace skr::ecs
{
inline static constexpr auto kSugoiStagingBufferSize = 4096;
using SpinLock = skr::shared_atomic_mutex;

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
struct ComponentViewBase
{
    void* Ptr;
    uint32_t LocalType;
};

// Task parameter for array-like component access
template <class T>
struct ComponentView : public ComponentViewBase
{
    using Storage = typename ComponentStorage<T>::Type;
    Storage& operator[](int32_t Index)
    {
        return ((Storage*)Ptr)[Index];
    }

    explicit operator bool() const
    {
        return Ptr != nullptr;
    }
};

struct SKR_RUNTIME_API CreationBuilder
{
    // Component type registry
    skr::Vector<sugoi_type_index_t> Components;
    // Meta entity links
    skr::Vector<sugoi_entity_t> MetaEntities;
    // Component field offsets
    skr::Vector<intptr_t> Fields;

    CreationBuilder& add_component(sugoi_type_index_t Component);

    template <class... Components>
    CreationBuilder& add_component()
    {
        (add_component(sugoi_id_of<Components>::get()), ...);
        return *this;
    }

    CreationBuilder& add_meta_entity(sugoi_entity_t Entity);

    CreationBuilder& add_component(sugoi_type_index_t Component, intptr_t Field);

    template <class T, class F>
    CreationBuilder& add_component(ComponentView<F> T::* Member)
    {
        return add_component(sugoi_id_of<T>::get(), (intptr_t)&(((T*)nullptr)->*Member));
    }

    CreationBuilder& commit();
};

struct CreationContext
{
    sugoi_chunk_view_t view;
    EIndex EntityIndex;

    uint32_t Num()
    {
        return view.count;
    }

    const sugoi_entity_t* Entities()
    {
        return sugoiV_get_entities(&view);
    }
};

struct SKR_RUNTIME_API World
{
public:
    void initialize();
    void finalize();

    template <class T>
    void create_entites(T& Creation, uint32_t Count, sugoi_entity_t* Reserved = nullptr)
    {
        Sugoi::CreationBuilder Builder;
        Creation.Build(Builder);
        Builder.Commit();
        sugoi_entity_type_t entityType;
        skr::Vector<sugoi_type_index_t> components = Builder.Components;
        skr::Vector<sugoi_entity_t> metaEntities = Builder.MetaEntities;
        entityType.type = { components.GetData(), (uint8_t)components.Num() };
        entityType.meta = { metaEntities.GetData(), (uint8_t)metaEntities.Num() };
        uint32 EntityIndex = 0;
        auto callback = [&](sugoi_chunk_view_t* view) {
            for (int i = 0; i < Builder.Fields.Num(); ++i)
            {
                auto field = Builder.Fields[i];
                if (field == -1)
                    continue;
                auto component = components[i];
                Sugoi::FComponentView* fieldPtr = (Sugoi::FComponentView*)((uint8*)&Creation + field);
                auto localType = sugoiV_get_local_type(view, component);
                fieldPtr->LocalType = localType;
                fieldPtr->Ptr = (void*)sugoiV_get_owned_ro_local(view, localType);
            }
            Sugoi::FCreationContext CreationContext = { *view, EntityIndex };
            Creation.Run(CreationContext);
            EntityIndex += view->count;
        };
        if (Reserved)
        {
            sugoiS_allocate_reserved_type(storage, &entityType, Reserved, Count, SUGOI_LAMBDA(callback));
        }
        else
        {
            sugoiS_allocate_type(storage, &entityType, Count, SUGOI_LAMBDA(callback));
        }
    }

protected:
    sugoi_storage_t* storage = nullptr;
};

} // namespace skr::ecs