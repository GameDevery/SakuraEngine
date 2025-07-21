#pragma once
#include "SkrCore/memory/sp.hpp"
#include "SkrRT/ecs/query.hpp"
#include "SkrRT/ecs/scheduler.hpp"
#include "SkrContainersDef/vector.hpp"

namespace skr::ecs
{

inline static constexpr intptr_t kInvalidFieldPtr = ~0;

struct SKR_RUNTIME_API ArchetypeBuilder
{
public:
    ArchetypeBuilder& add_meta_entity(Entity Entity);
    ArchetypeBuilder& add_component(TypeIndex Component);
    ArchetypeBuilder& add_component(TypeIndex Component, intptr_t Field);

    template <class... Components>
    ArchetypeBuilder& add_component()
    {
        (add_component(sugoi_id_of<Components>::get()), ...);
        return *this;
    }

    template <class T, class C>
    ArchetypeBuilder& add_component(ComponentView<C> T::* Member)
    {
        return add_component(sugoi_id_of<C>::get(), (intptr_t)&(((T*)nullptr)->*Member));
    }

    void commit() SKR_NOEXCEPT;

protected:
    friend struct World;
    skr::Vector<TypeIndex> types;
    skr::Vector<Entity> meta_entities;
    skr::Vector<intptr_t> fields;
};

struct SKR_RUNTIME_API AccessBuilder : public TaskSignature
{
public:
    inline AccessBuilder& read(TypeIndex type, EAccessMode mode = EAccessMode::Seq)
    {
        return _access(type, false, mode, kInvalidFieldPtr);
    }

    inline AccessBuilder& write(TypeIndex type, EAccessMode mode = EAccessMode::Seq)
    {
        return _access(type, true, mode, kInvalidFieldPtr);
    }

    template <class... Cs>
    AccessBuilder& read(EAccessMode mode = EAccessMode::Seq)
    {
        static_assert((std::is_const_v<Cs> && ...), "ComponentView must be const for read access");
        (_access(sugoi_id_of<std::remove_const_t<Cs>>::get(), false, mode, kInvalidFieldPtr), ...);
        return *this;
    }

    template <class... Cs>
    AccessBuilder& write(EAccessMode mode = EAccessMode::Seq)
    {
        static_assert((!std::is_const_v<Cs> && ...), "ComponentView must be non-const for read access");
        return (_access(sugoi_id_of<Cs>::get(), true, mode, kInvalidFieldPtr), ...);
    }

    template <class T, class C>
    AccessBuilder& read(ComponentView<C> T::* Member, EAccessMode mode = EAccessMode::Seq)
    {
        static_assert(std::is_const_v<C>, "ComponentView must be const for read access");
        return _access(sugoi_id_of<std::remove_const_t<C>>::get(), false, mode, (intptr_t)&(((T*)nullptr)->*Member));
    }

    template <class T, class C>
    AccessBuilder& write(ComponentView<C> T::* Member, EAccessMode mode = EAccessMode::Seq)
    {
        static_assert(!std::is_const_v<C>, "ComponentView must be non-const for read access");
        return _access(sugoi_id_of<C>::get(), true, mode, (intptr_t)&(((T*)nullptr)->*Member));
    }

    void commit() SKR_NOEXCEPT;

protected:
    AccessBuilder& _access(TypeIndex type, bool write, EAccessMode mode, intptr_t field);
    sugoi_query_t* create_query(sugoi_storage_t* storage) SKR_NOEXCEPT;

    friend struct World;
    skr::Vector<TypeIndex> types;
    skr::Vector<sugoi_operation_t> ops;
    skr::Vector<Entity> meta_entities;
    skr::Vector<intptr_t> fields;

    skr::Vector<TypeIndex> all;
    skr::Vector<TypeIndex> none;
};

struct TaskContext
{
public:
    uint32_t size()
    {
        return count;
    }

    const Entity* entities()
    {
        return (Entity*)sugoiV_get_entities(&view);
    }

