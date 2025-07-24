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

    template <class... Components>
    ArchetypeBuilder& add_component()
    {
        (add_component(sugoi_id_of<Components>::get()), ...);
        return *this;
    }

    template <class T, class C>
    ArchetypeBuilder& add_component(ComponentView<C> T::* Member)
    {
        add_component(sugoi_id_of<C>::get());
        return _access(sugoi_id_of<C>::get(), (intptr_t)&(((T*)nullptr)->*Member), EAccessMode::Seq);
    }

    template <class T, class C>
    ArchetypeBuilder& access(RandomComponentReader<C> T::* Member)
    {
        return _access(sugoi_id_of<C>::get(), (intptr_t)&(((T*)nullptr)->*Member), EAccessMode::Random);
    }

    template <class T, class C>
    ArchetypeBuilder& access(RandomComponentWriter<C> T::* Member)
    {
        return _access(sugoi_id_of<C>::get(), (intptr_t)&(((T*)nullptr)->*Member), EAccessMode::Random);
    }

    template <class T, class C>
    ArchetypeBuilder& access(RandomComponentReadWrite<C> T::* Member)
    {
        return _access(sugoi_id_of<C>::get(), (intptr_t)&(((T*)nullptr)->*Member), EAccessMode::Random);
    }

    void commit() SKR_NOEXCEPT;

protected:
    ArchetypeBuilder& _access(TypeIndex Component, intptr_t Field, EAccessMode FieldMode);

    friend struct World;
    skr::Vector<TypeIndex> types;
    skr::Vector<Entity> meta_entities;
    skr::Vector<intptr_t> fields;
    skr::Vector<TypeIndex> field_types;
    skr::Vector<EAccessMode> field_modes;
};

struct SKR_RUNTIME_API AccessBuilder : public TaskSignature
{
public:
    inline AccessBuilder& has(TypeIndex type)
    {
        return _add_has_filter(type, false);
    }

    template <class... Cs>
    AccessBuilder& has()
    {
        (has(sugoi_id_of<Cs>::get()), ...);
        return *this;
    }

    inline AccessBuilder& none(TypeIndex type)
    {
        return _add_none_filter(type);
    }

    template <class... Cs>
    AccessBuilder& none()
    {
        (none(sugoi_id_of<Cs>::get()), ...);
        return *this;
    }

    inline AccessBuilder& read(TypeIndex type, EAccessMode mode = EAccessMode::Seq)
    {
        _add_has_filter(type, false);
        return _access(type, false, mode, kInvalidFieldPtr);
    }

    inline AccessBuilder& write(TypeIndex type, EAccessMode mode = EAccessMode::Seq)
    {
        _add_has_filter(type, true);
        _access(type, false, mode, kInvalidFieldPtr);
        return _access(type, true, mode, kInvalidFieldPtr);
    }

    template <class... Cs>
    AccessBuilder& read()
    {
        static_assert((std::is_const_v<Cs> && ...), "ComponentView must be const for read access");
        (_add_has_filter(sugoi_id_of<std::remove_const_t<Cs>>::get(), false), ...);
        (_access(sugoi_id_of<std::remove_const_t<Cs>>::get(), false, EAccessMode::Seq, kInvalidFieldPtr), ...);
        return *this;
    }

    template <class... Cs>
    AccessBuilder& write()
    {
        static_assert((!std::is_const_v<Cs> && ...), "ComponentView must be non-const for read access");
        (_add_has_filter(sugoi_id_of<Cs>::get(), true), ...);
        (_access(sugoi_id_of<Cs>::get(), false, EAccessMode::Seq, kInvalidFieldPtr), ...);
        return (_access(sugoi_id_of<Cs>::get(), true, EAccessMode::Seq, kInvalidFieldPtr), ...);
    }

    template <class T, class C>
    AccessBuilder& read(ComponentView<C> T::* Member)
    {
        static_assert(std::is_const_v<C>, "ComponentView must be const for read access");
        _add_has_filter(sugoi_id_of<std::remove_const_t<C>>::get(), false);
        return _access(sugoi_id_of<std::remove_const_t<C>>::get(), false, EAccessMode::Seq, (intptr_t)&(((T*)nullptr)->*Member));
    }

    template <class T, class C>
    AccessBuilder& write(ComponentView<C> T::* Member)
    {
        static_assert(!std::is_const_v<C>, "ComponentView must be non-const for read access");
        _add_has_filter(sugoi_id_of<C>::get(), true);
        _access(sugoi_id_of<C>::get(), false, EAccessMode::Seq, kInvalidFieldPtr);
        return _access(sugoi_id_of<C>::get(), true, EAccessMode::Seq, (intptr_t)&(((T*)nullptr)->*Member));
    }

