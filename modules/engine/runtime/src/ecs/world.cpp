#include "SkrRT/ecs/world.hpp"

namespace skr::ecs
{

ArchetypeBuilder& ArchetypeBuilder::add_component(sugoi_type_index_t type)
{
	if(types.contains(type))
	{
		return *this;
	}
	types.add(type);
	fields.add(-1);
	return *this;
}

ArchetypeBuilder& ArchetypeBuilder::add_meta_entity(Entity Entity)
{
	if (meta_entities.contains(Entity))
	{
		return *this;
	}
	meta_entities.add(Entity);
	return *this;
}

ArchetypeBuilder& ArchetypeBuilder::add_component(sugoi_type_index_t type, intptr_t Field)
{
	if (auto index = types.find(type))
	{
		fields[index.index()] = Field;
		return *this;
	}
	types.add(type);
	fields.add(Field);
	return *this;
}

void ArchetypeBuilder::commit() SKR_NOEXCEPT
{
    types.sort();
    meta_entities.sort();
}

void AccessBuilder::commit() SKR_NOEXCEPT
{
	all.sort([](auto a, auto b) { return a < b; });
	none.sort([](auto a, auto b) { return a < b; });
}

sugoi_query_t* AccessBuilder::create_query(sugoi_storage_t* storage) SKR_NOEXCEPT
{
	sugoi_parameters_t parameters;
	parameters.types = types.data();
	parameters.accesses = ops.data();
	parameters.length = types.size();    

	SKR_DECLARE_ZERO(sugoi_filter_t, filter);
	filter.all.data = all.data();
	filter.all.length = all.size();

	/*
	none.sort([](auto a, auto b) { return a < b; });
	filter.none.data = none.data();
	filter.none.length = none.size();
	*/
	auto q = sugoiQ_create(storage, &filter, &parameters);
	if (q)
	{
		SKR_DECLARE_ZERO(sugoi_meta_filter_t, meta_filter);
		if (meta_entities.size())
		{
			meta_filter.all_meta.data = (const sugoi_entity_t*)meta_entities.data();
			meta_filter.all_meta.length = meta_entities.size();
		}
		/*
		if (none_meta.size())
		{
			meta_filter.none_meta.data = none_meta.data();
			meta_filter.none_meta.length = none_meta.size();
		}
		*/
		sugoiQ_set_meta(q, &meta_filter);
	}
	return q;
}

AccessBuilder& AccessBuilder::_access(TypeIndex type, bool write, EAccessMode mode, intptr_t field)
{
	// fill: base type accessors
	if (write)
		writes.add({type, mode});
	else
		reads.add({type, mode});

	// fill: access builder args
	if (field != kInvalidFieldPtr)
	{
		fields.add(field);
	}

	all.add(type);
	// share
	// exclude
	types.add(type);
	ops.add({ 
		.phase = 0, 
		.readonly = !write,
		.atomic = (mode == EAccessMode::Atomic),
		.randomAccess = SOS_SEQ
	});
	return *this;
}

static const auto kTSServiceDesc = ServiceThreadDesc {
	.name = u8"ECSTaskScheduler",
	.priority = SKR_THREAD_ABOVE_NORMAL,
};
World::World(skr::task::scheduler_t& scheduler) SKR_NOEXCEPT
	: TS(skr::UPtr<TaskScheduler>::New(kTSServiceDesc, scheduler))
{
	storage = sugoiS_create();
}

World::World() SKR_NOEXCEPT
{
	storage = sugoiS_create();
}

void World::initialize() SKR_NOEXCEPT
{
    storage = sugoiS_create();
	if (TS.get())
	{
		TS->run();
	}
}

void World::finalize() SKR_NOEXCEPT
{
	if (TS.get())
	{
		TS->sync_all();
		TS->stop_and_exit();
	}

    sugoiS_release(storage);
    storage = nullptr;
}

TaskScheduler* World::get_scheduler() SKR_NOEXCEPT
{
	return TS.get() ? TS.get() : nullptr;
}

sugoi_storage_t* World::get_storage() SKR_NOEXCEPT
{
    return storage;
}

QueryBuilder::QueryBuilder(World* world) SKR_NOEXCEPT
	: world(world)
{

}

QueryBuilderResult QueryBuilder::commit() SKR_NOEXCEPT
{
	sugoi_parameters_t parameters;
	parameters.types = types.data();
	parameters.accesses = ops.data();
	parameters.length = types.size();    

	SKR_DECLARE_ZERO(sugoi_filter_t, filter);
	all.sort([](auto a, auto b) { return a < b; });
	filter.all.data = all.data();
	filter.all.length = all.size();

	none.sort([](auto a, auto b) { return a < b; });
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

} // namespace skr::ecs