    template <typename T>
    ComponentView<T> components()
    {
        auto localType = sugoiV_get_local_type(&view, sugoi_id_of<T>::get());
        auto cspan = sugoi::get_owned_local<T>(&view, localType);
        return ComponentView<T>(cspan, localType, offset);
    }

private:
    friend struct World;
    TaskContext(sugoi_chunk_view_t& InView, uint32_t count, uint32_t offset)
        : view(InView), count(count), offset(offset)
    {
    }
    sugoi_chunk_view_t view;
    uint32_t count;
    uint32_t offset;
};

struct SKR_RUNTIME_API World
{
public:
    World() SKR_NOEXCEPT;
    World(skr::task::scheduler_t& scheduler) SKR_NOEXCEPT;

    void initialize() SKR_NOEXCEPT;
    void finalize() SKR_NOEXCEPT;
    TaskScheduler* get_scheduler() SKR_NOEXCEPT;

    template <typename T>
    requires std::is_copy_constructible_v<T>
    EntityQuery* dispatch_task(T TaskBody, uint32_t batch_size, sugoi_query_t* reuse_query)
    {
        SKR_ASSERT(TS.get());

        skr::RC<AccessBuilder> Access = skr::RC<AccessBuilder>::New();
        TaskBody.build(*Access);
        Access->commit();
        if (reuse_query == nullptr)
        {
            reuse_query = Access->create_query(storage);
        }

        skr::stl_function<void(sugoi_chunk_view_t, uint32_t, uint32_t)> TASK =
            [TaskBody, Access](sugoi_chunk_view_t view, uint32_t count, uint32_t offset) mutable
        {
            T TASK = TaskBody;
            for (int i = 0; i < Access->fields.size(); ++i)
            {
                auto field = Access->fields[i];
                if (field == kInvalidFieldPtr)
                    continue;
                auto component = Access->types[i];
                ComponentViewBase* fieldPtr = (ComponentViewBase*)((uint8_t*)&TASK + field);
                auto localType = sugoiV_get_local_type(&view, component);
                fieldPtr->_local_type = localType;
                if (Access->ops[i].readonly)
                {
                    fieldPtr->_ptr = (void*)sugoiV_get_owned_ro_local(&view, localType);
                }
                else
                {
                    fieldPtr->_ptr = (void*)sugoiV_get_owned_rw_local(&view, localType);
                }
                fieldPtr->_offset = offset;
            }
            TaskContext ctx = TaskContext(view, count, offset);
            TASK.run(ctx);
        };
        Access->query = reuse_query;
        Access->task = skr::RC<Task>::New();
        Access->task->func = std::move(TASK);
        Access->task->batch_size = batch_size;
        TS->add_task(Access);
        return reuse_query;
    }

    template <class T>
    void create_entites(T& Creation, uint32_t Count, Entity* Reserved = nullptr)
    {
        ArchetypeBuilder Builder;
        Creation.build(Builder);
        Builder.commit();
        sugoi_entity_type_t entityType;
        skr::Vector<sugoi_type_index_t> components = Builder.types;
        skr::Vector<Entity> metaEntities = Builder.meta_entities;
        entityType.type = { components.data(), (uint8_t)components.size() };
        entityType.meta = { (sugoi_entity_t*)metaEntities.data(), (uint8_t)metaEntities.size() };
        auto callback = [&](sugoi_chunk_view_t* view) {
            for (int i = 0; i < Builder.fields.size(); ++i)
            {
                auto field = Builder.fields[i];
                if (field == -1)
                    continue;
                auto component = components[i];
                ComponentViewBase* fieldPtr = (ComponentViewBase*)((uint8_t*)&Creation + field);
                auto localType = sugoiV_get_local_type(view, component);
                fieldPtr->_local_type = localType;
                fieldPtr->_ptr = (void*)sugoiV_get_owned_ro_local(view, localType);
            }
            TaskContext ctx = TaskContext(*view, view->count, 0);
            Creation.run(ctx);
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
    skr::UPtr<TaskScheduler> TS;
};

} // namespace skr::ecs