    inline AccessBuilder& optional_read(TypeIndex type, EAccessMode mode = EAccessMode::Seq)
    {
        return _access(type, false, mode, kInvalidFieldPtr);
    }

    inline AccessBuilder& optional_write(TypeIndex type, EAccessMode mode = EAccessMode::Seq)
    {
        _access(type, false, mode, kInvalidFieldPtr);
        return _access(type, true, mode, kInvalidFieldPtr);
    }

    template <class... Cs>
    AccessBuilder& optional_read()
    {
        static_assert((std::is_const_v<Cs> && ...), "ComponentView must be const for read access");
        (_access(sugoi_id_of<std::remove_const_t<Cs>>::get(), false, EAccessMode::Seq, kInvalidFieldPtr), ...);
        return *this;
    }

    template <class... Cs>
    AccessBuilder& optional_write()
    {
        static_assert((!std::is_const_v<Cs> && ...), "ComponentView must be non-const for read access");
        (_access(sugoi_id_of<Cs>::get(), false, EAccessMode::Seq, kInvalidFieldPtr), ...);
        return (_access(sugoi_id_of<Cs>::get(), true, EAccessMode::Seq, kInvalidFieldPtr), ...);
    }

    template <class T, class C>
    AccessBuilder& optional_read(ComponentView<C> T::* Member)
    {
        static_assert(std::is_const_v<C>, "ComponentView must be const for read access");
        return _access(sugoi_id_of<std::remove_const_t<C>>::get(), false, EAccessMode::Seq, (intptr_t)&(((T*)nullptr)->*Member));
    }

    template <class T, class C>
    AccessBuilder& optional_write(ComponentView<C> T::* Member)
    {
        static_assert(!std::is_const_v<C>, "ComponentView must be non-const for read access");
        _access(sugoi_id_of<C>::get(), false, EAccessMode::Seq, kInvalidFieldPtr);
        return _access(sugoi_id_of<C>::get(), true, EAccessMode::Seq, (intptr_t)&(((T*)nullptr)->*Member));
    }

    template <class T, class C>
    AccessBuilder& access(RandomComponentReader<C> T::* Member)
    {
        static_assert(std::is_const_v<C>, "ComponentView must be const for read access");
        return _access(sugoi_id_of<std::remove_const_t<C>>::get(), false, EAccessMode::Random, (intptr_t)&(((T*)nullptr)->*Member));
    }

    template <class T, class C>
    AccessBuilder& access(RandomComponentWriter<C> T::* Member)
    {
        static_assert(!std::is_const_v<C>, "ComponentView must be non-const for read access");
        return _access(sugoi_id_of<C>::get(), true, EAccessMode::Random, (intptr_t)&(((T*)nullptr)->*Member));
    }

    template <class T, class C>
    AccessBuilder& access(RandomComponentReadWrite<C> T::* Member)
    {
        static_assert(!std::is_const_v<C>, "ComponentView must be non-const for read access");
        _access(sugoi_id_of<C>::get(), false, EAccessMode::Random, kInvalidFieldPtr);
        return _access(sugoi_id_of<C>::get(), true, EAccessMode::Random, (intptr_t)&(((T*)nullptr)->*Member));
    }

    void commit() SKR_NOEXCEPT;

protected:
    AccessBuilder& _add_has_filter(TypeIndex type, bool write);
    AccessBuilder& _add_none_filter(TypeIndex type);
    AccessBuilder& _access(TypeIndex type, bool write, EAccessMode mode, intptr_t field);
    sugoi_query_t* create_query(sugoi_storage_t* storage) SKR_NOEXCEPT;

    friend struct World;
    skr::Vector<TypeIndex> types;
    skr::Vector<sugoi_operation_t> ops;
    skr::Vector<Entity> meta_entities;
    skr::Vector<intptr_t> fields;
    skr::Vector<TypeIndex> field_types;
    skr::Vector<EAccessMode> field_modes;

    skr::Vector<TypeIndex> all;
    skr::Vector<TypeIndex> nones;
};

struct TaskContext
{
public:
    const Entity* entities()
    {
        return (Entity*)sugoiV_get_entities(&view);
    }

    uint32_t task_index() const { return _task_index; }
    uint32_t size() const { return count; }

    template <typename T>
    ComponentView<T> components()
    {
        auto localType = sugoiV_get_local_type(&view, sugoi_id_of<T>::get());
        auto cspan = sugoi::get_owned_local<T>(&view, localType);
        return ComponentView<T>(cspan, localType, offset);
    }

private:
    friend struct World;
    TaskContext(sugoi_chunk_view_t& InView, uint32_t count, uint32_t offset, uint32_t task_index)
        : view(InView), count(count), offset(offset), _task_index(task_index)
    {
    }
    sugoi_chunk_view_t view;
    uint32_t count;
    uint32_t offset;
    const uint32_t _task_index;
};

