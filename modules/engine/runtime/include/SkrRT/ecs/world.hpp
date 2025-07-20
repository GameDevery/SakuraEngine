#pragma once
#include "SkrRT/ecs/component.hpp"
#include "SkrContainersDef/vector.hpp"
#include "SkrContainersDef/bitset.hpp"
#include "SkrContainersDef/map.hpp"

namespace skr::ecs
{

struct SKR_RUNTIME_API CreationBuilder
{
public:
    CreationBuilder& add_component(sugoi_type_index_t Component);

    template <class... Components>
    CreationBuilder& add_component()
    {
        (add_component(sugoi_id_of<Components>::get()), ...);
        return *this;
    }

    CreationBuilder& add_meta_entity(Entity Entity);

    CreationBuilder& add_component(sugoi_type_index_t Component, intptr_t Field);

    template <class T, class C>
    CreationBuilder& add_component(ComponentView<C> T::* Member)
    {
        return add_component(sugoi_id_of<C>::get(), (intptr_t)&(((T*)nullptr)->*Member));
    }

    void commit() SKR_NOEXCEPT;

protected:
    friend struct World;
    // Component type registry
    skr::Vector<sugoi_type_index_t> Components;
    // Meta entity links
    skr::Vector<Entity> MetaEntities;
    // Component field offsets
    skr::Vector<intptr_t> Fields;
};

struct CreationContext
{
public:
    uint32_t size()
    {
        return view.count;
    }

    const Entity* entities()
    {
        return (Entity*)sugoiV_get_entities(&view);
    }

    template <typename T>
    ComponentView<T> components()
    {
        auto cspan = sugoi::get_components<T>(&view);
        auto localType = sugoiV_get_local_type(&view, sugoi_id_of<T>::get());
        return ComponentView<T>(cspan.data(), localType);
    }

private:
    friend struct World;
    CreationContext(sugoi_chunk_view_t& InView, EIndex EntityIndex)
        : view(InView), EntityIndex(EntityIndex)
    {
    }
    sugoi_chunk_view_t view;
    EIndex EntityIndex;
};

struct SKR_RUNTIME_API World
{
public:
    World() SKR_NOEXCEPT = default;
    void initialize() SKR_NOEXCEPT;
    void finalize() SKR_NOEXCEPT;

    template <class T>
    void create_entites(T& Creation, uint32_t Count, Entity* Reserved = nullptr)
    {
        CreationBuilder Builder;
        Creation.build(Builder);
        Builder.commit();
        sugoi_entity_type_t entityType;
        skr::Vector<sugoi_type_index_t> components = Builder.Components;
        skr::Vector<Entity> metaEntities = Builder.MetaEntities;
        entityType.type = { components.data(), (uint8_t)components.size() };
        entityType.meta = { (sugoi_entity_t*)metaEntities.data(), (uint8_t)metaEntities.size() };
        uint32_t EntityIndex = 0;
        auto callback = [&](sugoi_chunk_view_t* view) {
            for (int i = 0; i < Builder.Fields.size(); ++i)
            {
                auto field = Builder.Fields[i];
                if (field == -1)
                    continue;
                auto component = components[i];
                ComponentViewBase* fieldPtr = (ComponentViewBase*)((uint8_t*)&Creation + field);
                auto localType = sugoiV_get_local_type(view, component);
                fieldPtr->_local_type = localType;
                fieldPtr->_ptr = (void*)sugoiV_get_owned_ro_local(view, localType);
            }
            CreationContext ctx = CreationContext(*view, EntityIndex);
            Creation.run(ctx);
            EntityIndex += view->count;
        };
        if (Reserved)
        {
            sugoiS_allocate_reserved_type(storage, &entityType, (sugoi_entity_t*)Reserved, Count, SUGOI_LAMBDA(callback));
        }
        else
        {
            sugoiS_allocate_type(storage, &entityType, Count, SUGOI_LAMBDA(callback));
        }
    }

	void destroy_entities(skr::span<Entity> ToDestroy)
	{
		sugoiS_destroy_entities(storage, (sugoi_entity_t*)ToDestroy.data(), ToDestroy.size());
	}

    void destroy_query(sugoi_query_t* q)
    {
        sugoiQ_release(q);
    }

    sugoi_storage_t* get_storage() SKR_NOEXCEPT;

protected:
    World(const World&) = delete;
    World& operator=(const World&) = delete;
    sugoi_storage_t* storage = nullptr;
};

} // namespace skr::ecs