struct SKR_RUNTIME_API World
{
public:
    World() SKR_NOEXCEPT;
    World(skr::task::scheduler_t& scheduler) SKR_NOEXCEPT;

    void initialize() SKR_NOEXCEPT;
    void finalize() SKR_NOEXCEPT;

    template <typename T>
        requires std::is_copy_constructible_v<T>
    EntityQuery* dispatch_task(T TaskBody, uint32_t batch_size, sugoi_query_t* reuse_query)
    {
        SKR_ASSERT(TaskScheduler::Get());

        skr::RC<AccessBuilder> Access = skr::RC<AccessBuilder>::New();
        TaskBody.build(*Access);
        Access->commit();
        if (reuse_query == nullptr)
        {
            reuse_query = Access->create_query(storage);
        }

        Access->query = reuse_query;
        Access->task = skr::RC<Task>::New();
        Access->task->batch_size = batch_size;
        skr::stl_function<void(sugoi_chunk_view_t, uint32_t, uint32_t)> TASK =
            [TaskBody, Access, Storage = this->storage](sugoi_chunk_view_t view, uint32_t count, uint32_t offset) mutable
        {
            T TASK = TaskBody;
            for (int i = 0; i < Access->fields.size(); ++i)
            {
                const auto field = Access->fields[i];
                const auto component = Access->field_types[i];
                const auto mode = Access->field_modes[i];
                if (field == kInvalidFieldPtr)
                    continue;
                if (mode == EAccessMode::Random)
                {
                    ComponentAccessorBase* fieldPtr = (ComponentAccessorBase*)((uint8_t*)&TASK + field);
                    fieldPtr->World = Storage;
                    fieldPtr->Type = component;
                    fieldPtr->CachedView = view;
                    fieldPtr->CachedPtr = nullptr;
                }
                else if (mode == EAccessMode::Seq)
                {
                    ComponentViewBase* fieldPtr = (ComponentViewBase*)((uint8_t*)&TASK + field);
                    auto localType = sugoiV_get_local_type(&view, component);
                    fieldPtr->_local_type = localType;
                    if (Access->ops[i].readonly)
                        fieldPtr->_ptr = (void*)sugoiV_get_owned_ro_local(&view, localType);
                    else
                        fieldPtr->_ptr = (void*)sugoiV_get_owned_rw_local(&view, localType);
                    fieldPtr->_offset = offset;
                }
            }
            TaskContext ctx = TaskContext(view, count, offset, Access->_exec_counter++);
            TASK.run(ctx);
        };
        Access->task->func = std::move(TASK);
        TaskScheduler::Get()->add_task(Access);
        return reuse_query;
    }

    template <class T>
    void create_entities(T& Creation, uint32_t Count, Entity* Reserved = nullptr)
    {
        ArchetypeBuilder Builder;
        Creation.build(Builder);
        Builder.commit();
        sugoi_entity_type_t entityType;
        const skr::Vector<sugoi_type_index_t>& components = Builder.types;
        const skr::Vector<Entity>& metaEntities = Builder.meta_entities;
        entityType.type = { components.data(), (uint8_t)components.size() };
        entityType.meta = { (sugoi_entity_t*)metaEntities.data(), (uint8_t)metaEntities.size() };
        auto callback = [&](sugoi_chunk_view_t* view) {
            for (int i = 0; i < Builder.fields.size(); ++i)
            {
                const auto field = Builder.fields[i];
                const auto component = Builder.field_types[i];
                const auto mode = Builder.field_modes[i];
                if (field == kInvalidFieldPtr)
                    continue;
                if (mode == EAccessMode::Random)
                {
                    ComponentAccessorBase* fieldPtr = (ComponentAccessorBase*)((uint8_t*)&Creation + field);
                    fieldPtr->World = this->storage;
                    fieldPtr->Type = component;
                    fieldPtr->CachedView = *view;
                    fieldPtr->CachedPtr = nullptr;
                }
                else if (mode == EAccessMode::Seq)
                {
                    ComponentViewBase* fieldPtr = (ComponentViewBase*)((uint8_t*)&Creation + field);
                    auto localType = sugoiV_get_local_type(view, component);
                    fieldPtr->_local_type = localType;
                    fieldPtr->_ptr = (void*)sugoiV_get_owned_ro_local(view, localType);
                }
            }
            TaskContext ctx = TaskContext(*view, view->count, 0, 0);
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
};

} // namespace skr::ecs